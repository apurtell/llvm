//===-- PPCCodeEmitter.cpp - JIT Code Emitter for PowerPC -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the PowerPC 32-bit CodeEmitter and associated machinery to
// JIT-compile bitcode to native PowerPC.
//
//===----------------------------------------------------------------------===//

#include "PPC.h"
#include "PPCRelocations.h"
#include "PPCTargetMachine.h"
#include "llvm/CodeGen/JITCodeEmitter.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"
using namespace llvm;

namespace {
  class PPCCodeEmitter : public MachineFunctionPass {
    TargetMachine &TM;
    JITCodeEmitter &MCE;
    MachineModuleInfo *MMI;
    
    void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<MachineModuleInfo>();
      MachineFunctionPass::getAnalysisUsage(AU);
    }
    
    static char ID;
    
    /// MovePCtoLROffset - When/if we see a MovePCtoLR instruction, we record
    /// its address in the function into this pointer.
    void *MovePCtoLROffset;
  public:
    
    PPCCodeEmitter(TargetMachine &tm, JITCodeEmitter &mce)
      : MachineFunctionPass(ID), TM(tm), MCE(mce) {}

    /// getBinaryCodeForInstr - This function, generated by the
    /// CodeEmitterGenerator using TableGen, produces the binary encoding for
    /// machine instructions.
    uint64_t getBinaryCodeForInstr(const MachineInstr &MI) const;

    
    MachineRelocation GetRelocation(const MachineOperand &MO,
                                    unsigned RelocID) const;
    
    /// getMachineOpValue - evaluates the MachineOperand of a given MachineInstr
    unsigned getMachineOpValue(const MachineInstr &MI,
                               const MachineOperand &MO) const;

    unsigned get_crbitm_encoding(const MachineInstr &MI, unsigned OpNo) const;
    unsigned getDirectBrEncoding(const MachineInstr &MI, unsigned OpNo) const;
    unsigned getCondBrEncoding(const MachineInstr &MI, unsigned OpNo) const;
    unsigned getAbsDirectBrEncoding(const MachineInstr &MI,
                                    unsigned OpNo) const;
    unsigned getAbsCondBrEncoding(const MachineInstr &MI, unsigned OpNo) const;

    unsigned getImm16Encoding(const MachineInstr &MI, unsigned OpNo) const;
    unsigned getMemRIEncoding(const MachineInstr &MI, unsigned OpNo) const;
    unsigned getMemRIXEncoding(const MachineInstr &MI, unsigned OpNo) const;
    unsigned getTLSRegEncoding(const MachineInstr &MI, unsigned OpNo) const;
    unsigned getTLSCallEncoding(const MachineInstr &MI, unsigned OpNo) const;

    const char *getPassName() const { return "PowerPC Machine Code Emitter"; }

    /// runOnMachineFunction - emits the given MachineFunction to memory
    ///
    bool runOnMachineFunction(MachineFunction &MF);

    /// emitBasicBlock - emits the given MachineBasicBlock to memory
    ///
    void emitBasicBlock(MachineBasicBlock &MBB);
  };
}

char PPCCodeEmitter::ID = 0;

/// createPPCCodeEmitterPass - Return a pass that emits the collected PPC code
/// to the specified MCE object.
FunctionPass *llvm::createPPCJITCodeEmitterPass(PPCTargetMachine &TM,
                                                JITCodeEmitter &JCE) {
  return new PPCCodeEmitter(TM, JCE);
}

bool PPCCodeEmitter::runOnMachineFunction(MachineFunction &MF) {
  assert((MF.getTarget().getRelocationModel() != Reloc::Default ||
          MF.getTarget().getRelocationModel() != Reloc::Static) &&
         "JIT relocation model must be set to static or default!");

  MMI = &getAnalysis<MachineModuleInfo>();
  MCE.setModuleInfo(MMI);
  do {
    MovePCtoLROffset = 0;
    MCE.startFunction(MF);
    for (MachineFunction::iterator BB = MF.begin(), E = MF.end(); BB != E; ++BB)
      emitBasicBlock(*BB);
  } while (MCE.finishFunction(MF));

  return false;
}

