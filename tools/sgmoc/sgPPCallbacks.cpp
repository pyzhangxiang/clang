
#include "sgPPCallbacks.h"
#include "sgmetadefs-injected.h"

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

    auto F = mPP.getSourceManager().getFileEntryForID(PrevFID);
    if (!F)
        return;

    llvm::StringRef name = F->getName();
    if (name.endswith("sgMetaDef.h")) {
        InjectQObjectDefs(Loc);
    }
}
