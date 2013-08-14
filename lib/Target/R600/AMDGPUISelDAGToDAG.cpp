//===-- AMDILISelDAGToDAG.cpp - A dag to dag inst selector for AMDIL ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
//
/// \file
/// \brief Defines an instruction selector for the AMDGPU target.
//
//===----------------------------------------------------------------------===//
#include "AMDGPUInstrInfo.h"
#include "AMDGPUISelLowering.h" // For AMDGPUISD
#include "AMDGPURegisterInfo.h"
#include "R600InstrInfo.h"
#include "SIISelLowering.h"
#include "llvm/ADT/ValueMap.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/Compiler.h"
#include <list>
#include <queue>

using namespace llvm;

//===----------------------------------------------------------------------===//
// Instruction Selector Implementation
//===----------------------------------------------------------------------===//

namespace {
/// AMDGPU specific code to select AMDGPU machine instructions for
/// SelectionDAG operations.
class AMDGPUDAGToDAGISel : public SelectionDAGISel {
  // Subtarget - Keep a pointer to the AMDGPU Subtarget around so that we can
  // make the right decision when generating code for different targets.
  const AMDGPUSubtarget &Subtarget;
public:
  AMDGPUDAGToDAGISel(TargetMachine &TM);
  virtual ~AMDGPUDAGToDAGISel();

  SDNode *Select(SDNode *N);
  virtual const char *getPassName() const;
  virtual void PostprocessISelDAG();

private:
  inline SDValue getSmallIPtrImm(unsigned Imm);
  bool FoldOperand(SDValue &Src, SDValue &Sel, SDValue &Neg, SDValue &Abs,
                   const R600InstrInfo *TII);
  bool FoldOperands(unsigned, const R600InstrInfo *, std::vector<SDValue> &);
  bool FoldDotOperands(unsigned, const R600InstrInfo *, std::vector<SDValue> &);

  // Complex pattern selectors
  bool SelectADDRParam(SDValue Addr, SDValue& R1, SDValue& R2);
  bool SelectADDR(SDValue N, SDValue &R1, SDValue &R2);
  bool SelectADDR64(SDValue N, SDValue &R1, SDValue &R2);
  SDValue SimplifyI24(SDValue &Op);
  bool SelectI24(SDValue Addr, SDValue &Op);
  bool SelectU24(SDValue Addr, SDValue &Op);

  static bool checkType(const Value *ptr, unsigned int addrspace);

  static bool isGlobalStore(const StoreSDNode *N);
  static bool isPrivateStore(const StoreSDNode *N);
  static bool isLocalStore(const StoreSDNode *N);
  static bool isRegionStore(const StoreSDNode *N);

  bool isCPLoad(const LoadSDNode *N) const;
  bool isConstantLoad(const LoadSDNode *N, int cbID) const;
  bool isGlobalLoad(const LoadSDNode *N) const;
  bool isParamLoad(const LoadSDNode *N) const;
  bool isPrivateLoad(const LoadSDNode *N) const;
  bool isLocalLoad(const LoadSDNode *N) const;
  bool isRegionLoad(const LoadSDNode *N) const;

  const TargetRegisterClass *getOperandRegClass(SDNode *N, unsigned OpNo) const;
  bool SelectGlobalValueConstantOffset(SDValue Addr, SDValue& IntPtr);
  bool SelectGlobalValueVariableOffset(SDValue Addr,
      SDValue &BaseReg, SDValue& Offset);
  bool SelectADDRVTX_READ(SDValue Addr, SDValue &Base, SDValue &Offset);
  bool SelectADDRIndirect(SDValue Addr, SDValue &Base, SDValue &Offset);

  // Include the pieces autogenerated from the target description.
#include "AMDGPUGenDAGISel.inc"
};
}  // end anonymous namespace

/// \brief This pass converts a legalized DAG into a AMDGPU-specific
// DAG, ready for instruction scheduling.
FunctionPass *llvm::createAMDGPUISelDag(TargetMachine &TM
                                       ) {
  return new AMDGPUDAGToDAGISel(TM);
}

AMDGPUDAGToDAGISel::AMDGPUDAGToDAGISel(TargetMachine &TM)
  : SelectionDAGISel(TM), Subtarget(TM.getSubtarget<AMDGPUSubtarget>()) {
}

AMDGPUDAGToDAGISel::~AMDGPUDAGToDAGISel() {
}

