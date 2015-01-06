//===-- PPCISelLowering.h - PPC32 DAG Lowering Interface --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that PPC uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_POWERPC_PPCISELLOWERING_H
#define LLVM_LIB_TARGET_POWERPC_PPCISELLOWERING_H

#include "PPC.h"
#include "PPCInstrInfo.h"
#include "PPCRegisterInfo.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/Target/TargetLowering.h"

namespace llvm {
  namespace PPCISD {
    enum NodeType {
      // Start the numbering where the builtin ops and target ops leave off.
      FIRST_NUMBER = ISD::BUILTIN_OP_END,

      /// FSEL - Traditional three-operand fsel node.
      ///
      FSEL,

      /// FCFID - The FCFID instruction, taking an f64 operand and producing
      /// and f64 value containing the FP representation of the integer that
      /// was temporarily in the f64 operand.
      FCFID,

      /// Newer FCFID[US] integer-to-floating-point conversion instructions for
      /// unsigned integers and single-precision outputs.
      FCFIDU, FCFIDS, FCFIDUS,

      /// FCTI[D,W]Z - The FCTIDZ and FCTIWZ instructions, taking an f32 or f64
      /// operand, producing an f64 value containing the integer representation
      /// of that FP value.
      FCTIDZ, FCTIWZ,

      /// Newer FCTI[D,W]UZ floating-point-to-integer conversion instructions for
      /// unsigned integers.
      FCTIDUZ, FCTIWUZ,

      /// Reciprocal estimate instructions (unary FP ops).
      FRE, FRSQRTE,

      // VMADDFP, VNMSUBFP - The VMADDFP and VNMSUBFP instructions, taking
      // three v4f32 operands and producing a v4f32 result.
      VMADDFP, VNMSUBFP,

      /// VPERM - The PPC VPERM Instruction.
      ///
      VPERM,

      /// The CMPB instruction (takes two operands of i32 or i64).
      CMPB,

      /// Hi/Lo - These represent the high and low 16-bit parts of a global
      /// address respectively.  These nodes have two operands, the first of
      /// which must be a TargetGlobalAddress, and the second of which must be a
      /// Constant.  Selected naively, these turn into 'lis G+C' and 'li G+C',
      /// though these are usually folded into other nodes.
      Hi, Lo,

      TOC_ENTRY,

      /// The following two target-specific nodes are used for calls through
      /// function pointers in the 64-bit SVR4 ABI.

      /// Like a regular LOAD but additionally taking/producing a flag.
      LOAD,

      /// Like LOAD (taking/producing a flag), but using r2 as hard-coded
      /// destination.
      LOAD_TOC,

      /// OPRC, CHAIN = DYNALLOC(CHAIN, NEGSIZE, FRAME_INDEX)
      /// This instruction is lowered in PPCRegisterInfo::eliminateFrameIndex to
      /// compute an allocation on the stack.
      DYNALLOC,

      /// GlobalBaseReg - On Darwin, this node represents the result of the mflr
      /// at function entry, used for PIC code.
      GlobalBaseReg,

      /// These nodes represent the 32-bit PPC shifts that operate on 6-bit
      /// shift amounts.  These nodes are generated by the multi-precision shift
      /// code.
      SRL, SRA, SHL,

      /// The combination of sra[wd]i and addze used to implemented signed
      /// integer division by a power of 2. The first operand is the dividend,
      /// and the second is the constant shift amount (representing the
      /// divisor).
      SRA_ADDZE,

      /// CALL - A direct function call.
      /// CALL_NOP is a call with the special NOP which follows 64-bit
      /// SVR4 calls.
      CALL, CALL_NOP,

      /// CALL_TLS and CALL_NOP_TLS - Versions of CALL and CALL_NOP used
      /// to access TLS variables.
      CALL_TLS, CALL_NOP_TLS,

      /// CHAIN,FLAG = MTCTR(VAL, CHAIN[, INFLAG]) - Directly corresponds to a
      /// MTCTR instruction.
      MTCTR,

      /// CHAIN,FLAG = BCTRL(CHAIN, INFLAG) - Directly corresponds to a
      /// BCTRL instruction.
      BCTRL,

      /// CHAIN,FLAG = BCTRL(CHAIN, ADDR, INFLAG) - The combination of a bctrl
      /// instruction and the TOC reload required on SVR4 PPC64.
      BCTRL_LOAD_TOC,

      /// Return with a flag operand, matched by 'blr'
      RET_FLAG,

      /// R32 = MFOCRF(CRREG, INFLAG) - Represents the MFOCRF instruction.
      /// This copies the bits corresponding to the specified CRREG into the
      /// resultant GPR.  Bits corresponding to other CR regs are undefined.
      MFOCRF,

