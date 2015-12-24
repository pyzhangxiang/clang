
#pragma once

#include <clang/Lex/Preprocessor.h>

class sgPPCallbacks : public clang::PPCallbacks 
{
private:
    clang::Preprocessor &mPP;
	bool mIsInMainFile;

public:

    sgPPCallbacks(clang::Preprocessor &PP) 
		: mPP(PP)
		, mIsInMainFile(false)
	{}

	bool IsInMainFile() const{ return mIsInMainFile; }

    void InjectQObjectDefs(clang::SourceLocation Loc);

protected:
    void FileChanged(clang::SourceLocation Loc, FileChangeReason Reason, clang::SrcMgr::CharacteristicKind FileType,
                             clang::FileID PrevFID) override;


};
