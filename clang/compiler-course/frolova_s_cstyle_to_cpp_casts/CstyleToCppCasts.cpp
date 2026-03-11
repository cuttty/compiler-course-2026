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

CppCastKind getCppCastKind(QualType srcType, QualType dstType, ASTContext &Context) {
    // 1. Работа с указателями и ссылками
    if ((srcType->isPointerType() && dstType->isPointerType()) ||
        (srcType->isReferenceType() && dstType->isReferenceType())) {
        
        QualType srcPointee = srcType->getPointeeType();
        QualType dstPointee = dstType->getPointeeType();

        // Проверка на const_cast (типы одинаковые, разница в квалификаторах)
        if (Context.hasSameUnqualifiedType(srcPointee, dstPointee)) {
            if (srcPointee.getCVRQualifiers() != dstPointee.getCVRQualifiers()) {
                return CK_Const;
            }
        }

        // Если приведение между неродственными типами (кроме void*) — reinterpret_cast
        if (!srcPointee->isVoidType() && !dstPointee->isVoidType() &&
            !Context.hasSameUnqualifiedType(srcPointee, dstPointee)) {
            // Упрощенная проверка: если нет явного наследования, считаем reinterpret
            return CK_Reinterpret;
        }
    }

    // 2. Приведение указателя к числу и наоборот
    if ((srcType->isPointerType() && dstType->isIntegerType()) ||
        (srcType->isIntegerType() && dstType->isPointerType())) {
        return CK_Reinterpret;
    }

    // 3. По умолчанию для примитивов и безопасных апкастов
    return CK_Static;
}

class CastVisitor final : public RecursiveASTVisitor<CastVisitor> {
public:
    CastVisitor(ASTContext *context, Rewriter &rewriter)
        : m_context(context), m_rewriter(rewriter) {}

    bool VisitCStyleCastExpr(CStyleCastExpr *cast) {
        SourceManager &SM = m_context->getSourceManager();

        // Игнорируем код не из основного файла и макросы
        if (!SM.isInMainFile(cast->getBeginLoc()) || cast->getBeginLoc().isMacroID())
            return true;

        Expr *subExpr = cast->getSubExpr();
        CppCastKind kind = getCppCastKind(subExpr->getType(), cast->getType(), *m_context);

        LangOptions LO = m_context->getLangOpts();
        std::string innerText = Lexer::getSourceText(
            CharSourceRange::getTokenRange(subExpr->getSourceRange()), SM, LO).str();
        
        if (innerText.empty()) return true;

        std::string dstTypeStr = cast->getTypeAsWritten().getAsString();
        std::string replacement;

        switch (kind) {
            case CK_Const:       replacement = "const_cast<" + dstTypeStr + ">(" + innerText + ")"; break;
            case CK_Reinterpret: replacement = "reinterpret_cast<" + dstTypeStr + ">(" + innerText + ")"; break;
            default:             replacement = "static_cast<" + dstTypeStr + ">(" + innerText + ")"; break;
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
        m_rewriter.getEditBuffer(m_rewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
    }

private:
    Rewriter m_rewriter;
};

class CastAction final : public PluginASTAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &ci, llvm::StringRef) override {
        return std::make_unique<CastConsumer>(ci);
    }
    bool ParseArgs(const CompilerInstance &ci, const std::vector<std::string> &args) override { return true; }
    ActionType getActionType() override { return AddBeforeMainAction; }
};

} // namespace

static FrontendPluginRegistry::Add<CastAction> X("cstyle_cast_to_cpp_cast", "C-style cast converter");