
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

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <fstream>

#include "sgASTConsumer.h"
#include <llvm/Support/MD5.h>
#include <clang/Tooling/CommonOptionsParser.h>


std::string GetFileName(const std::string& path);
std::string GetFileExtension(const std::string& path);
std::string ReplaceString(const std::string src, const std::string &oldstr, const std::string &newstr);
std::string Md5File(const std::string filename);
bool Moc(const std::string arg0, const std::string &inputFilePath, const std::string &outputDir);

class MocAction : public clang::ASTFrontendAction {

private:
	sgASTConsumer *mComsumerPtr;
	std::string mOutputFilePath;

public:
	MocAction(const std::string outputfilepath) : clang::ASTFrontendAction(), mOutputFilePath(outputfilepath) {}

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

		std::ofstream out(mOutputFilePath.c_str());
		if (out.fail())
		{
			return;
		}

		std::cout << "\n\n\nEnums:";
		for (size_t i = 0; i < mComsumerPtr->mExportEnums.size(); ++i)
		{
			std::cout << "\n\n===========================";
			const EnumDef &def = mComsumerPtr->mExportEnums[i];
			
			std::string metaname = ReplaceString(def.name, "::", "__");
			out << "\n\n" << "SG_META_ENUM_DEF_BEGIN(" << metaname << ", " << def.name << ")";

			for (size_t iv = 0; iv < def.values.size(); ++iv)
			{
				const EnumValueDef &pdef = def.values[iv];
				out << "\n\t" << "SG_ENUM_VALUE_DEF(" << pdef.name << ")";
			}
			
			out << "\n" << "SG_META_DEF_END";
		}

		std::cout << "\n\n\nClasses:";
		for (size_t i = 0; i < mComsumerPtr->mExportClasses.size(); ++i)
		{
			std::cout << "\n\n===========================";
			const ClassDef &def = mComsumerPtr->mExportClasses[i];

			std::string metaBegin;
			if (def.mt == MT_OBJ_ABSTRACT)
			{
				metaBegin = "SG_META_OBJ_ABSTRACT_DEF_BEGIN";
			}
			else if (def.mt == MT_OBJ)
			{
				metaBegin = "SG_META_OBJECT_DEF_BEGIN";
			}
			else
			{
				metaBegin = "SG_META_OTHER_DEF_BEGIN";
			}
			std::string metaname = ReplaceString(def.typeName, "::", "__");
			out << "\n\n" << metaBegin << "(" << metaname << ", " << def.typeName << ", " << def.baseClassTypeName << ")";

			std::cout << "\nName: " << def.name;
			std::cout << "\nType Name: " << def.typeName;
			std::cout << "\nBase Name: " << def.baseClassTypeName;
			std::cout << "\nProperty:";
			for (int ip = 0; ip < def.properties.size(); ++ip)
			{
				const PropertyDef &pdef = def.properties[ip];
				std::cout << "\n\t" << pdef.name << " [" << pdef.typeName << "]";
				if (pdef.isEnum)
				{
					out << "\n\t" << "SG_ENUM_PROPERTY_DEF(" << pdef.name << ", " << pdef.typeName << ")";
				}
				else if (pdef.isArray)
				{
					out << "\n\t" << "SG_ARRAY_PROPERTY_DEF(" << pdef.name << ")";
				}
				else
				{
					out << "\n\t" << "SG_PROPERTY_DEF(" << pdef.name << ")";
				}

				if (pdef.isPointer) std::cout << " <pointer>";
				if (pdef.isEnum) std::cout << " <enum>";
				if (pdef.isArray) std::cout << " <array>";
			}

			out << "\n" << "SG_META_DEF_END";

		}

		out.flush();
		out.close();

		ASTFrontendAction::EndSourceFileAction();
	}

public:
    // CHECK
    virtual bool hasCodeCompletionSupport() const { return true; }
};

class MocFrontendActionFactory : public clang::tooling::FrontendActionFactory {
public:
	std::string mOutputFilePath;
	MocFrontendActionFactory(const std::string outFilePath) : mOutputFilePath(outFilePath) {}
	clang::FrontendAction *create() override { return new MocAction(mOutputFilePath); }
};