void PPCCodeEmitter::emitBasicBlock(MachineBasicBlock &MBB) {
  MCE.StartMachineBasicBlock(&MBB);

  for (MachineBasicBlock::iterator I = MBB.begin(), E = MBB.end(); I != E; ++I){
    const MachineInstr &MI = *I;
    MCE.processDebugLoc(MI.getDebugLoc(), true);
    switch (MI.getOpcode()) {
    default:
      MCE.emitWordBE(getBinaryCodeForInstr(MI));
      break;
    case TargetOpcode::PROLOG_LABEL:
    case TargetOpcode::EH_LABEL:
      MCE.emitLabel(MI.getOperand(0).getMCSymbol());
      break;
    case TargetOpcode::IMPLICIT_DEF:
    case TargetOpcode::KILL:
      break; // pseudo opcode, no side effects
    case PPC::MovePCtoLR:
    case PPC::MovePCtoLR8:
      assert(TM.getRelocationModel() == Reloc::PIC_);
      MovePCtoLROffset = (void*)MCE.getCurrentPCValue();
      MCE.emitWordBE(0x48000005);   // bl 1
      break;
    }
    MCE.processDebugLoc(MI.getDebugLoc(), false);
  }
}

unsigned PPCCodeEmitter::get_crbitm_encoding(const MachineInstr &MI,
                                             unsigned OpNo) const {
  const MachineOperand &MO = MI.getOperand(OpNo);
  assert((MI.getOpcode() == PPC::MTCRF || MI.getOpcode() == PPC::MTCRF8 ||
            MI.getOpcode() == PPC::MFOCRF) &&
         (MO.getReg() >= PPC::CR0 && MO.getReg() <= PPC::CR7));
  return 0x80 >> TM.getRegisterInfo()->getEncodingValue(MO.getReg());
}

MachineRelocation PPCCodeEmitter::GetRelocation(const MachineOperand &MO, 
                                                unsigned RelocID) const {
  // If in PIC mode, we need to encode the negated address of the
  // 'movepctolr' into the unrelocated field.  After relocation, we'll have
  // &gv-&movepctolr-4 in the imm field.  Once &movepctolr is added to the imm
  // field, we get &gv.  This doesn't happen for branch relocations, which are
  // always implicitly pc relative.
  intptr_t Cst = 0;
  if (TM.getRelocationModel() == Reloc::PIC_) {
    assert(MovePCtoLROffset && "MovePCtoLR not seen yet?");
    Cst = -(intptr_t)MovePCtoLROffset - 4;
  }
  
  if (MO.isGlobal())
    return MachineRelocation::getGV(MCE.getCurrentPCOffset(), RelocID,
                                    const_cast<GlobalValue *>(MO.getGlobal()),
                                    Cst, isa<Function>(MO.getGlobal()));
  if (MO.isSymbol())
    return MachineRelocation::getExtSym(MCE.getCurrentPCOffset(),
                                        RelocID, MO.getSymbolName(), Cst);
  if (MO.isCPI())
    return MachineRelocation::getConstPool(MCE.getCurrentPCOffset(),
                                           RelocID, MO.getIndex(), Cst);

  if (MO.isMBB())
    return MachineRelocation::getBB(MCE.getCurrentPCOffset(),
                                    RelocID, MO.getMBB());
  
  assert(MO.isJTI());
  return MachineRelocation::getJumpTable(MCE.getCurrentPCOffset(),
                                         RelocID, MO.getIndex(), Cst);
}

unsigned PPCCodeEmitter::getDirectBrEncoding(const MachineInstr &MI,
                                             unsigned OpNo) const {
  const MachineOperand &MO = MI.getOperand(OpNo);
  if (MO.isReg() || MO.isImm()) return getMachineOpValue(MI, MO);
  
  MCE.addRelocation(GetRelocation(MO, PPC::reloc_pcrel_bx));
  return 0;
}

unsigned PPCCodeEmitter::getCondBrEncoding(const MachineInstr &MI,
                                           unsigned OpNo) const {
  const MachineOperand &MO = MI.getOperand(OpNo);
  MCE.addRelocation(GetRelocation(MO, PPC::reloc_pcrel_bcx));
  return 0;
}