/// \brief Determine the register class for \p OpNo
/// \returns The register class of the virtual register that will be used for
/// the given operand number \OpNo or NULL if the register class cannot be
/// determined.
const TargetRegisterClass *AMDGPUDAGToDAGISel::getOperandRegClass(SDNode *N,
                                                          unsigned OpNo) const {
  if (!N->isMachineOpcode()) {
    return NULL;
  }
  switch (N->getMachineOpcode()) {
  default: {
    const MCInstrDesc &Desc = TM.getInstrInfo()->get(N->getMachineOpcode());
    int RegClass = Desc.OpInfo[Desc.getNumDefs() + OpNo].RegClass;
    if (RegClass == -1) {
      return NULL;
    }
    return TM.getRegisterInfo()->getRegClass(RegClass);
  }
  case AMDGPU::REG_SEQUENCE: {
    const TargetRegisterClass *SuperRC = TM.getRegisterInfo()->getRegClass(
                      cast<ConstantSDNode>(N->getOperand(0))->getZExtValue());
    unsigned SubRegIdx =
            dyn_cast<ConstantSDNode>(N->getOperand(OpNo + 1))->getZExtValue();
    return TM.getRegisterInfo()->getSubClassWithSubReg(SuperRC, SubRegIdx);
  }
  }
}

SDValue AMDGPUDAGToDAGISel::getSmallIPtrImm(unsigned int Imm) {
  return CurDAG->getTargetConstant(Imm, MVT::i32);
}

bool AMDGPUDAGToDAGISel::SelectADDRParam(
    SDValue Addr, SDValue& R1, SDValue& R2) {

  if (Addr.getOpcode() == ISD::FrameIndex) {
    if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
      R1 = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
      R2 = CurDAG->getTargetConstant(0, MVT::i32);
    } else {
      R1 = Addr;
      R2 = CurDAG->getTargetConstant(0, MVT::i32);
    }
  } else if (Addr.getOpcode() == ISD::ADD) {
    R1 = Addr.getOperand(0);
    R2 = Addr.getOperand(1);
  } else {
    R1 = Addr;
    R2 = CurDAG->getTargetConstant(0, MVT::i32);
  }
  return true;
}

bool AMDGPUDAGToDAGISel::SelectADDR(SDValue Addr, SDValue& R1, SDValue& R2) {
  if (Addr.getOpcode() == ISD::TargetExternalSymbol ||
      Addr.getOpcode() == ISD::TargetGlobalAddress) {
    return false;
  }
  return SelectADDRParam(Addr, R1, R2);
}


bool AMDGPUDAGToDAGISel::SelectADDR64(SDValue Addr, SDValue& R1, SDValue& R2) {
  if (Addr.getOpcode() == ISD::TargetExternalSymbol ||
      Addr.getOpcode() == ISD::TargetGlobalAddress) {
    return false;
  }

  if (Addr.getOpcode() == ISD::FrameIndex) {
    if (FrameIndexSDNode *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
      R1 = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i64);
      R2 = CurDAG->getTargetConstant(0, MVT::i64);
    } else {
      R1 = Addr;
      R2 = CurDAG->getTargetConstant(0, MVT::i64);
    }
  } else if (Addr.getOpcode() == ISD::ADD) {
    R1 = Addr.getOperand(0);
    R2 = Addr.getOperand(1);
  } else {
    R1 = Addr;
    R2 = CurDAG->getTargetConstant(0, MVT::i64);
  }
  return true;
}

