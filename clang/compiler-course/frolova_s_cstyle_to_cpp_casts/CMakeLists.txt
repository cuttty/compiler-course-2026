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
    # На Linux/macOS отключаем явную линковку с LLVM и разрешаем неопределённые символы
    # (они будут взяты из основного процесса clang)
    target_link_options(${TARGET_NAME} PRIVATE "LINKER:-undefined,dynamic_lookup")
    clang_target_link_libraries(${TARGET_NAME} PRIVATE
        clangAST
        clangBasic
        clangFrontend
        clangRewrite
        clangLex
        # Не добавляем LLVMSupport и другие LLVM-библиотеки!
    )
endif()

set(CLANG_TEST_DEPS "${TARGET_NAME}" ${CLANG_TEST_DEPS} PARENT_SCOPE)