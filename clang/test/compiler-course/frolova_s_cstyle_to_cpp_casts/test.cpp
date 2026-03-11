// RUN: split-file %s %t
// RUN: %clang_cc1 -load %llvmshlibdir/CStyleToCppCasts_Frolova_Sofya_FIIT3_ClangAST%pluginext -plugin cstyle_cast_to_cpp_cast -fsyntax-only %t/with_casts.cpp 2>&1 | FileCheck %t/with_casts.cpp

//--- with_casts.cpp
void test_primitive_casts() {
    int a = 5;
    // CHECK: double b = static_cast<double>(a);
    double b = (double)a;

    // CHECK: int *p = reinterpret_cast<int *>(a);
    int *p = (int *)a;

    const int c = 10;
    // CHECK: int *q = const_cast<int *>(&c);
    int *q = (int *)&c;

    // CHECK: int &r = const_cast<int &>(c);
    int &r = (int &)c;
}

struct Point { int x; int y; };
class Base { public: virtual ~Base() {} };
class Derived : public Base { public: int id; };

void test_complex_types() {
    Point pt = {10, 20};
    // CHECK: char *ptr = reinterpret_cast<char *>(&pt);
    char *ptr = (char *)&pt;

    Derived d;
    // CHECK: Base *b = static_cast<Base *>(&d);
    Base *b = (Base *)&d;
}