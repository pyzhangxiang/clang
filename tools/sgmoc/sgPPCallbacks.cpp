
#include "sgPPCallbacks.h"
#include "sgmetadefs-injected.h"
#include <clang\Lex\MacroArgs.h>

clang::FileID CreateFileIDForMemBuffer(clang::Preprocessor &PP, std::unique_ptr<llvm::MemoryBuffer> Buf, clang::SourceLocation Loc)
{
#if CLANG_VERSION_MAJOR != 3 || CLANG_VERSION_MINOR > 4
    return PP.getSourceManager().createFileID(std::move(Buf), clang::SrcMgr::C_User, 0, 0, Loc);
#else
    return PP.getSourceManager().createFileIDForMemBuffer(Buf, clang::SrcMgr::C_User, 0, 0, Loc);
#endif
}


void sgPPCallbacks::InjectQObjectDefs(clang::SourceLocation Loc) 
{
    
    std::unique_ptr<llvm::MemoryBuffer> Buf = llvm::MemoryBuffer::getMemBuffer(Injected, "sgmetadefs-injected.inl");
    Loc = mPP.getSourceManager().getFileLoc(Loc);
    mPP.EnterSourceFile( CreateFileIDForMemBuffer(mPP, std::move(Buf), Loc), nullptr, Loc);
}

void sgPPCallbacks::FileChanged(clang::SourceLocation Loc, clang::PPCallbacks::FileChangeReason Reason, clang::SrcMgr::CharacteristicKind FileType, clang::FileID PrevFID) 
{

    clang::SourceManager &SM = mPP.getSourceManager();
	clang::FileID curFID = SM.getFileID(SM.getFileLoc(Loc));
    mIsInMainFile = (curFID == SM.getMainFileID());
	
    if (Reason != ExitFile)
        return;
	auto FMain = mPP.getSourceManager().getFileEntryForID(SM.getMainFileID());
	llvm::StringRef mainName = "";
	if(FMain)
		mainName = FMain->getName();

	auto FCurr = mPP.getSourceManager().getFileEntryForID(curFID);
	llvm::StringRef currName = "";
	if(FCurr) currName = FCurr->getName();

    auto F = mPP.getSourceManager().getFileEntryForID(PrevFID);
    if (!F)
        return;

    llvm::StringRef name = F->getName();
    if (name.endswith("sgClassMetaDef.h")) {
        InjectQObjectDefs(Loc);
    }
}

void sgPPCallbacks::MacroExpands(const clang::Token &MacroNameTok, const clang::MacroDefinition &MD, clang::SourceRange Range, const clang::MacroArgs *Args)
{
	if (!mIsInMainFile)
	{
		return;
	}

	clang::tok::TokenKind kid = MacroNameTok.getKind();
	if (kid != clang::tok::identifier)
	{
		return;
	}

	clang::IdentifierInfo *tokInfo = MacroNameTok.getIdentifierInfo();
	if (tokInfo && tokInfo->getName() == "metainclude" && Args && Args->getNumArguments() > 0)
	{
		const clang::Token *tok = Args->getUnexpArgument(0);
		if (!tok)
		{
			return;
		}

		clang::tok::TokenKind argKind = tok->getKind();
		if (argKind != clang::tok::string_literal)
		{
			return;
		}
		const char *str = tok->getLiteralData();
		if (!str)
		{
			return;
		}

		if (tok->getLength() < 3)
		{
			return;
		}

		char incpath[256];
		memset(incpath, 0, 256);
		strncpy(incpath, str, tok->getLength());
		
		incpath[tok->getLength() - 1] = '\0';

		mMetaIncludes.push_back(&(incpath[1]));

	}
}


