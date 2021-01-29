//
//! @file cIniSection.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cIniSection_H
#define _INC_cIniSection_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cIniBase.h"
#include "StrArg.h"
#include "cHeap.h"
#include "cTextPos.h"

namespace Gray
{
#define INI_CR "\r\n"	//!< use "\n" or "\r\n" like FILE_EOL, STR_NL? M$ likes Windows format ("\r\n") to work with notepad.

	class cStreamOutput;

	class GRAYCORE_LINK cIniWriter
	{
		//! @class Gray::cIniWriter
		//! Helper for writing an INI file/stream.
		//! similar to IScriptableSetter

	protected:
		cStreamOutput* m_pOut;	//!< write out to this stream.
		bool m_bStartedSection;	//!< Must write a newline to close the previous section when we start a new one.

	public:
		cIniWriter(cStreamOutput* pOut)
			: m_pOut(pOut)
			, m_bStartedSection(false)
		{
			ASSERT(pOut != nullptr);
		}

		HRESULT WriteSectionHead0(const IniChar_t* pszSectionData);
		HRESULT WriteSectionHead1(const IniChar_t* pszSectionType, const IniChar_t* pszSectionName);
		HRESULT WriteSectionHead1Q(const IniChar_t* pszSection, const IniChar_t* pszArg);
		HRESULT _cdecl WriteSectionHeadFormat(const IniChar_t* pszSectionType, const IniChar_t* pszArgFormat, ...);

		HRESULT WriteKeyUnk(const IniChar_t* pszKey, const IniChar_t* pszData);
		HRESULT WriteKeyStrQ(const IniChar_t* pszKey, const IniChar_t* pszVal);
		HRESULT WriteKeyInt(const IniChar_t* pszKey, int nVal);
		HRESULT WriteKeyUInt(const IniChar_t* pszKey, UINT nVal);
	};

	struct GRAYCORE_LINK cIniReader	// static
	{
		//! @struct Gray::cIniReader
		//! Helper for reading/parsing an INI file/stream.

		static bool GRAYCALL IsSectionHeader(const IniChar_t* pszLine);

		static bool GRAYCALL IsLineComment(const IniChar_t* pszLine);
		static IniChar_t* GRAYCALL FindLineArg(const IniChar_t* pszLine, bool bAllowSpace = false);
		static StrLen_t GRAYCALL FindScriptLineEnd(const IniChar_t* pLineStr);
		static cStringI GRAYCALL GetLineParse2(const IniChar_t* pszLine, IniChar_t** ppszArgs = nullptr);
		static cStringI GRAYCALL GetLineParse3(const IniChar_t* pszLine, OUT cStringI& rsArgs);
	};