      // FIXME: Remove these once the ANDI glue bug is fixed:
      /// i1 = ANDIo_1_[EQ|GT]_BIT(i32 or i64 x) - Represents the result of the
      /// eq or gt bit of CR0 after executing andi. x, 1. This is used to
      /// implement truncation of i32 or i64 to i1.
      ANDIo_1_EQ_BIT, ANDIo_1_GT_BIT,

      // READ_TIME_BASE - A read of the 64-bit time-base register on a 32-bit
      // target (returns (Lo, Hi)). It takes a chain operand.
      READ_TIME_BASE,

      // EH_SJLJ_SETJMP - SjLj exception handling setjmp.
      EH_SJLJ_SETJMP,

      // EH_SJLJ_LONGJMP - SjLj exception handling longjmp.
      EH_SJLJ_LONGJMP,

      /// RESVEC = VCMP(LHS, RHS, OPC) - Represents one of the altivec VCMP*
      /// instructions.  For lack of better number, we use the opcode number
      /// encoding for the OPC field to identify the compare.  For example, 838
      /// is VCMPGTSH.
      VCMP,

      /// RESVEC, OUTFLAG = VCMPo(LHS, RHS, OPC) - Represents one of the
      /// altivec VCMP*o instructions.  For lack of better number, we use the
      /// opcode number encoding for the OPC field to identify the compare.  For
      /// example, 838 is VCMPGTSH.
      VCMPo,

      /// CHAIN = COND_BRANCH CHAIN, CRRC, OPC, DESTBB [, INFLAG] - This
      /// corresponds to the COND_BRANCH pseudo instruction.  CRRC is the
      /// condition register to branch on, OPC is the branch opcode to use (e.g.
      /// PPC::BLE), DESTBB is the destination block to branch to, and INFLAG is
      /// an optional input flag argument.
      COND_BRANCH,

      /// CHAIN = BDNZ CHAIN, DESTBB - These are used to create counter-based
      /// loops.
      BDNZ, BDZ,

      /// F8RC = FADDRTZ F8RC, F8RC - This is an FADD done with rounding
      /// towards zero.  Used only as part of the long double-to-int
      /// conversion sequence.
      FADDRTZ,

      /// F8RC = MFFS - This moves the FPSCR (not modeled) into the register.
      MFFS,

      /// LARX = This corresponds to PPC l{w|d}arx instrcution: load and
      /// reserve indexed. This is used to implement atomic operations.
      LARX,

      /// STCX = This corresponds to PPC stcx. instrcution: store conditional
      /// indexed. This is used to implement atomic operations.
      STCX,

      /// TC_RETURN - A tail call return.
      ///   operand #0 chain
      ///   operand #1 callee (register or absolute)
      ///   operand #2 stack adjustment
      ///   operand #3 optional in flag
      TC_RETURN,

      /// ch, gl = CR6[UN]SET ch, inglue - Toggle CR bit 6 for SVR4 vararg calls
      CR6SET,
      CR6UNSET,

      /// GPRC = address of _GLOBAL_OFFSET_TABLE_. Used by initial-exec TLS
      /// on PPC32.
      PPC32_GOT,

      /// GPRC = address of _GLOBAL_OFFSET_TABLE_. Used by general dynamic and
      /// local dynamic TLS  on PPC32.
      PPC32_PICGOT,

      /// G8RC = ADDIS_GOT_TPREL_HA %X2, Symbol - Used by the initial-exec
      /// TLS model, produces an ADDIS8 instruction that adds the GOT
      /// base to sym\@got\@tprel\@ha.
      ADDIS_GOT_TPREL_HA,

      /// G8RC = LD_GOT_TPREL_L Symbol, G8RReg - Used by the initial-exec
      /// TLS model, produces a LD instruction with base register G8RReg
      /// and offset sym\@got\@tprel\@l.  This completes the addition that
      /// finds the offset of "sym" relative to the thread pointer.
      LD_GOT_TPREL_L,

      /// G8RC = ADD_TLS G8RReg, Symbol - Used by the initial-exec TLS
      /// model, produces an ADD instruction that adds the contents of
      /// G8RReg to the thread pointer.  Symbol contains a relocation
      /// sym\@tls which is to be replaced by the thread pointer and
      /// identifies to the linker that the instruction is part of a
      /// TLS sequence.
      ADD_TLS,

      /// G8RC = ADDIS_TLSGD_HA %X2, Symbol - For the general-dynamic TLS
      /// model, produces an ADDIS8 instruction that adds the GOT base
      /// register to sym\@got\@tlsgd\@ha.
      ADDIS_TLSGD_HA,

