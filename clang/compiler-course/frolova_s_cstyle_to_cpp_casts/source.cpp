#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"

namespace {

class CastRewriterVisitor final
    : public clang::RecursiveASTVisitor<CastRewriterVisitor> {
public:
  explicit CastRewriterVisitor(clang::ASTContext &C, clang::Rewriter &R)
      : Context(C), Rewrite(R) {}

  bool VisitCStyleCastExpr(clang::CStyleCastExpr *Node) {
    clang::SourceManager &SM = Context.getSourceManager();

    // Игнорируем макросы и системные заголовки
    if (!SM.isInMainFile(Node->getBeginLoc()) ||
        Node->getBeginLoc().isMacroID())
      return true;

    // Определяем подходящий C++ cast
    std::string CastName = "static_cast";
    clang::CastKind Kind = Node->getCastKind();

    if (Kind == clang::CK_BitCast || Kind == clang::CK_LValueBitCast) {
      CastName = "reinterpret_cast";
    } else if (Kind == clang::CK_NoOp &&
               Node->getSubExpr()->getType().isConstQualified() &&
               !Node->getType().isConstQualified()) {
      CastName = "const_cast";
    }

    // 1. Получаем текстовое представление целевого типа
    std::string TypeStr = Node->getTypeAsWritten().getAsString();

    // 2. Получаем текст выражения, которое кастим
    clang::SourceLocation SubExprLoc =
        Node->getSubExprAsWritten()->getBeginLoc();

    // 3. Формируем новую строку: cast_name<type>(expression)
    std::string Replacement = CastName + "<" + TypeStr + ">(";

    // Заменяем открывающую скобку и тип C-style каста
    // Находим диапазон от начала каста до начала подвыражения
    clang::SourceRange CastRange(Node->getBeginLoc(),
                                 SubExprLoc.getLocWithOffset(-1));

    Rewrite.ReplaceText(CastRange, Replacement);

    // Добавляем закрывающую скобку в конце выражения
    Rewrite.InsertTextAfter(Node->getEndLoc().getLocWithOffset(1), ")");

    return true;
  }

private:
  clang::ASTContext &Context;
  clang::Rewriter &Rewrite;
};

class CastConsumer final : public clang::ASTConsumer {
public:
  explicit CastConsumer(clang::CompilerInstance &CI) : CI(CI) {
    Rewrite.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
  }

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    CastRewriterVisitor Visitor(Context, Rewrite);
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());

    // Выводим измененный код в stdout или перезаписываем файлы
    Rewrite.getEditBuffer(CI.getSourceManager().getMainFileID())
        .write(llvm::outs());
  }

private:
  clang::CompilerInstance &CI;
  clang::Rewriter Rewrite;
};

class CastAction final : public clang::PluginASTAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef) override {
    return std::make_unique<CastConsumer>(CI);
  }

  bool ParseArgs(const clang::CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }

  ActionType getActionType() override { return AddBeforeMainAction; }
};
} // namespace

static clang::FrontendPluginRegistry::Add<CastAction>
    X("cstyle_cast_replacer",
      "Replace C-style casts with C++ casts and rewrite code");
