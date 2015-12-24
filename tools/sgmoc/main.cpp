
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

        return std::unique_ptr<sgASTConsumer>(new sgASTConsumer(CI));
    }

public:
    // CHECK
    virtual bool hasCodeCompletionSupport() const { return true; }
};

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

	//Argv.push_back("aa.h");

	clang::tooling::ToolInvocation Inv(Argv, new MocAction, Files.get());
	//Inv.mapVirtualFile(f->filename, {f->content , f->size } );

	return !Inv.run();

 }

