//===-- ARMTargetMachine.cpp - Define TargetMachine for ARM ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "ARM.h"
#include "ARMTargetMachine.h"
#include "ARMFrameLowering.h"
#include "ARMTargetObjectFile.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/Scalar.h"
using namespace llvm;

static cl::opt<bool>
DisableA15SDOptimization("disable-a15-sd-optimization", cl::Hidden,
                   cl::desc("Inhibit optimization of S->D register accesses on A15"),
                   cl::init(false));

static cl::opt<bool>
EnableAtomicTidy("arm-atomic-cfg-tidy", cl::Hidden,
                 cl::desc("Run SimplifyCFG after expanding atomic operations"
                          " to make use of cmpxchg flow-based information"),
                 cl::init(true));

extern "C" void LLVMInitializeARMTarget() {
  // Register the target.
  RegisterTargetMachine<ARMLETargetMachine> X(TheARMLETarget);
  RegisterTargetMachine<ARMBETargetMachine> Y(TheARMBETarget);
  RegisterTargetMachine<ThumbLETargetMachine> A(TheThumbLETarget);
  RegisterTargetMachine<ThumbBETargetMachine> B(TheThumbBETarget);
}

static std::unique_ptr<TargetLoweringObjectFile> createTLOF(const Triple &TT) {
  if (TT.isOSBinFormatMachO())
    return make_unique<TargetLoweringObjectFileMachO>();
  if (TT.isOSWindows())
    return make_unique<TargetLoweringObjectFileCOFF>();
  return make_unique<ARMElfTargetObjectFile>();
}

/// TargetMachine ctor - Create an ARM architecture model.
///
ARMBaseTargetMachine::ARMBaseTargetMachine(const Target &T, StringRef TT,
                                           StringRef CPU, StringRef FS,
                                           const TargetOptions &Options,
                                           Reloc::Model RM, CodeModel::Model CM,
                                           CodeGenOpt::Level OL, bool isLittle)
    : LLVMTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL),
      TLOF(createTLOF(Triple(getTargetTriple()))),
      Subtarget(TT, CPU, FS, *this, isLittle), isLittle(isLittle) {

  // Default to triple-appropriate float ABI
  if (Options.FloatABIType == FloatABI::Default)
    this->Options.FloatABIType =
        Subtarget.isTargetHardFloat() ? FloatABI::Hard : FloatABI::Soft;
}

ARMBaseTargetMachine::~ARMBaseTargetMachine() {}

const ARMSubtarget *
ARMBaseTargetMachine::getSubtargetImpl(const Function &F) const {
  AttributeSet FnAttrs = F.getAttributes();
  Attribute CPUAttr =
      FnAttrs.getAttribute(AttributeSet::FunctionIndex, "target-cpu");
  Attribute FSAttr =
      FnAttrs.getAttribute(AttributeSet::FunctionIndex, "target-features");

  std::string CPU = !CPUAttr.hasAttribute(Attribute::None)
                        ? CPUAttr.getValueAsString().str()
                        : TargetCPU;
  std::string FS = !FSAttr.hasAttribute(Attribute::None)
                       ? FSAttr.getValueAsString().str()
                       : TargetFS;

  // FIXME: This is related to the code below to reset the target options,
  // we need to know whether or not the soft float flag is set on the
  // function before we can generate a subtarget. We also need to use
  // it as a key for the subtarget since that can be the only difference
  // between two functions.
  Attribute SFAttr =
      FnAttrs.getAttribute(AttributeSet::FunctionIndex, "use-soft-float");
  bool SoftFloat = !SFAttr.hasAttribute(Attribute::None)
                       ? SFAttr.getValueAsString() == "true"
                       : Options.UseSoftFloat;

  auto &I = SubtargetMap[CPU + FS + (SoftFloat ? "use-soft-float=true"
                                               : "use-soft-float=false")];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = llvm::make_unique<ARMSubtarget>(TargetTriple, CPU, FS, *this, isLittle);
  }
  return I.get();
}

void ARMBaseTargetMachine::addAnalysisPasses(PassManagerBase &PM) {
  // Add first the target-independent BasicTTI pass, then our ARM pass. This
  // allows the ARM pass to delegate to the target independent layer when
  // appropriate.
  PM.add(createBasicTargetTransformInfoPass(this));
  PM.add(createARMTargetTransformInfoPass(this));
}


void ARMTargetMachine::anchor() { }

ARMTargetMachine::ARMTargetMachine(const Target &T, StringRef TT, StringRef CPU,
                                   StringRef FS, const TargetOptions &Options,
                                   Reloc::Model RM, CodeModel::Model CM,
                                   CodeGenOpt::Level OL, bool isLittle)
    : ARMBaseTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, isLittle) {
  initAsmInfo();
  if (!Subtarget.hasARMOps())
    report_fatal_error("CPU: '" + Subtarget.getCPUString() + "' does not "
                       "support ARM mode execution!");
}

void ARMLETargetMachine::anchor() { }

ARMLETargetMachine::ARMLETargetMachine(const Target &T, StringRef TT,
                                       StringRef CPU, StringRef FS,
                                       const TargetOptions &Options,
                                       Reloc::Model RM, CodeModel::Model CM,
                                       CodeGenOpt::Level OL)
    : ARMTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, true) {}

void ARMBETargetMachine::anchor() { }

ARMBETargetMachine::ARMBETargetMachine(const Target &T, StringRef TT,
                                       StringRef CPU, StringRef FS,
                                       const TargetOptions &Options,
                                       Reloc::Model RM, CodeModel::Model CM,
                                       CodeGenOpt::Level OL)
    : ARMTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, false) {}