      /// G8RC = ADDI_TLSGD_L G8RReg, Symbol - For the general-dynamic TLS
      /// model, produces an ADDI8 instruction that adds G8RReg to
      /// sym\@got\@tlsgd\@l.
      ADDI_TLSGD_L,

      /// G8RC = ADDIS_TLSLD_HA %X2, Symbol - For the local-dynamic TLS
      /// model, produces an ADDIS8 instruction that adds the GOT base
      /// register to sym\@got\@tlsld\@ha.
      ADDIS_TLSLD_HA,

      /// G8RC = ADDI_TLSLD_L G8RReg, Symbol - For the local-dynamic TLS
      /// model, produces an ADDI8 instruction that adds G8RReg to
      /// sym\@got\@tlsld\@l.
      ADDI_TLSLD_L,

      /// G8RC = ADDIS_DTPREL_HA %X3, Symbol, Chain - For the
      /// local-dynamic TLS model, produces an ADDIS8 instruction
      /// that adds X3 to sym\@dtprel\@ha. The Chain operand is needed
      /// to tie this in place following a copy to %X3 from the result
      /// of a GET_TLSLD_ADDR.
      ADDIS_DTPREL_HA,

      /// G8RC = ADDI_DTPREL_L G8RReg, Symbol - For the local-dynamic TLS
      /// model, produces an ADDI8 instruction that adds G8RReg to
      /// sym\@got\@dtprel\@l.
      ADDI_DTPREL_L,

      /// VRRC = VADD_SPLAT Elt, EltSize - Temporary node to be expanded
      /// during instruction selection to optimize a BUILD_VECTOR into
      /// operations on splats.  This is necessary to avoid losing these
      /// optimizations due to constant folding.
      VADD_SPLAT,

      /// CHAIN = SC CHAIN, Imm128 - System call.  The 7-bit unsigned
      /// operand identifies the operating system entry point.
      SC,

      /// VSRC, CHAIN = XXSWAPD CHAIN, VSRC - Occurs only for little
      /// endian.  Maps to an xxswapd instruction that corrects an lxvd2x
      /// or stxvd2x instruction.  The chain is necessary because the
      /// sequence replaces a load and needs to provide the same number
      /// of outputs.
      XXSWAPD,

      /// CHAIN = STBRX CHAIN, GPRC, Ptr, Type - This is a
      /// byte-swapping store instruction.  It byte-swaps the low "Type" bits of
      /// the GPRC input, then stores it through Ptr.  Type can be either i16 or
      /// i32.
      STBRX = ISD::FIRST_TARGET_MEMORY_OPCODE,

      /// GPRC, CHAIN = LBRX CHAIN, Ptr, Type - This is a
      /// byte-swapping load instruction.  It loads "Type" bits, byte swaps it,
      /// then puts it in the bottom bits of the GPRC.  TYPE can be either i16
      /// or i32.
      LBRX,

      /// STFIWX - The STFIWX instruction.  The first operand is an input token
      /// chain, then an f64 value to store, then an address to store it to.
      STFIWX,

      /// GPRC, CHAIN = LFIWAX CHAIN, Ptr - This is a floating-point
      /// load which sign-extends from a 32-bit integer value into the
      /// destination 64-bit register.
      LFIWAX,

      /// GPRC, CHAIN = LFIWZX CHAIN, Ptr - This is a floating-point
      /// load which zero-extends from a 32-bit integer value into the
      /// destination 64-bit register.
      LFIWZX,

      /// G8RC = ADDIS_TOC_HA %X2, Symbol - For medium and large code model,
      /// produces an ADDIS8 instruction that adds the TOC base register to
      /// sym\@toc\@ha.
      ADDIS_TOC_HA,

      /// G8RC = LD_TOC_L Symbol, G8RReg - For medium and large code model,
      /// produces a LD instruction with base register G8RReg and offset
      /// sym\@toc\@l. Preceded by an ADDIS_TOC_HA to form a full 32-bit offset.
      LD_TOC_L,

      /// G8RC = ADDI_TOC_L G8RReg, Symbol - For medium code model, produces
      /// an ADDI8 instruction that adds G8RReg to sym\@toc\@l.
      /// Preceded by an ADDIS_TOC_HA to form a full 32-bit offset.
      ADDI_TOC_L,

      /// VSRC, CHAIN = LXVD2X_LE CHAIN, Ptr - Occurs only for little endian.
      /// Maps directly to an lxvd2x instruction that will be followed by
      /// an xxswapd.
      LXVD2X,

