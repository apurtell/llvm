//===-- HexagonAsmPrinter.h - Print machine code to an Hexagon .s file ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Hexagon Assembly printer class.
//
//===----------------------------------------------------------------------===//

#ifndef HEXAGONASMPRINTER_H
#define HEXAGONASMPRINTER_H

#include "Hexagon.h"
#include "HexagonTargetMachine.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
  class HexagonAsmPrinter : public AsmPrinter {
    const HexagonSubtarget *Subtarget;

  public:
    explicit HexagonAsmPrinter(TargetMachine &TM, MCStreamer &Streamer)
      : AsmPrinter(TM, Streamer) {
      Subtarget = &TM.getSubtarget<HexagonSubtarget>();
    }

    virtual const char *getPassName() const {
      return "Hexagon Assembly Printer";
    }

    bool isBlockOnlyReachableByFallthrough(const MachineBasicBlock *MBB) const;

    virtual void EmitInstruction(const MachineInstr *MI);
    virtual void EmitAlignment(unsigned NumBits,
                               const GlobalValue *GV = 0) const;

    void printOperand(const MachineInstr *MI, unsigned OpNo, raw_ostream &O);
    bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                         unsigned AsmVariant, const char *ExtraCode,
                         raw_ostream &OS);
    bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo,
                               unsigned AsmVariant, const char *ExtraCode,
                               raw_ostream &OS);

    //    void printMachineInstruction(const MachineInstr *MI);
    void printOp(const MachineOperand &MO, raw_ostream &O);

    /// printRegister - Print register according to target requirements.
    ///
    void printRegister(const MachineOperand &MO, bool R0AsZero,
                       raw_ostream &O) {
      unsigned RegNo = MO.getReg();
      assert(TargetRegisterInfo::isPhysicalRegister(RegNo) && "Not physreg??");
      O << getRegisterName(RegNo);
    }

    void printImmOperand(const MachineInstr *MI, unsigned OpNo,
                                raw_ostream &O) {
      int value = MI->getOperand(OpNo).getImm();
      O << value;
    }

    void printNegImmOperand(const MachineInstr *MI, unsigned OpNo,
                                   raw_ostream &O) {
      int value = MI->getOperand(OpNo).getImm();
      O << -value;
    }

    void printMEMriOperand(const MachineInstr *MI, unsigned OpNo,
                                  raw_ostream &O) {
      const MachineOperand &MO1 = MI->getOperand(OpNo);
      const MachineOperand &MO2 = MI->getOperand(OpNo+1);

      O << getRegisterName(MO1.getReg())
        << " + #"
        << (int) MO2.getImm();
    }

    void printFrameIndexOperand(const MachineInstr *MI, unsigned OpNo,
                                       raw_ostream &O) {
      const MachineOperand &MO1 = MI->getOperand(OpNo);
      const MachineOperand &MO2 = MI->getOperand(OpNo+1);

      O << getRegisterName(MO1.getReg())
        << ", #"
        << MO2.getImm();
    }

    void printBranchOperand(const MachineInstr *MI, unsigned OpNo,
                            raw_ostream &O) {
      // Branches can take an immediate operand.  This is used by the branch
      // selection pass to print $+8, an eight byte displacement from the PC.
      if (MI->getOperand(OpNo).isImm()) {
        O << "$+" << MI->getOperand(OpNo).getImm()*4;
      } else {
        printOp(MI->getOperand(OpNo), O);
      }
    }

    void printCallOperand(const MachineInstr *MI, unsigned OpNo,
                          raw_ostream &O) {
    }

    void printAbsAddrOperand(const MachineInstr *MI, unsigned OpNo,
                             raw_ostream &O) {
    }

    void printSymbolHi(const MachineInstr *MI, unsigned OpNo, raw_ostream &O) {
      O << "#HI(";
      if (MI->getOperand(OpNo).isImm()) {
        printImmOperand(MI, OpNo, O);
      }
      else {
        printOp(MI->getOperand(OpNo), O);
      }
      O << ")";
    }

    void printSymbolLo(const MachineInstr *MI, unsigned OpNo, raw_ostream &O) {
      O << "#HI(";
      if (MI->getOperand(OpNo).isImm()) {
        printImmOperand(MI, OpNo, O);
      }
      else {
        printOp(MI->getOperand(OpNo), O);
      }
      O << ")";
    }

    void printPredicateOperand(const MachineInstr *MI, unsigned OpNo,
                               raw_ostream &O);

#if 0
    void printModuleLevelGV(const GlobalVariable* GVar, raw_ostream &O);
#endif

    void printAddrModeBasePlusOffset(const MachineInstr *MI, int OpNo,
                                     raw_ostream &O);

    void printGlobalOperand(const MachineInstr *MI, int OpNo, raw_ostream &O);
    void printJumpTable(const MachineInstr *MI, int OpNo, raw_ostream &O);
    void printConstantPool(const MachineInstr *MI, int OpNo, raw_ostream &O);

    static const char *getRegisterName(unsigned RegNo);

#if 0
    void EmitStartOfAsmFile(Module &M);
#endif
  };

} // end of llvm namespace

#endif
