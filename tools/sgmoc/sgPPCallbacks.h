
#pragma once

#include <clang/Lex/Preprocessor.h>

class sgPPCallbacks : public clang::PPCallbacks 
{
private:
    clang::Preprocessor &mPP;
	bool mIsInMainFile;
public:
	std::vector<std::string> mMetaIncludes;
public:

    sgPPCallbacks(clang::Preprocessor &PP) 
		: mPP(PP)
		, mIsInMainFile(false)
	{}

	bool IsInMainFile() const{ return mIsInMainFile; }

    void InjectQObjectDefs(clang::SourceLocation Loc);

public:
    virtual void FileChanged(clang::SourceLocation Loc, FileChangeReason Reason, clang::SrcMgr::CharacteristicKind FileType,
                             clang::FileID PrevFID) override;

	virtual void MacroExpands(const clang::Token &MacroNameTok,
		const clang::MacroDefinition &MD, clang::SourceRange Range,
		const clang::MacroArgs *Args) override;


};

