// RUN: split-file %s %t
// RUN: %clang_cc1 -load %llvmshlibdir/CStyleToCppCasts_Frolova_Sofya_FIIT3_ClangAST%pluginext -plugin cstyle_cast_to_cpp_cast -fsyntax-only %t/with_casts.cpp | FileCheck %t/with_casts.cpp
// RUN: %clang_cc1 -load %llvmshlibdir/CStyleToCppCasts_Frolova_Sofya_FIIT3_ClangAST%pluginext -plugin cstyle_cast_to_cpp_cast -fsyntax-only %t/without_casts.cpp | FileCheck %t/without_casts.cpp --allow-empty --implicit-check-not="{{(const|static|reinterpret)_cast<}}"

//--- with_casts.cpp
void test_const() {
    const int x = 42;
    // CHECK: int* p = const_cast<int*>(&x);
    int* p = (int*)&x;
}

void test_static() {
    double d = 3.14;
    // CHECK: int i = static_cast<int>(d);
    int i = (int)d;
}

void test_reinterpret() {
    long addr = 0xFF00;
    // CHECK: int* ptr = reinterpret_cast<int*>(addr);
    int* ptr = (int*)addr;
}

void test_reinterpret_ptr() {
    float f = 1.23f;
    // CHECK: int* bad = reinterpret_cast<int*>(&f);
    int* bad = (int*)&f;
}

void test_const_volatile() {
    const volatile int cv = 100;
    // CHECK: int* p = const_cast<int*>(&cv);
    int* p = (int*)&cv;
}

struct Base { int a; };
struct Derived : Base { int b; };
void test_inheritance() {
    Derived d;
    Base* base = &d;
    // CHECK: Derived* derived = static_cast<Derived*>(base);
    Derived* derived = (Derived*)base;
}

void test_complex() {
    int x = 5, y = 7;
    // CHECK: double res = static_cast<double>(x + y) / 2.0;
    double res = (double)(x + y) / 2.0;
}

//--- without_casts.cpp
void no_casts() {
    int a = 10;
    int b = a + 20;
    double c = b * 2.5;
}