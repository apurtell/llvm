//===-- R600ControlFlowFinalizer.cpp - Finalize Control Flow Inst----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This pass compute turns all control flow pseudo instructions into native one
/// computing their address on the fly ; it also sets STACK_SIZE info.
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "r600cf"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "AMDGPU.h"
#include "R600Defines.h"
#include "R600InstrInfo.h"
#include "R600MachineFunctionInfo.h"
#include "R600RegisterInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

namespace llvm {

class R600ControlFlowFinalizer : public MachineFunctionPass {

private:
  enum ControlFlowInstruction {
    CF_TC,
    CF_CALL_FS,
    CF_WHILE_LOOP,
    CF_END_LOOP,
    CF_LOOP_BREAK,
    CF_LOOP_CONTINUE,
    CF_JUMP,
    CF_ELSE,
    CF_POP,
    CF_END
  };

  static char ID;
  const R600InstrInfo *TII;
  unsigned MaxFetchInst;
  const AMDGPUSubtarget &ST;

  bool isFetch(const MachineInstr *MI) const {
    switch (MI->getOpcode()) {
    case AMDGPU::TEX_VTX_CONSTBUF:
    case AMDGPU::TEX_VTX_TEXBUF:
    case AMDGPU::TEX_LD:
    case AMDGPU::TEX_GET_TEXTURE_RESINFO:
    case AMDGPU::TEX_GET_GRADIENTS_H:
    case AMDGPU::TEX_GET_GRADIENTS_V:
    case AMDGPU::TEX_SET_GRADIENTS_H:
    case AMDGPU::TEX_SET_GRADIENTS_V:
    case AMDGPU::TEX_SAMPLE:
    case AMDGPU::TEX_SAMPLE_C:
    case AMDGPU::TEX_SAMPLE_L:
    case AMDGPU::TEX_SAMPLE_C_L:
    case AMDGPU::TEX_SAMPLE_LB:
    case AMDGPU::TEX_SAMPLE_C_LB:
    case AMDGPU::TEX_SAMPLE_G:
    case AMDGPU::TEX_SAMPLE_C_G:
    case AMDGPU::TXD:
    case AMDGPU::TXD_SHADOW:
    case AMDGPU::VTX_READ_GLOBAL_8_eg:
    case AMDGPU::VTX_READ_GLOBAL_32_eg:
    case AMDGPU::VTX_READ_GLOBAL_128_eg:
    case AMDGPU::VTX_READ_PARAM_8_eg:
    case AMDGPU::VTX_READ_PARAM_16_eg:
    case AMDGPU::VTX_READ_PARAM_32_eg:
    case AMDGPU::VTX_READ_PARAM_128_eg:
     return true;
    default:
      return false;
    }
  }

  bool IsTrivialInst(MachineInstr *MI) const {
    switch (MI->getOpcode()) {
    case AMDGPU::KILL:
    case AMDGPU::RETURN:
      return true;
    default:
      return false;
    }
  }

  const MCInstrDesc &getHWInstrDesc(ControlFlowInstruction CFI) const {
    unsigned Opcode = 0;
    bool isEg = (ST.device()->getGeneration() >= AMDGPUDeviceInfo::HD5XXX);
    switch (CFI) {
    case CF_TC:
      Opcode = isEg ? AMDGPU::CF_TC_EG : AMDGPU::CF_TC_R600;
      break;
    case CF_CALL_FS:
      Opcode = isEg ? AMDGPU::CF_CALL_FS_EG : AMDGPU::CF_CALL_FS_R600;
      break;
    case CF_WHILE_LOOP:
      Opcode = isEg ? AMDGPU::WHILE_LOOP_EG : AMDGPU::WHILE_LOOP_R600;
      break;
    case CF_END_LOOP:
      Opcode = isEg ? AMDGPU::END_LOOP_EG : AMDGPU::END_LOOP_R600;
      break;
    case CF_LOOP_BREAK:
      Opcode = isEg ? AMDGPU::LOOP_BREAK_EG : AMDGPU::LOOP_BREAK_R600;
      break;
    case CF_LOOP_CONTINUE:
      Opcode = isEg ? AMDGPU::CF_CONTINUE_EG : AMDGPU::CF_CONTINUE_R600;
      break;
    case CF_JUMP:
      Opcode = isEg ? AMDGPU::CF_JUMP_EG : AMDGPU::CF_JUMP_R600;
      break;
    case CF_ELSE:
      Opcode = isEg ? AMDGPU::CF_ELSE_EG : AMDGPU::CF_ELSE_R600;
      break;
    case CF_POP:
      Opcode = isEg ? AMDGPU::POP_EG : AMDGPU::POP_R600;
      break;
    case CF_END:
      if (ST.device()->getDeviceFlag() == OCL_DEVICE_CAYMAN) {
        Opcode = AMDGPU::CF_END_CM;
        break;
      }
      Opcode = isEg ? AMDGPU::CF_END_EG : AMDGPU::CF_END_R600;
      break;
    }
    assert (Opcode && "No opcode selected");
    return TII->get(Opcode);
  }