void ThumbTargetMachine::anchor() { }

ThumbTargetMachine::ThumbTargetMachine(const Target &T, StringRef TT,
                                       StringRef CPU, StringRef FS,
                                       const TargetOptions &Options,
                                       Reloc::Model RM, CodeModel::Model CM,
                                       CodeGenOpt::Level OL, bool isLittle)
    : ARMBaseTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL,
                           isLittle) {
  initAsmInfo();
}

void ThumbLETargetMachine::anchor() { }

ThumbLETargetMachine::ThumbLETargetMachine(const Target &T, StringRef TT,
                                           StringRef CPU, StringRef FS,
                                           const TargetOptions &Options,
                                           Reloc::Model RM, CodeModel::Model CM,
                                           CodeGenOpt::Level OL)
    : ThumbTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, true) {}

void ThumbBETargetMachine::anchor() { }

ThumbBETargetMachine::ThumbBETargetMachine(const Target &T, StringRef TT,
                                           StringRef CPU, StringRef FS,
                                           const TargetOptions &Options,
                                           Reloc::Model RM, CodeModel::Model CM,
                                           CodeGenOpt::Level OL)
    : ThumbTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, false) {}

namespace {
/// ARM Code Generator Pass Configuration Options.
class ARMPassConfig : public TargetPassConfig {
public:
  ARMPassConfig(ARMBaseTargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  ARMBaseTargetMachine &getARMTargetMachine() const {
    return getTM<ARMBaseTargetMachine>();
  }

  const ARMSubtarget &getARMSubtarget() const {
    return *getARMTargetMachine().getSubtargetImpl();
  }

  void addIRPasses() override;
  bool addPreISel() override;
  bool addInstSelector() override;
  void addPreRegAlloc() override;
  void addPreSched2() override;
  void addPreEmitPass() override;
};
} // namespace

TargetPassConfig *ARMBaseTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new ARMPassConfig(this, PM);
}

void ARMPassConfig::addIRPasses() {
  if (TM->Options.ThreadModel == ThreadModel::Single)
    addPass(createLowerAtomicPass());
  else
    addPass(createAtomicExpandPass(TM));

  // Cmpxchg instructions are often used with a subsequent comparison to
  // determine whether it succeeded. We can exploit existing control-flow in
  // ldrex/strex loops to simplify this, but it needs tidying up.
  const ARMSubtarget *Subtarget = &getARMSubtarget();
  if (Subtarget->hasAnyDataBarrier() && !Subtarget->isThumb1Only())
    if (TM->getOptLevel() != CodeGenOpt::None && EnableAtomicTidy)
      addPass(createCFGSimplificationPass());

  TargetPassConfig::addIRPasses();
}

bool ARMPassConfig::addPreISel() {
  if (TM->getOptLevel() != CodeGenOpt::None)
    addPass(createGlobalMergePass(TM));

  return false;
}

bool ARMPassConfig::addInstSelector() {
  addPass(createARMISelDag(getARMTargetMachine(), getOptLevel()));

  const ARMSubtarget *Subtarget = &getARMSubtarget();
  if (Subtarget->isTargetELF() && !Subtarget->isThumb1Only() &&
      TM->Options.EnableFastISel)
    addPass(createARMGlobalBaseRegPass());
  return false;
}

void ARMPassConfig::addPreRegAlloc() {
  if (getOptLevel() != CodeGenOpt::None)
    addPass(createARMLoadStoreOptimizationPass(true), false);
  if (getOptLevel() != CodeGenOpt::None && getARMSubtarget().isCortexA9())
    addPass(createMLxExpansionPass(), false);
  // Since the A15SDOptimizer pass can insert VDUP instructions, it can only be
  // enabled when NEON is available.
  if (getOptLevel() != CodeGenOpt::None && getARMSubtarget().isCortexA15() &&
    getARMSubtarget().hasNEON() && !DisableA15SDOptimization) {
    addPass(createA15SDOptimizerPass());
  }
}

void ARMPassConfig::addPreSched2() {
  if (getOptLevel() != CodeGenOpt::None) {
    addPass(createARMLoadStoreOptimizationPass(), false);

    if (getARMSubtarget().hasNEON())
      addPass(createExecutionDependencyFixPass(&ARM::DPRRegClass), false);
  }

  // Expand some pseudo instructions into multiple instructions to allow
  // proper scheduling.
  addPass(createARMExpandPseudoPass(), false);

  if (getOptLevel() != CodeGenOpt::None) {
    if (!getARMSubtarget().isThumb1Only()) {
      // in v8, IfConversion depends on Thumb instruction widths
      if (getARMSubtarget().restrictIT() &&
          !getARMSubtarget().prefers32BitThumb())
        addPass(createThumb2SizeReductionPass(), false);
      addPass(&IfConverterID, false);
    }
  }
  if (getARMSubtarget().isThumb2())
    addPass(createThumb2ITBlockPass());
}

void ARMPassConfig::addPreEmitPass() {
  if (getARMSubtarget().isThumb2()) {
    if (!getARMSubtarget().prefers32BitThumb())
      addPass(createThumb2SizeReductionPass(), false);

    // Constant island pass work on unbundled instructions.
    addPass(&UnpackMachineBundlesID, false);
  }

  addPass(createARMOptimizeBarriersPass(), false);
  addPass(createARMConstantIslandPass());
}
