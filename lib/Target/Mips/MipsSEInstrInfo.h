//===-- MipsSEInstrInfo.h - Mips32/64 Instruction Information ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Mips32/64 implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef MIPSSEINSTRUCTIONINFO_H
#define MIPSSEINSTRUCTIONINFO_H

#include "MipsInstrInfo.h"
#include "MipsSERegisterInfo.h"

namespace llvm {

class MipsSEInstrInfo : public MipsInstrInfo {
  const MipsSERegisterInfo RI;
  bool IsN64;

public:
  explicit MipsSEInstrInfo(MipsTargetMachine &TM);

  virtual const MipsRegisterInfo &getRegisterInfo() const;

  /// isLoadFromStackSlot - If the specified machine instruction is a direct
  /// load from a stack slot, return the virtual or physical register number of
  /// the destination along with the FrameIndex of the loaded stack slot.  If
  /// not, return 0.  This predicate must return 0 if the instruction has
  /// any side effects other than loading from the stack slot.
  virtual unsigned isLoadFromStackSlot(const MachineInstr *MI,
                                       int &FrameIndex) const;

  /// isStoreToStackSlot - If the specified machine instruction is a direct
  /// store to a stack slot, return the virtual or physical register number of
  /// the source reg along with the FrameIndex of the loaded stack slot.  If
  /// not, return 0.  This predicate must return 0 if the instruction has
  /// any side effects other than storing to the stack slot.
  virtual unsigned isStoreToStackSlot(const MachineInstr *MI,
                                      int &FrameIndex) const;

  virtual void copyPhysReg(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, DebugLoc DL,
                           unsigned DestReg, unsigned SrcReg,
                           bool KillSrc) const;

  virtual void storeRegToStack(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MI,
                               unsigned SrcReg, bool isKill, int FrameIndex,
                               const TargetRegisterClass *RC,
                               const TargetRegisterInfo *TRI,
                               int64_t Offset) const;

  virtual void loadRegFromStack(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MI,
                                unsigned DestReg, int FrameIndex,
                                const TargetRegisterClass *RC,
                                const TargetRegisterInfo *TRI,
                                int64_t Offset) const;

  virtual bool expandPostRAPseudo(MachineBasicBlock::iterator MI) const;

  virtual unsigned getOppositeBranchOpc(unsigned Opc) const;

  /// Adjust SP by Amount bytes.
  void adjustStackPtr(unsigned SP, int64_t Amount, MachineBasicBlock &MBB,
                      MachineBasicBlock::iterator I) const;

  /// Emit a series of instructions to load an immediate. If NewImm is a
  /// non-NULL parameter, the last instruction is not emitted, but instead
  /// its immediate operand is returned in NewImm.
  unsigned loadImmediate(int64_t Imm, MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator II, DebugLoc DL,
                         unsigned *NewImm) const;

private:
  virtual unsigned getAnalyzableBrOpc(unsigned Opc) const;

  void expandRetRA(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                   unsigned Opc) const;

  std::pair<bool, bool> compareOpndSize(unsigned Opc,
                                        const MachineFunction &MF) const;

  /// Expand pseudo Int-to-FP conversion instructions.
  ///
  /// For example, the following pseudo instruction
  ///  PseudoCVT_D32_W D2, A5
  /// gets expanded into these two instructions:
  ///  MTC1 F4, A5
  ///  CVT_D32_W D2, F4
  ///
  /// We do this expansion post-RA to avoid inserting a floating point copy
  /// instruction between MTC1 and CVT_D32_W.
  void expandCvtFPInt(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                      unsigned CvtOpc, unsigned MovOpc, bool IsI64) const;

  void expandExtractElementF64(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator I, bool FP64) const;
  void expandBuildPairF64(MachineBasicBlock &MBB,
                          MachineBasicBlock::iterator I, bool FP64) const;
  void expandEhReturn(MachineBasicBlock &MBB,
                      MachineBasicBlock::iterator I) const;
};

}

#endif
