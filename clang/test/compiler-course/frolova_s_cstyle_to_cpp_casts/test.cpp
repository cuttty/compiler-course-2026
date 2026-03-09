// RUN: split-file %s %t
// RUN: %clang_cc1 -load %llvmshlibdir/CStyleToCppCasts_Frolova_Sofya_FIIT3_ClangAST%pluginext -plugin cstyle_cast_to_cpp_cast -fsyntax-only %t/with_casts.cpp 2>&1 | FileCheck %t/with_casts.cpp
// RUN: %clang_cc1 -load %llvmshlibdir/CStyleToCppCasts_Frolova_Sofya_FIIT3_ClangAST%pluginext -plugin cstyle_cast_to_cpp_cast -fsyntax-only %t/without_casts.cpp 2>&1 | FileCheck %t/without_casts.cpp --allow-empty --implicit-check-not="{{(const|static|reinterpret)_cast<}}"

//--- with_casts.cpp
// Проверка базовых преобразований
void test_primitive_casts() {
    int a = 5;
    // CHECK: double b = static_cast<double>(a);
    double b = (double)a;

    // CHECK: int *p = reinterpret_cast<int *>(a);
    int *p = (int *)a;

    const int c = 10;
    // CHECK: int *q = const_cast<int *>(&c);
    int *q = (int *)&c;
}

// Вспомогательные классы для проверки иерархии
struct Point { int x; int y; };
struct DataBlock { float values[4]; };
class Base { public: virtual ~Base() {} };
class Derived : public Base { public: int id; };

void test_custom_types() {
    Point pt = {10, 20};
    // CHECK: DataBlock *data = reinterpret_cast<DataBlock *>(&pt);
    DataBlock *data = (DataBlock *)&pt;

    Derived derived_obj;
    // CHECK: Base *base_ptr = static_cast<Base *>(&derived_obj);
    Base *base_ptr = (Base *)&derived_obj;

    Base *b_ptr = new Derived();
    // CHECK: Derived *d_ptr = static_cast<Derived *>(b_ptr);
    Derived *d_ptr = (Derived *)b_ptr;

    const Point const_pt = {0, 0};
    // CHECK: Point *mut_pt = const_cast<Point *>(&const_pt);
    Point *mut_pt = (Point *)&const_pt;
}

// Проверка сложных выражений
void test_complex() {
    int x = 5, y = 7;
    // CHECK: double res = static_cast<double>(x + y) / 2.0;
    double res = (double)(x + y) / 2.0;
}

//--- without_casts.cpp
// Файл без C-style кастов – плагин не должен ничего менять
void no_casts() {
    int a = 10;
    int b = a + 20;
    double c = b * 2.5;
}