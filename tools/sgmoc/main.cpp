
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
#include <sstream>

#include "sgASTConsumer.h"
#include "sgPPCallbacks.h"
#include <llvm/Support/MD5.h>
#include <clang/Tooling/CommonOptionsParser.h>

int MSVC_VERTION = 0;

class StrTokenizer
{
public:
	// ctor/dtor
	StrTokenizer();
	StrTokenizer(const std::string& str, const std::string& delimiter);
	~StrTokenizer();

	// Set string and delimiter
	void Set(const std::string& str, const std::string& delimiter);
	void SetString(const std::string& str);             // Set source string only
	void SetDelimiter(const std::string& delimiter);    // Set delimiter string only

	bool IsEnd() const;
	void Reset();
	void Join(std::string& str, const std::string& separator);
	bool Next(std::string& str);
	void GetSubStr(std::vector<std::string>& subStr);

private:
	void SkipDelimiter();                               // ignore leading delimiters
	bool IsDelimiter(char c);                           // check if the current char is delimiter

	std::string mBuffer;                                 // input string
	std::string mToken;                                  // output string
	std::string mDelimiter;                              // delimiter string
	std::string::iterator mCurrPos;                // string iterator pointing the current position
};

/*
*	string tokenizer
*/

StrTokenizer::StrTokenizer() : mBuffer(""), mToken(""), mDelimiter(" ")
{
	mCurrPos = mBuffer.begin();
}

StrTokenizer::StrTokenizer(const std::string& str, const std::string& delimiter) : mBuffer(str), mToken(""), mDelimiter(delimiter)
{
	mCurrPos = mBuffer.begin();
}

StrTokenizer::~StrTokenizer()
{
}

void StrTokenizer::Set(const std::string& str, const std::string& delimiter)
{
	mBuffer = str;
	mDelimiter = delimiter;
	mCurrPos = mBuffer.begin();
}

void StrTokenizer::SetString(const std::string& str)
{
	mBuffer = str;
	mCurrPos = mBuffer.begin();
}

void StrTokenizer::SetDelimiter(const std::string& delimiter)
{
	mDelimiter = delimiter;
	mCurrPos = mBuffer.begin();
}