      /// CHAIN = STXVD2X CHAIN, VSRC, Ptr - Occurs only for little endian.
      /// Maps directly to an stxvd2x instruction that will be preceded by
      /// an xxswapd.
      STXVD2X
    };
  }

  /// Define some predicates that are used for node matching.
  namespace PPC {
    /// isVPKUHUMShuffleMask - Return true if this is the shuffle mask for a
    /// VPKUHUM instruction.
    bool isVPKUHUMShuffleMask(ShuffleVectorSDNode *N, unsigned ShuffleKind,
                              SelectionDAG &DAG);

    /// isVPKUWUMShuffleMask - Return true if this is the shuffle mask for a
    /// VPKUWUM instruction.
    bool isVPKUWUMShuffleMask(ShuffleVectorSDNode *N, unsigned ShuffleKind,
                              SelectionDAG &DAG);

    /// isVMRGLShuffleMask - Return true if this is a shuffle mask suitable for
    /// a VRGL* instruction with the specified unit size (1,2 or 4 bytes).
    bool isVMRGLShuffleMask(ShuffleVectorSDNode *N, unsigned UnitSize,
                            unsigned ShuffleKind, SelectionDAG &DAG);

    /// isVMRGHShuffleMask - Return true if this is a shuffle mask suitable for
    /// a VRGH* instruction with the specified unit size (1,2 or 4 bytes).
    bool isVMRGHShuffleMask(ShuffleVectorSDNode *N, unsigned UnitSize,
                            unsigned ShuffleKind, SelectionDAG &DAG);

    /// isVSLDOIShuffleMask - If this is a vsldoi shuffle mask, return the
    /// shift amount, otherwise return -1.
    int isVSLDOIShuffleMask(SDNode *N, unsigned ShuffleKind,
                            SelectionDAG &DAG);

    /// isSplatShuffleMask - Return true if the specified VECTOR_SHUFFLE operand
    /// specifies a splat of a single element that is suitable for input to
    /// VSPLTB/VSPLTH/VSPLTW.
    bool isSplatShuffleMask(ShuffleVectorSDNode *N, unsigned EltSize);

    /// isAllNegativeZeroVector - Returns true if all elements of build_vector
    /// are -0.0.
    bool isAllNegativeZeroVector(SDNode *N);

    /// getVSPLTImmediate - Return the appropriate VSPLT* immediate to splat the
    /// specified isSplatShuffleMask VECTOR_SHUFFLE mask.
    unsigned getVSPLTImmediate(SDNode *N, unsigned EltSize, SelectionDAG &DAG);

    /// get_VSPLTI_elt - If this is a build_vector of constants which can be
    /// formed by using a vspltis[bhw] instruction of the specified element
    /// size, return the constant being splatted.  The ByteSize field indicates
    /// the number of bytes of each element [124] -> [bhw].
    SDValue get_VSPLTI_elt(SDNode *N, unsigned ByteSize, SelectionDAG &DAG);
  }

  class PPCSubtarget;
  class PPCTargetLowering : public TargetLowering {
    const PPCSubtarget &Subtarget;

  public:
    explicit PPCTargetLowering(const PPCTargetMachine &TM);

    /// getTargetNodeName() - This method returns the name of a target specific
    /// DAG node.
    const char *getTargetNodeName(unsigned Opcode) const override;

    MVT getScalarShiftAmountTy(EVT LHSTy) const override { return MVT::i32; }

    bool isCheapToSpeculateCttz() const override {
      return true;
    }

    bool isCheapToSpeculateCtlz() const override {
      return true;
    }

    /// getSetCCResultType - Return the ISD::SETCC ValueType
    EVT getSetCCResultType(LLVMContext &Context, EVT VT) const override;

    /// Return true if target always beneficiates from combining into FMA for a
    /// given value type. This must typically return false on targets where FMA
    /// takes more cycles to execute than FADD.
    bool enableAggressiveFMAFusion(EVT VT) const override;

    /// getPreIndexedAddressParts - returns true by value, base pointer and
    /// offset pointer and addressing mode by reference if the node's address
    /// can be legally represented as pre-indexed load / store address.
    bool getPreIndexedAddressParts(SDNode *N, SDValue &Base,
                                   SDValue &Offset,
                                   ISD::MemIndexedMode &AM,
                                   SelectionDAG &DAG) const override;

    /// SelectAddressRegReg - Given the specified addressed, check to see if it
    /// can be represented as an indexed [r+r] operation.  Returns false if it
    /// can be more efficiently represented with [r+imm].
    bool SelectAddressRegReg(SDValue N, SDValue &Base, SDValue &Index,
                             SelectionDAG &DAG) const;