SDNode *AMDGPUDAGToDAGISel::Select(SDNode *N) {
  const R600InstrInfo *TII =
                      static_cast<const R600InstrInfo*>(TM.getInstrInfo());
  unsigned int Opc = N->getOpcode();
  if (N->isMachineOpcode()) {
    return NULL;   // Already selected.
  }
  switch (Opc) {
  default: break;
  case AMDGPUISD::CONST_ADDRESS: {
    for (SDNode::use_iterator I = N->use_begin(), Next = llvm::next(I);
                              I != SDNode::use_end(); I = Next) {
      Next = llvm::next(I);
      if (!I->isMachineOpcode()) {
        continue;
      }
      unsigned Opcode = I->getMachineOpcode();
      bool HasDst = TII->getOperandIdx(Opcode, AMDGPU::OpName::dst) > -1;
      int SrcIdx = I.getOperandNo();
      int SelIdx;
      // Unlike MachineInstrs, SDNodes do not have results in their operand
      // list, so we need to increment the SrcIdx, since
      // R600InstrInfo::getOperandIdx is based on the MachineInstr indices.
      if (HasDst) {
        SrcIdx++;
      }

      SelIdx = TII->getSelIdx(I->getMachineOpcode(), SrcIdx);
      if (SelIdx < 0) {
        continue;
      }

      SDValue CstOffset;
      if (N->getValueType(0).isVector() ||
          !SelectGlobalValueConstantOffset(N->getOperand(0), CstOffset))
        continue;

      // Gather constants values
      int SrcIndices[] = {
        TII->getOperandIdx(Opcode, AMDGPU::OpName::src0),
        TII->getOperandIdx(Opcode, AMDGPU::OpName::src1),
        TII->getOperandIdx(Opcode, AMDGPU::OpName::src2),
        TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_X),
        TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_Y),
        TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_Z),
        TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_W),
        TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_X),
        TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_Y),
        TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_Z),
        TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_W)
      };
      std::vector<unsigned> Consts;
      for (unsigned i = 0; i < sizeof(SrcIndices) / sizeof(int); i++) {
        int OtherSrcIdx = SrcIndices[i];
        int OtherSelIdx = TII->getSelIdx(Opcode, OtherSrcIdx);
        if (OtherSrcIdx < 0 || OtherSelIdx < 0) {
          continue;
        }
        if (HasDst) {
          OtherSrcIdx--;
          OtherSelIdx--;
        }
        if (RegisterSDNode *Reg =
                         dyn_cast<RegisterSDNode>(I->getOperand(OtherSrcIdx))) {
          if (Reg->getReg() == AMDGPU::ALU_CONST) {
            ConstantSDNode *Cst = dyn_cast<ConstantSDNode>(I->getOperand(OtherSelIdx));
            Consts.push_back(Cst->getZExtValue());
          }
        }
      }

      ConstantSDNode *Cst = dyn_cast<ConstantSDNode>(CstOffset);
      Consts.push_back(Cst->getZExtValue());
      if (!TII->fitsConstReadLimitations(Consts))
        continue;

      // Convert back to SDNode indices
      if (HasDst) {
        SrcIdx--;
        SelIdx--;
      }
      std::vector<SDValue> Ops;
      for (int i = 0, e = I->getNumOperands(); i != e; ++i) {
        if (i == SrcIdx) {
          Ops.push_back(CurDAG->getRegister(AMDGPU::ALU_CONST, MVT::f32));
        } else if (i == SelIdx) {
          Ops.push_back(CstOffset);
        } else {
          Ops.push_back(I->getOperand(i));
        }
      }
      CurDAG->UpdateNodeOperands(*I, Ops.data(), Ops.size());
    }
    break;
  }
  case ISD::BUILD_VECTOR: {
    unsigned RegClassID;
    const AMDGPUSubtarget &ST = TM.getSubtarget<AMDGPUSubtarget>();
    const AMDGPURegisterInfo *TRI =
                   static_cast<const AMDGPURegisterInfo*>(TM.getRegisterInfo());
    const SIRegisterInfo *SIRI =
                   static_cast<const SIRegisterInfo*>(TM.getRegisterInfo());
    EVT VT = N->getValueType(0);
    unsigned NumVectorElts = VT.getVectorNumElements();
    assert(VT.getVectorElementType().bitsEq(MVT::i32));
    if (ST.getGeneration() >= AMDGPUSubtarget::SOUTHERN_ISLANDS) {
      bool UseVReg = true;
      for (SDNode::use_iterator U = N->use_begin(), E = SDNode::use_end();
                                                    U != E; ++U) {
        if (!U->isMachineOpcode()) {
          continue;
        }
        const TargetRegisterClass *RC = getOperandRegClass(*U, U.getOperandNo());
        if (!RC) {
          continue;
        }
        if (SIRI->isSGPRClass(RC)) {
          UseVReg = false;
        }
      }
      switch(NumVectorElts) {
      case 1: RegClassID = UseVReg ? AMDGPU::VReg_32RegClassID :
                                     AMDGPU::SReg_32RegClassID;
        break;
      case 2: RegClassID = UseVReg ? AMDGPU::VReg_64RegClassID :
                                     AMDGPU::SReg_64RegClassID;
        break;
      case 4: RegClassID = UseVReg ? AMDGPU::VReg_128RegClassID :
                                     AMDGPU::SReg_128RegClassID;
        break;
      case 8: RegClassID = UseVReg ? AMDGPU::VReg_256RegClassID :
                                     AMDGPU::SReg_256RegClassID;
        break;
      case 16: RegClassID = UseVReg ? AMDGPU::VReg_512RegClassID :
                                      AMDGPU::SReg_512RegClassID;
        break;
      }
    } else {
      // BUILD_VECTOR was lowered into an IMPLICIT_DEF + 4 INSERT_SUBREG
      // that adds a 128 bits reg copy when going through TwoAddressInstructions
      // pass. We want to avoid 128 bits copies as much as possible because they
      // can't be bundled by our scheduler.
      switch(NumVectorElts) {
      case 2: RegClassID = AMDGPU::R600_Reg64RegClassID; break;
      case 4: RegClassID = AMDGPU::R600_Reg128RegClassID; break;
      default: llvm_unreachable("Do not know how to lower this BUILD_VECTOR");
      }
    }

    SDValue RegClass = CurDAG->getTargetConstant(RegClassID, MVT::i32);

    if (NumVectorElts == 1) {
      return CurDAG->SelectNodeTo(N, AMDGPU::COPY_TO_REGCLASS,
                                  VT.getVectorElementType(),
                                  N->getOperand(0), RegClass);
    }

    assert(NumVectorElts <= 16 && "Vectors with more than 16 elements not "
                                  "supported yet");
    // 16 = Max Num Vector Elements
    // 2 = 2 REG_SEQUENCE operands per element (value, subreg index)
    // 1 = Vector Register Class
    SDValue RegSeqArgs[16 * 2 + 1];

    RegSeqArgs[0] = CurDAG->getTargetConstant(RegClassID, MVT::i32);
    bool IsRegSeq = true;
    for (unsigned i = 0; i < N->getNumOperands(); i++) {
      // XXX: Why is this here?
      if (dyn_cast<RegisterSDNode>(N->getOperand(i))) {
        IsRegSeq = false;
        break;
      }
      RegSeqArgs[1 + (2 * i)] = N->getOperand(i);
      RegSeqArgs[1 + (2 * i) + 1] =
              CurDAG->getTargetConstant(TRI->getSubRegFromChannel(i), MVT::i32);
    }
    if (!IsRegSeq)
      break;
    return CurDAG->SelectNodeTo(N, AMDGPU::REG_SEQUENCE, N->getVTList(),
        RegSeqArgs, 2 * N->getNumOperands() + 1);
  }
  case ISD::BUILD_PAIR: {
    SDValue RC, SubReg0, SubReg1;
    const AMDGPUSubtarget &ST = TM.getSubtarget<AMDGPUSubtarget>();
    if (ST.getGeneration() <= AMDGPUSubtarget::NORTHERN_ISLANDS) {
      break;
    }
    if (N->getValueType(0) == MVT::i128) {
      RC = CurDAG->getTargetConstant(AMDGPU::SReg_128RegClassID, MVT::i32);
      SubReg0 = CurDAG->getTargetConstant(AMDGPU::sub0_sub1, MVT::i32);
      SubReg1 = CurDAG->getTargetConstant(AMDGPU::sub2_sub3, MVT::i32);
    } else if (N->getValueType(0) == MVT::i64) {
      RC = CurDAG->getTargetConstant(AMDGPU::VSrc_64RegClassID, MVT::i32);
      SubReg0 = CurDAG->getTargetConstant(AMDGPU::sub0, MVT::i32);
      SubReg1 = CurDAG->getTargetConstant(AMDGPU::sub1, MVT::i32);
    } else {
      llvm_unreachable("Unhandled value type for BUILD_PAIR");
    }
    const SDValue Ops[] = { RC, N->getOperand(0), SubReg0,
                            N->getOperand(1), SubReg1 };
    return CurDAG->getMachineNode(TargetOpcode::REG_SEQUENCE,
                                  SDLoc(N), N->getValueType(0), Ops);
  }

  case ISD::ConstantFP:
  case ISD::Constant: {
    const AMDGPUSubtarget &ST = TM.getSubtarget<AMDGPUSubtarget>();
    // XXX: Custom immediate lowering not implemented yet.  Instead we use
    // pseudo instructions defined in SIInstructions.td
    if (ST.getGeneration() > AMDGPUSubtarget::NORTHERN_ISLANDS) {
      break;
    }

    uint64_t ImmValue = 0;
    unsigned ImmReg = AMDGPU::ALU_LITERAL_X;

    if (N->getOpcode() == ISD::ConstantFP) {
      // XXX: 64-bit Immediates not supported yet
      assert(N->getValueType(0) != MVT::f64);

      ConstantFPSDNode *C = dyn_cast<ConstantFPSDNode>(N);
      APFloat Value = C->getValueAPF();
      float FloatValue = Value.convertToFloat();
      if (FloatValue == 0.0) {
        ImmReg = AMDGPU::ZERO;
      } else if (FloatValue == 0.5) {
        ImmReg = AMDGPU::HALF;
      } else if (FloatValue == 1.0) {
        ImmReg = AMDGPU::ONE;
      } else {
        ImmValue = Value.bitcastToAPInt().getZExtValue();
      }
    } else {
      // XXX: 64-bit Immediates not supported yet
      assert(N->getValueType(0) != MVT::i64);

      ConstantSDNode *C = dyn_cast<ConstantSDNode>(N);
      if (C->getZExtValue() == 0) {
        ImmReg = AMDGPU::ZERO;
      } else if (C->getZExtValue() == 1) {
        ImmReg = AMDGPU::ONE_INT;
      } else {
        ImmValue = C->getZExtValue();
      }
    }

    for (SDNode::use_iterator Use = N->use_begin(), Next = llvm::next(Use);
                              Use != SDNode::use_end(); Use = Next) {
      Next = llvm::next(Use);
      std::vector<SDValue> Ops;
      for (unsigned i = 0; i < Use->getNumOperands(); ++i) {
        Ops.push_back(Use->getOperand(i));
      }

      if (!Use->isMachineOpcode()) {
          if (ImmReg == AMDGPU::ALU_LITERAL_X) {
            // We can only use literal constants (e.g. AMDGPU::ZERO,
            // AMDGPU::ONE, etc) in machine opcodes.
            continue;
          }
      } else {
        if (!TII->isALUInstr(Use->getMachineOpcode()) ||
            (TII->get(Use->getMachineOpcode()).TSFlags &
            R600_InstFlag::VECTOR)) {
          continue;
        }

        int ImmIdx = TII->getOperandIdx(Use->getMachineOpcode(),
                                        AMDGPU::OpName::literal);
        if (ImmIdx == -1) {
          continue;
        }

        if (TII->getOperandIdx(Use->getMachineOpcode(),
                               AMDGPU::OpName::dst) != -1) {
          // subtract one from ImmIdx, because the DST operand is usually index
          // 0 for MachineInstrs, but we have no DST in the Ops vector.
          ImmIdx--;
        }

        // Check that we aren't already using an immediate.
        // XXX: It's possible for an instruction to have more than one
        // immediate operand, but this is not supported yet.
        if (ImmReg == AMDGPU::ALU_LITERAL_X) {
          ConstantSDNode *C = dyn_cast<ConstantSDNode>(Use->getOperand(ImmIdx));
          assert(C);

          if (C->getZExtValue() != 0) {
            // This instruction is already using an immediate.
            continue;
          }

          // Set the immediate value
          Ops[ImmIdx] = CurDAG->getTargetConstant(ImmValue, MVT::i32);
        }
      }
      // Set the immediate register
      Ops[Use.getOperandNo()] = CurDAG->getRegister(ImmReg, MVT::i32);

      CurDAG->UpdateNodeOperands(*Use, Ops.data(), Use->getNumOperands());
    }
    break;
  }
  }
  SDNode *Result = SelectCode(N);

  // Fold operands of selected node

  const AMDGPUSubtarget &ST = TM.getSubtarget<AMDGPUSubtarget>();
  if (ST.getGeneration() <= AMDGPUSubtarget::NORTHERN_ISLANDS) {
    const R600InstrInfo *TII =
        static_cast<const R600InstrInfo*>(TM.getInstrInfo());
    if (Result && Result->isMachineOpcode() && Result->getMachineOpcode() == AMDGPU::DOT_4) {
      bool IsModified = false;
      do {
        std::vector<SDValue> Ops;
        for(SDNode::op_iterator I = Result->op_begin(), E = Result->op_end();
            I != E; ++I)
          Ops.push_back(*I);
        IsModified = FoldDotOperands(Result->getMachineOpcode(), TII, Ops);
        if (IsModified) {
          Result = CurDAG->UpdateNodeOperands(Result, Ops.data(), Ops.size());
        }
      } while (IsModified);

    }
    if (Result && Result->isMachineOpcode() &&
        !(TII->get(Result->getMachineOpcode()).TSFlags & R600_InstFlag::VECTOR)
        && TII->hasInstrModifiers(Result->getMachineOpcode())) {
      // Fold FNEG/FABS
      // TODO: Isel can generate multiple MachineInst, we need to recursively
      // parse Result
      bool IsModified = false;
      do {
        std::vector<SDValue> Ops;
        for(SDNode::op_iterator I = Result->op_begin(), E = Result->op_end();
            I != E; ++I)
          Ops.push_back(*I);
        IsModified = FoldOperands(Result->getMachineOpcode(), TII, Ops);
        if (IsModified) {
          Result = CurDAG->UpdateNodeOperands(Result, Ops.data(), Ops.size());
        }
      } while (IsModified);

      // If node has a single use which is CLAMP_R600, folds it
      if (Result->hasOneUse() && Result->isMachineOpcode()) {
        SDNode *PotentialClamp = *Result->use_begin();
        if (PotentialClamp->isMachineOpcode() &&
            PotentialClamp->getMachineOpcode() == AMDGPU::CLAMP_R600) {
          unsigned ClampIdx =
            TII->getOperandIdx(Result->getMachineOpcode(), AMDGPU::OpName::clamp);
          std::vector<SDValue> Ops;
          unsigned NumOp = Result->getNumOperands();
          for (unsigned i = 0; i < NumOp; ++i) {
            Ops.push_back(Result->getOperand(i));
          }
          Ops[ClampIdx - 1] = CurDAG->getTargetConstant(1, MVT::i32);
          Result = CurDAG->SelectNodeTo(PotentialClamp,
              Result->getMachineOpcode(), PotentialClamp->getVTList(),
              Ops.data(), NumOp);
        }
      }
    }
  }

  return Result;
}