	class GRAYCORE_LINK cIniSectionData
		: public IIniBaseEnumerator
		, public IIniBaseGetter
		, public IIniBaseSetter
		, public cIniReader
	{
		//! @class Gray::cIniSectionData
		//! An array of lines (typically) in "Tag=Val" format or "Tag: Val". (comments and junk space is preserved)
		//! Mostly used as read only.
		//! Typically the data inside a [section] in an INI file.
		//! similar to _WIN32 GetPrivateProfileSection()
		//! @note the [section] name itself is NOT stored here. That is in cIniSection
		//! @note Allows ad hoc lines "hi this is a line" (with no = or formatting)
		//! Comments are decoded in cIniReader::FindScriptLineEnd()

		friend class cIniFile;
	public:
		static const StrLen_t k_SECTION_SIZE_MAX = (256 * 1024); //!< (chars) max size for whole section. (Windows Me/98/95 = 32K for INI)
		static const ITERATE_t k_LINE_QTY_MAX = (8 * 1024);	//!< max number of lines i support. (per section)
		static const StrLen_t k_LINE_LEN_MAX = (4 * 1024);	//!< max size for a single line (in chars).
		static const StrLen_t k_LINE_LEN_DEF = (1024);		//!< suggested/guessed/average size for lines (in chars). for alloc guessing.

	protected:
		bool m_bStripComments;	//!< has been stripped of blank lines, comments, leading and trailing line spaces.

	private:
		cHeapBlock m_Buffer;		//!< raw/processed data buffer for m_ppLines locally (new) allocated. (null term)
		StrLen_t m_iBufferUsed;		//!< how much of the buffer have we used ? including null.

		IniChar_t** m_ppLines;		//!< array of pointers to lines inside m_Buffer. (e.g. "Tag=Val" but not required to have mapped values.)
		ITERATE_t m_iLinesAlloc;	//!< m_ppLines Max alloc space
		ITERATE_t m_iLinesUsed;		//!< how many lines do we have? Not all lines are validly used.

	private:
		void MoveLineOffsets(ITERATE_t iLineStart, INT_PTR iDiffChars);

	protected:
		//! For raw access to the m_Buffer.
		IniChar_t* AllocBuffer(StrLen_t nSizeChars);
		void AllocLines(ITERATE_t iLinesAlloc);

		IniChar_t* AllocBeginMin(StrLen_t nSizeChars);

	public:
		cIniSectionData(bool bStripComments = false) noexcept;
		virtual ~cIniSectionData();
		void DisposeThis();

		bool isStripped() const noexcept
		{
			//! has been stripped of blank lines, comments, leading and trailing line spaces.
			return m_bStripComments;
		}

		StrLen_t get_BufferUsed() const noexcept
		{
			//! @return actual buffer size used.
			if (m_iLinesUsed <= 0)
				return 0;
			return m_iBufferUsed;
		}
		StrLen_t get_BufferSize() const noexcept
		{
			//! @return total buffer size allocated.
			if (m_iLinesUsed <= 0)
				return 0;
			return (StrLen_t)m_Buffer.get_DataSize();
		}

		ITERATE_t get_LineQty() const noexcept
		{
			//! @return index of the nullptr entry. at the end.
			return m_iLinesUsed;
		}
		IniChar_t* GetLineEnum(ITERATE_t iLine = 0) const noexcept
		{
			//! enum the lines in the section.
			//! @arg iLine = line in this section. 0 based.
			//! @return The line text. nullptr = Last.
			if (IS_INDEX_BAD(iLine, m_iLinesUsed))
				return nullptr;
			return m_ppLines[iLine];
		}

		static StrLen_t GRAYCALL IsLineTrigger(const IniChar_t* pszLine);
		ITERATE_t FindTriggerName(const IniChar_t* pszTrigName) const;

		virtual HRESULT PropEnum(IPROPIDX_t ePropIdx, OUT cStringI& rsValue, cStringI* psKey = nullptr) const override;
		void ClearLineQty();

		ITERATE_t FindKeyLine(const IniChar_t* pszKeyName, bool bPrefixOnly = false) const; //!< Find a key in the section (key=args)
		const IniChar_t* FindKeyLinePtr(const IniChar_t* pszKey) const;
		const IniChar_t* FindArgForKey(const IniChar_t* pszKey, const IniChar_t* pszDefault = nullptr) const;
		int FindIntForKey(const IniChar_t* pszKey, int iDefault = 0) const;

		virtual HRESULT PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const override;

		bool IsValidLines() const;
		void SetLinesCopy(const cIniSectionData& section);	// Dupe another section.
		ITERATE_t AddLine(const IniChar_t* pszLine);
		bool SetLine(ITERATE_t iLine, const IniChar_t* pszLine = nullptr);
		bool RemoveLine(ITERATE_t iLine)
		{
			return SetLine(iLine, nullptr);
		}

		static StrLen_t GRAYCALL MakeLine(IniChar_t* pszTmp, StrLen_t iSizeMax, const IniChar_t* pszKey, const IniChar_t* pszArg, IniChar_t chSep = '=');

		ITERATE_t AddKeyArg(const IniChar_t* pszKey, const IniChar_t* pszArg);
		ITERATE_t AddKeyInt(const IniChar_t* pszKey, int iArg)
		{
			return AddKeyArg(pszKey, StrArg<IniChar_t>(iArg));
		}
		ITERATE_t SetKeyArg(const IniChar_t* pszKey, const IniChar_t* pszArg);
		ITERATE_t SetKeyInt(const IniChar_t* pszKey, int iArg)
		{
			return SetKeyArg(pszKey, StrArg<IniChar_t>(iArg));
		}
		void AllocComplete();
		virtual HRESULT PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) override;

