//
//! @file CIniFile.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CIniFile.h"
#include "CLogMgr.h"
#include "CHeap.h"
#include "CString.h"
#include "CCodeProfiler.h"
#include "CAppState.h"
#include "CFileText.h"	// CFileText CFileTextReader

#ifdef USE_STDIO
namespace Gray
{
	CIniFile::CIniFile()
	{
	}

	CIniFile::~CIniFile()
	{
	}

	HRESULT CIniFile::ReadIniStream(CStreamInput& s, bool bStripComments)
	{
		//! Read in all the sections in the file.
		//! @arg bStripComments = strip comments and whitespace. else preserve them.
		//! @todo USE CIniSectionData::ReadSectionData() ??

 		CIniSectionEntryPtr pSection;
		int iLine = 0;
		for (;;)
		{
			IniChar_t szBuffer[CIniSection::k_LINE_LEN_MAX];
			// read string strips new lines.
			HRESULT hRes = s.ReadStringLine(szBuffer, STRMAX(szBuffer));
			if (FAILED(hRes) || hRes == 0)
			{
				return hRes; // 0 = normal end.
			}
			iLine++;
			// is [Section header]?
			if (CIniReader::IsSectionHeader(szBuffer))
			{
				if (pSection != nullptr)
				{
					pSection->AllocComplete(); // complete the previous section.
				}
				// strip []
				IniChar_t* pszBlockEnd = StrT::FindBlockEnd(STR_BLOCK_SQUARE, szBuffer + 1);
				if (pszBlockEnd == nullptr || pszBlockEnd[0] != ']')
				{
					break;	// error. bad line format.
				}
				*pszBlockEnd = '\0';
				pSection = AddSection(szBuffer + 1, bStripComments, iLine);
			}
			else
			{
				if (pSection == nullptr)
				{
					pSection = AddSection("", bStripComments);	// add root/null section at the top.
				}
				IniChar_t* pszLine = szBuffer;
				if (bStripComments)
				{
					pszLine = StrT::GetNonWhitespace(pszLine);	// skip starting whitespace.
					StrLen_t iLen = CIniReader::FindScriptLineEnd(pszLine);
					pszLine[iLen] = '\0';
				}
				pSection->AddLine(pszLine);
			}
		}
		return HRESULT_WIN32_C(ERROR_BAD_FORMAT);	// error. bad line format. terminate read.
	}

	HRESULT CIniFile::ReadIniFile(const FILECHAR_t* pszFilePath, bool bStripComments)
	{
		//! Open and read a whole INI file.
		//! @arg bStripComments = strip comments from the file.
		//! @note we need to read a file before writing it. (gets all the comments etc)

		CODEPROFILEFUNC();
		if (pszFilePath == nullptr)
		{
			return E_POINTER;
		}

		CFileText fileReader;
		HRESULT hRes = fileReader.OpenX(pszFilePath, OF_READ | OF_TEXT | OF_CACHE_SEQ);
		if (FAILED(hRes))
		{
			return hRes;
		}

		return ReadIniStream(fileReader, bStripComments);
	}

	HRESULT CIniFile::WriteIniFile(const FILECHAR_t* pszFilePath) const
	{
		//! Write the whole INI file. preserve line comments (if the didn't get stripped via bStripComments).
		CODEPROFILEFUNC();
		if (pszFilePath == nullptr)
		{
			return E_POINTER;
		}

		cFile file;
		HRESULT hRes = file.OpenX(pszFilePath, OF_CREATE | OF_WRITE); // OF_WRITE|OF_TEXT
		if (FAILED(hRes))
		{
			return hRes;
		}

		GRAY_FOREACH_S(CIniSectionEntry, pSection, m_aSections)
		{
			ASSERT(pSection != nullptr);
			hRes = pSection->WriteSection(file);
			if (FAILED(hRes))
				return hRes;
		}

		return S_OK;
	}

	HRESULT CIniFile::PropEnum(IPROPIDX_t ePropIdx, OUT CStringI& rsValue, CStringI* psPropTag) const // virtual
	{
		//! IIniBaseEnumerator
		//! Enumerate the sections.
		if (!m_aSections.IsValidIndex(ePropIdx))
			return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);

