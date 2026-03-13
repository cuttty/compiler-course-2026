// RUN: %clang_cc1 -load %llvmshlibdir/CStyleCastReplacerPlugin_Frolova_Sofya_FIIT3_ClangAST%pluginext -add-plugin cstyle_cast_replacer %s 2>&1 | FileCheck %s

void test_casts() {
    double d = 10.5;
    
    // CHECK: int i = static_cast<int>(d);
    int i = (int)d; 

    // CHECK: int* p = reinterpret_cast<int*>(0x12345);
    int* p = (int*)0x12345;

    const int ci = 5;
    // CHECK: int i2 = static_cast<int>(ci);
    int i2 = (int)ci;
}