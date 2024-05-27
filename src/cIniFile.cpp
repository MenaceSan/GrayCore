//! @file cIniFile.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cAppState.h"
#include "cCodeProfiler.h"
#include "cFileText.h"  // cFileText or cFileTextReader
#include "cIniFile.h"
#include "cLogMgr.h"
#include "cString.h"

namespace Gray {
const IniChar_t cIniFile::k_SectionDefault[1] = "";  // static "" = default section name for tags not in a section.

HRESULT cIniFile::ReadIniStream(cStreamInput& s, bool bStripComments) {
    cRefPtr<cIniSectionEntry> pSection;
    int iLine = 0;
    for (;;) {
        IniChar_t szBuffer[cIniSection::k_LINE_LEN_MAX];
        // read string strips new lines.
        const HRESULT hRes = s.ReadStringLine(TOSPAN(szBuffer));
        if (FAILED(hRes) || hRes == 0) return hRes;  // 0 = normal end.
        iLine++;
        // is [Section header]?
        if (cIniReader::IsSectionHeader(szBuffer)) {
            if (pSection != nullptr) pSection->AllocComplete();  // complete the previous section.

            // strip []
            IniChar_t* pszBlockEnd = StrT::FindBlockEnd(STR_BLOCK_t::_SQUARE, szBuffer + 1);
            if (pszBlockEnd == nullptr || pszBlockEnd[0] != ']') break;  // error. bad line format.

            *pszBlockEnd = '\0';
            pSection = AddSection(szBuffer + 1, bStripComments, iLine);
        } else {
            if (pSection == nullptr) pSection = AddSection(k_SectionDefault, bStripComments);  // add root/null section at the top.

            IniChar_t* pszLine = szBuffer;
            if (bStripComments) {
                pszLine = StrT::GetNonWhitespace(pszLine);  // skip starting whitespace.
                StrLen_t iLen = cIniReader::FindScriptLineEnd(pszLine);
                pszLine[iLen] = '\0';
            }
            pSection->AddLine(pszLine);
        }
    }
    return HRESULT_WIN32_C(ERROR_BAD_FORMAT);  // error. bad line format. terminate read.
}

HRESULT cIniFile::ReadIniFile(const FILECHAR_t* pszFilePath, bool bStripComments) {
    CODEPROFILEFUNC();
    if (pszFilePath == nullptr) return E_POINTER;

    cFileTextReader fileReader;
    const HRESULT hRes = fileReader.OpenX(pszFilePath, OF_READ | OF_TEXT | OF_CACHE_SEQ);
    if (FAILED(hRes)) return hRes;

    return ReadIniStream(fileReader, bStripComments);
}

HRESULT cIniFile::WriteIniFile(const FILECHAR_t* pszFilePath) const {
    CODEPROFILEFUNC();
    if (pszFilePath == nullptr) return E_POINTER;

    cFile file;
    HRESULT hRes = file.OpenX(pszFilePath, OF_CREATE | OF_WRITE);  // OF_WRITE|OF_TEXT
    if (FAILED(hRes)) return hRes;
    for (auto section : m_aSections) {
        ASSERT(section.isValidPtr());
        hRes = section->WriteSection(file);
        if (FAILED(hRes)) return hRes;
    }

    return S_OK;
}

HRESULT cIniFile::PropGetEnum(PROPIDX_t ePropIdx, OUT cStringI& rsValue, OUT cStringI* psPropTag) const {  // virtual
    const cIniSectionEntry* pSec = EnumSection(ePropIdx);
    if (pSec == nullptr) return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
    rsValue = cIniSection::GetSectionTitleParse(pSec->get_SectionTitle(), psPropTag);
    return CastN(HRESULT, ePropIdx);
}

cRefPtr<cIniSectionEntry> cIniFile::FindSection(const IniChar_t* pszSectionTitle, bool bPrefixOnly) const {
    CODEPROFILEFUNC();
    if (pszSectionTitle == nullptr) pszSectionTitle = k_SectionDefault;  // global scope. Not in a section.
    const StrLen_t iLen = StrT::Len(pszSectionTitle);
    if (iLen >= k_LEN_MAX_CSYM) return nullptr;  // not a valid name ! // or truncate it?

    for (const cIniSectionEntry* pSection : m_aSections) {
        ASSERT_NN(pSection);
        const IniChar_t* pszLine = pSection->get_SectionTitle();
        if (StrT::CmpIN(pszLine, pszSectionTitle, iLen)) continue;  // NO match ?
        if (bPrefixOnly) return pSection;                           // good enough match. // else only a full match is OK.
        if (!StrChar::IsCSym(pszLine[iLen])) return pSection;       // good enough match. // not a full match.
    }
    return nullptr;
}

cRefPtr<cIniSectionEntry> cIniFile::AddSection(const IniChar_t* pszSectionTitle, bool bStripComments, int iLine) {  // virtual
    if (pszSectionTitle == nullptr) pszSectionTitle = k_SectionDefault;
    cRefPtr<cIniSectionEntry> pSection = new cIniSectionEntry(pszSectionTitle, bStripComments, iLine);
    m_aSections.Add(pSection);
    return pSection;
}

const IniChar_t* cIniFile::FindKeyLinePtr(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey) const {
    cRefPtr<cIniSectionEntry> pSection = FindSection(pszSectionTitle, false);
    if (pSection == nullptr) return nullptr;
    return pSection->FindKeyLinePtr(pszKey);
}

HRESULT cIniFile::SetKeyLine(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszLine) {
    ITERATE_t iLine;
    cRefPtr<cIniSectionEntry> pSection = FindSection(pszSectionTitle);
    if (pSection == nullptr) {
        // add a new section if it doesn't exist.
        pSection = AddSection(pszSectionTitle);
        iLine = k_ITERATE_BAD;
    } else {
        iLine = pSection->FindKeyLine(pszKey, false);
    }
    if (iLine < 0) {
        // Add new line
        if (pszLine != nullptr) {
            pSection->AddLine(pszLine);
        }
    } else {
        // Set/replace existing line.
        pSection->SetLine(iLine, pszLine);
    }
    return S_OK;
}

HRESULT cIniFile::SetKeyArg(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszArg) {
    //! OK for pszSectionTitle == nullptr
    if (pszKey == nullptr || pszArg == nullptr) return 0;
    IniChar_t szTmp[cFilePath::k_MaxLen + cFilePath::k_MaxLen];
    cIniSectionData::MakeLine(TOSPAN(szTmp), pszKey, pszArg);
    return SetKeyLine(pszSectionTitle, pszKey, szTmp);
}
}  // namespace Gray
