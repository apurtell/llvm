//===-- MipsISelDAGToDAG.cpp - A dag to dag inst selector for Mips --------===//
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

#define DEBUG_TYPE "mips-isel"
#include "Mips.h"
#include "MipsISelLowering.h"
#include "MipsMachineFunction.h"
#include "MipsRegisterInfo.h"
#include "MipsSubtarget.h"
#include "MipsTargetMachine.h"
#include "llvm/GlobalValue.h"
#include "llvm/Instructions.h"
#include "llvm/Intrinsics.h"
#include "llvm/Support/CFG.h"
#include "llvm/Type.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
using namespace llvm;

//===----------------------------------------------------------------------===//
// Instruction Selector Implementation
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// MipsDAGToDAGISel - MIPS specific code to select MIPS machine
// instructions for SelectionDAG operations.
//===----------------------------------------------------------------------===//
namespace {

class VISIBILITY_HIDDEN MipsDAGToDAGISel : public SelectionDAGISel {

  /// TM - Keep a reference to MipsTargetMachine.
  MipsTargetMachine &TM;

  /// Subtarget - Keep a pointer to the MipsSubtarget around so that we can
  /// make the right decision when generating code for different targets.
  const MipsSubtarget &Subtarget;
 
public:
  explicit MipsDAGToDAGISel(MipsTargetMachine &tm) :
  SelectionDAGISel(*tm.getTargetLowering()),
  TM(tm), Subtarget(tm.getSubtarget<MipsSubtarget>()) {}
  
  virtual void InstructionSelect();

  // Pass Name
  virtual const char *getPassName() const {
    return "MIPS DAG->DAG Pattern Instruction Selection";
  } 
  

private:  
  // Include the pieces autogenerated from the target description.
  #include "MipsGenDAGISel.inc"

  SDValue getGlobalBaseReg();
  SDNode *Select(SDValue N);

  // Complex Pattern.
  bool SelectAddr(SDValue Op, SDValue N, 
                  SDValue &Base, SDValue &Offset);


  // getI32Imm - Return a target constant with the specified
  // value, of type i32.
  inline SDValue getI32Imm(unsigned Imm) {
    return CurDAG->getTargetConstant(Imm, MVT::i32);
  }


  #ifndef NDEBUG
  unsigned Indent;
  #endif
};

}

/// InstructionSelect - This callback is invoked by
/// SelectionDAGISel when it has created a SelectionDAG for us to codegen.
void MipsDAGToDAGISel::
InstructionSelect() 
{
  DEBUG(BB->dump());
  // Codegen the basic block.
  #ifndef NDEBUG
  DOUT << "===== Instruction selection begins:\n";
  Indent = 0;
  #endif

  // Select target instructions for the DAG.
  SelectRoot(*CurDAG);

  #ifndef NDEBUG
  DOUT << "===== Instruction selection ends:\n";
  #endif

  CurDAG->RemoveDeadNodes();
}

/// getGlobalBaseReg - Output the instructions required to put the
/// GOT address into a register.
SDValue MipsDAGToDAGISel::getGlobalBaseReg() {
  MachineFunction* MF = BB->getParent();
  unsigned GP = 0;
  for(MachineRegisterInfo::livein_iterator ii = MF->getRegInfo().livein_begin(),
        ee = MF->getRegInfo().livein_end(); ii != ee; ++ii)
    if (ii->first == Mips::GP) {
      GP = ii->second;
      break;
    }
  assert(GP && "GOT PTR not in liveins");
  return CurDAG->getCopyFromReg(CurDAG->getEntryNode(), 
                                GP, MVT::i32);
}

/// ComplexPattern used on MipsInstrInfo
/// Used on Mips Load/Store instructions
bool MipsDAGToDAGISel::
SelectAddr(SDValue Op, SDValue Addr, SDValue &Offset, SDValue &Base)
{
  // if Address is FI, get the TargetFrameIndex.
  if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    Base   = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
    Offset = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }
    
  // on PIC code Load GA
  if (TM.getRelocationModel() == Reloc::PIC_) {
    if ((Addr.getOpcode() == ISD::TargetGlobalAddress) || 
        (Addr.getOpcode() == ISD::TargetJumpTable)){
      Base   = CurDAG->getRegister(Mips::GP, MVT::i32);
      Offset = Addr;
      return true;
    }
  } else {
    if ((Addr.getOpcode() == ISD::TargetExternalSymbol ||
        Addr.getOpcode() == ISD::TargetGlobalAddress))
      return false;
  }    
  
  // Operand is a result from an ADD.
  if (Addr.getOpcode() == ISD::ADD) {
    if (ConstantSDNode *CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1))) {
      if (Predicate_immSExt16(CN)) {

        // If the first operand is a FI, get the TargetFI Node
        if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>
                                    (Addr.getOperand(0))) {
          Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
        } else {
          Base = Addr.getOperand(0);
        }

        Offset = CurDAG->getTargetConstant(CN->getZExtValue(), MVT::i32);
        return true;
      }
    }
  }

  Base   = Addr;
  Offset = CurDAG->getTargetConstant(0, MVT::i32);
  return true;
}

