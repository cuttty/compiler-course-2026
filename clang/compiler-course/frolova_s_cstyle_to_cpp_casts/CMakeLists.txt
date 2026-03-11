set(Title "CStyleToCppCasts")
set(Student "Frolova_Sofya")
set(Group "FIIT3")
set(TARGET_NAME "${Title}_${Student}_${Group}_ClangAST")

file(GLOB_RECURSE SOURCES *.cpp *.h *.hpp)

add_llvm_library(${TARGET_NAME} MODULE ${SOURCES} PLUGIN_TOOL clang)

if(WIN32 OR CYGWIN)
    set(LLVM_LINK_COMPONENTS Support)
    clang_target_link_libraries(${TARGET_NAME} PRIVATE
        clangAST
        clangBasic
        clangFrontend
        clangRewrite
        clangLex
    )
else()
    # На Linux/macOS отключаем явную линковку с LLVM и Clang, 
    # разрешаем неопределённые символы.
    # (они будут взяты из основного процесса clang во время загрузки плагина)
    target_link_options(${TARGET_NAME} PRIVATE "LINKER:-undefined,dynamic_lookup")
    
    # ВНИМАНИЕ: вызов clang_target_link_libraries здесь удален!
endif()

set(CLANG_TEST_DEPS "${TARGET_NAME}" ${CLANG_TEST_DEPS} PARENT_SCOPE)