unsigned PPCCodeEmitter::getAbsDirectBrEncoding(const MachineInstr &MI,
                                                unsigned OpNo) const {
  const MachineOperand &MO = MI.getOperand(OpNo);
  if (MO.isReg() || MO.isImm()) return getMachineOpValue(MI, MO);

  llvm_unreachable("Absolute branch relocations unsupported on the old JIT.");
}

unsigned PPCCodeEmitter::getAbsCondBrEncoding(const MachineInstr &MI,
                                              unsigned OpNo) const {
  llvm_unreachable("Absolute branch relocations unsupported on the old JIT.");
}

unsigned PPCCodeEmitter::getImm16Encoding(const MachineInstr &MI,
                                          unsigned OpNo) const {
  const MachineOperand &MO = MI.getOperand(OpNo);
  if (MO.isReg() || MO.isImm()) return getMachineOpValue(MI, MO);

  unsigned RelocID;
  switch (MO.getTargetFlags() & PPCII::MO_ACCESS_MASK) {
    default: llvm_unreachable("Unsupported target operand flags!");
    case PPCII::MO_LO: RelocID = PPC::reloc_absolute_low; break;
    case PPCII::MO_HA: RelocID = PPC::reloc_absolute_high; break;
  }

  MCE.addRelocation(GetRelocation(MO, RelocID));
  return 0;
}

unsigned PPCCodeEmitter::getMemRIEncoding(const MachineInstr &MI,
                                          unsigned OpNo) const {
  // Encode (imm, reg) as a memri, which has the low 16-bits as the
  // displacement and the next 5 bits as the register #.
  assert(MI.getOperand(OpNo+1).isReg());
  unsigned RegBits = getMachineOpValue(MI, MI.getOperand(OpNo+1)) << 16;
  
  const MachineOperand &MO = MI.getOperand(OpNo);
  if (MO.isImm())
    return (getMachineOpValue(MI, MO) & 0xFFFF) | RegBits;
  
  // Add a fixup for the displacement field.
  MCE.addRelocation(GetRelocation(MO, PPC::reloc_absolute_low));
  return RegBits;
}

unsigned PPCCodeEmitter::getMemRIXEncoding(const MachineInstr &MI,
                                           unsigned OpNo) const {
  // Encode (imm, reg) as a memrix, which has the low 14-bits as the
  // displacement and the next 5 bits as the register #.
  assert(MI.getOperand(OpNo+1).isReg());
  unsigned RegBits = getMachineOpValue(MI, MI.getOperand(OpNo+1)) << 14;
  
  const MachineOperand &MO = MI.getOperand(OpNo);
  if (MO.isImm())
    return ((getMachineOpValue(MI, MO) >> 2) & 0x3FFF) | RegBits;
  
  MCE.addRelocation(GetRelocation(MO, PPC::reloc_absolute_low_ix));
  return RegBits;
}


unsigned PPCCodeEmitter::getTLSRegEncoding(const MachineInstr &MI,
                                           unsigned OpNo) const {
  llvm_unreachable("TLS not supported on the old JIT.");
  return 0;
}

unsigned PPCCodeEmitter::getTLSCallEncoding(const MachineInstr &MI,
                                            unsigned OpNo) const {
  llvm_unreachable("TLS not supported on the old JIT.");
  return 0;
}

unsigned PPCCodeEmitter::getMachineOpValue(const MachineInstr &MI,
                                           const MachineOperand &MO) const {

  if (MO.isReg()) {
    // MTCRF/MFOCRF should go through get_crbitm_encoding for the CR operand.
    // The GPR operand should come through here though.
    assert((MI.getOpcode() != PPC::MTCRF && MI.getOpcode() != PPC::MTCRF8 &&
             MI.getOpcode() != PPC::MFOCRF) ||
           MO.getReg() < PPC::CR0 || MO.getReg() > PPC::CR7);
    return TM.getRegisterInfo()->getEncodingValue(MO.getReg());
  }
  
  assert(MO.isImm() &&
         "Relocation required in an instruction that we cannot encode!");
  return MO.getImm();
}

#include "PPCGenCodeEmitter.inc"