int main(int argc, const char **argv) 
{
	

	if (argc < 3)
	{
		std::cout << "Usage is [-force] <outdir> <infiles> ...\n";
		return 1;
	}

	int outdirIndex = 1;
	int infileIndex = 2;
	bool forceMoc = false;
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			if (strcmp(argv[i], "-force") == 0)
			{
				forceMoc = true;
				outdirIndex = 2;
				infileIndex = 3;
			}
		}
	}

	if (forceMoc && argc < 4)
	{
		std::cout << "Usage is [-force] <outdir> <infiles> ...\n";
		return 1;
	}

	std::cout << "\n============================= Start SG Moc =====================";
	std::string outDir = argv[outdirIndex];
	if (outDir[outDir.size() - 1] != '/' || outDir[outDir.size() - 1] != '\\')
	{
		outDir += "/";
	}
	std::cout << "\n\nOut dir: " << outDir;

	// get headers' md5    
	std::map<std::string, std::string> mHeaderMd5;
	std::string md5file = outDir + "md5.txt";
	std::ifstream md5In(md5file.c_str());
	if (!md5In.fail())
	{
		std::string filename, md5;
		while (md5In >> filename && md5In >> md5)
		{
			mHeaderMd5[filename] = md5;
		}
	}

	std::vector<std::string> inFiles;
	inFiles.reserve(argc - infileIndex);
	for (int i = infileIndex; i < argc; ++i)
	{
		inFiles.push_back(argv[i]);
		//std::cout << " " << argv[i];
	}
	std::cout << "\n";

	for (size_t i = 0; i < inFiles.size(); ++i)
	{
		std::string &filepath = inFiles[i];

		if (!forceMoc)
		{
			std::string newmd5 = Md5File(filepath);
			if (newmd5.empty())
			{
				std::cout << "\n";
				continue;
			}

			std::string oldmd5;
			auto it = mHeaderMd5.find(filepath);
			if (it != mHeaderMd5.end() && it->second == newmd5)
			{
				//std::cout << "\n no need to generate: " << filepath;
				continue;
			}

			mHeaderMd5[filepath] = newmd5;
		}
		

		Moc(argv[0], filepath, outDir);
		

	}

	if (!forceMoc)
	{
		// output md5
		std::ofstream md5Out(md5file.c_str());
		if (md5Out.fail())
		{
			std::cout << "Error, cannot open md5file: " << md5file.c_str() << "\n";
		}
		else
		{
			for (auto it = mHeaderMd5.begin(); it != mHeaderMd5.end(); ++it)
			{
				md5Out << it->first << " " << it->second << "\n";
			}
			md5Out.close();
		}
	}
	

	return 0;

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

std::string ReplaceString(const std::string src, const std::string &oldstr, const std::string &newstr)
{
	std::string ret = src;

	size_t pos = ret.find(oldstr);
	while (pos != std::string::npos)
	{
		ret.replace(pos, oldstr.size(), newstr);
		pos = ret.find(oldstr, pos);
	}
	return ret;
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

std::string Md5File(const std::string filepath)
{
	std::ifstream fileIn(filepath.c_str());
	if (fileIn.fail())
		return "";

	fileIn.seekg(0, fileIn.end);
	int length = fileIn.tellg();
	fileIn.seekg(0, fileIn.beg);

	char *buffer = new char[length];
	fileIn.read(buffer, length);

	llvm::StringRef md5Input(buffer);

	llvm::MD5 Hash;
	Hash.update(md5Input);
	llvm::MD5::MD5Result MD5Res;
	Hash.final(MD5Res);
	llvm::SmallString<32> Res;
	llvm::MD5::stringifyResult(MD5Res, Res);

	delete []buffer;

	return std::string(Res.c_str());

}

bool Moc(const std::string arg0, const std::string &inputFilePath, const std::string &outputDir)
{
	std::vector<std::string> Argv;
	Argv.push_back(arg0);
	//Argv.push_back("-x");  // Type need to go first
	//Argv.push_back("c++");
	//Argv.push_back("-fms-compatibility-version=19");
	//Argv.push_back("-Wall");
	//Argv.push_back("-Wmicrosoft-include");
	//Argv.push_back("-ID:\\projects\\llvm\\tools\\clang\\tools\\sgmoc\\aa");
	//Argv.push_back("-std=c++11");
	//Argv.push_back("-fsyntax-only");

	llvm::IntrusiveRefCntPtr<clang::FileManager> Files(
		new clang::FileManager(clang::FileSystemOptions()));

	Argv.push_back(inputFilePath);

	Argv.push_back("-x");  // Type need to go first
	Argv.push_back("c++");
	Argv.push_back("-fms-compatibility-version=19");
	//Argv.push_back("-Wall");
	//Argv.push_back("-Wmicrosoft-include");
	Argv.push_back("-ID:\\projects\\llvm\\tools\\clang\\tools\\sgmoc\\aa");
	Argv.push_back("-std=c++11");
	Argv.push_back("-fsyntax-only"); 

	std::string outfile = outputDir + "gen_" + GetFileName(inputFilePath) + ".cpp";
	MocAction *action = new MocAction(outfile);
	clang::tooling::ToolInvocation Inv(Argv, action, Files.get());
	//Inv.mapVirtualFile(f->filename, {f->content , f->size } );

	bool ret = !Inv.run();

	return ret;
}