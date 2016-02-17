
#pragma once

#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Decl.h>
#include <clang/AST/RecursiveASTVisitor.h>

class sgPPCallbacks;

struct EnumValueDef
{
	std::string name;
	int value;
};
struct EnumDef
{
	clang::EnumDecl *enumDecl;
	std::string name;
	std::vector<EnumValueDef> values;
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

enum MetaType
{
	MT_OBJ,
	MT_OBJ_ABSTRACT,
	MT_OTHER
};
struct ClassDef
{
	clang::CXXRecordDecl *classDecl;
	
	MetaType mt;

	std::string name;
	std::string typeName;	// full namespace path
	std::string baseClassTypeName;

	std::vector<PropertyDef> properties;
};

class sgASTConsumer;
class SGVisitor : public clang::RecursiveASTVisitor<SGVisitor>
{
	public:
	clang::ASTContext *context;
	sgASTConsumer *consumer;

public:
	bool VisitDecl(clang::Decl *D);
};


class sgASTConsumer : public clang::ASTConsumer
{
public:
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
	virtual void HandleTranslationUnit(clang::ASTContext& Ctx) override;

public:
	void ParseClass(clang::CXXRecordDecl *RD);
	void ParseEnum(clang::EnumDecl *ED);

	void TopLevelParseDecl(clang::Decl *decl);

};

