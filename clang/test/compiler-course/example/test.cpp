
// RUN: split-file %s %t
// RUN: %clang_cc1 -load %llvmshlibdir/CStyleToCppCasts_Frolova_Sofya_FIIT3_ClangAST%pluginext -plugin cstyle_cast_to_cpp_cast -fsyntax-only %t/with_casts.cpp | FileCheck %t/with_casts.cpp
// RUN: %clang_cc1 -load %llvmshlibdir/CStyleToCppCasts_Frolova_Sofya_FIIT3_ClangAST%pluginext -plugin cstyle_cast_to_cpp_cast -fsyntax-only %t/without_casts.cpp | FileCheck %t/without_casts.cpp --allow-empty --implicit-check-not="{{(const|static|reinterpret)_cast<}}"

//--- with_casts.cpp
void test_const() {
    const int x = 42;
    // CHECK: int* p = const_cast<int*>(&x);
    int* p = (int*)&x;
}

void test_static_numeric() {
    double d = 3.14159;
    // CHECK: int i = static_cast<int>(d);
    int i = (int)d;
    
    float f = 2.7f;
    // CHECK: double dbl = static_cast<double>(f);
    double dbl = (double)f;
}

void test_reinterpret_pointer() {
    long addr = 0xFF00FF00;
    // CHECK: int* ptr = reinterpret_cast<int*>(addr);
    int* ptr = (int*)addr;
    
    char c = 'A';
    // CHECK: int* bad = reinterpret_cast<int*>(&c);
    int* bad = (int*)&c;
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


void test_complex_expr() {
    int a = 5, b = 7;
    // CHECK: double res = static_cast<double>(a + b) / 2.0;
    double res = (double)(a + b) / 2.0;
}


#define CAST_INT(x) ((int)(x))

void test_macro() {
    double val = 3.14;
    // CHECK: int from_macro = CAST_INT(val);
    int from_macro = CAST_INT(val); // не должно измениться
}

//--- without_casts.cpp
void no_casts() {
    int x = 10;
    int y = x + 20;
    double z = y * 1.5;
}