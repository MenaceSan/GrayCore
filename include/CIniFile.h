//
//! @file cIniFile.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cIniFile_H
#define _INC_cIniFile_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cIniSection.h"
#include "cArrayRef.h"
#include "HResult.h"
#include "FileName.h"
#include "cUnitTestDecl.h"

namespace Gray
{
	class GRAYCORE_LINK cIniFile
		: public IIniBaseEnumerator	// enumerate the sections.
	{
		//! @class Gray::cIniFile
		//! Very simple interface to read/write an ".INI" MIME_EXT_ini format file.
		//! Reads the whole file into memory so we may parse it further.
		//! Allows initial data keys without [SECTIONTYPE Sectionnamedata] (unlike windows)
	private:
		cArrayRef<cIniSectionEntry> m_aSections;	//!< store all my sections. not sorted, dupes allowed.

	public:
		cIniFile();
		virtual ~cIniFile();

		bool isRead() const
		{
			//! Was this read ?
			return m_aSections.GetSize() > 0;
		}

		HRESULT ReadIniStream(cStreamInput& s, bool bStripComments = false);
		HRESULT ReadIniFile(const FILECHAR_t* pszFilePath, bool bStripComments = false);
		HRESULT WriteIniFile(const FILECHAR_t* pszFilePath) const;

		cIniSectionEntryPtr EnumSection(ITERATE_t i = 0) const
		{
			return m_aSections.GetAtCheck(i);
		}
		virtual HRESULT PropEnum(IPROPIDX_t ePropIdx, OUT cStringI& rsValue, cStringI* psPropTag = nullptr) const override;
		cIniSectionEntryPtr FindSection(const IniChar_t* pszSectionTitle = nullptr, bool bPrefix = false) const;

		const IniChar_t* FindKeyLinePtr(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey) const;
		virtual cIniSectionEntryPtr AddSection(const IniChar_t* pszSectionTitle = nullptr, bool bStripComments = false, int iLine = 0);
		HRESULT SetKeyLine(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszLine);
		HRESULT SetKeyArg(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszArg);

		UNITTEST_FRIEND(cIniFile);
	};
};

#endif // _INC_cIniFile_H
