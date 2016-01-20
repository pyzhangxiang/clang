
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
	
	clang::SourceManager &SM = mCompilerInstance.getPreprocessor().getSourceManager();
	//clang::StringRef fname = SM.getFilename(decl->getLocation());
	if (SM.getFileID(decl->getLocation()) != SM.getMainFileID())
	{
		return true;
	}

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

	std::string name = RD->getQualifiedNameAsString();// RD->getTypeForDecl()->getCanonicalTypeInternal().getAsString();// getLocallyUnqualifiedSingleStepDesugaredType().getAsString();
	//clang::ASTContext ct(;
	//RD->viewInheritance(ct);
	
	clang::Decl::Kind classKind = RD->getKind();

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
		}
	}

	if (Def.baseClassTypeName.empty())
	{
		Def.baseClassTypeName = "0";
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
                }
				else if (key == "sg_meta_object_abstract")
				{
					Def.mt = MT_OBJ_ABSTRACT;
					localToExport = true;
				}
				else if (key == "sg_meta_other")
				{
					Def.mt = MT_OTHER;
					localToExport = true;
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

			pd.name = fieldName;
			pd.typeName = GetTypeName(typeName);

			if(fieldType->isEnumeralType())
			{
				pd.isEnum = true;
			}
			if(fieldType->isPointerType())
			{
				pd.isPointer = true;
			}
			if(fieldType->isArrayType())
			{
				pd.isArray = true;
			}
			if(fieldType->isBuiltinType())
			{
			}
			if(fieldType->isBooleanType())
			{
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
					break;
				}
			}

			if (toExport)
			{
				Def.properties.push_back(pd);
			}
		}
// 		else if (clang::CXXMethodDecl *M = llvm::dyn_cast<clang::CXXMethodDecl>(*it))
// 		{
// 			std::string name = M->getNameAsString();
// 			std::string retType = M->getReturnType().getAsString();
//             for (auto attr_it = M->specific_attr_begin<clang::AnnotateAttr>();
//                 attr_it != M->specific_attr_end<clang::AnnotateAttr>();
//                 ++attr_it) 
// 			{
//                 llvm::StringRef annotation = attr_it->getAnnotation();
// 				if(annotation == "sg_meta_no_export")
// 				{
// 					break;
// 				}
//             }
//         }
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

	def.name = ED->getQualifiedNameAsString();

	bool localToExport = false;
	for (auto attr_it = ED->specific_attr_begin<clang::AnnotateAttr>();
	attr_it != ED->specific_attr_end<clang::AnnotateAttr>();
		++attr_it)
	{
		llvm::StringRef annotation = attr_it->getAnnotation();
		if (annotation == "sg_meta_enum")
		{
			localToExport = true;
			break;
		}
		else if (annotation == "sg_meta_no_export")
		{
			localToExport = false;
			break;
		}
	}

	clang::EnumDecl::enumerator_range range = ED->enumerators();
	for (auto it = range.begin(); it != range.end(); ++it)
	{
		EnumValueDef pd;
		pd.name = it->getQualifiedNameAsString();
		pd.value = *(it->getInitVal().getRawData());
		def.values.push_back(pd);
	}

	if (uplevelExport || localToExport)
	{
		mExportEnums.push_back(def);
	}
	
}

void sgASTConsumer::TopLevelParseDecl(clang::Decl *decl)
{
	
	if (clang::EnumDecl *ED = llvm::dyn_cast<clang::EnumDecl>(decl))
	{
		ParseEnum(ED, false);
	}
// 	else if (clang::FunctionDecl *MD = llvm::dyn_cast<clang::FunctionDecl>(decl))
// 	{
// 		// global function
// 		std::string funcName = MD->getNameAsString();
// 		std::string retType = MD->getReturnType().getAsString();
// 
// 		bool toExport = true;
// 		for (auto attr_it = MD->specific_attr_begin<clang::AnnotateAttr>();
// 		attr_it != MD->specific_attr_end<clang::AnnotateAttr>();
// 			++attr_it)
// 		{
// 			llvm::StringRef annotation = attr_it->getAnnotation();
// 			if (annotation == "sg_meta_no_export")
// 			{
// 				toExport = false;
// 				break;
// 			}
// 		}
// 
// 		if (toExport)
// 		{
// 			mExportFunctions.push_back(MD);
// 		}
// 	}
	else if (clang::CXXRecordDecl *RD = llvm::dyn_cast<clang::CXXRecordDecl>(decl))
	{
		ParseClass(RD, false);
	}
	else if (clang::NamespaceDecl *ND = llvm::dyn_cast<clang::NamespaceDecl>(decl))
	{
		for (auto it = ND->decls_begin(); it != ND->decls_end(); ++it)
		{
			TopLevelParseDecl(*it);
		}
	}
}

void sgASTConsumer::HandleTranslationUnit(clang::ASTContext& Ctx)
{
	// all parsed, output
// 	SGVisitor visitor;
// 	visitor.consumer = this;
// 	visitor.context = &Ctx;
// 	visitor.TraverseDecl(Ctx.getTranslationUnitDecl());
}

bool SGVisitor::VisitDecl(clang::Decl *D)
{
	if (!consumer->mPPCallbacks->IsInMainFile())
	{
		return true;
	}
	consumer->TopLevelParseDecl(D);
	return true;
}