		StrLen_t SetLinesParse(const IniChar_t* pszData, StrLen_t iLen = k_StrLen_UNK, const IniChar_t* pszSep = nullptr, STRP_MASK_t uFlags = (STRP_START_WHITE | STRP_MERGE_CRNL | STRP_END_WHITE | STRP_EMPTY_STOP));
		cStringA GetStringAll(const IniChar_t* pszSep = nullptr) const;

		HRESULT ReadSectionData(cStringA& rsSectionNext, cStreamInput& stream, bool bStripComments);
		HRESULT WriteSectionData(cStreamOutput& file);
	};

	class GRAYCORE_LINK cIniSection : public cIniSectionData
	{
		//! @class Gray::cIniSection
		//! cIniSectionData + section title info
		typedef cIniSectionData SUPER_t;

	protected:
		cStringI m_sSectionTitle;	//!< "SECTIONTYPE SECTIONNAME" = everything that was inside [] without the []

	public:
		cIniSection(bool bStripComments = false) noexcept
			: cIniSectionData(bStripComments)
		{
		}
		cIniSection(cStringI sSectionTitle, bool bStripComments = false)
			: cIniSectionData(bStripComments)
			, m_sSectionTitle(sSectionTitle)
		{
		}
		cIniSection(const cIniSection& rSectionCopy);

		const cStringI& get_SectionTitle() const noexcept
		{
			//! @return everything that was inside [] without the []. Not parsed.
			return m_sSectionTitle;
		}
		cString get_Name() const noexcept
		{
			return cString(get_SectionTitle());
		}

		static cStringI GRAYCALL GetSectionTitleParse(cStringI sSectionTitle, cStringI* psPropTag);

		static bool GRAYCALL IsSectionTypeRoot(const IniChar_t* pszSection) noexcept
		{
			//! stuff at the top of the file with no [section] header.
			return StrT::IsNullOrEmpty(pszSection);
		}
		static bool GRAYCALL IsSectionTypeMatch(const IniChar_t* pszSection1, const IniChar_t* pszSection2) noexcept;

		HRESULT WriteSection(cStreamOutput& file);

		bool IsSectionType(const IniChar_t* pszSectionType) const noexcept
		{
			return IsSectionTypeMatch(m_sSectionTitle, pszSectionType);
		}
		UNITTEST_FRIEND(cIniSection);
	};

	class GRAYCORE_LINK cIniSectionEntry : public cRefBase, public cIniSection
	{
		//! @class Gray::cIniSectionEntry
		//! For storing an array of cIniSection(s).
		//! We might Discard body and reload it again later from the file.

	public:
		cTextPos m_FilePos;	//!< Where in parent/source file is this? for error reporting. 1 based. ITERATE_t

	public:
		cIniSectionEntry(cStringI sSectionTitle, bool bStripComments = false, int iLine = 0)
			: cIniSection(sSectionTitle, bStripComments)
			, m_FilePos(0, iLine)
		{
		}
		cIniSectionEntry(const cIniSectionEntry& rSectionCopy)
			: cIniSection(rSectionCopy)
			, m_FilePos(rSectionCopy.m_FilePos)
		{
		}
		virtual ~cIniSectionEntry()
		{
		}

		int get_HashCode() const noexcept
		{
			return (int)m_FilePos.get_Line1();
		}
	};
	typedef cRefPtr<cIniSectionEntry> cIniSectionEntryPtr;
};

#endif