bool AMDGPUDAGToDAGISel::FoldOperand(SDValue &Src, SDValue &Sel, SDValue &Neg,
                                     SDValue &Abs, const R600InstrInfo *TII) {
  switch (Src.getOpcode()) {
  case ISD::FNEG:
    Src = Src.getOperand(0);
    Neg = CurDAG->getTargetConstant(1, MVT::i32);
    return true;
  case ISD::FABS:
    if (!Abs.getNode())
      return false;
    Src = Src.getOperand(0);
    Abs = CurDAG->getTargetConstant(1, MVT::i32);
    return true;
  case ISD::BITCAST:
    Src = Src.getOperand(0);
    return true;
  default:
    return false;
  }
}

bool AMDGPUDAGToDAGISel::FoldOperands(unsigned Opcode,
    const R600InstrInfo *TII, std::vector<SDValue> &Ops) {
  int OperandIdx[] = {
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src2)
  };
  int SelIdx[] = {
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_sel),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_sel),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src2_sel)
  };
  int NegIdx[] = {
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_neg),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_neg),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src2_neg)
  };
  int AbsIdx[] = {
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_abs),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_abs),
    -1
  };


  for (unsigned i = 0; i < 3; i++) {
    if (OperandIdx[i] < 0)
      return false;
    SDValue &Src = Ops[OperandIdx[i] - 1];
    SDValue &Sel = Ops[SelIdx[i] - 1];
    SDValue &Neg = Ops[NegIdx[i] - 1];
    SDValue FakeAbs;
    SDValue &Abs = (AbsIdx[i] > -1) ? Ops[AbsIdx[i] - 1] : FakeAbs;
    if (FoldOperand(Src, Sel, Neg, Abs, TII))
      return true;
  }
  return false;
}

