
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

std::string GetTypeName(const std::string fullTypeName)
{
	if (fullTypeName.find_first_of("class ") == 0)
	{
		return fullTypeName.substr(6);
	}
	else if (fullTypeName.find_first_of("struct ") == 0)
	{
		return fullTypeName.substr(7);
	}
	else if (fullTypeName.find_first_of("enum ") == 0)
	{
		return fullTypeName.substr(5);
	}

	return fullTypeName;
}

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
	
	TopLevelParseDecl(decl);
		
	return true;
}

void sgASTConsumer::HandleTagDeclDefinition(clang::TagDecl* D)
{
	if(!mPPCallbacks->IsInMainFile())
	{
		return ;
	}
	
	//clang::Decl::Kind knd = D->getKind();

// 	if(clang::CXXRecordDecl *RD = llvm::dyn_cast<clang::CXXRecordDecl>(D))
// 	{
// 		clang::Decl::Kind classKind = RD->getKind();
// 		/*if(classKind == clang::Decl::ClassTemplate
// 			|| classKind == clang::Decl::ClassTemplateSpecialization
// 			|| classKind == clang::Decl::ClassTemplatePartialSpecialization)
// 		{
// 			return ;
// 		}*/
// 
// 		if(classKind != clang::Decl::CXXRecord)
// 		{
// 			return ;
// 		}
// 
// 		// class
// 		ClassDef def = ParseClass(RD);
// 		if(def.toExport)
// 		{
// 			mExportClasses.push_back(def);
// 		}
// 	}

}

void sgASTConsumer::ParseClass(clang::CXXRecordDecl *RD, bool uplevelExport)
{
	if (!RD->isCompleteDefinition())
	{
		return;
	}

	clang::Preprocessor &PP = mCompilerInstance.getPreprocessor();
    

	std::string name = RD->getNameAsString();

	name = RD->getTypeForDecl()->getCanonicalTypeInternal().getAsString();// getLocallyUnqualifiedSingleStepDesugaredType().getAsString();
	//clang::ASTContext ct(;
	//RD->viewInheritance(ct);
	
	clang::Decl::Kind classKind = RD->getKind();
	std::cout << "\nParsing Class: " << name << " kind " << classKind;

	if (classKind != clang::Decl::CXXRecord)
	{
		//clang::Decl::ClassTemplate
		//clang::Decl::ClassTemplateSpecialization
		//clang::Decl::ClassTemplatePartialSpecialization)

		return;
	}

	ClassDef Def;
	Def.classDecl = RD;
	Def.name = RD->getNameAsString();
	Def.typeName = GetTypeName(name);

	bool localToExport = false;

	if(RD->hasDefinition())
	{
		auto firstBase = RD->bases_begin();
		if(firstBase != RD->bases_end())
		{
			Def.baseClassTypeName = GetTypeName(firstBase->getType().getAsString());

			std::cout << "\n\tBase class: " << Def.baseClassTypeName;
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
					Def.mt = MT_OBJ;
					localToExport = true;
					
					std::cout << "\n\tsg_meta_object";
                }
				else if (key == "sg_meta_object_abstract")
				{
					Def.mt = MT_OBJ_ABSTRACT;
					localToExport = true;
					std::cout << "\n\tsg_meta_object_abstract";
				}
				else if (key == "sg_meta_other")
				{
					Def.mt = MT_OTHER;
					localToExport = true;
					std::cout << "\n\tsg_meta_other";
				}
            }
        } 
		else if (clang::CXXRecordDecl *rd = llvm::dyn_cast<clang::CXXRecordDecl>(*it))
		{
			if(rd != RD)
				ParseClass(rd, localToExport);
		}
		else if(clang::FieldDecl *fd = llvm::dyn_cast<clang::FieldDecl>(*it))
		{
			PropertyDef pd;

			std::string fieldName = fd->getNameAsString();
			clang::QualType fieldType = fd->getType();
			std::string typeName = fieldType.getAsString();
			std::string typeClassName = fieldType->getTypeClassName();

			pd.name = fieldName;
			pd.typeName = GetTypeName(typeName);

			std::cout << "\n\tParsing Field: " << typeName << " " << fieldName;
			if(fieldType->isEnumeralType())
			{
				pd.isEnum = true;
				std::cout << "\n\t\tis Enum";
			}
			if(fieldType->isPointerType())
			{
				pd.isPointer = true;
				std::cout << "\n\t\tis Pointer";
			}
			if(fieldType->isArrayType())
			{
				pd.isArray = true;

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

			bool toExport = true;
			for(auto attrIt = fd->specific_attr_begin<clang::AnnotateAttr>();
				attrIt != fd->specific_attr_end<clang::AnnotateAttr>();
				++attrIt)
			{
				llvm::StringRef annotation = attrIt->getAnnotation();
				if(annotation == "sg_meta_no_export")
				{
					toExport = false;
					std::cout << "\n\t\tno export";
					break;
				}
			}

			if (toExport)
			{
				Def.properties.push_back(pd);
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
			ParseEnum(ED, localToExport);
		}
    }

	if (uplevelExport || localToExport)
	{
		mExportClasses.push_back(Def);
	}
}

void sgASTConsumer::ParseEnum(clang::EnumDecl *ED, bool uplevelExport)
{
	EnumDef def;
	def.enumDecl = ED;

	std::string enumName = ED->getNameAsString();
	def.name = enumName;
	std::cout << "\nParsing Enum: " << enumName;

	bool localToExport = false;
	for (auto attr_it = ED->specific_attr_begin<clang::AnnotateAttr>();
	attr_it != ED->specific_attr_end<clang::AnnotateAttr>();
		++attr_it)
	{
		llvm::StringRef annotation = attr_it->getAnnotation();
		if (annotation == "sg_meta_enum")
		{
			std::cout << "\n\tsg_meta_enum";
			localToExport = true;
			break;
		}
		else if (annotation == "sg_meta_no_export")
		{
			std::cout << "\n\t\tno export";
			localToExport = false;
			break;
		}
	}

	clang::EnumDecl::enumerator_range range = ED->enumerators();
	for (auto it = range.begin(); it != range.end(); ++it)
	{
		EnumValueDef pd;
		std::string name = it->getNameAsString();
		pd.name = name;
		pd.value = *(it->getInitVal().getRawData());
		def.values.push_back(pd);

		std::cout << "\n\tEnumConstant: " << name << " " << *(it->getInitVal().getRawData());
	}

	if (uplevelExport || localToExport)
	{
		mExportEnums.push_back(def);
	}
	
}

void sgASTConsumer::TopLevelParseDecl(clang::Decl *decl)
{
	if (clang::FunctionDecl *MD = llvm::dyn_cast<clang::FunctionDecl>(decl))
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
			if (annotation == "sg_meta_no_export")
			{
				std::cout << "\n\tno export";
				toExport = false;
				break;
			}
		}

		if (toExport)
		{
			mExportFunctions.push_back(MD);
		}
	}
	else if (clang::EnumDecl *ED = llvm::dyn_cast<clang::EnumDecl>(decl))
	{
		ParseEnum(ED, false);
	}
	else if (clang::CXXRecordDecl *RD = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
	{
		ParseClass(RD, false);
	}
	else if (clang::NamespaceDecl *ND = llvm::dyn_cast<clang::NamespaceDecl>(decl))
	{
		std::cout << "\nParse Namespace " << ND->getNameAsString();
		for (auto it = ND->decls_begin(); it != ND->decls_end(); ++it)
		{
			TopLevelParseDecl(*it);
		}
	}
}
