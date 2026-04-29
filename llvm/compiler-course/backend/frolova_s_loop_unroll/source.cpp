#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

namespace {
class FrolovaSLoopUnroll : public ModulePass {
public:
  static char ID;
  FrolovaSLoopUnroll() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineModuleInfoWrapperPass>();
    ModulePass::getAnalysisUsage(AU);
  }

  StringRef getPassName() const override {
    return "Frolova's Loop Unroll & Branch Canonicalization Pass";
  }

  bool runOnModule(Module &M) override;

private:
  bool processLoop(MachineLoop *L, MachineFunction &MF,
                   const TargetInstrInfo *TII);
  bool unrollLoop(MachineLoop *L, MachineFunction &MF,
                  const TargetInstrInfo *TII, unsigned UnrollCount);
  void canonizeBranches(MachineFunction &MF);
};

char FrolovaSLoopUnroll::ID = 0;

bool FrolovaSLoopUnroll::processLoop(MachineLoop *L, MachineFunction &MF,
                                     const TargetInstrInfo *TII) {
  bool Changed = false;

  for (MachineLoop *InnerLoop : *L) {
    Changed |= processLoop(InnerLoop, MF, TII);
  }

  unsigned MaxUnrollFactor = 5;
  unsigned UnrollCount = MaxUnrollFactor;

  Changed |= unrollLoop(L, MF, TII, UnrollCount);
  return Changed;
}

bool FrolovaSLoopUnroll::unrollLoop(MachineLoop *L, MachineFunction &MF,
                                    const TargetInstrInfo *TII,
                                    unsigned UnrollCount) {
  if (UnrollCount <= 1)
    return false;

  MachineBasicBlock *LoopMBB = L->getHeader();
  if (L->getNumBlocks() != 1) {
    llvm::outs() << "Skipping complex loop (multiple blocks) in "
                 << MF.getName() << "\n";
    return false;
  }

  llvm::outs() << "Unrolling loop in " << MF.getName()
               << " (Factor: " << UnrollCount << ")\n";

  SmallVector<MachineInstr *, 8> InstrsToClone;
  for (MachineInstr &MI : *LoopMBB) {
    if (!MI.isBranch() && !MI.isTerminator()) {
      InstrsToClone.push_back(&MI);
    }
  }

  MachineBasicBlock::iterator InsertPos = LoopMBB->getFirstTerminator();

  for (unsigned i = 1; i < UnrollCount; ++i) {
    for (MachineInstr *MI : InstrsToClone) {
      MachineInstr *ClonedMI = MF.CloneMachineInstr(MI);
      LoopMBB->insert(InsertPos, ClonedMI);
    }
  }

  return true;
}

void FrolovaSLoopUnroll::canonizeBranches(MachineFunction &MF) {
  const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
  for (MachineBasicBlock &MBB : MF) {
    for (MachineInstr &MI : MBB) {
      if (MI.getOpcode() == X86::JCC_1) {
        // операнд условия – третий (индекс 2)
        unsigned Cond = MI.getOperand(2).getImm();
        if (Cond == X86::COND_E) {
          MI.setDesc(TII->get(X86::JE_1));
          MI.RemoveOperand(2);
        }
        // можно добавить другие условия по необходимости
      }
    }
  }
}

bool FrolovaSLoopUnroll::runOnModule(Module &M) {
  MachineModuleInfo &MMI = getAnalysis<MachineModuleInfoWrapperPass>().getMMI();
  bool Changed = false;

  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    MachineFunction *MF = MMI.getMachineFunction(F);
    if (!MF)
      continue;

    llvm::outs() << "Running FrolovaSLoopUnroll on function: " << MF->getName()
                 << '\n';

    // Вычисляем информацию о циклах для данной функции
    MachineLoopInfo MLI;
    MLI.runOnMachineFunction(*MF);

    const TargetInstrInfo *TII = MF->getSubtarget().getInstrInfo();

    // Разворачивание циклов
    for (MachineLoop *L : MLI) {
      Changed |= processLoop(L, *MF, TII);
    }

    // Канонизация обобщённых условных переходов
    canonizeBranches(*MF);
  }

  return Changed;
}
} // namespace

static RegisterPass<FrolovaSLoopUnroll>
    X("example-x86", "Frolova's Loop Unroll & Branch Canonicalization", false, false);