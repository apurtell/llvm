//===- X86RegisterInfo.cpp - X86 Register Information -----------*- C++ -*-===//
//
// This file contains the X86 implementation of the MRegisterInfo class.  This
// file is responsible for the frame pointer elimination optimization on X86.
//
//===----------------------------------------------------------------------===//

#include "X86.h"
#include "X86RegisterInfo.h"
#include "X86InstrBuilder.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/FunctionFrameInfo.h"
#include "Support/CommandLine.h"

namespace {
  cl::opt<bool>
  NoFPElim("no-fp-elim",
	   cl::desc("Disable frame pointer elimination optimization"));
}

static unsigned getIdx(const TargetRegisterClass *RC) {
  switch (RC->getSize()) {
  default: assert(0 && "Invalid data size!");
  case 1:  return 0;
  case 2:  return 1;
  case 4:  return 2;
  case 10: return 3;
  }
}

void X86RegisterInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
					  MachineBasicBlock::iterator &MBBI,
					  unsigned SrcReg, int FrameIdx,
					  const TargetRegisterClass *RC) const {
  static const unsigned Opcode[] =
    { X86::MOVrm8, X86::MOVrm16, X86::MOVrm32, X86::FSTPr80 };
  MachineInstr *MI = addFrameReference(BuildMI(Opcode[getIdx(RC)], 5),
				       FrameIdx).addReg(SrcReg);
  MBBI = MBB.insert(MBBI, MI)+1;
}

void X86RegisterInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
					   MachineBasicBlock::iterator &MBBI,
					   unsigned DestReg, int FrameIdx,
					   const TargetRegisterClass *RC) const{
  static const unsigned Opcode[] =
    { X86::MOVmr8, X86::MOVmr16, X86::MOVmr32, X86::FLDr80 };
  MachineInstr *MI = addFrameReference(BuildMI(Opcode[getIdx(RC)], 4, DestReg),
				       FrameIdx);
  MBBI = MBB.insert(MBBI, MI)+1;
}

void X86RegisterInfo::copyRegToReg(MachineBasicBlock &MBB,
				   MachineBasicBlock::iterator &MBBI,
				   unsigned DestReg, unsigned SrcReg,
				   const TargetRegisterClass *RC) const {
  static const unsigned Opcode[] =
    { X86::MOVrr8, X86::MOVrr16, X86::MOVrr32, X86::FpMOV };
  MachineInstr *MI = BuildMI(Opcode[getIdx(RC)],1,DestReg).addReg(SrcReg);
  MBBI = MBB.insert(MBBI, MI)+1;
}

const unsigned* X86RegisterInfo::getCalleeSaveRegs() const {
  static const unsigned CalleeSaveRegs[] = {
    X86::ESI, X86::EDI, X86::EBX, X86::EBP, 0
  };
  return CalleeSaveRegs;
}


//===----------------------------------------------------------------------===//
// Stack Frame Processing methods
//===----------------------------------------------------------------------===//

// hasFP - Return true if the specified function should have a dedicated frame
// pointer register.  This is true if the function has variable sized allocas or
// if frame pointer elimination is disabled.
//
static bool hasFP(MachineFunction &MF) {
  return NoFPElim || MF.getFrameInfo()->hasVarSizedObjects();
}

// hasSPAdjust - Return true if this function has ESP adjustment instructions in
// the prolog and epilog which allocate local stack space.  This is neccesary
// because we elide these instructions if there are no function calls in the
// current function (ie, this is a leaf function).  In this case, we can refer
// beyond the stack pointer because we know that nothing will trample on that
// part of the stack.
//
static bool hasSPAdjust(MachineFunction &MF) {
  assert(!hasFP(MF) && "Can only eliminate SP adjustment if no frame-pointer!");
  return MF.getFrameInfo()->hasCalls();
}

void X86RegisterInfo::eliminateCallFramePseudoInstr(MachineFunction &MF,
						    MachineBasicBlock &MBB,
	                                 MachineBasicBlock::iterator &I) const {
  MachineInstr *New = 0, *Old = *I;;
  if (hasFP(MF)) {
    // If we have a frame pointer, turn the adjcallstackup instruction into a
    // 'sub ESP, <amt>' and the adjcallstackdown instruction into 'add ESP,
    // <amt>'
    unsigned Amount = Old->getOperand(0).getImmedValue();
    if (Amount != 0) {
      if (Old->getOpcode() == X86::ADJCALLSTACKDOWN) {
	New=BuildMI(X86::SUBri32, 2, X86::ESP).addReg(X86::ESP).addZImm(Amount);
      } else {
	assert(Old->getOpcode() == X86::ADJCALLSTACKUP);
	New=BuildMI(X86::ADDri32, 2, X86::ESP).addReg(X86::ESP).addZImm(Amount);
      }
    }
  }

  if (New)
    *I = New;        // Replace the pseudo instruction with a new instruction...
  else
    I = MBB.erase(I);// Just delete the pseudo instruction...
  delete Old;
}

