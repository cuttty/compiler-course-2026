set(Title "CStyleToCppCasts")
set(Student "Frolova_Sofya")
set(Group "FIIT3")
set(TARGET_NAME "${Title}_${Student}_${Group}_ClangAST")

file(GLOB_RECURSE SOURCES *.cpp *.h *.hpp)

# Создаём обычную динамическую библиотеку (не add_llvm_library)
add_library(${TARGET_NAME} SHARED ${SOURCES})

# Находим LLVM и Clang (обычно уже есть в родительском проекте, но для надёжности)
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

# Добавляем пути к заголовкам и определения
target_include_directories(${TARGET_NAME} PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_compile_definitions(${TARGET_NAME} PRIVATE ${LLVM_DEFINITIONS})

# Линкуем только библиотеки Clang (без LLVM)
target_link_libraries(${TARGET_NAME} PRIVATE
    clangAST
    clangBasic
    clangFrontend
    clangRewrite
    clangLex
)

# На Linux/macOS разрешаем неопределённые символы (будут взяты из основного процесса)
if(NOT WIN32 AND NOT CYGWIN)
    target_link_options(${TARGET_NAME} PRIVATE "LINKER:-undefined,dynamic_lookup")
endif()

# Явно указываем стандарт C++ (опционально)
set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

# Для совместимости с тестовой инфраструктурой
set(CLANG_TEST_DEPS "${TARGET_NAME}" ${CLANG_TEST_DEPS} PARENT_SCOPE)