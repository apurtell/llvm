//= MSP430InstPrinter.h - Convert MSP430 MCInst to assembly syntax -*- C++ -*-//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints a MSP430 MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#ifndef MSP430INSTPRINTER_H
#define MSP430INSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"

namespace llvm {
  class MCOperand;

  class MSP430InstPrinter : public MCInstPrinter {
  public:
    MSP430InstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                      const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}

    virtual void printInst(const MCInst *MI, raw_ostream &O, StringRef Annot);

    // Autogenerated by tblgen.
    void printInstruction(const MCInst *MI, raw_ostream &O);
    static const char *getRegisterName(unsigned RegNo);

    void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O,
                      const char *Modifier = nullptr);
    void printPCRelImmOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
    void printSrcMemOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O,
                            const char *Modifier = nullptr);
    void printCCOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);

  };
}

#endif
