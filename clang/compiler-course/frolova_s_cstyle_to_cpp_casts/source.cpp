#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/raw_ostream.h"

namespace {

class CastRewriterVisitor final
    : public clang::RecursiveASTVisitor<CastRewriterVisitor> {
public:
  explicit CastRewriterVisitor(clang::ASTContext &C, clang::Rewriter &R)
      : Context(C), Rewrite(R) {}

  bool VisitCStyleCastExpr(clang::CStyleCastExpr *Node) {
    clang::SourceManager &SM = Context.getSourceManager();

    // Работаем только с кодом из основного файла (не из заголовков)
    // и пропускаем макросы
    if (!SM.isInMainFile(Node->getBeginLoc()) ||
        Node->getBeginLoc().isMacroID())
      return true;

    // Определяем, какой C++ cast нужно использовать
    std::string CastName = "static_cast";
    clang::CastKind Kind = Node->getCastKind();

    // Все варианты, требующие reinterpret_cast
    if (Kind == clang::CK_BitCast ||
        Kind == clang::CK_LValueBitCast ||
        Kind == clang::CK_PointerToIntegral ||   // указатель → целое (ваш случай)
        Kind == clang::CK_IntegralToPointer ||   // целое → указатель
        Kind == clang::CK_ReinterpretMemberPointer) {
      CastName = "reinterpret_cast";
    }
    else if (Kind == clang::CK_NoOp) {
      // Проверяем, не является ли приведение const_cast-ом
      clang::QualType SubType = Node->getSubExpr()->getType();
      clang::QualType TargetType = Node->getType();

      auto isConstCastCompatible = [&](clang::QualType From,
                                        clang::QualType To) -> bool {
        // Убираем ссылки
        if (From->isReferenceType())
          From = From.getNonReferenceType();
        if (To->isReferenceType())
          To = To.getNonReferenceType();

        // Для указателей сравниваем типы, на которые они указывают
        if (From->isPointerType() && To->isPointerType()) {
          clang::QualType FromPointee = From->getPointeeType();
          clang::QualType ToPointee = To->getPointeeType();
          return Context.hasSameUnqualifiedType(FromPointee, ToPointee) &&
                 (FromPointee.getCVRQualifiers() != ToPointee.getCVRQualifiers());
        }

        // Для обычных типов
        return Context.hasSameUnqualifiedType(From, To) &&
               (From.getCVRQualifiers() != To.getCVRQualifiers());
      };

      if (isConstCastCompatible(SubType, TargetType)) {
        CastName = "const_cast";
      }
    }

    // Получаем тип, как он был написан в исходном коде
    std::string TypeStr = Node->getTypeAsWritten().getAsString();

    // Находим позицию подвыражения (то, что внутри скобок)
    clang::SourceLocation SubExprLoc =
        Node->getSubExprAsWritten()->getBeginLoc();

    // Заменяем открывающую часть "(тип)" на "reinterpret_cast<тип>("
    clang::SourceRange CastRange(Node->getBeginLoc(),
                                 SubExprLoc.getLocWithOffset(-1));
    std::string Replacement = CastName + "<" + TypeStr + ">(";
    Rewrite.ReplaceText(CastRange, Replacement);

    // Вставляем закрывающую скобку после окончания подвыражения
    clang::SourceLocation EndAfterSubExpr = clang::Lexer::getLocForEndOfToken(
        Node->getSubExpr()->getEndLoc(), 0, SM, Context.getLangOpts());
    Rewrite.InsertTextAfter(EndAfterSubExpr, ")");

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

    // Выводим изменённый код в stdout
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
