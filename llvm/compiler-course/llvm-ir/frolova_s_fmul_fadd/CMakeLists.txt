get_filename_component(CURRENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TARGET_NAME "${CURRENT_DIR_NAME}_LLVM_IR")

file(GLOB_RECURSE SOURCES *.cpp *.h *.hpp)
add_llvm_pass_plugin(frolova_s_fmul_fadd
    source.cpp
    DEPENDS
    intrinsics_gen
)

# Для тестов (если используются)
set(LLVM_TEST_DEPENDS ${TARGET_NAME} ${LLVM_TEST_DEPENDS} PARENT_SCOPE)