/// Select instructions not customized! Used for
/// expanded, promoted and normal instructions
SDNode* MipsDAGToDAGISel::
Select(SDValue N) 
{
  SDNode *Node = N.getNode();
  unsigned Opcode = Node->getOpcode();

  // Dump information about the Node being selected
  #ifndef NDEBUG
  DOUT << std::string(Indent, ' ') << "Selecting: ";
  DEBUG(Node->dump(CurDAG));
  DOUT << "\n";
  Indent += 2;
  #endif

  // If we have a custom node, we already have selected!
  if (Node->isMachineOpcode()) {
    #ifndef NDEBUG
    DOUT << std::string(Indent-2, ' ') << "== ";
    DEBUG(Node->dump(CurDAG));
    DOUT << "\n";
    Indent -= 2;
    #endif
    return NULL;
  }

  ///
  // Instruction Selection not handled by the auto-generated 
  // tablegen selection should be handled here.
  /// 
  switch(Opcode) {

    default: break;

    case ISD::SUBE: 
    case ISD::ADDE: {
      SDValue InFlag = Node->getOperand(2), CmpLHS;
      unsigned Opc = InFlag.getOpcode(); Opc=Opc;
      assert(((Opc == ISD::ADDC || Opc == ISD::ADDE) || 
              (Opc == ISD::SUBC || Opc == ISD::SUBE)) &&  
             "(ADD|SUB)E flag operand must come from (ADD|SUB)C/E insn");

      unsigned MOp;
      if (Opcode == ISD::ADDE) {
        CmpLHS = InFlag.getValue(0);
        MOp = Mips::ADDu;
      } else { 
        CmpLHS = InFlag.getOperand(0);
        MOp = Mips::SUBu;
      }

      SDValue Ops[] = { CmpLHS, InFlag.getOperand(1) };

      SDValue LHS = Node->getOperand(0);
      SDValue RHS = Node->getOperand(1);

      MVT VT = LHS.getValueType();
      SDNode *Carry = CurDAG->getTargetNode(Mips::SLTu, VT, Ops, 2);
      SDNode *AddCarry = CurDAG->getTargetNode(Mips::ADDu, VT, 
                                               SDValue(Carry,0), RHS);

      return CurDAG->SelectNodeTo(N.getNode(), MOp, VT, MVT::Flag, 
                                  LHS, SDValue(AddCarry,0));
    }

    /// Mul/Div with two results
    case ISD::SDIVREM:
    case ISD::UDIVREM:
    case ISD::SMUL_LOHI:
    case ISD::UMUL_LOHI: {
      SDValue Op1 = Node->getOperand(0);
      SDValue Op2 = Node->getOperand(1);

      unsigned Op;
      if (Opcode == ISD::UMUL_LOHI || Opcode == ISD::SMUL_LOHI)
        Op = (Opcode == ISD::UMUL_LOHI ? Mips::MULTu : Mips::MULT);
      else
        Op = (Opcode == ISD::UDIVREM ? Mips::DIVu : Mips::DIV);

      SDNode *Node = CurDAG->getTargetNode(Op, MVT::Flag, Op1, Op2);

      SDValue InFlag = SDValue(Node, 0);
      SDNode *Lo = CurDAG->getTargetNode(Mips::MFLO, MVT::i32, 
                                         MVT::Flag, InFlag);
      InFlag = SDValue(Lo,1);
      SDNode *Hi = CurDAG->getTargetNode(Mips::MFHI, MVT::i32, InFlag);

      if (!N.getValue(0).use_empty()) 
        ReplaceUses(N.getValue(0), SDValue(Lo,0));

      if (!N.getValue(1).use_empty()) 
        ReplaceUses(N.getValue(1), SDValue(Hi,0));

      return NULL;
    }

    /// Special Muls
    case ISD::MUL: 
    case ISD::MULHS:
    case ISD::MULHU: {
      SDValue MulOp1 = Node->getOperand(0);
      SDValue MulOp2 = Node->getOperand(1);

      unsigned MulOp  = (Opcode == ISD::MULHU ? Mips::MULTu : Mips::MULT);
      SDNode *MulNode = CurDAG->getTargetNode(MulOp, MVT::Flag, MulOp1, MulOp2);

      SDValue InFlag = SDValue(MulNode, 0);

      if (MulOp == ISD::MUL)
        return CurDAG->getTargetNode(Mips::MFLO, MVT::i32, InFlag);
      else
        return CurDAG->getTargetNode(Mips::MFHI, MVT::i32, InFlag);
    }

    /// Div/Rem operations
    case ISD::SREM:
    case ISD::UREM:
    case ISD::SDIV: 
    case ISD::UDIV: {
      SDValue Op1 = Node->getOperand(0);
      SDValue Op2 = Node->getOperand(1);

      unsigned Op, MOp;
      if (Opcode == ISD::SDIV || Opcode == ISD::UDIV) {
        Op  = (Opcode == ISD::SDIV ? Mips::DIV : Mips::DIVu);
        MOp = Mips::MFLO;
      } else {
        Op  = (Opcode == ISD::SREM ? Mips::DIV : Mips::DIVu);
        MOp = Mips::MFHI;
      }
      SDNode *Node = CurDAG->getTargetNode(Op, MVT::Flag, Op1, Op2);

      SDValue InFlag = SDValue(Node, 0);
      return CurDAG->getTargetNode(MOp, MVT::i32, InFlag);
    }

    // Get target GOT address.
    case ISD::GLOBAL_OFFSET_TABLE: {
      SDValue Result = getGlobalBaseReg();
      ReplaceUses(N, Result);
      return NULL;
    }

    /// Handle direct and indirect calls when using PIC. On PIC, when 
    /// GOT is smaller than about 64k (small code) the GA target is 
    /// loaded with only one instruction. Otherwise GA's target must 
    /// be loaded with 3 instructions. 
    case MipsISD::JmpLink: {
      if (TM.getRelocationModel() == Reloc::PIC_) {
        //bool isCodeLarge = (TM.getCodeModel() == CodeModel::Large);
        SDValue Chain  = Node->getOperand(0);
        SDValue Callee = Node->getOperand(1);
        SDValue T9Reg = CurDAG->getRegister(Mips::T9, MVT::i32);
        SDValue InFlag(0, 0);

        if ( (isa<GlobalAddressSDNode>(Callee)) ||
             (isa<ExternalSymbolSDNode>(Callee)) )
        {
          /// Direct call for global addresses and external symbols
          SDValue GPReg = CurDAG->getRegister(Mips::GP, MVT::i32);

          // Use load to get GOT target
          SDValue Ops[] = { Callee, GPReg, Chain };
          SDValue Load = SDValue(CurDAG->getTargetNode(Mips::LW, MVT::i32, 
                                     MVT::Other, Ops, 3), 0);
          Chain = Load.getValue(1);

          // Call target must be on T9
          Chain = CurDAG->getCopyToReg(Chain, T9Reg, Load, InFlag);
        } else 
          /// Indirect call
          Chain = CurDAG->getCopyToReg(Chain, T9Reg, Callee, InFlag);

        // Emit Jump and Link Register
        SDNode *ResNode = CurDAG->getTargetNode(Mips::JALR, MVT::Other,
                                  MVT::Flag, T9Reg, Chain);
        Chain  = SDValue(ResNode, 0);
        InFlag = SDValue(ResNode, 1);
        ReplaceUses(SDValue(Node, 0), Chain);
        ReplaceUses(SDValue(Node, 1), InFlag);
        return ResNode;
      } 
    }
  }

  // Select the default instruction
  SDNode *ResNode = SelectCode(N);

  #ifndef NDEBUG
  DOUT << std::string(Indent-2, ' ') << "=> ";
  if (ResNode == NULL || ResNode == N.getNode())
    DEBUG(N.getNode()->dump(CurDAG));
  else
    DEBUG(ResNode->dump(CurDAG));
  DOUT << "\n";
  Indent -= 2;
  #endif

  return ResNode;
}

/// createMipsISelDag - This pass converts a legalized DAG into a 
/// MIPS-specific DAG, ready for instruction scheduling.
FunctionPass *llvm::createMipsISelDag(MipsTargetMachine &TM) {
  return new MipsDAGToDAGISel(TM);
}