    /// SelectAddressRegImm - Returns true if the address N can be represented
    /// by a base register plus a signed 16-bit displacement [r+imm], and if it
    /// is not better represented as reg+reg.  If Aligned is true, only accept
    /// displacements suitable for STD and friends, i.e. multiples of 4.
    bool SelectAddressRegImm(SDValue N, SDValue &Disp, SDValue &Base,
                             SelectionDAG &DAG, bool Aligned) const;

    /// SelectAddressRegRegOnly - Given the specified addressed, force it to be
    /// represented as an indexed [r+r] operation.
    bool SelectAddressRegRegOnly(SDValue N, SDValue &Base, SDValue &Index,
                                 SelectionDAG &DAG) const;

    Sched::Preference getSchedulingPreference(SDNode *N) const override;

    /// LowerOperation - Provide custom lowering hooks for some operations.
    ///
    SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

    /// ReplaceNodeResults - Replace the results of node with an illegal result
    /// type with new values built out of custom code.
    ///
    void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue>&Results,
                            SelectionDAG &DAG) const override;

    SDValue expandVSXLoadForLE(SDNode *N, DAGCombinerInfo &DCI) const;
    SDValue expandVSXStoreForLE(SDNode *N, DAGCombinerInfo &DCI) const;

    SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

    SDValue BuildSDIVPow2(SDNode *N, const APInt &Divisor, SelectionDAG &DAG,
                          std::vector<SDNode *> *Created) const override;

    unsigned getRegisterByName(const char* RegName, EVT VT) const override;

    void computeKnownBitsForTargetNode(const SDValue Op,
                                       APInt &KnownZero,
                                       APInt &KnownOne,
                                       const SelectionDAG &DAG,
                                       unsigned Depth = 0) const override;

    unsigned getPrefLoopAlignment(MachineLoop *ML) const override;

    Instruction* emitLeadingFence(IRBuilder<> &Builder, AtomicOrdering Ord,
                                  bool IsStore, bool IsLoad) const override;
    Instruction* emitTrailingFence(IRBuilder<> &Builder, AtomicOrdering Ord,
                                   bool IsStore, bool IsLoad) const override;

    MachineBasicBlock *
      EmitInstrWithCustomInserter(MachineInstr *MI,
                                  MachineBasicBlock *MBB) const override;
    MachineBasicBlock *EmitAtomicBinary(MachineInstr *MI,
                                        MachineBasicBlock *MBB, bool is64Bit,
                                        unsigned BinOpcode) const;
    MachineBasicBlock *EmitPartwordAtomicBinary(MachineInstr *MI,
                                                MachineBasicBlock *MBB,
                                            bool is8bit, unsigned Opcode) const;

    MachineBasicBlock *emitEHSjLjSetJmp(MachineInstr *MI,
                                        MachineBasicBlock *MBB) const;

    MachineBasicBlock *emitEHSjLjLongJmp(MachineInstr *MI,
                                         MachineBasicBlock *MBB) const;

    ConstraintType
    getConstraintType(const std::string &Constraint) const override;

    /// Examine constraint string and operand type and determine a weight value.
    /// The operand object must already have been set up with the operand type.
    ConstraintWeight getSingleConstraintMatchWeight(
      AsmOperandInfo &info, const char *constraint) const override;

    std::pair<unsigned, const TargetRegisterClass*>
      getRegForInlineAsmConstraint(const std::string &Constraint,
                                   MVT VT) const override;

    /// getByValTypeAlignment - Return the desired alignment for ByVal aggregate
    /// function arguments in the caller parameter area.  This is the actual
    /// alignment, not its logarithm.
    unsigned getByValTypeAlignment(Type *Ty) const override;

    /// LowerAsmOperandForConstraint - Lower the specified operand into the Ops
    /// vector.  If it is invalid, don't add anything to Ops.
    void LowerAsmOperandForConstraint(SDValue Op,
                                      std::string &Constraint,
                                      std::vector<SDValue> &Ops,
                                      SelectionDAG &DAG) const override;

    /// isLegalAddressingMode - Return true if the addressing mode represented
    /// by AM is legal for this target, for a load/store of the specified type.
    bool isLegalAddressingMode(const AddrMode &AM, Type *Ty) const override;

    /// isLegalICmpImmediate - Return true if the specified immediate is legal
    /// icmp immediate, that is the target has icmp instructions which can
    /// compare a register against the immediate without having to materialize
    /// the immediate into a register.
    bool isLegalICmpImmediate(int64_t Imm) const override;

