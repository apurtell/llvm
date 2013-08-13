//===---- MipsISelDAGToDAG.h - A Dag to Dag Inst Selector for Mips --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the MIPS target.
//
//===----------------------------------------------------------------------===//

#ifndef MIPSISELDAGTODAG_H
#define MIPSISELDAGTODAG_H

#include "Mips.h"
#include "MipsSubtarget.h"
#include "MipsTargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"

//===----------------------------------------------------------------------===//
// Instruction Selector Implementation
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// MipsDAGToDAGISel - MIPS specific code to select MIPS machine
// instructions for SelectionDAG operations.
//===----------------------------------------------------------------------===//
namespace llvm {

class MipsDAGToDAGISel : public SelectionDAGISel {
public:
  explicit MipsDAGToDAGISel(MipsTargetMachine &TM)
    : SelectionDAGISel(TM), Subtarget(TM.getSubtarget<MipsSubtarget>()) {}

  // Pass Name
  virtual const char *getPassName() const {
    return "MIPS DAG->DAG Pattern Instruction Selection";
  }

  virtual bool runOnMachineFunction(MachineFunction &MF);

protected:
  SDNode *getGlobalBaseReg();

  /// Keep a pointer to the MipsSubtarget around so that we can make the right
  /// decision when generating code for different targets.
  const MipsSubtarget &Subtarget;

private:
  // Include the pieces autogenerated from the target description.
  #include "MipsGenDAGISel.inc"

  // Complex Pattern.
  /// (reg + imm).
  virtual bool selectAddrRegImm(SDValue Addr, SDValue &Base,
                                SDValue &Offset) const;

  /// Fall back on this function if all else fails.
  virtual bool selectAddrDefault(SDValue Addr, SDValue &Base,
                                 SDValue &Offset) const;

  /// Match integer address pattern.
  virtual bool selectIntAddr(SDValue Addr, SDValue &Base,
                             SDValue &Offset) const;

  virtual bool selectIntAddrMM(SDValue Addr, SDValue &Base,
                               SDValue &Offset) const;

  virtual bool selectAddr16(SDNode *Parent, SDValue N, SDValue &Base,
                            SDValue &Offset, SDValue &Alias);

  virtual SDNode *Select(SDNode *N);

  virtual std::pair<bool, SDNode*> selectNode(SDNode *Node) = 0;

  // getImm - Return a target constant with the specified value.
  inline SDValue getImm(const SDNode *Node, uint64_t Imm) {
    return CurDAG->getTargetConstant(Imm, Node->getValueType(0));
  }

  virtual void processFunctionAfterISel(MachineFunction &MF) = 0;

  virtual bool SelectInlineAsmMemoryOperand(const SDValue &Op,
                                            char ConstraintCode,
                                            std::vector<SDValue> &OutOps);
};

/// createMipsISelDag - This pass converts a legalized DAG into a
/// MIPS-specific DAG, ready for instruction scheduling.
FunctionPass *createMipsISelDag(MipsTargetMachine &TM);

}

#endif
