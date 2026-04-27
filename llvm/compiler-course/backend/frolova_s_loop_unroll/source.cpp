#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

using namespace llvm;

namespace {
class FrolovaSLoopUnroll : public MachineFunctionPass {
public:
  static char ID;
  FrolovaSLoopUnroll() : MachineFunctionPass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineLoopInfo>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  bool processLoop(MachineLoop *L, MachineFunction &MF,
                   const TargetInstrInfo *TII);
  bool unrollLoop(MachineLoop *L, MachineFunction &MF,
                  const TargetInstrInfo *TII, unsigned UnrollCount);
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

bool FrolovaSLoopUnroll::runOnMachineFunction(MachineFunction &MF) {
  llvm::outs() << "Running FrolovaSLoopUnroll on function: " << MF.getName()
               << '\n';

  MachineLoopInfo &MLI = getAnalysis<MachineLoopInfo>();
  const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();

  bool Changed = false;

  for (MachineLoop *L : MLI) {
    Changed |= processLoop(L, MF, TII);
  }

  return Changed;
}
} // namespace

static RegisterPass<FrolovaSLoopUnroll> X("example-x86", "FrolovaSLoopUnrollPass", false, false);

