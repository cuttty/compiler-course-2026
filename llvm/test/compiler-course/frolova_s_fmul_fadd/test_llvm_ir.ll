; RUN: opt -load-pass-plugin %llvmshlibdir/frolova_s_fmul_fadd%pluginext \
; RUN:   -passes=frolova_s_fmul_fadd -S < %s | FileCheck %s


; scalar_float: обычная замена fmuladd на fmul + fadd с именами p, q

; CHECK-LABEL: @scalar_float
; CHECK: %p = fmul float %a, %b
; CHECK: %q = fadd float %p, %c
; CHECK-NOT: fmuladd
; CHECK: ret float %q
define float @scalar_float(float %a, float %b, float %c) {
  %r = call float @llvm.fmuladd.f32(float %a, float %b, float %c)
  ret float %r
}

; scalar_double: замена для double

; CHECK-LABEL: @scalar_double
; CHECK: %pd = fmul double %x, %y
; CHECK: %qd = fadd double %pd, %z
; CHECK-NOT: fmuladd
define double @scalar_double(double %x, double %y, double %z) {
  %res = call double @llvm.fmuladd.f64(double %x, double %y, double %z)
  ret double %res
}

; vector_2f32: векторный случай, имена pm, pa

; CHECK-LABEL: @vector_2f32
; CHECK: %pm = fmul <2 x float> %v1, %v2
; CHECK: %pa = fadd <2 x float> %pm, %v3
define <2 x float> @vector_2f32(<2 x float> %v1, <2 x float> %v2, <2 x float> %v3) {
  %r = call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %v1, <2 x float> %v2, <2 x float> %v3)
  ret <2 x float> %r
}


; triple_chain: цепочка из трёх вызовов, нумерованные имена m1,a1,m2,a2,...

; CHECK-LABEL: @triple_chain
; CHECK: %m1 = fmul float %a, %b
; CHECK: %a1 = fadd float %m1, %c
; CHECK: %m2 = fmul float %a1, %b
; CHECK: %a2 = fadd float %m2, %c
; CHECK: %m3 = fmul float %a2, %b
; CHECK: %a3 = fadd float %m3, %c
; CHECK: ret float %a3
define float @triple_chain(float %a, float %b, float %c) {
  %t1 = call float @llvm.fmuladd.f32(float %a, float %b, float %c)
  %t2 = call float @llvm.fmuladd.f32(float %t1, float %b, float %c)
  %t3 = call float @llvm.fmuladd.f32(float %t2, float %b, float %c)
  ret float %t3
}

; conditional: замена в разных блоках, суффиксы then/else/part

; CHECK-LABEL: @conditional
; CHECK: %cond = fcmp ogt float %a, 0.0
; CHECK: br i1 %cond, label %then, label %else
; CHECK: then:
; CHECK: %m_then = fmul float %a, %b
; CHECK: %a_then = fadd float %m_then, %c
; CHECK: br label %merge
; CHECK: else:
; CHECK: %m_else = fmul float %b, %c
; CHECK: %a_else = fadd float %m_else, %a
; CHECK: br label %merge
; CHECK: merge:
; CHECK: %phi = phi float [ %a_then, %then ], [ %a_else, %else ]
; CHECK-NOT: fmuladd
define float @conditional(float %a, float %b, float %c) {
  %cond = fcmp ogt float %a, 0.0
  br i1 %cond, label %then, label %else

then:
  %r1 = call float @llvm.fmuladd.f32(float %a, float %b, float %c)
  br label %merge

else:
  %r2 = call float @llvm.fmuladd.f32(float %b, float %c, float %a)
  br label %merge

merge:
  %result = phi float [ %r1, %then ], [ %r2, %else ]
  ret float %result
}

; fast_flags: проверка переноса флага fast на новые инструкции

; CHECK-LABEL: @fast_flags
; CHECK: fmul fast float %x, %y
; CHECK: fadd fast float %fmul, %z
define float @fast_flags(float %x, float %y, float %z) {
  %r = call fast float @llvm.fmuladd.f32(float %x, float %y, float %z)
  ret float %r
}

; contract_flag: убираем contract, т.к. он не должен оставаться на fmul/fadd

; CHECK-LABEL: @contract_flag
; CHECK: fmul float %a, %b
; CHECK: fadd float %fmul, %c
; CHECK-NOT: contract
define float @contract_flag(float %a, float %b, float %c) {
  %r = call contract float @llvm.fmuladd.f32(float %a, float %b, float %c)
  ret float %r
}

; multi_use: результат fmuladd используется несколько раз

; CHECK-LABEL: @multi_use
; CHECK: %m = fmul float %u, %v
; CHECK: %a = fadd float %m, %w
; CHECK: %mul_user = fmul float %a, 2.0
; CHECK: %add_user = fadd float %a, %mul_user
; CHECK: ret float %add_user
define float @multi_use(float %u, float %v, float %w) {
  %t = call float @llvm.fmuladd.f32(float %u, float %v, float %w)
  %t2 = fmul float %t, 2.0
  %t3 = fadd float %t, %t2
  ret float %t3
}

; no_fmuladd: функция без fmuladd – не должна измениться

; CHECK-LABEL: @no_fmuladd
; CHECK-NOT: fmuladd
; CHECK: fmul float %p, %q
define float @no_fmuladd(float %p, float %q) {
  %r = fmul float %p, %q
  ret float %r
}