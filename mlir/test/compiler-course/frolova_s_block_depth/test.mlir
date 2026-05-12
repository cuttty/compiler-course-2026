// RUN: mlir-opt -load-pass-plugin=%mlir_lib_dir/FrolovaSBlockDepth%shlibext \
// RUN:   --pass-pipeline="builtin.module(frolova_s_block_depth)" %s \
// RUN:   | FileCheck %s

#set = affine_set<(d0)[s0] : (d0 >= 0, s0 - d0 >= 0)>

// глубина 0 (пустая функция)
// CHECK-LABEL: func.func @empty
// CHECK-SAME: max_block_depth = 0
func.func @empty() {
  return
}

// только scf.for - глубина 1
// CHECK-LABEL: func.func @one_for
// CHECK-SAME: max_block_depth = 1
func.func @one_for(%lb : index, %ub : index, %step : index) {
  scf.for %i = %lb to %ub step %step {
    %c = arith.constant 1 : i32
  }
  return
}

// только scf.if - глубина 1
// CHECK-LABEL: func.func @one_if
// CHECK-SAME: max_block_depth = 1
func.func @one_if(%cond : i1) {
  scf.if %cond {
    %c = arith.constant 2 : i32
  }
  return
}

// scf.for внутри scf.if - глубина 2
// CHECK-LABEL: func.func @if_in_for
// CHECK-SAME: max_block_depth = 2
func.func @if_in_for(%lb : index, %ub : index, %step : index, %cond : i1) {
  scf.for %i = %lb to %ub step %step {
    scf.if %cond {
      %c = arith.constant 3 : i32
    }
  }
  return
}

// scf.while с телом - глубина 1, а внутри тела scf.for с глубиной 2
// CHECK-LABEL: func.func @while_with_for
// CHECK-SAME: max_block_depth = 2
func.func @while_with_for(%cond : i1, %lb : index, %ub : index, %step : index) {
  scf.while (%arg = %cond) : (i1) -> i1 {
    scf.condition(%arg) %arg : i1
  } do {
  ^bb0(%arg1 : i1):
    scf.for %i = %lb to %ub step %step {
      %c = arith.constant 4 : i32
    }
    scf.yield %arg1 : i1
  }
  return
}

// affine.for и внутри affine.if - глубина 2
// CHECK-LABEL: func.func @affine_nested
// CHECK-SAME: max_block_depth = 2
func.func @affine_nested() {
  %c0 = arith.constant 0 : index
  affine.for %i = 0 to 10 {
    affine.if #set(%i)[%c0] {
      %c = arith.constant 5 : i32
    }
  }
  return
}

// тройная вложенность: scf.for, scf.if, scf.for
// CHECK-LABEL: func.func @triple_nested
// CHECK-SAME: max_block_depth = 3
func.func @triple_nested(%lb : index, %ub : index, %step : index, %cond : i1) {
  scf.for %i = %lb to %ub step %step {
    scf.if %cond {
      scf.for %j = %lb to %ub step %step {
        %c = arith.constant 6 : i32
      }
    }
  }
  return
}

// if с else - глубина 2
// CHECK-LABEL: func.func @if_else_branches
// CHECK-SAME: max_block_depth = 2
func.func @if_else_branches(%cond : i1, %lb : index, %ub : index, %step : index) {
  scf.if %cond {
    scf.for %i = %lb to %ub step %step {
      %c = arith.constant 7 : i32
    }
  } else {
    scf.for %j = %lb to %ub step %step {
      %c2 = arith.constant 8 : i32
    }
  }
  return
}