		const CIniSectionEntry* pSec = m_aSections.GetAt(ePropIdx);
		rsValue = CIniSection::GetSectionTitleParse(pSec->get_SectionTitle(), psPropTag);
		return (HRESULT)ePropIdx;
	}

	CIniSectionEntryPtr CIniFile::FindSection(const IniChar_t* pszSectionTitle, bool bPrefixOnly) const
	{
		//! Assume file has been read into memory already.
		CODEPROFILEFUNC();
		if (pszSectionTitle == nullptr)
		{
			pszSectionTitle = "";
		}
		StrLen_t iLen = StrT::Len(pszSectionTitle);
		if (iLen >= StrT::k_LEN_MAX_KEY)
		{
			return nullptr;
		}
		for (ITERATE_t i = 0; i < m_aSections.GetSize(); i++)
		{
			CIniSectionEntryPtr pSection(const_cast<CIniSectionEntry*>(m_aSections.GetAt(i)));
			const IniChar_t* pszLine = pSection->get_SectionTitle();
			if (StrT::CmpIN(pszLine, pszSectionTitle, iLen)) // NO match ?
				continue;
			if (bPrefixOnly)
				return pSection; // good enough match.
			// only a full match is OK.
			if (!StrChar::IsCSym(pszLine[iLen]))
				return pSection; // good enough match.
			// not a full match.
		}
		return nullptr;
	}

	CIniSectionEntryPtr CIniFile::AddSection(const IniChar_t* pszSectionTitle, bool bStripComments, int iLine) // virtual
	{
		//! Create a new section in the file.
		//! don't care if the key exists or not. dupes are OK.
		//! @arg pszSectionTitle = "SECTIONTYPE SECTIONNAME" (ASSUME already stripped [])
		//!  pszSectionTitle = "" = default;
		if (pszSectionTitle == nullptr)
		{
			pszSectionTitle = "";
		}
		CIniSectionEntryPtr pSection = new CIniSectionEntry(pszSectionTitle, bStripComments, iLine);
		m_aSections.Add(pSection);
		return pSection;
	}

	const IniChar_t* CIniFile::FindKeyLinePtr(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey) const
	{
		//! Find a line in the [pszSectionTitle] with a key looking like pszKey=

		CIniSectionEntryPtr pSection = FindSection(pszSectionTitle, false);
		if (pSection == nullptr)
		{
			return nullptr;
		}
		return pSection->FindKeyLinePtr(pszKey);
	}

	HRESULT CIniFile::SetKeyLine(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszLine)
	{
		//! @arg pszSectionTitle = OK for nullptr
		//! @arg pszLine = nullptr = delete;
		ITERATE_t iLine;
		CIniSectionEntryPtr pSection = FindSection(pszSectionTitle);
		if (pSection == nullptr)
		{
			// add a new section if it doesn't exist.
			pSection = AddSection(pszSectionTitle);
			iLine = k_ITERATE_BAD;
		}
		else
		{
			iLine = pSection->FindKeyLine(pszKey, false);
		}
		if (iLine < 0)
		{
			// Add new line
			if (pszLine != nullptr)
			{
				pSection->AddLine(pszLine);
			}
		}
		else
		{
			// Set/replace existing line.
			pSection->SetLine(iLine, pszLine);
		}
		return S_OK;
	}

	HRESULT CIniFile::SetKeyArg(const IniChar_t* pszSectionTitle, const IniChar_t* pszKey, const IniChar_t* pszArg)
	{
		//! OK for pszSectionTitle == nullptr
		if (pszKey == nullptr || pszArg == nullptr)
			return 0;
		IniChar_t szTmp[_MAX_PATH + _MAX_PATH];
		CIniSectionData::MakeLine(szTmp, STRMAX(szTmp), pszKey, pszArg);
		return SetKeyLine(pszSectionTitle, pszKey, szTmp);
	}
}

//******************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"
#include "CFilePath.h"
#include "CMime.h"