  MachineBasicBlock::iterator
  MakeFetchClause(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
      unsigned CfAddress) const {
    MachineBasicBlock::iterator ClauseHead = I;
    unsigned AluInstCount = 0;
    for (MachineBasicBlock::iterator E = MBB.end(); I != E; ++I) {
      if (IsTrivialInst(I))
        continue;
      if (!isFetch(I))
        break;
      AluInstCount ++;
      if (AluInstCount > MaxFetchInst)
        break;
    }
    BuildMI(MBB, ClauseHead, MBB.findDebugLoc(ClauseHead),
        getHWInstrDesc(CF_TC))
        .addImm(CfAddress) // ADDR
        .addImm(AluInstCount); // COUNT
    return I;
  }
  void CounterPropagateAddr(MachineInstr *MI, unsigned Addr) const {
    MI->getOperand(0).setImm(Addr + MI->getOperand(0).getImm());
  }
  void CounterPropagateAddr(std::set<MachineInstr *> MIs, unsigned Addr)
      const {
    for (std::set<MachineInstr *>::iterator It = MIs.begin(), E = MIs.end();
        It != E; ++It) {
      MachineInstr *MI = *It;
      CounterPropagateAddr(MI, Addr);
    }
  }

  unsigned getHWStackSize(unsigned StackSubEntry, bool hasPush) const {
    switch (ST.device()->getGeneration()) {
    case AMDGPUDeviceInfo::HD4XXX:
      if (hasPush)
        StackSubEntry += 2;
      break;
    case AMDGPUDeviceInfo::HD5XXX:
      if (hasPush)
        StackSubEntry ++;
    case AMDGPUDeviceInfo::HD6XXX:
      StackSubEntry += 2;
      break;
    }
    return (StackSubEntry + 3)/4; // Need ceil value of StackSubEntry/4
  }

public:
  R600ControlFlowFinalizer(TargetMachine &tm) : MachineFunctionPass(ID),
    TII (static_cast<const R600InstrInfo *>(tm.getInstrInfo())),
    ST(tm.getSubtarget<AMDGPUSubtarget>()) {
      const AMDGPUSubtarget &ST = tm.getSubtarget<AMDGPUSubtarget>();
      if (ST.device()->getGeneration() <= AMDGPUDeviceInfo::HD4XXX)
        MaxFetchInst = 8;
      else
        MaxFetchInst = 16;
  }