    /// isLegalAddImmediate - Return true if the specified immediate is legal
    /// add immediate, that is the target has add instructions which can
    /// add a register and the immediate without having to materialize
    /// the immediate into a register.
    bool isLegalAddImmediate(int64_t Imm) const override;

    /// isTruncateFree - Return true if it's free to truncate a value of
    /// type Ty1 to type Ty2. e.g. On PPC it's free to truncate a i64 value in
    /// register X1 to i32 by referencing its sub-register R1.
    bool isTruncateFree(Type *Ty1, Type *Ty2) const override;
    bool isTruncateFree(EVT VT1, EVT VT2) const override;

    /// \brief Returns true if it is beneficial to convert a load of a constant
    /// to just the constant itself.
    bool shouldConvertConstantLoadToIntImm(const APInt &Imm,
                                           Type *Ty) const override;

    bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const override;

    bool getTgtMemIntrinsic(IntrinsicInfo &Info,
                            const CallInst &I,
                            unsigned Intrinsic) const override;

    /// getOptimalMemOpType - Returns the target specific optimal type for load
    /// and store operations as a result of memset, memcpy, and memmove
    /// lowering. If DstAlign is zero that means it's safe to destination
    /// alignment can satisfy any constraint. Similarly if SrcAlign is zero it
    /// means there isn't a need to check it against alignment requirement,
    /// probably because the source does not need to be loaded. If 'IsMemset' is
    /// true, that means it's expanding a memset. If 'ZeroMemset' is true, that
    /// means it's a memset of zero. 'MemcpyStrSrc' indicates whether the memcpy
    /// source is constant so it does not need to be loaded.
    /// It returns EVT::Other if the type should be determined using generic
    /// target-independent logic.
    EVT
    getOptimalMemOpType(uint64_t Size, unsigned DstAlign, unsigned SrcAlign,
                        bool IsMemset, bool ZeroMemset, bool MemcpyStrSrc,
                        MachineFunction &MF) const override;

    /// Is unaligned memory access allowed for the given type, and is it fast
    /// relative to software emulation.
    bool allowsMisalignedMemoryAccesses(EVT VT,
                                        unsigned AddrSpace,
                                        unsigned Align = 1,
                                        bool *Fast = nullptr) const override;

    /// isFMAFasterThanFMulAndFAdd - Return true if an FMA operation is faster
    /// than a pair of fmul and fadd instructions. fmuladd intrinsics will be
    /// expanded to FMAs when this method returns true, otherwise fmuladd is
    /// expanded to fmul + fadd.
    bool isFMAFasterThanFMulAndFAdd(EVT VT) const override;

    // Should we expand the build vector with shuffles?
    bool
    shouldExpandBuildVectorWithShuffles(EVT VT,
                                        unsigned DefinedValues) const override;

    /// createFastISel - This method returns a target-specific FastISel object,
    /// or null if the target does not support "fast" instruction selection.
    FastISel *createFastISel(FunctionLoweringInfo &FuncInfo,
                             const TargetLibraryInfo *LibInfo) const override;

    /// \brief Returns true if an argument of type Ty needs to be passed in a
    /// contiguous block of registers in calling convention CallConv.
    bool functionArgumentNeedsConsecutiveRegisters(
      Type *Ty, CallingConv::ID CallConv, bool isVarArg) const override {
      // We support any array type as "consecutive" block in the parameter
      // save area.  The element type defines the alignment requirement and
      // whether the argument should go in GPRs, FPRs, or VRs if available.
      //
      // Note that clang uses this capability both to implement the ELFv2
      // homogeneous float/vector aggregate ABI, and to avoid having to use
      // "byval" when passing aggregates that might fully fit in registers.
      return Ty->isArrayTy();
    }

  private:

    struct ReuseLoadInfo {
      SDValue Ptr;
      SDValue Chain;
      SDValue ResChain;
      MachinePointerInfo MPI;
      bool IsInvariant;
      unsigned Alignment;
      AAMDNodes AAInfo;
      const MDNode *Ranges;

      ReuseLoadInfo() : IsInvariant(false), Alignment(0), Ranges(nullptr) {}
    };

    bool canReuseLoadAddress(SDValue Op, EVT MemVT, ReuseLoadInfo &RLI,
                             SelectionDAG &DAG) const;
    void spliceIntoChain(SDValue ResChain, SDValue NewResChain,
                         SelectionDAG &DAG) const;

    void LowerFP_TO_INTForReuse(SDValue Op, ReuseLoadInfo &RLI,
                                SelectionDAG &DAG, SDLoc dl) const;

