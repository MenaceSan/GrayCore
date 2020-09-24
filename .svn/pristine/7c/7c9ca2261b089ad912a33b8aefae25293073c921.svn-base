//
//! @file CIniFile.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CIniFile_H
#define _INC_CIniFile_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CIniSection.h"
#include "CArraySmart.h"
#include "HResult.h"
#include "FileName.h"

UNITTEST_PREDEF(CIniFile);

namespace Gray
{
	class GRAYCORE_LINK CIniFile
		: public IIniBaseEnumerator	// enumerate the sections.
	{
		//! @class Gray::CIniFile
		//! Reads the whole file into memory so we may parse it further.
		//! Very simple interface to read/write an ".INI" MIME_EXT_ini format file.
		//! Allows initial data keys without [SECTIONTYPE Sectionnamedata] (unlike windows)
	private:
		CArraySmart<CIniSectionEntry> m_aSections;	//!< store all my sections. not sorted, dupes allowed.

	public:
		CIniFile();
		virtual ~CIniFile();

		bool isRead() const
		{
			//! Was this read ?
			return m_aSections.GetSize() > 0;
		}

		HRESULT ReadIniStream(CStreamInput& s, bool bStripComments = false);
		HRESULT ReadIniFile(const FILECHAR_t* pszFilePath, bool bStripComments = false);
		HRESULT WriteIniFile(const FILECHAR_t* pszFilePath) const;

		CIniSectionEntryPtr EnumSection(ITERATE_t i = 0) const
		{
			return m_aSections.GetAtCheck(i);
		}
		virtual HRESULT PropEnum(IPROPIDX_t ePropIdx, OUT CStringI& rsValue, CStringI* psPropTag = nullptr) const override;
		CIniSectionEntryPtr FindSection(const IniChar_t* pszSectionTitle = nullptr, bool bPrefix = false) const;

		const IniChar_t* FindKeyLinePtr(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey) const;
		virtual CIniSectionEntryPtr AddSection(const IniChar_t* pszSectionTitle = nullptr, bool bStripped = false, int iLine = 0);
		HRESULT SetKeyLine(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszLine);
		HRESULT SetKeyArg(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszArg);

#ifdef USE_UNITTESTS
		static const FILECHAR_t* k_UnitTestFile;
		UNITTEST_FRIEND(CIniFile);
#endif
	};
};
#endif // _INC_CIniFile_H