void X86RegisterInfo::eliminateFrameIndex(MachineFunction &MF,
					MachineBasicBlock::iterator &II) const {
  unsigned i = 3;
  MachineInstr &MI = **II;
  while (!MI.getOperand(i).isFrameIndex()) {
    ++i;
    assert(i < MI.getNumOperands() && "Instr doesn't have FrameIndex operand!");
  }

  // This must be part of a four operand memory reference.  Replace the
  // FrameIndex with the offset and the base register with EBP.
  MI.SetMachineOperandReg(i-3, hasFP(MF) ? X86::EBP : X86::ESP);

  // Now replace the frame index itself with the offset from EBP.
  int FrameIndex = MI.getOperand(i).getFrameIndex();
  int Offset = MF.getFrameInfo()->getObjectOffset(FrameIndex);

  if (!hasFP(MF) && hasSPAdjust(MF)) {
    const FunctionFrameInfo *FFI = MF.getFrameInfo();
    Offset += FFI->getStackSize() + FFI->getMaxCallFrameSize();
  }

  MI.SetMachineOperandConst(i, MachineOperand::MO_SignExtendedImmed, Offset);
}

void X86RegisterInfo::processFunctionBeforeFrameFinalized(MachineFunction &MF)
  const {
  if (hasFP(MF)) {
    // Create a frame entry for the EBP register that must be saved.
    int FrameIdx = MF.getFrameInfo()->CreateStackObject(4, 4);
    assert(FrameIdx == MF.getFrameInfo()->getObjectIndexEnd()-1 &&
	   "Slot for EBP register must be last in order to be found!");
  }
}

void X86RegisterInfo::emitPrologue(MachineFunction &MF) const {
  MachineBasicBlock &MBB = MF.front();   // Prolog goes in entry BB
  MachineBasicBlock::iterator MBBI = MBB.begin();
  const FunctionFrameInfo *FFI = MF.getFrameInfo();
  MachineInstr *MI;

  // Get the number of bytes to allocate from the FrameInfo
  unsigned NumBytes = FFI->getStackSize();
  if (hasFP(MF)) {
    // Get the offset of the stack slot for the EBP register... which is
    // guaranteed to be the last slot by processFunctionBeforeFrameFinalized.
    int EBPOffset = FFI->getObjectOffset(FFI->getObjectIndexEnd()-1);

    MI = addRegOffset(BuildMI(X86::MOVrm32, 5),    // mov [ESP-<offset>], EBP
		      X86::ESP, EBPOffset).addReg(X86::EBP);
    MBBI = MBB.insert(MBBI, MI)+1;
    
    MI = BuildMI(X86::MOVrr32, 2, X86::EBP).addReg(X86::ESP);
    MBBI = MBB.insert(MBBI, MI)+1;
  } else {
    // If we don't have a frame pointer, and the function contains no call sites
    // (it's a leaf function), we don't have to emit ANY stack adjustment
    // instructions at all, we can just refer to the area beyond the stack
    // pointer.  This can be important for small functions.
    //
    if (!hasSPAdjust(MF)) return;

    // When we have no frame pointer, we reserve argument space for call sites
    // in the function immediately on entry to the current function.  This
    // eliminates the need for add/sub ESP brackets around call sites.
    //
    NumBytes += FFI->getMaxCallFrameSize();
  }

  if (NumBytes) {
    // adjust stack pointer: ESP -= numbytes
    MI  = BuildMI(X86::SUBri32, 2, X86::ESP).addReg(X86::ESP).addZImm(NumBytes);
    MBBI = 1+MBB.insert(MBBI, MI);
  }
}

void X86RegisterInfo::emitEpilogue(MachineFunction &MF,
				   MachineBasicBlock &MBB) const {
  const FunctionFrameInfo *FFI = MF.getFrameInfo();
  MachineBasicBlock::iterator MBBI = MBB.end()-1;
  MachineInstr *MI;
  assert((*MBBI)->getOpcode() == X86::RET &&
         "Can only insert epilog into returning blocks");

  if (hasFP(MF)) {
    // Get the offset of the stack slot for the EBP register... which is
    // guaranteed to be the last slot by processFunctionBeforeFrameFinalized.
    int EBPOffset = FFI->getObjectOffset(FFI->getObjectIndexEnd()-1);
    
    // mov ESP, EBP
    MI = BuildMI(X86::MOVrr32, 1,X86::ESP).addReg(X86::EBP);
    MBBI = 1+MBB.insert(MBBI, MI);

    // mov EBP, [ESP-<offset>]
    MI = addRegOffset(BuildMI(X86::MOVmr32, 5, X86::EBP), X86::ESP, EBPOffset);
    MBBI = 1+MBB.insert(MBBI, MI);
  } else {
    if (!hasSPAdjust(MF)) return;

    // Get the number of bytes allocated from the FrameInfo...
    unsigned NumBytes = FFI->getStackSize();
    NumBytes += FFI->getMaxCallFrameSize();

    if (NumBytes) {    // adjust stack pointer back: ESP += numbytes
      MI =BuildMI(X86::ADDri32, 2, X86::ESP).addReg(X86::ESP).addZImm(NumBytes);
      MBBI = 1+MBB.insert(MBBI, MI);
    }
  }
}