  virtual bool runOnMachineFunction(MachineFunction &MF) {
    unsigned MaxStack = 0;
    unsigned CurrentStack = 0;
    bool hasPush;
    for (MachineFunction::iterator MB = MF.begin(), ME = MF.end(); MB != ME;
        ++MB) {
      MachineBasicBlock &MBB = *MB;
      unsigned CfCount = 0;
      std::vector<std::pair<unsigned, std::set<MachineInstr *> > > LoopStack;
      std::vector<MachineInstr * > IfThenElseStack;
      R600MachineFunctionInfo *MFI = MF.getInfo<R600MachineFunctionInfo>();
      if (MFI->ShaderType == 1) {
        BuildMI(MBB, MBB.begin(), MBB.findDebugLoc(MBB.begin()),
            getHWInstrDesc(CF_CALL_FS));
        CfCount++;
      }
      for (MachineBasicBlock::iterator I = MBB.begin(), E = MBB.end();
          I != E;) {
        if (isFetch(I)) {
          DEBUG(dbgs() << CfCount << ":"; I->dump(););
          I = MakeFetchClause(MBB, I, 0);
          CfCount++;
          continue;
        }

        MachineBasicBlock::iterator MI = I;
        I++;
        switch (MI->getOpcode()) {
        case AMDGPU::CF_ALU_PUSH_BEFORE:
          CurrentStack++;
          MaxStack = std::max(MaxStack, CurrentStack);
          hasPush = true;
        case AMDGPU::CF_ALU:
        case AMDGPU::EG_ExportBuf:
        case AMDGPU::EG_ExportSwz:
        case AMDGPU::R600_ExportBuf:
        case AMDGPU::R600_ExportSwz:
        case AMDGPU::RAT_WRITE_CACHELESS_32_eg:
        case AMDGPU::RAT_WRITE_CACHELESS_128_eg:
          DEBUG(dbgs() << CfCount << ":"; MI->dump(););
          CfCount++;
          break;
        case AMDGPU::WHILELOOP: {
          CurrentStack+=4;
          MaxStack = std::max(MaxStack, CurrentStack);
          MachineInstr *MIb = BuildMI(MBB, MI, MBB.findDebugLoc(MI),
              getHWInstrDesc(CF_WHILE_LOOP))
              .addImm(1);
          std::pair<unsigned, std::set<MachineInstr *> > Pair(CfCount,
              std::set<MachineInstr *>());
          Pair.second.insert(MIb);
          LoopStack.push_back(Pair);
          MI->eraseFromParent();
          CfCount++;
          break;
        }
        case AMDGPU::ENDLOOP: {
          CurrentStack-=4;
          std::pair<unsigned, std::set<MachineInstr *> > Pair =
              LoopStack.back();
          LoopStack.pop_back();
          CounterPropagateAddr(Pair.second, CfCount);
          BuildMI(MBB, MI, MBB.findDebugLoc(MI), getHWInstrDesc(CF_END_LOOP))
              .addImm(Pair.first + 1);
          MI->eraseFromParent();
          CfCount++;
          break;
        }
        case AMDGPU::IF_PREDICATE_SET: {
          MachineInstr *MIb = BuildMI(MBB, MI, MBB.findDebugLoc(MI),
              getHWInstrDesc(CF_JUMP))
              .addImm(0)
              .addImm(0);
          IfThenElseStack.push_back(MIb);
          DEBUG(dbgs() << CfCount << ":"; MIb->dump(););
          MI->eraseFromParent();
          CfCount++;
          break;
        }
        case AMDGPU::ELSE: {
          MachineInstr * JumpInst = IfThenElseStack.back();
          IfThenElseStack.pop_back();
          CounterPropagateAddr(JumpInst, CfCount);
          MachineInstr *MIb = BuildMI(MBB, MI, MBB.findDebugLoc(MI),
              getHWInstrDesc(CF_ELSE))
              .addImm(0)
              .addImm(1);
          DEBUG(dbgs() << CfCount << ":"; MIb->dump(););
          IfThenElseStack.push_back(MIb);
          MI->eraseFromParent();
          CfCount++;
          break;
        }
        case AMDGPU::ENDIF: {
          CurrentStack--;
          MachineInstr *IfOrElseInst = IfThenElseStack.back();
          IfThenElseStack.pop_back();
          CounterPropagateAddr(IfOrElseInst, CfCount + 1);
          MachineInstr *MIb = BuildMI(MBB, MI, MBB.findDebugLoc(MI),
              getHWInstrDesc(CF_POP))
              .addImm(CfCount + 1)
              .addImm(1);
          (void)MIb;
          DEBUG(dbgs() << CfCount << ":"; MIb->dump(););
          MI->eraseFromParent();
          CfCount++;
          break;
        }
        case AMDGPU::PREDICATED_BREAK: {
          CurrentStack--;
          CfCount += 3;
          BuildMI(MBB, MI, MBB.findDebugLoc(MI), getHWInstrDesc(CF_JUMP))
              .addImm(CfCount)
              .addImm(1);
          MachineInstr *MIb = BuildMI(MBB, MI, MBB.findDebugLoc(MI),
              getHWInstrDesc(CF_LOOP_BREAK))
              .addImm(0);
          BuildMI(MBB, MI, MBB.findDebugLoc(MI), getHWInstrDesc(CF_POP))
              .addImm(CfCount)
              .addImm(1);
          LoopStack.back().second.insert(MIb);
          MI->eraseFromParent();
          break;
        }
        case AMDGPU::CONTINUE: {
          MachineInstr *MIb = BuildMI(MBB, MI, MBB.findDebugLoc(MI),
              getHWInstrDesc(CF_LOOP_CONTINUE))
              .addImm(0);
          LoopStack.back().second.insert(MIb);
          MI->eraseFromParent();
          CfCount++;
          break;
        }
        case AMDGPU::RETURN: {
          BuildMI(MBB, MI, MBB.findDebugLoc(MI), getHWInstrDesc(CF_END));
          CfCount++;
          MI->eraseFromParent();
          if (CfCount % 2) {
            BuildMI(MBB, I, MBB.findDebugLoc(MI), TII->get(AMDGPU::PAD));
            CfCount++;
          }
        }
        default:
          break;
        }
      }
      MFI->StackSize = getHWStackSize(MaxStack, hasPush);
    }

    return false;
  }

  const char *getPassName() const {
    return "R600 Control Flow Finalizer Pass";
  }
};

char R600ControlFlowFinalizer::ID = 0;

}


llvm::FunctionPass *llvm::createR600ControlFlowFinalizer(TargetMachine &TM) {
  return new R600ControlFlowFinalizer(TM);
}
