
#pragma once

#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ASTConsumer.h>

class sgPPCallbacks;

struct ClassDef
{
	clang::CXXRecordDecl *Record;
	bool Export;
	std::string Name;
	std::string BaseClassName;
};

class sgASTConsumer : public clang::ASTConsumer
{
protected:
    protected:
    clang::CompilerInstance &mCompilerInstance;
    clang::ASTContext *mASTContext;

	sgPPCallbacks *mPPCallbacks;

	std::vector<ClassDef> mExportClasses;
	std::vector<clang::FunctionDecl*> mExportFunctions;

public:
    sgASTConsumer(clang::CompilerInstance &ci);

    virtual void Initialize(clang::ASTContext& Ctx) override
	{
		mASTContext = &Ctx;
	}

	virtual bool HandleTopLevelDecl(clang::DeclGroupRef GroupRef) override;
    virtual void HandleTagDeclDefinition(clang::TagDecl* D) override;
	virtual void HandleTranslationUnit(clang::ASTContext& Ctx) override {
		// all parsed, output
    }

private:
	ClassDef ParseClass(clang::CXXRecordDecl *RD);

};