//===----------------------------------------------------------------------===//
// Register Class Implementation Code
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//   8 Bit Integer Registers
//
namespace {
  const unsigned ByteRegClassRegs[] = {
    X86::AL, X86::CL, X86::DL, X86::BL, X86::AH, X86::CH, X86::DH, X86::BH,
  };

  TargetRegisterClass X86ByteRegisterClassInstance(1, 1, ByteRegClassRegs,
 ByteRegClassRegs+sizeof(ByteRegClassRegs)/sizeof(ByteRegClassRegs[0]));

//===----------------------------------------------------------------------===//
//   16 Bit Integer Registers
//
  const unsigned ShortRegClassRegs[] = {
    X86::AX, X86::CX, X86::DX, X86::BX, X86::SI, X86::DI, X86::BP, X86::SP
  };

  struct R16CL : public TargetRegisterClass {
    R16CL():TargetRegisterClass(2, 2, ShortRegClassRegs, ShortRegClassRegs+8) {}
    iterator allocation_order_end(MachineFunction &MF)   const {
      if (hasFP(MF))     // Does the function dedicate EBP to being a frame ptr?
	return end()-2;  // Don't allocate SP or BP
      else
	return end()-1;  // Don't allocate SP
    }
  } X86ShortRegisterClassInstance;

//===----------------------------------------------------------------------===//
//   32 Bit Integer Registers
//
  const unsigned IntRegClassRegs[] = {
    X86::EAX, X86::ECX, X86::EDX, X86::EBX,
    X86::ESI, X86::EDI, X86::EBP, X86::ESP
  };

  struct R32CL : public TargetRegisterClass {
    R32CL() : TargetRegisterClass(4, 4, IntRegClassRegs, IntRegClassRegs+8) {}
    iterator allocation_order_end(MachineFunction &MF)   const {
      if (hasFP(MF))     // Does the function dedicate EBP to being a frame ptr?
	return end()-2;  // Don't allocate ESP or EBP
      else
	return end()-1;  // Don't allocate ESP
    }
  } X86IntRegisterClassInstance;

//===----------------------------------------------------------------------===//
//   Pseudo Floating Point Registers
//
  const unsigned PFPRegClassRegs[] = {
#define PFP(ENUM, NAME, FLAGS, TSFLAGS, ALIAS_SET) X86::ENUM,
#include "X86RegisterInfo.def"
  };

  TargetRegisterClass X86FPRegisterClassInstance(10, 4, PFPRegClassRegs,
      PFPRegClassRegs+sizeof(PFPRegClassRegs)/sizeof(PFPRegClassRegs[0]));

//===----------------------------------------------------------------------===//
// Register class array...
//
  const TargetRegisterClass * const X86RegClasses[] = {
    &X86ByteRegisterClassInstance,
    &X86ShortRegisterClassInstance,
    &X86IntRegisterClassInstance,
    &X86FPRegisterClassInstance,
  };
}


// Create static lists to contain register alias sets...
#define ALIASLIST(NAME, ...) \
  static const unsigned NAME[] = { __VA_ARGS__ };
#include "X86RegisterInfo.def"


// X86Regs - Turn the X86RegisterInfo.def file into a bunch of register
// descriptors
//
static const MRegisterDesc X86Regs[] = {
#define R(ENUM, NAME, FLAGS, TSFLAGS, ALIAS_SET) \
         { NAME, ALIAS_SET, FLAGS, TSFLAGS },
#include "X86RegisterInfo.def"
};

X86RegisterInfo::X86RegisterInfo()
  : MRegisterInfo(X86Regs, sizeof(X86Regs)/sizeof(X86Regs[0]),
                  X86RegClasses,
                  X86RegClasses+sizeof(X86RegClasses)/sizeof(X86RegClasses[0]),
		  X86::ADJCALLSTACKDOWN, X86::ADJCALLSTACKUP) {
}



const TargetRegisterClass*
X86RegisterInfo::getRegClassForType(const Type* Ty) const {
  switch (Ty->getPrimitiveID()) {
  default:                assert(0 && "Invalid type to getClass!");
  case Type::BoolTyID:
  case Type::SByteTyID:
  case Type::UByteTyID:   return &X86ByteRegisterClassInstance;
  case Type::ShortTyID:
  case Type::UShortTyID:  return &X86ShortRegisterClassInstance;
  case Type::LongTyID:    // FIXME: Longs are not handled yet!
  case Type::ULongTyID:   // FIXME: Treat these like ints, this is bogus!
    
  case Type::IntTyID:
  case Type::UIntTyID:
  case Type::PointerTyID: return &X86IntRegisterClassInstance;
    
  case Type::FloatTyID:
  case Type::DoubleTyID: return &X86FPRegisterClassInstance;
  }
}