const FILECHAR_t* CIniFile::k_UnitTestFile = _FN(GRAY_NAMES) _FN("Core/test/CIniFileUnitTest") _FN(MIME_EXT_ini); // static

UNITTEST_CLASS(CIniFile)
{
	HRESULT UnitTest_Section(const CIniSection* pSection)
	{
		//! k_asTextLines
		//! dump contexts of a section.
		ASSERT_N(pSection != nullptr);
		ITERATE_t iLines = 0;
		for (iLines = 0; iLines < pSection->get_LineQty(); iLines++)
		{
			IniChar_t* pszLine = pSection->GetLineEnum(iLines);
			UNITTEST_TRUE(pszLine != nullptr);
			// DEBUG_MSG(( "%s", LOGSTR(pszLine) ));
		}
		return (HRESULT)iLines;
	}

	UNITTEST_METHOD(CIniFile)
	{
		//! read and write tests.
		//! open some INI file.

		CStringF sTestInpFile = CFilePath::CombineFilePathX(get_TestInpDir(), CIniFile::k_UnitTestFile);

		CIniFile file;
		HRESULT hRes = file.ReadIniFile(sTestInpFile);
		UNITTEST_TRUE(SUCCEEDED(hRes));

		// read headless section first. has no [SECTION] header.
		CIniSectionEntryPtr pSection = file.FindSection(nullptr);
		if (pSection != nullptr)
		{
			UnitTest_Section(pSection);
		}

		for (ITERATE_t i = 0; i < file.m_aSections.GetSize(); i++)
		{
			pSection = file.FindSection(file.m_aSections.GetAt(i)->get_SectionTitle(), false);
			UNITTEST_TRUE(pSection != nullptr);
			UnitTest_Section(pSection);
		}

		// make a change to the INI file.
		const IniChar_t* pszTestSection = "TestSection";
		const IniChar_t* pszTestKey = "TestKey";
		const IniChar_t* pszTestLine = "TestKey=9";
		// const IniChar_t* pszTestLine2 = "TestKey2:3";

		const IniChar_t* pszLine = file.FindKeyLinePtr(pszTestSection, pszTestKey);
		file.SetKeyLine(pszTestSection, pszTestKey, pszTestLine);

		// now write it back out.
		CStringF sIniWriteFile = CFilePath::CombineFilePathX(get_TestOutDir(), _FN(GRAY_NAMES) _FN("IniFileUnitTest") _FN(MIME_EXT_ini));
		hRes = file.WriteIniFile(sIniWriteFile);
		UNITTEST_TRUE(SUCCEEDED(hRes));

		// read it back again to make sure its correct ?
		CIniFile file2;
		hRes = file2.ReadIniFile(sIniWriteFile);
		UNITTEST_TRUE(SUCCEEDED(hRes));

		pszLine = file2.FindKeyLinePtr(pszTestSection, pszTestKey);
		UNITTEST_TRUE(pszLine != nullptr);
		UNITTEST_TRUE(!StrT::Cmp(pszLine, pszTestLine));

#if 0
		filewrite.WriteString(
			"//\n"
			"// CScriptTest.scp\n"
			"// This is a test script to read and see if it is read correctly.\n"
			"//\n"
			"\n");

		filewrite.WriteKeyStrQ("primaryfileprop", "123asd   ");

		filewrite.WriteSectionHead1("SECTION", "1");
		filewrite.WriteKeyStrQ("SECTION1Key", "SECTION1Keydata");

		filewrite.WriteSectionHead1("SECTION", "b2");
		filewrite.WriteKeyInt("SECTIONb2KeyValue", 123);
		filewrite.WriteKeyInt("SECTIONb2KeyValue123", 123);

		filewrite.WriteString("bbb=has a comment at the end of the line // sdf\n"); // comment at end of line.

		filewrite.WriteSectionHead1("SECTION", "c3isempty");
#endif

	}
};
UNITTEST_REGISTER(CIniFile, UNITTEST_LEVEL_Core);
#endif // USE_UNITTESTS

#endif
