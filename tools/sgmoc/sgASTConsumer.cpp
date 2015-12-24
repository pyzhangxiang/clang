
#include "sgASTConsumer.h"
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Compilation.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>

#include <iostream>

#include "sgPPCallbacks.h"

sgASTConsumer::sgASTConsumer(clang::CompilerInstance &ci) : mCompilerInstance(ci)
{
	mPPCallbacks = new sgPPCallbacks(mCompilerInstance.getPreprocessor());
	mCompilerInstance.getPreprocessor().addPPCallbacks(std::unique_ptr<sgPPCallbacks>(mPPCallbacks));
}

bool sgASTConsumer::HandleTopLevelDecl(clang::DeclGroupRef GroupRef) 
{
	if(!mPPCallbacks->IsInMainFile())
	{
		return true;
	}
	clang::Decl *decl = *GroupRef.begin();
	
	if(clang::FunctionDecl *MD = llvm::dyn_cast<clang::FunctionDecl>(decl))
	{
		// global function
		std::string funcName = MD->getNameAsString();
		std::string retType = MD->getReturnType().getAsString();
		std::cout << "\nParsing Function: " << retType << " " << funcName;

		bool toExport = true;
		for (auto attr_it = MD->specific_attr_begin<clang::AnnotateAttr>();
                attr_it != MD->specific_attr_end<clang::AnnotateAttr>();
                ++attr_it) 
		{
				llvm::StringRef annotation = attr_it->getAnnotation();
				if(annotation == "sg_meta_no_export")
				{
					std::cout << "\n\tno export";
					toExport = false;
					break;
				}
		}

		if(toExport)
		{
			mExportFunctions.push_back(MD);
		}
	}
	else if(clang::EnumDecl *ED = llvm::dyn_cast<clang::EnumDecl>(decl))
	{
		std::string enumName = ED->getNameAsString();
		std::cout << "\nParsing Enum: " << enumName;

		for (auto attr_it = ED->specific_attr_begin<clang::AnnotateAttr>();
                attr_it != ED->specific_attr_end<clang::AnnotateAttr>();
                ++attr_it) 
		{
			llvm::StringRef annotation = attr_it->getAnnotation();
			if(annotation == "sg_meta_enum")
			{
				std::cout << "\n\tsg_meta_enum";
				break;
			}
		}
	}
		
	return true;
}

void sgASTConsumer::HandleTagDeclDefinition(clang::TagDecl* D)
{
	if(!mPPCallbacks->IsInMainFile())
	{
		return ;
	}
	
	//clang::Decl::Kind knd = D->getKind();

	if(clang::CXXRecordDecl *RD = llvm::dyn_cast<clang::CXXRecordDecl>(D))
	{
		clang::Decl::Kind classKind = RD->getKind();
		/*if(classKind == clang::Decl::ClassTemplate
			|| classKind == clang::Decl::ClassTemplateSpecialization
			|| classKind == clang::Decl::ClassTemplatePartialSpecialization)
		{
			return ;
		}*/

		if(classKind != clang::Decl::CXXRecord)
		{
			return ;
		}

		// class
		ClassDef def = ParseClass(RD);
		if(def.Export)
		{
			mExportClasses.push_back(def);
		}
	}

}

ClassDef sgASTConsumer::ParseClass(clang::CXXRecordDecl *RD)
{
	clang::Preprocessor &PP = mCompilerInstance.getPreprocessor();
    ClassDef Def;
	Def.Export = false;
    Def.Record = RD;
	Def.Name = RD->getNameAsString();

	clang::Decl::Kind classKind = RD->getKind();

	std::string name = RD->getNameAsString();
	std::cout << "\nParsing Class: " << name;

	if(RD->hasDefinition())
	{
		auto firstBase = RD->bases_begin();
		if(firstBase != RD->bases_end())
		{
			Def.BaseClassName = firstBase->getType().getAsString();

			std::cout << "\n\tBase class: " << Def.BaseClassName;
		}
	}

    for (auto it = RD->decls_begin(); it != RD->decls_end(); ++it) {

		clang::Decl::Kind kind = it->getKind();

		if(clang::AccessSpecDecl *as = llvm::dyn_cast<clang::AccessSpecDecl>(*it))
		{
			clang::AccessSpecifier asName = as->getAccess();
		}
		else if (clang::StaticAssertDecl *S = llvm::dyn_cast<clang::StaticAssertDecl>(*it) ) 
		{
            if (auto *E = llvm::dyn_cast<clang::UnaryExprOrTypeTraitExpr>(S->getAssertExpr()))
                if (clang::ParenExpr *PE = llvm::dyn_cast<clang::ParenExpr>(E->getArgumentExpr()))
            {
                llvm::StringRef key = S->getMessage()->getString();
                if (key == "sg_meta_object") 
				{
					Def.Export = true;

					std::cout << "\n\tsg_meta_object";
                }
            }
        } 
		else if(clang::FieldDecl *fd = llvm::dyn_cast<clang::FieldDecl>(*it))
		{
			std::string fieldName = fd->getNameAsString();
			clang::QualType fieldType = fd->getType();
			std::string typeName = fieldType.getAsString();
			std::string typeClassName = fieldType->getTypeClassName();

			std::cout << "\n\tParsing Field: " << typeName << " " << fieldName;

			if(fieldType->isPointerType())
			{
				std::cout << "\n\t\tis Pointer";
			}
			if(fieldType->isArrayType())
			{
				std::cout << "\n\t\tis Array";
			}
			if(fieldType->isBuiltinType())
			{
				std::cout << "\n\t\tis Buildin";
			}
			if(fieldType->isBooleanType())
			{
				std::cout << "\n\t\tis Boolean";
			}

			for(auto attrIt = fd->specific_attr_begin<clang::AnnotateAttr>();
				attrIt != fd->specific_attr_end<clang::AnnotateAttr>();
				++attrIt)
			{
				llvm::StringRef annotation = attrIt->getAnnotation();
				if(annotation == "sg_meta_no_export")
				{
					std::cout << "\n\t\tno export";
					break;
				}
			}
		}
		else if (clang::CXXMethodDecl *M = llvm::dyn_cast<clang::CXXMethodDecl>(*it))
		{
			std::string name = M->getNameAsString();
			std::string retType = M->getReturnType().getAsString();
			std::cout << "\n\tParsing Method: " << retType << " " << name;
            for (auto attr_it = M->specific_attr_begin<clang::AnnotateAttr>();
                attr_it != M->specific_attr_end<clang::AnnotateAttr>();
                ++attr_it) 
			{
                llvm::StringRef annotation = attr_it->getAnnotation();
				if(annotation == "sg_meta_no_export")
				{
					std::cout << "\n\t\tno export";
					break;
				}
            }
        }
		else if(clang::EnumDecl *ED = llvm::dyn_cast<clang::EnumDecl>(*it))
		{
			std::string enumName = ED->getNameAsString();
			std::cout << "\n\tParsing Enum: " << enumName;
			for (auto attr_it = ED->specific_attr_begin<clang::AnnotateAttr>();
					attr_it != ED->specific_attr_end<clang::AnnotateAttr>();
					++attr_it) 
			{
				llvm::StringRef annotation = attr_it->getAnnotation();
				if(annotation == "sg_meta_no_export")
				{
					std::cout << "\n\t\tno export";
					break;
				}
			}
		}
    }

    return Def;
}