    SDValue getFramePointerFrameIndex(SelectionDAG & DAG) const;
    SDValue getReturnAddrFrameIndex(SelectionDAG & DAG) const;

    bool
    IsEligibleForTailCallOptimization(SDValue Callee,
                                      CallingConv::ID CalleeCC,
                                      bool isVarArg,
                                      const SmallVectorImpl<ISD::InputArg> &Ins,
                                      SelectionDAG& DAG) const;

    SDValue EmitTailCallLoadFPAndRetAddr(SelectionDAG & DAG,
                                         int SPDiff,
                                         SDValue Chain,
                                         SDValue &LROpOut,
                                         SDValue &FPOpOut,
                                         bool isDarwinABI,
                                         SDLoc dl) const;

    SDValue LowerRETURNADDR(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerFRAMEADDR(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
    std::pair<SDValue,SDValue> lowerTLSCall(SDValue Op, SDLoc dl,
                                            SelectionDAG &DAG) const;
    SDValue LowerGlobalTLSAddress(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerJumpTable(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerINIT_TRAMPOLINE(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerADJUST_TRAMPOLINE(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG,
                         const PPCSubtarget &Subtarget) const;
    SDValue LowerVAARG(SDValue Op, SelectionDAG &DAG,
                       const PPCSubtarget &Subtarget) const;
    SDValue LowerVACOPY(SDValue Op, SelectionDAG &DAG,
                        const PPCSubtarget &Subtarget) const;
    SDValue LowerSTACKRESTORE(SDValue Op, SelectionDAG &DAG,
                                const PPCSubtarget &Subtarget) const;
    SDValue LowerDYNAMIC_STACKALLOC(SDValue Op, SelectionDAG &DAG,
                                      const PPCSubtarget &Subtarget) const;
    SDValue LowerLOAD(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSTORE(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerTRUNCATE(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerFP_TO_INT(SDValue Op, SelectionDAG &DAG, SDLoc dl) const;
    SDValue LowerINT_TO_FP(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerFLT_ROUNDS_(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSHL_PARTS(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSRL_PARTS(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSRA_PARTS(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerBUILD_VECTOR(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerVECTOR_SHUFFLE(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerINTRINSIC_WO_CHAIN(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSCALAR_TO_VECTOR(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSIGN_EXTEND_INREG(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerMUL(SDValue Op, SelectionDAG &DAG) const;

    SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                            CallingConv::ID CallConv, bool isVarArg,
                            const SmallVectorImpl<ISD::InputArg> &Ins,
                            SDLoc dl, SelectionDAG &DAG,
                            SmallVectorImpl<SDValue> &InVals) const;
    SDValue FinishCall(CallingConv::ID CallConv, SDLoc dl, bool isTailCall,
                       bool isVarArg,
                       SelectionDAG &DAG,
                       SmallVector<std::pair<unsigned, SDValue>, 8>
                         &RegsToPass,
                       SDValue InFlag, SDValue Chain,
                       SDValue &Callee,
                       int SPDiff, unsigned NumBytes,
                       const SmallVectorImpl<ISD::InputArg> &Ins,
                       SmallVectorImpl<SDValue> &InVals) const;

    SDValue
      LowerFormalArguments(SDValue Chain,
                           CallingConv::ID CallConv, bool isVarArg,
                           const SmallVectorImpl<ISD::InputArg> &Ins,
                           SDLoc dl, SelectionDAG &DAG,
                           SmallVectorImpl<SDValue> &InVals) const override;

    SDValue
      LowerCall(TargetLowering::CallLoweringInfo &CLI,
                SmallVectorImpl<SDValue> &InVals) const override;

    bool
      CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                   bool isVarArg,
                   const SmallVectorImpl<ISD::OutputArg> &Outs,
                   LLVMContext &Context) const override;

    SDValue
      LowerReturn(SDValue Chain,
                  CallingConv::ID CallConv, bool isVarArg,
                  const SmallVectorImpl<ISD::OutputArg> &Outs,
                  const SmallVectorImpl<SDValue> &OutVals,
                  SDLoc dl, SelectionDAG &DAG) const override;

    SDValue
      extendArgForPPC64(ISD::ArgFlagsTy Flags, EVT ObjectVT, SelectionDAG &DAG,
                        SDValue ArgVal, SDLoc dl) const;

    SDValue
      LowerFormalArguments_Darwin(SDValue Chain,
                                  CallingConv::ID CallConv, bool isVarArg,
                                  const SmallVectorImpl<ISD::InputArg> &Ins,
                                  SDLoc dl, SelectionDAG &DAG,
                                  SmallVectorImpl<SDValue> &InVals) const;
    SDValue
      LowerFormalArguments_64SVR4(SDValue Chain,
                                  CallingConv::ID CallConv, bool isVarArg,
                                  const SmallVectorImpl<ISD::InputArg> &Ins,
                                  SDLoc dl, SelectionDAG &DAG,
                                  SmallVectorImpl<SDValue> &InVals) const;
    SDValue
      LowerFormalArguments_32SVR4(SDValue Chain,
                                  CallingConv::ID CallConv, bool isVarArg,
                                  const SmallVectorImpl<ISD::InputArg> &Ins,
                                  SDLoc dl, SelectionDAG &DAG,
                                  SmallVectorImpl<SDValue> &InVals) const;

    SDValue
      createMemcpyOutsideCallSeq(SDValue Arg, SDValue PtrOff,
                                 SDValue CallSeqStart, ISD::ArgFlagsTy Flags,
                                 SelectionDAG &DAG, SDLoc dl) const;

    SDValue
      LowerCall_Darwin(SDValue Chain, SDValue Callee,
                       CallingConv::ID CallConv,
                       bool isVarArg, bool isTailCall,
                       const SmallVectorImpl<ISD::OutputArg> &Outs,
                       const SmallVectorImpl<SDValue> &OutVals,
                       const SmallVectorImpl<ISD::InputArg> &Ins,
                       SDLoc dl, SelectionDAG &DAG,
                       SmallVectorImpl<SDValue> &InVals) const;
    SDValue
      LowerCall_64SVR4(SDValue Chain, SDValue Callee,
                       CallingConv::ID CallConv,
                       bool isVarArg, bool isTailCall,
                       const SmallVectorImpl<ISD::OutputArg> &Outs,
                       const SmallVectorImpl<SDValue> &OutVals,
                       const SmallVectorImpl<ISD::InputArg> &Ins,
                       SDLoc dl, SelectionDAG &DAG,
                       SmallVectorImpl<SDValue> &InVals) const;
    SDValue
    LowerCall_32SVR4(SDValue Chain, SDValue Callee, CallingConv::ID CallConv,
                     bool isVarArg, bool isTailCall,
                     const SmallVectorImpl<ISD::OutputArg> &Outs,
                     const SmallVectorImpl<SDValue> &OutVals,
                     const SmallVectorImpl<ISD::InputArg> &Ins,
                     SDLoc dl, SelectionDAG &DAG,
                     SmallVectorImpl<SDValue> &InVals) const;

    SDValue lowerEH_SJLJ_SETJMP(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerEH_SJLJ_LONGJMP(SDValue Op, SelectionDAG &DAG) const;

    SDValue DAGCombineExtBoolTrunc(SDNode *N, DAGCombinerInfo &DCI) const;
    SDValue DAGCombineTruncBoolExt(SDNode *N, DAGCombinerInfo &DCI) const;
    SDValue combineFPToIntToFP(SDNode *N, DAGCombinerInfo &DCI) const;

    SDValue getRsqrtEstimate(SDValue Operand, DAGCombinerInfo &DCI,
                             unsigned &RefinementSteps,
                             bool &UseOneConstNR) const override;
    SDValue getRecipEstimate(SDValue Operand, DAGCombinerInfo &DCI,
                             unsigned &RefinementSteps) const override;
    bool combineRepeatedFPDivisors(unsigned NumUsers) const override;

    CCAssignFn *useFastISelCCs(unsigned Flag) const;
  };

  namespace PPC {
    FastISel *createFastISel(FunctionLoweringInfo &FuncInfo,
                             const TargetLibraryInfo *LibInfo);
  }

  bool CC_PPC32_SVR4_Custom_Dummy(unsigned &ValNo, MVT &ValVT, MVT &LocVT,
                                  CCValAssign::LocInfo &LocInfo,
                                  ISD::ArgFlagsTy &ArgFlags,
                                  CCState &State);

  bool CC_PPC32_SVR4_Custom_AlignArgRegs(unsigned &ValNo, MVT &ValVT,
                                         MVT &LocVT,
                                         CCValAssign::LocInfo &LocInfo,
                                         ISD::ArgFlagsTy &ArgFlags,
                                         CCState &State);

  bool CC_PPC32_SVR4_Custom_AlignFPArgRegs(unsigned &ValNo, MVT &ValVT,
                                           MVT &LocVT,
                                           CCValAssign::LocInfo &LocInfo,
                                           ISD::ArgFlagsTy &ArgFlags,
                                           CCState &State);
}

#endif   // LLVM_TARGET_POWERPC_PPC32ISELLOWERING_H