bool AMDGPUDAGToDAGISel::FoldDotOperands(unsigned Opcode,
    const R600InstrInfo *TII, std::vector<SDValue> &Ops) {
  int OperandIdx[] = {
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_X),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_Y),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_Z),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_W),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_X),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_Y),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_Z),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_W)
  };
  int SelIdx[] = {
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_sel_X),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_sel_Y),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_sel_Z),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_sel_W),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_sel_X),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_sel_Y),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_sel_Z),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_sel_W)
  };
  int NegIdx[] = {
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_neg_X),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_neg_Y),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_neg_Z),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_neg_W),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_neg_X),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_neg_Y),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_neg_Z),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_neg_W)
  };
  int AbsIdx[] = {
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_abs_X),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_abs_Y),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_abs_Z),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src0_abs_W),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_abs_X),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_abs_Y),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_abs_Z),
    TII->getOperandIdx(Opcode, AMDGPU::OpName::src1_abs_W)
  };

  for (unsigned i = 0; i < 8; i++) {
    if (OperandIdx[i] < 0)
      return false;
    SDValue &Src = Ops[OperandIdx[i] - 1];
    SDValue &Sel = Ops[SelIdx[i] - 1];
    SDValue &Neg = Ops[NegIdx[i] - 1];
    SDValue &Abs = Ops[AbsIdx[i] - 1];
    if (FoldOperand(Src, Sel, Neg, Abs, TII))
      return true;
  }
  return false;
}

