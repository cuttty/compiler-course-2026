#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace {

enum CppCastKind { CK_Const, CK_Static, CK_Reinterpret };

CppCastKind getCppCastKind(QualType srcType, QualType dstType,
                           ASTContext &Context) {
  if (Context.hasSameUnqualifiedType(srcType, dstType)) {
    if (srcType.isConstQualified() != dstType.isConstQualified() ||
        srcType.isVolatileQualified() != dstType.isVolatileQualified()) {
      return CK_Const;
    }
  }

  if (srcType->isPointerType() && dstType->isIntegerType())
    return CK_Reinterpret;
  if (srcType->isIntegerType() && dstType->isPointerType())
    return CK_Reinterpret;
  if (srcType->isPointerType() && dstType->isPointerType()) {
    QualType srcPointee = srcType->getPointeeType();
    QualType dstPointee = dstType->getPointeeType();
    if (!Context.hasSameUnqualifiedType(srcPointee, dstPointee) &&
        !srcPointee->isVoidType() && !dstPointee->isVoidType()) {
      return CK_Reinterpret;
    }
  }

  return CK_Static;
}

class CastVisitor final : public RecursiveASTVisitor<CastVisitor> {
public:
  CastVisitor(ASTContext *context, Rewriter &rewriter)
      : m_context(context), m_rewriter(rewriter) {}

  bool VisitCStyleCastExpr(CStyleCastExpr *cast) {
    SourceManager &SM = m_context->getSourceManager();

    if (!SM.isInMainFile(cast->getBeginLoc()))
      return true;

    Expr *subExpr = cast->getSubExpr();
    QualType srcType = subExpr->getType();
    QualType dstType = cast->getType();

    CppCastKind kind = getCppCastKind(srcType, dstType, *m_context);

    LangOptions LO = m_context->getLangOpts();
    CharSourceRange subRange =
        CharSourceRange::getTokenRange(subExpr->getSourceRange());
    std::string innerText = Lexer::getSourceText(subRange, SM, LO).str();
    if (innerText.empty())
      return true;

    std::string dstTypeStr = dstType.getAsString();

    std::string replacement;
    switch (kind) {
    case CK_Const:
      replacement = "const_cast<" + dstTypeStr + ">(" + innerText + ")";
      break;
    case CK_Static:
      replacement = "static_cast<" + dstTypeStr + ">(" + innerText + ")";
      break;
    case CK_Reinterpret:
      replacement = "reinterpret_cast<" + dstTypeStr + ">(" + innerText + ")";
      break;
    }

    m_rewriter.ReplaceText(cast->getSourceRange(), replacement);

    return true;
  }

private:
  ASTContext *m_context;
  Rewriter &m_rewriter;
};

class CastConsumer final : public ASTConsumer {
public:
  CastConsumer(CompilerInstance &ci)
      : m_rewriter(ci.getSourceManager(), ci.getLangOpts()) {}

  void HandleTranslationUnit(ASTContext &context) override {
    CastVisitor visitor(&context, m_rewriter);
    visitor.TraverseDecl(context.getTranslationUnitDecl());

    m_rewriter.getEditBuffer(m_rewriter.getSourceMgr().getMainFileID())
        .write(llvm::outs());
  }

private:
  Rewriter m_rewriter;
};

class CastAction final : public PluginASTAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &ci,
                                                 llvm::StringRef) override {
    return std::make_unique<CastConsumer>(ci);
  }

  bool ParseArgs(const CompilerInstance &ci,
                 const std::vector<std::string> &args) override {
    return true;
  }

  ActionType getActionType() override { return AddBeforeMainAction; }
};

} // namespace

static FrontendPluginRegistry::Add<CastAction>
    X("cstyle_cast_to_cpp_cast", "C-style cast to C++ cast converter");
```