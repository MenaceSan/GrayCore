//! @file cIniFile.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cIniFile_H
#define _INC_cIniFile_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "FileName.h"
#include "HResult.h"
#include "cArrayRef.h"
#include "cIniSection.h"

namespace Gray {
/// <summary>
/// Very simple interface to read/write an ".INI" MIME_EXT_ini format file.
/// Reads the whole file into memory so we may parse it further.
/// Allows initial data keys without [SECTIONTYPE Sectionnamedata] (unlike windows)
/// </summary>
class GRAYCORE_LINK cIniFile : public IIniBaseEnumerator { // enumerate the sections.
    cArrayRef<cIniSectionEntry> m_aSections;  /// store all my sections. not sorted, dupes allowed.

 public:
    static const IniChar_t k_SectionDefault[1];  /// "" = default section name for tags not in a section.

 public:
    cIniFile();
    virtual ~cIniFile();

    ITERATE_t get_SectionsQty() const noexcept {
        //! Was this read ?
        return m_aSections.GetSize();
    }
    cRefPtr<cIniSectionEntry> EnumSection(ITERATE_t i = 0) const {
        return m_aSections.GetAtCheck(i);
    }

    HRESULT ReadIniStream(cStreamInput& s, bool bStripComments = false);
    HRESULT ReadIniFile(const FILECHAR_t* pszFilePath, bool bStripComments = false);
    HRESULT WriteIniFile(const FILECHAR_t* pszFilePath) const;

    HRESULT PropGetEnum(PROPIDX_t ePropIdx, OUT cStringI& rsValue, OUT cStringI* psPropTag = nullptr) const override;
    cRefPtr<cIniSectionEntry> FindSection(const IniChar_t* pszSectionTitle = nullptr, bool bPrefix = false) const;

    const IniChar_t* FindKeyLinePtr(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey) const;
    virtual cRefPtr<cIniSectionEntry> AddSection(const IniChar_t* pszSectionTitle = nullptr, bool bStripComments = false, int iLine = 0);
    HRESULT SetKeyLine(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszLine);
    HRESULT SetKeyArg(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszArg);
};
}  // namespace Gray

#endif  // _INC_cIniFile_H