bool AMDGPUDAGToDAGISel::checkType(const Value *ptr, unsigned int addrspace) {
  if (!ptr) {
    return false;
  }
  Type *ptrType = ptr->getType();
  return dyn_cast<PointerType>(ptrType)->getAddressSpace() == addrspace;
}

bool AMDGPUDAGToDAGISel::isGlobalStore(const StoreSDNode *N) {
  return checkType(N->getSrcValue(), AMDGPUAS::GLOBAL_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isPrivateStore(const StoreSDNode *N) {
  return (!checkType(N->getSrcValue(), AMDGPUAS::LOCAL_ADDRESS)
          && !checkType(N->getSrcValue(), AMDGPUAS::GLOBAL_ADDRESS)
          && !checkType(N->getSrcValue(), AMDGPUAS::REGION_ADDRESS));
}

bool AMDGPUDAGToDAGISel::isLocalStore(const StoreSDNode *N) {
  return checkType(N->getSrcValue(), AMDGPUAS::LOCAL_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isRegionStore(const StoreSDNode *N) {
  return checkType(N->getSrcValue(), AMDGPUAS::REGION_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isConstantLoad(const LoadSDNode *N, int CbId) const {
  if (CbId == -1) {
    return checkType(N->getSrcValue(), AMDGPUAS::CONSTANT_ADDRESS);
  }
  return checkType(N->getSrcValue(), AMDGPUAS::CONSTANT_BUFFER_0 + CbId);
}

bool AMDGPUDAGToDAGISel::isGlobalLoad(const LoadSDNode *N) const {
  if (N->getAddressSpace() == AMDGPUAS::CONSTANT_ADDRESS) {
    const AMDGPUSubtarget &ST = TM.getSubtarget<AMDGPUSubtarget>();
    if (ST.getGeneration() < AMDGPUSubtarget::SOUTHERN_ISLANDS ||
        N->getMemoryVT().bitsLT(MVT::i32)) {
      return true;
    }
  }
  return checkType(N->getSrcValue(), AMDGPUAS::GLOBAL_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isParamLoad(const LoadSDNode *N) const {
  return checkType(N->getSrcValue(), AMDGPUAS::PARAM_I_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isLocalLoad(const  LoadSDNode *N) const {
  return checkType(N->getSrcValue(), AMDGPUAS::LOCAL_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isRegionLoad(const  LoadSDNode *N) const {
  return checkType(N->getSrcValue(), AMDGPUAS::REGION_ADDRESS);
}

bool AMDGPUDAGToDAGISel::isCPLoad(const LoadSDNode *N) const {
  MachineMemOperand *MMO = N->getMemOperand();
  if (checkType(N->getSrcValue(), AMDGPUAS::PRIVATE_ADDRESS)) {
    if (MMO) {
      const Value *V = MMO->getValue();
      const PseudoSourceValue *PSV = dyn_cast<PseudoSourceValue>(V);
      if (PSV && PSV == PseudoSourceValue::getConstantPool()) {
        return true;
      }
    }
  }
  return false;
}

bool AMDGPUDAGToDAGISel::isPrivateLoad(const LoadSDNode *N) const {
  if (checkType(N->getSrcValue(), AMDGPUAS::PRIVATE_ADDRESS)) {
    // Check to make sure we are not a constant pool load or a constant load
    // that is marked as a private load
    if (isCPLoad(N) || isConstantLoad(N, -1)) {
      return false;
    }
  }
  if (!checkType(N->getSrcValue(), AMDGPUAS::LOCAL_ADDRESS)
      && !checkType(N->getSrcValue(), AMDGPUAS::GLOBAL_ADDRESS)
      && !checkType(N->getSrcValue(), AMDGPUAS::REGION_ADDRESS)
      && !checkType(N->getSrcValue(), AMDGPUAS::CONSTANT_ADDRESS)
      && !checkType(N->getSrcValue(), AMDGPUAS::PARAM_D_ADDRESS)
      && !checkType(N->getSrcValue(), AMDGPUAS::PARAM_I_ADDRESS)) {
    return true;
  }
  return false;
}

const char *AMDGPUDAGToDAGISel::getPassName() const {
  return "AMDGPU DAG->DAG Pattern Instruction Selection";
}

#ifdef DEBUGTMP
#undef INT64_C
#endif
#undef DEBUGTMP

//===----------------------------------------------------------------------===//
// Complex Patterns
//===----------------------------------------------------------------------===//

bool AMDGPUDAGToDAGISel::SelectGlobalValueConstantOffset(SDValue Addr,
    SDValue& IntPtr) {
  if (ConstantSDNode *Cst = dyn_cast<ConstantSDNode>(Addr)) {
    IntPtr = CurDAG->getIntPtrConstant(Cst->getZExtValue() / 4, true);
    return true;
  }
  return false;
}

bool AMDGPUDAGToDAGISel::SelectGlobalValueVariableOffset(SDValue Addr,
    SDValue& BaseReg, SDValue &Offset) {
  if (!dyn_cast<ConstantSDNode>(Addr)) {
    BaseReg = Addr;
    Offset = CurDAG->getIntPtrConstant(0, true);
    return true;
  }
  return false;
}

bool AMDGPUDAGToDAGISel::SelectADDRVTX_READ(SDValue Addr, SDValue &Base,
                                           SDValue &Offset) {
  ConstantSDNode * IMMOffset;

  if (Addr.getOpcode() == ISD::ADD
      && (IMMOffset = dyn_cast<ConstantSDNode>(Addr.getOperand(1)))
      && isInt<16>(IMMOffset->getZExtValue())) {

      Base = Addr.getOperand(0);
      Offset = CurDAG->getTargetConstant(IMMOffset->getZExtValue(), MVT::i32);
      return true;
  // If the pointer address is constant, we can move it to the offset field.
  } else if ((IMMOffset = dyn_cast<ConstantSDNode>(Addr))
             && isInt<16>(IMMOffset->getZExtValue())) {
    Base = CurDAG->getCopyFromReg(CurDAG->getEntryNode(),
                                  SDLoc(CurDAG->getEntryNode()),
                                  AMDGPU::ZERO, MVT::i32);
    Offset = CurDAG->getTargetConstant(IMMOffset->getZExtValue(), MVT::i32);
    return true;
  }

  // Default case, no offset
  Base = Addr;
  Offset = CurDAG->getTargetConstant(0, MVT::i32);
  return true;
}

bool AMDGPUDAGToDAGISel::SelectADDRIndirect(SDValue Addr, SDValue &Base,
                                            SDValue &Offset) {
  ConstantSDNode *C;

  if ((C = dyn_cast<ConstantSDNode>(Addr))) {
    Base = CurDAG->getRegister(AMDGPU::INDIRECT_BASE_ADDR, MVT::i32);
    Offset = CurDAG->getTargetConstant(C->getZExtValue(), MVT::i32);
  } else if ((Addr.getOpcode() == ISD::ADD || Addr.getOpcode() == ISD::OR) &&
            (C = dyn_cast<ConstantSDNode>(Addr.getOperand(1)))) {
    Base = Addr.getOperand(0);
    Offset = CurDAG->getTargetConstant(C->getZExtValue(), MVT::i32);
  } else {
    Base = Addr;
    Offset = CurDAG->getTargetConstant(0, MVT::i32);
  }

  return true;
}

SDValue AMDGPUDAGToDAGISel::SimplifyI24(SDValue &Op) {
  APInt Demanded = APInt(32, 0x00FFFFFF);
  APInt KnownZero, KnownOne;
  TargetLowering::TargetLoweringOpt TLO(*CurDAG, true, true);
  const TargetLowering *TLI = getTargetLowering();
  if (TLI->SimplifyDemandedBits(Op, Demanded, KnownZero, KnownOne, TLO)) {
    CurDAG->ReplaceAllUsesWith(Op, TLO.New);
    CurDAG->RepositionNode(Op.getNode(), TLO.New.getNode());
    return SimplifyI24(TLO.New);
  } else {
    return  Op;
  }
}

bool AMDGPUDAGToDAGISel::SelectI24(SDValue Op, SDValue &I24) {

  assert(Op.getValueType() == MVT::i32);

  if (CurDAG->ComputeNumSignBits(Op) == 9) {
    I24 = SimplifyI24(Op);
    return true;
  }
  return false;
}

bool AMDGPUDAGToDAGISel::SelectU24(SDValue Op, SDValue &U24) {
  APInt KnownZero;
  APInt KnownOne;
  CurDAG->ComputeMaskedBits(Op, KnownZero, KnownOne);

  assert (Op.getValueType() == MVT::i32);

  // ANY_EXTEND and EXTLOAD operations can only be done on types smaller than
  // i32.  These smaller types are legal to use with the i24 instructions.
  if ((KnownZero & APInt(KnownZero.getBitWidth(), 0xFF000000)) == 0xFF000000 ||
       Op.getOpcode() == ISD::ANY_EXTEND ||
       ISD::isEXTLoad(Op.getNode())) {
    U24 = SimplifyI24(Op);
    return true;
  }
  return false;
}

void AMDGPUDAGToDAGISel::PostprocessISelDAG() {

  if (Subtarget.getGeneration() < AMDGPUSubtarget::SOUTHERN_ISLANDS) {
    return;
  }

  // Go over all selected nodes and try to fold them a bit more
  const AMDGPUTargetLowering& Lowering =
    (*(const AMDGPUTargetLowering*)getTargetLowering());
  for (SelectionDAG::allnodes_iterator I = CurDAG->allnodes_begin(),
       E = CurDAG->allnodes_end(); I != E; ++I) {

    SDNode *Node = I;

    MachineSDNode *MachineNode = dyn_cast<MachineSDNode>(I);
    if (!MachineNode)
      continue;

    SDNode *ResNode = Lowering.PostISelFolding(MachineNode, *CurDAG);
    if (ResNode != Node) {
      ReplaceUses(Node, ResNode);
    }
  }
}
