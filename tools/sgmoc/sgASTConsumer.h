
#pragma once

#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Decl.h>

class sgPPCallbacks;

struct EnumPropertyDef
{
	std::string name;
	int value;
};
struct EnumDef
{
	clang::EnumDecl *enumDecl;
	std::string name;
	std::vector<EnumPropertyDef> properties;
};

struct PropertyDef
{
	clang::FieldDecl *fieldDecl;
	std::string name;
	std::string typeName;
	bool isPointer;
	bool isEnum;
	bool isArray;

	PropertyDef() : isPointer(false), isEnum(false), isArray(false) {}
};

struct ClassDef
{
	clang::CXXRecordDecl *classDecl;
	std::string name;
	std::string typeName;	// full namespace path
	std::string baseClassTypeName;

	std::vector<PropertyDef> properties;
};

class sgASTConsumer : public clang::ASTConsumer
{
protected:
    protected:
    clang::CompilerInstance &mCompilerInstance;
    clang::ASTContext *mASTContext;

	sgPPCallbacks *mPPCallbacks;

public:
	std::vector<ClassDef> mExportClasses;
	std::vector<EnumDef> mExportEnums;
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
	void ParseClass(clang::CXXRecordDecl *RD, bool uplevelExport);
	void ParseEnum(clang::EnumDecl *ED, bool uplevelExport);

	void TopLevelParseDecl(clang::Decl *decl);

};

