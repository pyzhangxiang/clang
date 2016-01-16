
#include "sgASTConsumer.h"
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/Sema/Sema.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Tool.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Lex/LexDiagnostic.h>

#include <clang/Driver/Job.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>

#include <iostream>

#include "sgASTConsumer.h"

class MocAction : public clang::ASTFrontendAction {

private:
	sgASTConsumer *mComsumerPtr;

protected:

    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI,
                                           llvm::StringRef InFile) override {

        CI.getFrontendOpts().SkipFunctionBodies = true;
        CI.getPreprocessor().enableIncrementalProcessing(true);
        CI.getPreprocessor().SetSuppressIncludeNotFoundError(true);
        CI.getLangOpts().DelayedTemplateParsing = true;

        //enable all the extension
        CI.getLangOpts().MicrosoftExt = true;
        CI.getLangOpts().DollarIdents = true;
#if CLANG_VERSION_MAJOR != 3 || CLANG_VERSION_MINOR > 2
        CI.getLangOpts().CPlusPlus11 = true;
#else
        CI.getLangOpts().CPlusPlus0x = true;
#endif
        //CI.getLangOpts().CPlusPlus1y = true;
        //CI.getLangOpts().GNUMode = true;

		// Override the resources path.
		//CI.getHeaderSearchOpts().ResourceDir = ResourceFilesPath;

		mComsumerPtr = new sgASTConsumer(CI);
        return std::unique_ptr<sgASTConsumer>(mComsumerPtr);
    }

	virtual void EndSourceFileAction()
	{
		//clang::CompilerInstance &CI = getCompilerInstance();
		//clang::ASTConsumer &consumer = CI.getASTConsumer();

		std::cout << "\n\n\nParsing End";
		std::cout << "\n\n\nClasses:";
		for (int i = 0; i < mComsumerPtr->mExportClasses.size(); ++i)
		{
			std::cout << "\n\n===========================";
			const ClassDef &def = mComsumerPtr->mExportClasses[i];
			std::cout << "\nName: " << def.name;
			std::cout << "\nType Name: " << def.typeName;
			std::cout << "\nBase Name: " << def.baseClassTypeName;
			std::cout << "\nProperty:";
			for (int ip = 0; ip < def.properties.size(); ++ip)
			{
				const PropertyDef &pdef = def.properties[ip];
				std::cout << "\n\t" << pdef.name << " [" << pdef.typeName << "]";
				if (pdef.isPointer) std::cout << " <pointer>";
				if (pdef.isEnum) std::cout << " <enum>";
				if (pdef.isArray) std::cout << " <array>";
			}

		}


		ASTFrontendAction::EndSourceFileAction();
	}

public:
    // CHECK
    virtual bool hasCodeCompletionSupport() const { return true; }
};

std::string GetFileName(const std::string& path);
std::string GetFileExtension(const std::string& path);

int main(int argc, const char **argv) 
{
	std::vector<std::string> Argv;
	Argv.push_back(argv[0]);
	Argv.push_back("-x");  // Type need to go first
	Argv.push_back("c++");
	Argv.push_back("-std=c++11");
	Argv.push_back("-fsyntax-only");

	//Options.Output = "-";
	for (int I = 1 ; I < argc; ++I) 
	{
		if (argv[I][0] == '-') 
		{
			switch (argv[I][1]) 
			{
				case 'o':
					// output file
					continue;
				default:
					break;
			}
		}
		Argv.push_back(argv[I]);        
	}

	llvm::IntrusiveRefCntPtr<clang::FileManager> Files(
		new clang::FileManager(clang::FileSystemOptions()));

	Argv.push_back("D:\\projects\\llvm\\tools\\clang\\tools\\sgmoc\\sample.h");
	//Argv.push_back("D:\\projects\\llvm\\tools\\clang\\tools\\sgmoc\\sgMetaDef.h");

	MocAction *action = new MocAction;
	clang::tooling::ToolInvocation Inv(Argv, action, Files.get());
	//Inv.mapVirtualFile(f->filename, {f->content , f->size } );

	bool ret = !Inv.run();

	int i = 0;
	++i;
	return ret;

 }


std::string GetFileExtension(const std::string& path)
{
	size_t posDot = path.find_last_of('.');
	if (posDot != std::string::npos)
	{
		return path.substr(posDot + 1, path.size() - posDot);
	}

	return "";
}
std::string GetFileName(const std::string& path)
{
	size_t pos0 = path.find_last_of('/');
	size_t pos1 = path.find_last_of('\\', pos0);

	size_t posDot = path.find_last_of('.');

	size_t pos;

	if (pos1 == std::string::npos)
	{
		if (pos0 == std::string::npos)
		{
			pos = 0;
		}
		else
		{
			pos = pos0 + 1;
		}
	}
	else
	{
		pos = pos1 + 1;
	}

	size_t size;
	if (posDot == std::string::npos)
	{
		size = path.size() - pos;
	}
	else
	{
		size = posDot - pos;
	}

	return path.substr(pos, size);
}
