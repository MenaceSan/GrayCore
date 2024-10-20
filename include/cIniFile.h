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
    cArrayRef<cIniSectionEntry> _aSections;  /// store all my sections. not sorted, dupes allowed.

 public:
    static const IniChar_t k_SectionDefault[1];  /// "" = default section name for tags not in a section.

 public:
    virtual ~cIniFile() {}

    /// <summary>
    /// Was this read ?
    /// </summary>
    ITERATE_t get_SectionsQty() const noexcept {
        return _aSections.GetSize();
    }
    cRefPtr<cIniSectionEntry> EnumSection(ITERATE_t i = 0) const {
        return _aSections.GetAtCheck(i);
    }

    /// <summary>
    /// Read in all the sections in the file.
    /// @todo USE cIniSectionData::ReadSectionData() ??
    /// </summary>
    /// <param name="s"></param>
    /// <param name="isStripComments">strip comments and whitespace. else preserve them.</param>
    /// <returns></returns>
    HRESULT ReadIniStream(cStreamInput& s, bool isStripComments = false);

    /// <summary>
    /// Open and read a whole INI file.
    /// @note we need to read a file before writing it. (gets all the comments etc)
    /// </summary>
    /// <param name="pszFilePath"></param>
    /// <param name="isStripComments">strip comments from the file.</param>
    /// <returns></returns>
    HRESULT ReadIniFile(const FILECHAR_t* pszFilePath, bool isStripComments = false);

    /// <summary>
    /// Write the whole INI file. preserve line comments (if the didn't get stripped via isStripComments).
    /// </summary>
    /// <param name="pszFilePath"></param>
    /// <returns></returns>
    HRESULT WriteIniFile(const FILECHAR_t* pszFilePath) const;

    /// <summary>
    /// Enumerate the sections. IIniBaseEnumerator
    /// </summary>
    /// <param name="ePropIdx"></param>
    /// <param name="rsValue"></param>
    /// <param name="psPropTag">return the section type as [SECTION Value] if it applies.</param>
    /// <returns></returns>
    HRESULT PropGetEnum(PROPIDX_t ePropIdx, OUT cStringI& rsValue, OUT cStringI* psPropTag = nullptr) const override;

    /// <summary>
    /// Find section. Assume file has been read into memory already.
    /// </summary>
    /// <param name="pszSectionTitle"></param>
    /// <param name="bPrefix"></param>
    /// <returns></returns>
    cRefPtr<cIniSectionEntry> FindSection(const IniChar_t* pszSectionTitle = nullptr, bool bPrefix = false) const;

    /// <summary>
    /// Find a line in the [pszSectionTitle] with a key looking like pszKey=
    /// </summary>
    /// <param name="pszSectionTitle"></param>
    /// <param name="pszKey"></param>
    /// <returns></returns>
    const IniChar_t* FindKeyLinePtr(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey) const;

    /// <summary>
    /// Create a new section in the file.
    /// Don't care if the key exists or not. dupes are OK.
    /// </summary>
    /// <param name="pszSectionTitle">"SECTIONTYPE SECTIONNAME" (ASSUME already stripped []). "" = k_SectionDefault;</param>
    /// <param name="isStripComments"></param>
    /// <param name="iLine"></param>
    /// <returns></returns>
    virtual cRefPtr<cIniSectionEntry> AddSection(const IniChar_t* pszSectionTitle = nullptr, bool isStripComments = false, const cTextPos& pos = cTextPos::k_Invalid);

    /// <summary>
    /// Set
    /// </summary>
    /// <param name="pszSectionTitle">OK for nullptr</param>
    /// <param name="pszKey"></param>
    /// <param name="pszLine">nullptr = delete;</param>
    /// <returns></returns>
    HRESULT SetKeyLine(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszLine);

    HRESULT SetKeyArg(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszArg);
};
}  // namespace Gray

#endif  // _INC_cIniFile_H