bool StrTokenizer::Next(std::string& str)
{
	if (IsEnd())
		return false;

	mToken.clear();                       // reset token string

	SkipDelimiter();                      // skip leading delimiters

										  // append each char to token string until it meets delimiter
	int beginIdx = mCurrPos - mBuffer.begin(), len = 0;
	while (mCurrPos != mBuffer.end() && !IsDelimiter(*mCurrPos))
	{
		++mCurrPos;
		++len;
	}

	mToken = mBuffer.substr(beginIdx, len);
	str = mToken;
	SkipDelimiter();

	// add by joewan 2011/8/11, avoid "" in the result vec.
	if (len == 0)
	{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// skip ang leading delimiters
///////////////////////////////////////////////////////////////////////////////
void StrTokenizer::SkipDelimiter()
{
	while (mCurrPos != mBuffer.end() && IsDelimiter(*mCurrPos))
	{
		++mCurrPos;
	}
}

///////////////////////////////////////////////////////////////////////////////
// return true if the current character is delimiter
///////////////////////////////////////////////////////////////////////////////
bool StrTokenizer::IsDelimiter(char c)
{
	return (mDelimiter.find(c) != std::string::npos);
}

void StrTokenizer::GetSubStr(std::vector<std::string>& subStr)
{
	mCurrPos = mBuffer.begin();
	std::string token;
	while (Next(token))
	{
		subStr.push_back(token);
	}
}

void StrTokenizer::Reset()
{
	mCurrPos = mBuffer.begin();
}

void StrTokenizer::Join(std::string& str, const std::string& separator)
{
	Reset();
	bool first = true;
	std::string token;
	while (Next(token))
	{
		if (first)
		{
			str = token;
			first = false;
		}
		else
			str += separator + token;
	}
}

bool StrTokenizer::IsEnd() const
{
	if (mBuffer.size() <= 0 || mCurrPos == mBuffer.end())
	{
		return true;
	}
	return false;
}




std::string GetFileName(const std::string& path);
std::string GetFileDir(const std::string& path);
std::string GetFileExtension(const std::string& path);
std::string ReplaceString(const std::string src, const std::string &oldstr, const std::string &newstr);
std::string ReplaceChar(const std::string src, char oldc, char newc);
std::string Md5File(const std::string filename);
std::string GetFileLastModifyTime(const std::string filepath);
const char* GetRelativeFilename(const std::string &dir, const std::string &filepath);
std::string GetAbsoluteFilename(const std::string &dir, const std::string &relativefilepath);
void StringSplit(const std::string& target, const std::string& delim, std::vector< std::string >& rkVec);
int GetBestMsvcVersion();

bool Moc(const std::string arg0, const std::string &outputDir
	, const std::vector<std::string> &inputFiles
	, const std::vector<std::string> &includeDirs);

class MocAction : public clang::ASTFrontendAction {

private:
	sgASTConsumer *mComsumerPtr;
	std::string mOutputDir;
	std::string mInputFilePath;

public:
	MocAction(const std::string &outDir, const std::string &inputfile) 
		: clang::ASTFrontendAction()
		, mOutputDir(outDir)
		, mInputFilePath(inputfile){}

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

		if (mComsumerPtr->mExportEnums.empty() && mComsumerPtr->mExportClasses.empty())
		{
			std::cout << "\n\tNothing to export";
			return;
		}

		std::string outfile = mOutputDir + "meta_" + GetFileName(mInputFilePath) + ".cpp";
		std::ofstream out(outfile.c_str());
		if (out.fail())
		{
			return;
		}

		out << "#include \"" << GetRelativeFilename(mOutputDir, mInputFilePath) << "\"\n";
		// meta include
		std::string inputeFileDir = GetFileDir(mInputFilePath);
		for (const std::string &inc : (mComsumerPtr->mPPCallbacks->mMetaIncludes))
		{
			std::string incAbsFilepath = GetAbsoluteFilename(inputeFileDir, inc);
			out << "#include \"" << GetRelativeFilename(mOutputDir, incAbsFilepath) << "\"\n";
		}

		for (size_t i = 0; i < mComsumerPtr->mExportEnums.size(); ++i)
		{
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

		for (size_t i = 0; i < mComsumerPtr->mExportClasses.size(); ++i)
		{
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

			for (size_t ip = 0; ip < def.properties.size(); ++ip)
			{
				const PropertyDef &pdef = def.properties[ip];
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

			}

			out << "\n" << "SG_META_DEF_END";

		}
		
		out << "\n";

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
	clang::FrontendAction *create() override { return new MocAction(mOutputFilePath, ""); }
};



#define SOURCES_MD5_FILE "src_info.txt"
int main(int argc, const char **argv) 
{
	if (argc < 3)
	{
		std::cout << "Usage is [-force] <-OOutputDir> [-IIncludeDir] <InputFile [InputFile [...]]> \n";
		return 1;
	}

	std::vector<std::string> includePathes;
	std::vector<std::string> inFiles;
	std::string outDir;

	bool forceMoc = false;
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			if (strcmp(argv[i], "-force") == 0)
			{
				forceMoc = true;
			}
			else if (argv[i][1] == 'I')
			{
				//std::cout << "\nInclude " << argv[i];
				includePathes.push_back(argv[i]);
			}
			else if (argv[i][1] == 'O')
			{
				outDir = &(argv[i][2]);
			}
		}
		else
		{
			inFiles.push_back(argv[i]);
		}
	}

	int clangUsedVcVersion = GetBestMsvcVersion();
	if (clangUsedVcVersion >= 14)
	{
		MSVC_VERTION = 1900;
	}
	else if (clangUsedVcVersion >= 10)
	{
		MSVC_VERTION = 1600;
	}
	else
	{
		MSVC_VERTION = 0;
	}

	std::cout << "\n============================= SG Moc Start =====================";
	if (outDir[outDir.size() - 1] != '/' || outDir[outDir.size() - 1] != '\\')
	{
		outDir += "/";
	}
	std::cout << "\n\nOut dir: " << outDir;

	// get headers' md5    
	std::map<std::string, std::string> mHeaderMd5;
	std::string md5file = outDir + SOURCES_MD5_FILE;
	std::ifstream md5In(md5file.c_str());
	if (!md5In.fail())
	{
		std::string filename, md5;
		while (md5In >> filename && md5In >> md5)
		{
			mHeaderMd5[filename] = md5;
		}
	}

	std::vector<std::string> needMocFiles;
	for (size_t i = 0; i < inFiles.size(); ++i)
	{
		std::string &filepath = inFiles[i];

		if (!forceMoc)
		{
			std::string newmd5 = GetFileLastModifyTime(filepath);
			if (newmd5.empty())
			{
				continue;
			}

			auto it = mHeaderMd5.find(filepath);
			if (it != mHeaderMd5.end() && it->second == newmd5)
			{
				continue;
			}

			mHeaderMd5[filepath] = newmd5;
		}
		
		needMocFiles.push_back(filepath);
	}

	if (needMocFiles.empty())
	{
		std::cout << "\nNothing to Moc";
	}
	else
	{
		Moc(argv[0], outDir, needMocFiles, includePathes);
	}
	
	std::cout << "\n============================= SG Moc End =====================\n\n";

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

std::string ReplaceChar(const std::string src, char oldc, char newc)
{
	std::string ret = src;

	std::replace(ret.begin(), ret.end(), oldc, newc);
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

std::string GetFileDir(const std::string& path)
{
	size_t pos0 = path.find_last_of('/');
	size_t pos1 = path.find_last_of('\\');

	size_t pos = std::string::npos;
	if (pos0 != std::string::npos && pos1 != std::string::npos)
	{
		pos = pos0 > pos1 ? pos0 : pos1;
	}
	else if (pos0 != std::string::npos)
	{
		pos = pos0;
	}
	else if (pos1 != std::string::npos)
	{
		pos = pos1;
	}
	else
	{
		return "./";
	}

	return path.substr(0, pos + 1);

}
std::string Md5File(const std::string filepath)
{
	FILE *file = fopen(filepath.c_str(), "rb");
	if (file == NULL)
	{
		return "";
	}

	unsigned char buffer[1024];
	int len = 0;
	llvm::MD5 Hash;
	while (0 != (len = fread(buffer, 1, 1024, file)))
	{
		Hash.update(buffer);
	}
	llvm::MD5::MD5Result MD5Res;
	Hash.final(MD5Res);
	llvm::SmallString<32> Res;
	llvm::MD5::stringifyResult(MD5Res, Res);

	return std::string(Res.c_str());

}

std::string GetFileLastModifyTime(const std::string filepath)
{
	int fileID;
	if (auto EC = llvm::sys::fs::openFileForRead(filepath, fileID))
	{
		return "";
	}
	if (fileID == -1)
	{
		return "";
	}

	llvm::sys::fs::file_status st;
	if (auto EC = llvm::sys::fs::status(fileID, st))
	{
		return "";
	}
		
	llvm::sys::TimeValue tv = st.getLastModificationTime();

	std::stringstream ss;
	ss << tv.seconds() << "." << tv.nanoseconds();
	return ss.str();
}

bool Moc(const std::string arg0, const std::string &outputDir
	, const std::vector<std::string> &inputFiles
	, const std::vector<std::string> &includeDirs)
{
	std::vector<std::string> Argv;
	Argv.push_back(arg0);
	//Argv.push_back(inputFilePath);
	
	Argv.push_back("-x");  // Type need to go first
	Argv.push_back("c++");
	Argv.push_back("-fPIE");
	Argv.push_back("-fPIC");
	Argv.push_back("-w");

	if (MSVC_VERTION == 1900)
	{
		Argv.push_back("-fms-compatibility-version=19");

		std::cout << "\nUse MSVC 19";
	}
	else if (MSVC_VERTION == 1600)
	{
		Argv.push_back("-fms-compatibility-version=16");
		std::cout << "\nUse MSVC 16";
	}
	//Argv.push_back("-Wall");
	//Argv.push_back("-Wmicrosoft-include");
	
	Argv.push_back("-std=c++11");
	Argv.push_back("-fsyntax-only");

	//Argv.push_back("-I\"D:\\projects\\llvm\\tools\\clang\\tools\\sgmoc\\aa\"");
	//Argv.push_back("-ID:\\projects\\llvm\\tools\\clang\\tools\\sgmoc");
	Argv.insert(Argv.end(), includeDirs.begin(), includeDirs.end());
	//Argv.push_back("/TP");
	//Argv.push_back("/Zs");

	llvm::IntrusiveRefCntPtr<clang::FileManager> Files(
		new clang::FileManager(clang::FileSystemOptions()));

	std::shared_ptr<clang::PCHContainerOperations> PCHContainerOps = std::make_shared<clang::PCHContainerOperations>();
	MocAction *action = 0;
	for (const std::string &inputFilePath : inputFiles)
	{
		std::vector<std::string> args;
		args.reserve(Argv.size() + 1);
		args.insert(args.end(), Argv.begin(), Argv.end());
		args.push_back(inputFilePath);

		std::cout << "\nsgmoc " << inputFilePath;
		
		action = new MocAction(outputDir, inputFilePath);

		clang::tooling::ToolInvocation Inv(args, action, Files.get(), PCHContainerOps);
		//Inv.mapVirtualFile(f->filename, {f->content , f->size } );

		bool ret = Inv.run();
		if (!ret)
		{
			;
		}
		//std::cout << "\n=========================end" << inputFilePath;
	}

	int i = 0;
	++i;
	return true;
}

// GetRelativeFilename(), by Rob Fisher.
// rfisher@iee.org
// http://come.to/robfisher
// includes

// defines
#define MAX_FILENAME_LEN 512
// The number of characters at the start of an absolute filename.  e.g. in DOS,
// absolute filenames start with "X:\" so this value should be 3, in UNIX they start
// with "\" so this value should be 1.
#define ABSOLUTE_NAME_START 3
// set this to '\\' for DOS or '/' for UNIX
#define SLASH '\\'
#define InvSLASH '/'
// Given the absolute current directory and an absolute file name, returns a relative file name.
// For example, if the current directory is C:\foo\bar and the filename C:\foo\whee\text.txt is given,
// GetRelativeFilename will return ..\whee\text.txt.
const char* GetRelativeFilename(const std::string &dir, const std::string &filepath)
{
	std::string cd = ReplaceChar(dir, '\\', '/');
	std::string af = ReplaceChar(filepath, '\\', '/');

	const char *currentDirectory = cd.c_str();
	const char *absoluteFilename = af.c_str();

	// declarations - put here so this should work in a C compiler
	int afMarker = 0, rfMarker = 0;
	int cdLen = 0, afLen = 0;
	int i = 0;
	int levels = 0;
	static char relativeFilename[MAX_FILENAME_LEN + 1];
	cdLen = strlen(currentDirectory);
	afLen = strlen(absoluteFilename);

	// make sure the names are not too long or too short
	if (cdLen > MAX_FILENAME_LEN || cdLen < ABSOLUTE_NAME_START + 1 ||
		afLen > MAX_FILENAME_LEN || afLen < ABSOLUTE_NAME_START + 1)
	{
		return NULL;
	}

	// Handle DOS names that are on different drives:
	if (currentDirectory[0] != absoluteFilename[0])
	{
		// not on the same drive, so only absolute filename will do
		strcpy(relativeFilename, absoluteFilename);
		return relativeFilename;
	}
	// they are on the same drive, find out how much of the current directory
	// is in the absolute filename
	i = ABSOLUTE_NAME_START;
	while (i < afLen && i < cdLen && currentDirectory[i] == absoluteFilename[i])
	{
		i++;
	}
	if (i == cdLen && (absoluteFilename[i] == InvSLASH || absoluteFilename[i - 1] == InvSLASH))
	{
		// the whole current directory name is in the file name,
		// so we just trim off the current directory name to get the
		// current file name.
		if (absoluteFilename[i] == InvSLASH)
		{
			// a directory name might have a trailing slash but a relative
			// file name should not have a leading one...
			i++;
		}
		strcpy(relativeFilename, &absoluteFilename[i]);
		return relativeFilename;
	}
	// The file is not in a child directory of the current directory, so we
	// need to step back the appropriate number of parent directories by
	// using "..\"s.  First find out how many levels deeper we are than the
	// common directory
	afMarker = i;
	levels = 1;
	// count the number of directory levels we have to go up to get to the
	// common directory
	while (i < cdLen)
	{
		i++;
		if (currentDirectory[i] == InvSLASH)
		{
			// make sure it's not a trailing slash
			i++;
			if (currentDirectory[i] != '\0')
			{
				levels++;
			}
		}
	}
	// move the absolute filename marker back to the start of the directory name
	// that it has stopped in.
	while (afMarker > 0 && absoluteFilename[afMarker - 1] != InvSLASH)
	{
		afMarker--;
	}
	// check that the result will not be too long
	if (levels * 3 + afLen - afMarker > MAX_FILENAME_LEN)
	{
		return NULL;
	}

	// add the appropriate number of "..\"s.
	rfMarker = 0;
	for (i = 0; i < levels; i++)
	{
		relativeFilename[rfMarker++] = '.';
		relativeFilename[rfMarker++] = '.';
		relativeFilename[rfMarker++] = InvSLASH;
	}
	// copy the rest of the filename into the result string
	strcpy(&relativeFilename[rfMarker], &absoluteFilename[afMarker]);
	return relativeFilename;
}

std::string GetAbsoluteFilename(const std::string &dir, const std::string &relativefilepath)
{
	std::string cd = ReplaceChar(dir, '\\', '/');
	std::string af = ReplaceChar(relativefilepath, '\\', '/');

	std::vector<std::string> dirToken;
	StringSplit(cd, "/", dirToken);

	std::vector<std::string> rfileToken;
	StringSplit(af, "/", rfileToken);

	for (const std::string &pathname : rfileToken)
	{
		if (pathname == ".")
		{
			continue;
		}
		else if (pathname == "..")
		{
			dirToken.erase(dirToken.end()-1);
		}
		else
		{
			dirToken.push_back(pathname);
		}
	}

	std::string ret;
	for (size_t i = 0; i < dirToken.size() - 1; ++i)
	{
		ret += dirToken[i] + "/";
	}
	ret += dirToken[dirToken.size() - 1];

	return ret;
}

void StringSplit(const std::string& target, const std::string& delim, std::vector< std::string >& rkVec)
{
	StrTokenizer st(target, delim);
	st.GetSubStr(rkVec);
}

#ifdef _MSC_VER
//

#include <windows.h>
#include <cstdio>

static bool ReadRegFullStringValue(HKEY hkey, const char *valueName,
	std::string &value) {
	// FIXME: We should be using the W versions of the registry functions, but
	// doing so requires UTF8 / UTF16 conversions similar to how we handle command
	// line arguments.  The UTF8 conversion functions are not exposed publicly
	// from LLVM though, so in order to do this we will probably need to create
	// a registry abstraction in LLVMSupport that is Windows only.
	DWORD result = 0;
	DWORD valueSize = 0;
	DWORD type = 0;
	// First just query for the required size.
	result = RegQueryValueEx(hkey, valueName, NULL, &type, NULL, &valueSize);
	if (result != ERROR_SUCCESS || type != REG_SZ)
		return false;
	std::vector<BYTE> buffer(valueSize);
	result = RegQueryValueEx(hkey, valueName, NULL, NULL, &buffer[0], &valueSize);
	if (result == ERROR_SUCCESS)
		value.assign(reinterpret_cast<const char *>(buffer.data()));
	return result;
}

/// \brief Read registry string. from MSVCToolChain.cpp
/// This also supports a means to look for high-versioned keys by use
/// of a $VERSION placeholder in the key path.
/// $VERSION in the key path is a placeholder for the version number,
/// causing the highest value path to be searched for and used.
/// I.e. "SOFTWARE\\Microsoft\\VisualStudio\\$VERSION".
/// There can be additional characters in the component.  Only the numeric
/// characters are compared.  This function only searches HKLM.
static bool GetBestVCVersion(const char *keyPath, const char *valueName,
	std::string &value, std::string *phValue, int &bestVersion)
{
	bestVersion = 0;

	HKEY hRootKey = HKEY_LOCAL_MACHINE;
	HKEY hKey = NULL;
	long lResult;
	bool returnValue = false;

	const char *placeHolder = strstr(keyPath, "$VERSION");
	std::string bestName;
	// If we have a $VERSION placeholder, do the highest-version search.
	if (placeHolder) {
		const char *keyEnd = placeHolder - 1;
		const char *nextKey = placeHolder;
		// Find end of previous key.
		while ((keyEnd > keyPath) && (*keyEnd != '\\'))
			keyEnd--;
		// Find end of key containing $VERSION.
		while (*nextKey && (*nextKey != '\\'))
			nextKey++;
		size_t partialKeyLength = keyEnd - keyPath;
		char partialKey[256];
		if (partialKeyLength > sizeof(partialKey))
			partialKeyLength = sizeof(partialKey);
		strncpy(partialKey, keyPath, partialKeyLength);
		partialKey[partialKeyLength] = '\0';
		HKEY hTopKey = NULL;
		lResult = RegOpenKeyEx(hRootKey, partialKey, 0, KEY_READ | KEY_WOW64_32KEY,
			&hTopKey);
		if (lResult == ERROR_SUCCESS) {
			char keyName[256];
			double bestValue = 0.0;
			DWORD index, size = sizeof(keyName) - 1;
			for (index = 0; RegEnumKeyEx(hTopKey, index, keyName, &size, NULL,
				NULL, NULL, NULL) == ERROR_SUCCESS; index++) {
				const char *sp = keyName;
				while (*sp && !isdigit(*sp))
					sp++;
				if (!*sp)
					continue;
				const char *ep = sp + 1;
				while (*ep && (isdigit(*ep) || (*ep == '.')))
					ep++;
				char numBuf[32];
				strncpy(numBuf, sp, sizeof(numBuf) - 1);
				numBuf[sizeof(numBuf) - 1] = '\0';
				double dvalue = strtod(numBuf, NULL);
				if (dvalue > bestValue) {
					// Test that InstallDir is indeed there before keeping this index.
					// Open the chosen key path remainder.
					bestName = keyName;
					// Append rest of key.
					bestName.append(nextKey);
					lResult = RegOpenKeyEx(hTopKey, bestName.c_str(), 0,
						KEY_READ | KEY_WOW64_32KEY, &hKey);
					if (lResult == ERROR_SUCCESS) {
						lResult = ReadRegFullStringValue(hKey, valueName, value);
						if (lResult == ERROR_SUCCESS) {
							bestValue = dvalue;
							if (phValue)
								*phValue = bestName;
							returnValue = true;

							bestVersion = bestValue;
						}
						RegCloseKey(hKey);
					}
				}
				size = sizeof(keyName) - 1;
			}
			RegCloseKey(hTopKey);
		}
	}
	else {
		lResult =
			RegOpenKeyEx(hRootKey, keyPath, 0, KEY_READ | KEY_WOW64_32KEY, &hKey);
		if (lResult == ERROR_SUCCESS) {
			lResult = ReadRegFullStringValue(hKey, valueName, value);
			if (lResult == ERROR_SUCCESS)
				returnValue = true;
			if (phValue)
				phValue->clear();
			RegCloseKey(hKey);
		}
	}
	return returnValue;
}

#endif

int GetBestMsvcVersion()
{
#ifdef _MSC_VER
	std::string vsIDEInstallDir;
	int bestVersion = 0;
	if (!GetBestVCVersion("SOFTWARE\\Microsoft\\VisualStudio\\$VERSION", "InstallDir", vsIDEInstallDir, nullptr, bestVersion))
	{
		return 0;
	}

	return bestVersion;
#else
	return 0;
#endif
}