#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Region.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Tools/Plugins/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace {

static bool isControlFlowOp(Operation *op) {
  return isa<scf::ForOp, scf::IfOp, scf::WhileOp, affine::AffineForOp,
             affine::AffineIfOp>(op);
}

static int64_t getMaxBlockDepth(Region &region) {
  int64_t maxDepth = 0;
  for (Block &block : region) {
    for (Operation &op : block) {
      if (isControlFlowOp(&op)) {
        int64_t opDepth = 1;
        for (Region &subRegion : op.getRegions())
          opDepth = std::max(opDepth, 1 + getMaxBlockDepth(subRegion));
        maxDepth = std::max(maxDepth, opDepth);
      } else {
        for (Region &subRegion : op.getRegions())
          maxDepth = std::max(maxDepth, getMaxBlockDepth(subRegion));
      }
    }
  }
  return maxDepth;
}

class FrolovaSBlockDepthPass
    : public PassWrapper<FrolovaSBlockDepthPass, OperationPass<ModuleOp>> {
public:
  StringRef getArgument() const final { return "frolova_s_block_depth"; }
  StringRef getDescription() const final {
    return "Compute max depth of scf/affine if/for/while blocks and attach as "
           "function attribute";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    MLIRContext *ctx = &getContext();
    OpBuilder builder(ctx);

    module.walk([&](func::FuncOp funcOp) {
      int64_t depth = getMaxBlockDepth(funcOp.getBody());
      funcOp->setAttr("max_block_depth",
                      IntegerAttr::get(IntegerType::get(ctx, 64), depth));
    });
  }
};

} // namespace

MLIR_DECLARE_EXPLICIT_TYPE_ID(FrolovaSBlockDepthPass)
MLIR_DEFINE_EXPLICIT_TYPE_ID(FrolovaSBlockDepthPass)

mlir::PassPluginLibraryInfo getFrolovaSBlockDepthPluginInfo() {
  return {MLIR_PLUGIN_API_VERSION, "FrolovaSBlockDepth", "1.0",
          []() { mlir::PassRegistration<FrolovaSBlockDepthPass>(); }};
}

extern "C" LLVM_ATTRIBUTE_WEAK mlir::PassPluginLibraryInfo
mlirGetPassPluginInfo() {
  return getFrolovaSBlockDepthPluginInfo();
}
