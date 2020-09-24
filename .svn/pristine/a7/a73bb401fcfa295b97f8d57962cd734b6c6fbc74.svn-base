//
//! @file CIniSection.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CIniSection.h"
#include "CCodeProfiler.h"
#include "CHeap.h"
#include "CString.h"
#include "CLogMgr.h"
#include "StrChar.h"

namespace Gray
{
	HRESULT CIniWriter::WriteSectionHead0(const IniChar_t* pszSectionTitle)
	{
		//! Write Raw section header data.
		//! Write a section header with no arguments.
		//! pszSectionTitle = "SECTIONTYPE SECTIONNAME"

		ASSERT_N(pszSectionTitle != nullptr);
		ASSERT(pszSectionTitle[0] != '\0');

		HRESULT hRes;
		if (m_bStartedSection)
		{
			// New line to separate from last section.
			hRes = m_pOut->WriteString(INI_CR);
			if (FAILED(hRes))
			{
				return hRes; // HRESULT_WIN32_C(ERROR_WRITE_FAULT);
			}
			m_bStartedSection = false;
		}

		hRes = m_pOut->Printf("[%s]" INI_CR, StrArg<IniChar_t>(pszSectionTitle));
		if (FAILED(hRes))
		{
			return hRes; // HRESULT_WIN32_C(ERROR_WRITE_FAULT);
		}
		m_bStartedSection = true;
		return hRes;
	}

	HRESULT CIniWriter::WriteSectionHead1(const IniChar_t* pszSectionType, const IniChar_t* pszSectionName)
	{
		//! Write a section header with a raw (NOT quoted) argument.

		ASSERT_N(pszSectionType != nullptr);
		ASSERT(pszSectionType[0] != '\0');

		if (StrT::IsNullOrEmpty(pszSectionName))
		{
			return WriteSectionHead0(pszSectionType);
		}

		IniChar_t szTmp[_MAX_PATH];
		StrLen_t iLenTmp = StrT::sprintfN(szTmp, STRMAX(szTmp), "%s %s", StrArg<IniChar_t>(pszSectionType), StrArg<IniChar_t>(pszSectionName));
		UNREFERENCED_PARAMETER(iLenTmp);

		return WriteSectionHead0(szTmp);
	}

	HRESULT CIniWriter::WriteSectionHead1Q(const IniChar_t* pszSectionType, const IniChar_t* pszArg)
	{
		//! Write a section header with a quoted/escaped argument.
		if (StrT::IsNullOrEmpty(pszArg))
		{
			return WriteSectionHead0(pszSectionType);
		}

		IniChar_t szTmp[_MAX_PATH];
		StrLen_t iLenTmp = StrT::EscSeqAddQ(szTmp, pszArg, STRMAX(szTmp));
		UNREFERENCED_PARAMETER(iLenTmp);

		return WriteSectionHead1(pszSectionType, szTmp);
	}

	HRESULT _cdecl CIniWriter::WriteSectionHeadFormat(const IniChar_t* pszSectionType, const IniChar_t* pszFormat, ...)
	{
		//! Write out the section header.
		ASSERT_N(!StrT::IsNullOrEmpty(pszFormat));

		IniChar_t szTmp[_MAX_PATH];
		va_list vargs;
		va_start(vargs, pszFormat);
		StrLen_t iLenTmp = StrT::vsprintfN(szTmp, STRMAX(szTmp), pszFormat, vargs);
		UNREFERENCED_PARAMETER(iLenTmp);
		va_end(vargs);

		return(WriteSectionHead1(pszSectionType, szTmp));
	}

	HRESULT CIniWriter::WriteKeyUnk(const IniChar_t* pszKey, const IniChar_t* pszValue)
	{
		//! Write raw/naked string value to file.
		if (StrT::IsNullOrEmpty(pszKey))
		{
			return S_OK;
		}
		if (StrT::IsNullOrEmpty(pszValue))
		{
			// Some INI/script files have no keys. they are just raw lines.
			return m_pOut->Printf("%s" INI_CR, StrArg<IniChar_t>(pszKey));
		}
		else
		{
			return m_pOut->Printf("%s=%s" INI_CR, StrArg<IniChar_t>(pszKey), StrArg<IniChar_t>(pszValue));
		}
	}

	HRESULT CIniWriter::WriteKeyStrQ(const IniChar_t* pszKey, const IniChar_t* pszValue)
	{
		//! Write a quoted/escaped string.
		if (pszValue == nullptr)
		{
			// No real keys.
			return(WriteKeyUnk(pszKey, nullptr));
		}

		IniChar_t szValTmp[StrT::k_LEN_MAX];
		StrLen_t iLenTmp = StrT::EscSeqAddQ(szValTmp, pszValue, STRMAX(szValTmp));
		UNREFERENCED_PARAMETER(iLenTmp);

		return(WriteKeyUnk(pszKey, szValTmp));
	}
	HRESULT CIniWriter::WriteKeyInt(const IniChar_t* pszKey, int iVal)
	{
		IniChar_t szTmp[StrT::k_LEN_MAX_KEY];
		StrT::ItoA(iVal, szTmp, STRMAX(szTmp));
		return WriteKeyUnk(pszKey, szTmp);
	}
	HRESULT CIniWriter::WriteKeyUInt(const IniChar_t* pszKey, UINT dwVal)
	{
		IniChar_t szTmp[StrT::k_LEN_MAX_KEY];
		StrLen_t iLenTmp = StrT::UtoA((UINT)dwVal, szTmp, STRMAX(szTmp), 0x10);
		UNREFERENCED_PARAMETER(iLenTmp);
		return WriteKeyUnk(pszKey, szTmp);
	}

	//**************************************************************

	bool GRAYCALL CIniReader::IsSectionHeader(const IniChar_t* pszLine) // static
	{
		//! is this line a [section header] of some sort ?
		if (pszLine == nullptr)
			return false;
		if (pszLine[0] == '[')
			return true;
		return false;
	}

	bool GRAYCALL CIniReader::IsLineComment(const IniChar_t* pszLine) // static
	{
		//! Whole line is just a comment?
		//! DOS Bat files might use REM XXX (NOT supported here)
		//! :: Comment line

		if (pszLine == nullptr)
		{
			return true;
		}

		pszLine = StrT::GetNonWhitespace(pszLine);
		switch (*pszLine)
		{
		case '#':	// Ogre3d .CFG files.
		case ';':	// DOS config.sys (.BAT?)
		case '\0':
			return true;
		case '/':	// C++ format
			if (pszLine[1] == '/')
				return true;
			break;
		case ':':	// DOS .BAT format
			if (pszLine[1] == ':')
				return true;
			break;
		}
		return false;
	}

	IniChar_t* GRAYCALL CIniReader::FindLineArg(const IniChar_t* pszLine, bool bAllowSpace) // static
	{
		//! Find the argument/value portion of a line.
		//! "TAG=Args", "TAG: Args" (for HTTP) or "Tag Args" (if bAllowSpace)
		//! @note this does not strip comments from the end of the line !

		if (IsLineComment(pszLine))
		{
			return nullptr;
		}
		bool bFoundSpace = false;
		for (ITERATE_t i = 0;; i++)
		{
			IniChar_t ch = pszLine[i];
			if (ch == '\0')	// it seems to have no args.
			{
				break;
			}
			if (bAllowSpace && ch == ' ')
			{
				bFoundSpace = true;
				continue;
			}
			if (ch == '=' || ch == ':')
			{
				return const_cast<IniChar_t*>(pszLine + i + 1);
			}
			if (bFoundSpace)
			{
				return const_cast<IniChar_t*>(pszLine + i);
			}
		}
		return nullptr;
	}

	StrLen_t GRAYCALL CIniReader::FindScriptLineEnd(const IniChar_t* pszLine)	// static
	{
		//! Now parse the line for comments and trailing whitespace junk
		//! comments are #; as first char or // comment at end of line
		//! be careful not to strip http://xx so we require a space after // for comments.
		//! Similar to StrT::FindBlockEnd()
		//! @return
		//!  new length of the line. (without ending whitespace and comments)

		if (pszLine == nullptr)
			return 0;

		// Skip leading whitespace?
		StrLen_t iLenChars = 0; // StrT::GetNonWhitespaceI(pszLine);
		IniChar_t ch = pszLine[iLenChars];
		if (ch == '\0' || ch == ';' || ch == '#') // indicate comments. (only if first char)
		{
			return 0;
		}

		bool bInQuote = false;
		for (; iLenChars < StrT::k_LEN_MAX; iLenChars++)
		{
			ch = pszLine[iLenChars];
			if (ch == '\0')
				break;
			if (ch == '"')	// k_szBlockStart[STR_BLOCK_QUOTE]
			{
				bInQuote = !bInQuote;
				continue;
			}
			if (bInQuote)
			{
				if (ch == '\\' && pszLine[iLenChars + 1] > (IniChar_t)StrChar::k_Space)	// ignore this and the next char (ESCAPE char)
				{
					// ignore Octal escape. ! IsSpaceX() ?
					iLenChars++;
					continue;
				}
			}
			else
			{
				if (ch == '/' && pszLine[iLenChars + 1] == '/' && StrChar::IsSpace(pszLine[iLenChars + 2]))
				{
					// Remove comment at end of line.
					break;
				}
			}
		}

		if (bInQuote)
		{
			// end of line with open quote is bad !
		}

		// Remove space, CR and LF from the end of the line.
		return StrT::GetWhitespaceEnd(pszLine, iLenChars);
	}

	CStringI GRAYCALL CIniReader::GetLineParse2(const IniChar_t* pszLine, IniChar_t** ppszArgs) // static
	{
		//! Parse the pszLine into 2 parts. "TAG=Args" or "Tag Args"
		//! @note Does NOT clip // comment from end of line.
		//! @return Tag/Property/Key text.

		IniChar_t* pszArgs = FindLineArg(pszLine, true);
		if (ppszArgs != nullptr)
		{
			*ppszArgs = pszArgs;
		}
		if (pszArgs == nullptr)
		{
			if (IsLineComment(pszLine))
			{
				return "";
			}
			return CStringI(pszLine);
		}

		StrLen_t iLen = StrT::Diff(pszArgs, pszLine) - 1;
		iLen = StrT::GetWhitespaceEnd(pszLine, iLen);

		return CStringI(pszLine, iLen);
	}

	CStringI GRAYCALL CIniReader::GetLineParse3(const IniChar_t* pszLine, OUT CStringI& rsArgs) // static
	{
		//! Parse a line in the format of: "TAG=Value // comments."
		//! Clip comments off the end of the arg.
		//! @return Tag/Property/Key text. rsArgs = Value.

		IniChar_t* pszArgs = nullptr;
		CStringI sKey = GetLineParse2(pszLine, &pszArgs);
		StrLen_t nLenArg = FindScriptLineEnd(pszArgs);

		// Maybe empty if this is a comment line.
		rsArgs = CStringI(pszArgs, nLenArg);
		return sKey;
	}

	//**************************************************************

	CIniSectionData::CIniSectionData(bool bStripComments)
		: m_bStripComments(bStripComments)
		, m_iBufferUsed(0)
		, m_ppLines(nullptr)
		, m_iLinesAlloc(0)
		, m_iLinesUsed(0)
	{
		CODEPROFILEFUNC();
	}

	CIniSectionData::~CIniSectionData()
	{
		CODEPROFILEFUNC();
		CHeap::FreePtr(m_ppLines);
	}

	void CIniSectionData::DisposeThis()
	{
		//! Dispose any data. we might reload it when needed. this might just be a stub.
		m_bStripComments = false;
		m_Buffer.Free();
		m_iBufferUsed = 0;
		CHeap::FreePtr(m_ppLines);
		m_ppLines = nullptr;
		m_iLinesAlloc = 0;
		m_iLinesUsed = 0;	// ClearLineQty()
	}

	StrLen_t GRAYCALL CIniSectionData::IsLineTrigger(const IniChar_t* pszLine) // static
	{
		//! Is this line a script trigger/label in "@NAME" type format?
		//! Similar to other languages use of labels:
		//!  ":DOSLabel" for .bat files.
		//!  "CLabel:" for C++
		//! @return prefix length

		ASSERT(pszLine != nullptr);
		if (pszLine[0] != '@')
			return 0;
		if (StrChar::IsSpaceX(pszLine[1]))	// got it.
			return 0;
		return 1;	// Not a trigger/label.
	}

	ITERATE_t CIniSectionData::FindTriggerName(const IniChar_t* pszTrigName) const
	{
		//! Find Unique Named trigger/label in block.
		//! @arg pszTrigName = the script section we want.

		for (ITERATE_t iLine = 0; iLine < get_LineQty(); iLine++)
		{
			// Is it a trigger ?
			const IniChar_t* pszLine = GetLineEnum(iLine);
			StrLen_t iLenPrefix = IsLineTrigger(pszLine);
			if (iLenPrefix <= 0)
				continue;

			// Is it the right trigger ?
			if (!StrT::CmpI(pszLine + iLenPrefix, pszTrigName))
			{
				return(iLine);
			}
		}
		return(-1);	// cant find it.
	}

	HRESULT CIniSectionData::PropEnum(IPROPIDX_t ePropIdx, OUT CStringI& rsValue, CStringI* psKey) const // virtual
	{
		//! IIniBaseEnumerator
		//! @arg = optionally return psKey. nullptr = don't care.
		const IniChar_t* pszLine = GetLineEnum(ePropIdx);
		if (pszLine == nullptr)
			return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
		// Ignore comment lines ?
		CStringI sKey = CIniReader::GetLineParse3(pszLine, rsValue);
		if (psKey != nullptr)
			*psKey = sKey;
		return S_OK;
	}

	void CIniSectionData::ClearLineQty()
	{
		//! don't re-alloc. just clear what is used.
		CODEPROFILEFUNC();
		m_iLinesUsed = 0;
		m_iBufferUsed = 0;
		if (m_Buffer.IsValidIndex(0))
		{
			m_Buffer.get_DataChars()[0] = '\0';
		}
	}

	IniChar_t* CIniSectionData::AllocBuffer(StrLen_t nSizeChars)
	{
		// nSizeChars = in chars including space for null.
		CODEPROFILEFUNC();
		ASSERT(nSizeChars > m_iBufferUsed || !m_iBufferUsed);
		if (nSizeChars > k_SECTION_SIZE_MAX)
		{
			DEBUG_CHECK(0);
			return nullptr;
		}
		if (m_Buffer.get_Size() == (size_t)nSizeChars)
		{
			// No change.
			ASSERT(m_Buffer.isValid() || m_Buffer.get_Size() == 0);
			return m_Buffer.get_DataChars();
		}
		const IniChar_t* pszBufferPrev = m_Buffer.get_DataChars();
		m_Buffer.ReAlloc(nSizeChars * sizeof(IniChar_t));
		IniChar_t* pszBufferNew = m_Buffer.get_DataChars();
		if (!m_Buffer.isValid())
		{
			ASSERT(0 == nSizeChars);
		}
		else if (pszBufferPrev != nullptr && m_iLinesUsed > 0 && pszBufferPrev != pszBufferNew)
		{
			// Translate all the lines to the new buffer!
			ASSERT(pszBufferPrev != nullptr);
			MoveLineOffsets(0, pszBufferNew - pszBufferPrev);
		}
		return pszBufferNew;
	}

	bool CIniSectionData::IsValidLines() const
	{
		//! All lines MUST be in the m_Buffer.

		for (int i = 0; i < m_iLinesUsed; i++)
		{
			if (!m_Buffer.IsValidPtr(m_ppLines[i]))
				return false;
		}
		return true;
	}

	void CIniSectionData::AllocLines(ITERATE_t iLinesAlloc)
	{
		CODEPROFILEFUNC();
		ASSERT(iLinesAlloc > m_iLinesUsed);	// + nullptr
		if (m_iLinesAlloc == iLinesAlloc)
		{
			ASSERT(m_ppLines != nullptr || m_iLinesAlloc == 0);
			return;
		}
		m_ppLines = (IniChar_t**)CHeap::ReAllocPtr(m_ppLines, sizeof(IniChar_t*) * iLinesAlloc);
		ASSERT(m_ppLines != nullptr);
		m_iLinesAlloc = iLinesAlloc;
	}

	IniChar_t* CIniSectionData::AllocBeginMin(StrLen_t nSizeChars)
	{
		//! Alloc at least this estimated amount.
		//! Meant to be used with ProcessBuffer
		//! ASSUME AllocComplete() will be called later.
		CODEPROFILEFUNC();
		if (nSizeChars > (StrLen_t)m_Buffer.get_Size())
		{
			AllocBuffer(nSizeChars);
		}
		ITERATE_t iLineQty = MIN(k_LINE_QTY_MAX, (nSizeChars)+1);
		if (iLineQty > m_iLinesAlloc)
		{
			AllocLines(iLineQty);
		}
		return m_Buffer.get_DataChars();
	}

	void CIniSectionData::AllocComplete()
	{
		//! done loading, so trim to its used size.
		CODEPROFILEFUNC();
		AllocLines(m_iLinesUsed + 1);		// add null line at end.
		AllocBuffer(m_iBufferUsed + 1);	// include a second null char.
	}

	void CIniSectionData::MoveLineOffsets(ITERATE_t iLineStart, INT_PTR iDiffChars)
	{
		//! adjust all the m_ppLines offsets in the m_Buffer.
		//! INT_PTR iDiffBytes = can be move from one malloc to a separate malloc.
		CODEPROFILEFUNC();
		if (iDiffChars == 0)
			return;
		ASSERT_N(m_ppLines != nullptr);
		for (ITERATE_t i = iLineStart; i < m_iLinesUsed; i++)
		{
			m_ppLines[i] += iDiffChars;
		}
	}

	void CIniSectionData::SetLinesCopy(const CIniSectionData& section)
	{
		CODEPROFILEFUNC();
		m_iLinesUsed = section.get_LineQty();
		AllocLines(m_iLinesUsed + 1);
		AllocBuffer(section.get_BufferSize());
		m_iBufferUsed = section.get_BufferUsed();

		CMem::Copy(m_ppLines, section.m_ppLines, m_iLinesUsed * sizeof(IniChar_t*));
		CMem::Copy(m_Buffer.get_Data(), section.m_Buffer.get_Data(), m_Buffer.get_Size());

		MoveLineOffsets(0, StrT::Diff(m_Buffer.get_DataChars(), section.m_Buffer.get_DataChars()));
	}

	ITERATE_t CIniSectionData::FindKeyLine(const IniChar_t* pszKeyName, bool bPrefixOnly) const
	{
		//! Find the first instance of a key in the section (key=args)
		//! From the top of the section find a specific key.
		//! @note
		//!  There is no rule that keys should be unique!
		//! @arg
		//!  pszKeyName = nullptr = ""
		//!  bPrefixOnly = key is allowed to be just a prefix. else if must fully match. (default=false)
		//! @return
		//!  -1 = failed
		//!  the line number.

		CODEPROFILEFUNC();
		if (pszKeyName == nullptr)
		{
			pszKeyName = "";
		}

		StrLen_t iLen = StrT::Len(pszKeyName);
		if (iLen >= StrT::k_LEN_MAX_KEY)
		{
			DEBUG_ERR(("CIniSectionData FindIForKey Bad key name"));
			return k_ITERATE_BAD;
		}

		for (ITERATE_t i = 0; i < m_iLinesUsed; i++)
		{
			const IniChar_t* pszLine = GetLineEnum(i);
			pszLine = StrT::GetNonWhitespace(pszLine);
			if (StrT::CmpIN(pszLine, pszKeyName, iLen)) // NO match ?
				continue;
			if (bPrefixOnly)
				return i; // good enough match.
			// only a full match is OK.
			if (!StrChar::IsCSym(pszLine[iLen]))
				return i; // good enough match.
			// not a full match.
		}
		return k_ITERATE_BAD;
	}

	const IniChar_t* CIniSectionData::FindKeyLinePtr(const IniChar_t* pszKey) const
	{
		//! Find the line with this key in the section.
		//! @return Text for the line.
		ITERATE_t i = FindKeyLine(pszKey, false);
		if (i < 0)
		{
			return nullptr;
		}
		return GetLineEnum(i);
	}

	const IniChar_t* CIniSectionData::FindArgForKey(const IniChar_t* pszKey, const IniChar_t* pszDefault) const
	{
		//! Find pszKey in the section.
		//! @note this does not strip comments from the end of the line !
		//! @return the corresponding Args (e.g.Key=Args) for the first instance of pszKey.

		const IniChar_t* pszLine = FindKeyLinePtr(pszKey);
		if (pszLine == nullptr)
			return pszDefault;
		pszLine = CIniReader::FindLineArg(pszLine);
		if (pszLine == nullptr)
			return pszDefault;
		return pszLine;
	}

	int CIniSectionData::FindIntForKey(const IniChar_t* pszKey, int iDefault) const
	{
		//! Find pszKey in the section.
		//! @return the corresponding Args (e.g.Key=Args) for the first instance of pszKey.
		const IniChar_t* pszVal = FindArgForKey(pszKey, nullptr);
		if (pszVal == nullptr)
			return iDefault;
		return StrT::toI(pszVal);
	}

	HRESULT CIniSectionData::PropGet(const IniChar_t* pszPropTag, OUT CStringI& rsValue) const // virtual 
	{
		//! IIniBaseGetter
		const IniChar_t* pszVal = FindArgForKey(pszPropTag, nullptr);
		if (pszVal == nullptr)
		{
			rsValue = "";
			return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
		}
		rsValue = CStringI(pszVal, CIniReader::FindScriptLineEnd(pszVal));
		return S_OK;
	}

	ITERATE_t CIniSectionData::AddLine(const IniChar_t* pszLine)
	{
		//! add a text line to the end of this section.
		//! ASSUME m_bStripped has already been applied if used.
		//! Allow IsLineComment() and blank lines if NOT m_bStripped.
		//! strip newlines off.
		//! strip extra spaces off the end.
		//! allow leading spaces

		CODEPROFILEFUNC();

		// copy pszLine into m_Buffer space.
		StrLen_t iBufferAlloc = (StrLen_t)m_Buffer.get_Size();
		if (m_iBufferUsed + k_LINE_LEN_MAX > iBufferAlloc)
		{
			iBufferAlloc = m_iBufferUsed + (2 * k_LINE_LEN_MAX);
			AllocBuffer(iBufferAlloc);
		}

		IniChar_t* pszLineDest = m_Buffer.get_DataChars() + m_iBufferUsed;
		StrLen_t iLenMax = (iBufferAlloc - m_iBufferUsed);
		StrLen_t iLen = StrT::CopyLen(m_Buffer.get_DataChars() + m_iBufferUsed, pszLine, iLenMax);
		if (iLen >= iLenMax - 1)
		{
			// We chopped the line !!
			DEBUG_CHECK(0);
		}

		// strip \r\n stuff off the end.
		iLen = StrT::GetWhitespaceEnd(pszLineDest, iLen);

		// add the '/0' to the end of the line.
		pszLineDest[iLen++] = '\0';
		m_iBufferUsed += iLen;

		// add lines pointer
		if (m_iLinesUsed + 1 >= m_iLinesAlloc)
		{
			AllocLines(m_iLinesUsed + 256);
		}

		ITERATE_t iLine = m_iLinesUsed;
		m_ppLines[m_iLinesUsed++] = pszLineDest;
		m_ppLines[m_iLinesUsed] = nullptr;
		ASSERT(m_iLinesUsed < m_iLinesAlloc);
		return iLine;
	}

	bool CIniSectionData::SetLine(ITERATE_t iLine, const IniChar_t* pszLine)
	{
		//! set the contents of a specific line. (NOT insert a line)
		//! pszLine = nullptr = delete.

		if (IS_INDEX_BAD(iLine, m_iLinesUsed))
		{
			if (pszLine == nullptr)
				return true;
			iLine = AddLine(pszLine);
			if (iLine < 0)
				return false;
			return true;
		}

		IniChar_t* pszDst = m_ppLines[iLine];
		StrLen_t iLenOld = StrT::Len(pszDst);

		StrLen_t iLenNew;
		StrLen_t iLenDiff;
		if (pszLine == nullptr)
		{
			// delete line.
			iLenNew = 0;
			iLenDiff = -iLenOld;
		}
		else
		{
			// expand the buffer to make room for the new line data.
			iLenNew = StrT::Len(pszLine);
			iLenDiff = iLenNew - iLenOld;
		}

		StrLen_t iBufferStart = StrT::Diff(pszDst, m_Buffer.get_DataChars());
		iBufferStart += iLenOld;

		// Move above data to make room.
		StrLen_t iBufferNew = m_iBufferUsed + iLenDiff;
		if (iBufferNew + 1 >= (StrLen_t)m_Buffer.get_Size())
		{
			AllocBuffer(iBufferNew + k_LINE_LEN_MAX);
			pszDst = m_ppLines[iLine];
		}

		CMem::CopyOverlap(pszDst + iLenNew, pszDst + iLenOld, ((m_iBufferUsed - iBufferStart) + 1) * sizeof(IniChar_t));
		MoveLineOffsets(iLine + 1, iLenDiff);
		m_iBufferUsed = iBufferNew;

		if (iLenNew <= 0) // erasing line?
		{
			m_iLinesUsed--;
			CMem::CopyOverlap(m_ppLines + iLine, m_ppLines + iLine + 1, sizeof(m_ppLines[0]) * (m_iLinesUsed - iLine));
		}
		else
		{
			CMem::Copy(pszDst, pszLine, iLenNew * sizeof(IniChar_t));
		}

		return true;
	}

	StrLen_t GRAYCALL CIniSectionData::MakeLine(IniChar_t* pszTmp, StrLen_t iSizeMax, const IniChar_t* pszKey, const IniChar_t* pszArg, IniChar_t chSep) // static
	{
		//! Build a line in the form of KEY=ARG
		//! chSep = ':' or '='
		StrLen_t iLen = StrT::CopyLen(pszTmp, pszKey, iSizeMax);
		if (iLen < iSizeMax)
		{
			pszTmp[iLen++] = chSep;
		}
		iLen += StrT::CopyLen(pszTmp + iLen, pszArg, iSizeMax - iLen);
		return iLen;
	}

	ITERATE_t CIniSectionData::AddKeyArg(const IniChar_t* pszKey, const IniChar_t* pszArg)
	{
		//! Add the line, even if it is duplicated key.
		if (pszKey == nullptr || pszArg == nullptr)
			return 0;
		IniChar_t szTmp[_MAX_PATH + _MAX_PATH];
		MakeLine(szTmp, STRMAX(szTmp), pszKey, pszArg);
		return AddLine(szTmp);
	}

	ITERATE_t CIniSectionData::SetKeyArg(const IniChar_t* pszKey, const IniChar_t* pszArg)
	{
		//! Replace a line with an existing key. else just add to the end.
		//! @return the line we changed.

		IniChar_t szTmp[_MAX_PATH + _MAX_PATH];
		MakeLine(szTmp, STRMAX(szTmp), pszKey, pszArg);

		ITERATE_t iLine = FindKeyLine(pszKey, true);
		if (iLine >= 0)
		{
			SetLine(iLine, szTmp);
			return iLine;
		}
		else
		{
			return AddLine(szTmp);
		}
	}

	HRESULT CIniSectionData::PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) // override 
	{
		//! IIniBaseSetter
		//! @return E_INVALIDARG,  HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY)
		ITERATE_t iLine = SetKeyArg(pszPropTag, pszValue);
		return (HRESULT)iLine;
	}

	StrLen_t CIniSectionData::SetLinesParse(const IniChar_t* pszData, StrLen_t iLen, const IniChar_t* pszSep, STRP_MASK_t uFlags)
	{
		//! Set the section from a big data/text blob. Parse lines.
		//! @arg
		//!  pszData = raw data blob to be parsed into lines. e.g. "TAG=Val\nTAG2=VAl2\n"
		//!  iLen = max data string length to parse.
		//!  pszSep = the separator = nullptr = Assume standard syntax. newlines at ends of lines.
		//!  uFlags = STRP_START_WHITE|STRP_MERGE_CRNL|STRP_END_WHITE|STRP_EMPTY_STOP
		//! @return
		//!  size of pszData buffer actually used + 1 '\0'.

		if (iLen < 0)
		{
			iLen = StrT::Len(pszData);
		}

		ClearLineQty();	// no lines yet.
		IniChar_t* pDataNew = AllocBeginMin(iLen + 2);	// alloc 2 '\0's at the end.
		CMem::Copy(pDataNew, pszData, iLen);
		pDataNew[iLen] = '\0';
		pDataNew[iLen + 1] = '\0';

		if (pszSep == nullptr)
		{
			pszSep = STR_CRLF;
		}

		// NOTE: Leave empty lines.
		ASSERT(m_iLinesAlloc >= 1);	// ASSUME m_iLinesAlloc has been set big enough.
		m_iLinesUsed = StrT::ParseCmds(pDataNew, iLen + 1, m_ppLines, m_iLinesAlloc, pszSep, uFlags);

		if (uFlags & STRP_EMPTY_STOP) // did we really use it all ? How much did we use ?
		{
			if (m_iLinesUsed > 0)
			{
				IniChar_t* pszLineLast = m_ppLines[m_iLinesUsed - 1];
				iLen = StrT::Diff(pszLineLast, pDataNew) + StrT::Len(pszLineLast);
			}
			else
			{
				iLen = 0;
			}
		}
		m_iBufferUsed = iLen + 1;	// add 1 '\0'.

		AllocComplete();	// adds another '\0'

		return m_iBufferUsed;
	}

	cStringA CIniSectionData::GetStringAll(const IniChar_t* pszSep) const
	{
		//! Build a single string with all the section lines.
		if (pszSep == nullptr)
		{
			pszSep = " ";
		}
		cStringA sArgs;
		for (ITERATE_t i = 0; i < this->get_LineQty(); i++)
		{
			if (i > 0)
			{
				sArgs += pszSep;
			}
			sArgs += this->GetLineEnum(i);
		}
		return sArgs;
	}

	HRESULT CIniSectionData::ReadSectionData(cStringA& rsSectionNext, CStreamInput& stmIn, bool bStripComments)
	{
		//! Read the CIniSectionData from a CStreamInput. Up to EOF or [next section]
		//! Might be first section (with no [Section header] text in rsSectionNext)
		//! @arg
		//!  rsSectionNext = the name of this section. without [].
		//!  bStripComments = strip leading spaces and trailing spaces and comments.
		//! @return
		//!  S_OK = read a section. S_FALSE = empty section
		//!  rsSectionNext = the name of the next section. (if any)
		//! @note
		//!  Never strip whole blank lines because the line count will not match for error reporting !

		ASSERT(rsSectionNext.IsEmpty() || rsSectionNext[0] != '[');

		rsSectionNext.Empty();	// don't care about the name of the section here.
		m_bStripComments = bStripComments;
		ClearLineQty();

		ASSERT(k_SECTION_SIZE_MAX >= 4 * k_LINE_LEN_DEF);
		StrLen_t dwLenBufferMax = k_SECTION_SIZE_MAX;
		IniChar_t* pszBuffer = AllocBeginMin(dwLenBufferMax);
		StrLen_t i = 0;
		ITERATE_t iLines = 0;
		HRESULT hRes = S_FALSE;
		for (;;)
		{
			if (i >= dwLenBufferMax - k_LINE_LEN_DEF)
			{
				dwLenBufferMax += k_SECTION_SIZE_MAX;	// just make it bigger.
				pszBuffer = AllocBuffer(dwLenBufferMax);
				ASSERT(pszBuffer != nullptr);
			}
			IniChar_t* pszLine = pszBuffer + i;
			// NOTE: lines getting clipped because they are too short is very bad!!
			ASSERT((dwLenBufferMax - i) >= k_LINE_LEN_DEF);

			// Read a line.
			hRes = stmIn.ReadStringLine(pszLine, dwLenBufferMax - i);
			if (FAILED(hRes))
				break;
			if (hRes == 0) // end of the file. S_OK
			{
				// hit the end of the file. thats OK. done with section
				if (i <= 0)
				{
					hRes = S_FALSE;	// but its empty!
				}
				break;
			}
			iLines++;
			if (CIniReader::IsSectionHeader(pszLine))
			{
				pszLine = StrT::GetNonWhitespace(pszLine + 1);
				IniChar_t* pszBlockEnd = StrT::FindBlockEnd(STR_BLOCK_SQUARE, pszLine);
				if (pszBlockEnd != nullptr && pszBlockEnd[0] == ']')
				{
					rsSectionNext = cStringA(pszLine, StrT::Diff(pszBlockEnd, pszLine));	// Save next section header. m_sSectionNext
				}
				else
				{
					// Next section header is BAD ! HRESULT_WIN32_C(ERROR_BAD_FORMAT)
					rsSectionNext = "";	// bad !
				}
				hRes = S_OK;
				break;	// start of next section = end of this section.
			}

			StrLen_t iLen;
			if (bStripComments)	// strip it.
			{
				const char* pszLineStart = pszLine;
				pszLine = StrT::GetNonWhitespace(pszLine);	// TODO Actually CMem::CopyOverlap the leading spaces out!
				i += StrT::Diff(pszLine, pszLineStart);
				iLen = CIniReader::FindScriptLineEnd(pszLine);	// kill comments. try not to strip http://xx ?
				pszLine[iLen] = '\0';
				// leave blank lines to keep the line count consistent.
			}
			else
			{
				iLen = StrT::Len(pszLine);
				// No such thing as a blank line, \n is something.
			}

			// Add to the list of lines.
			AddLine(pszLine);
			i += iLen + 1;
		}

		AllocComplete();	// trim buffer to actual needed size.
		return hRes;
	}

	HRESULT CIniSectionData::WriteSectionData(CStreamOutput& file)
	{
		ASSERT(IsValidLines());

		for (ITERATE_t j = 0; j < this->get_LineQty(); j++)
		{
			const IniChar_t* pszLine = this->GetLineEnum(j);
			if (pszLine == nullptr)	// end.
				break;
			HRESULT hRes = file.WriteString(pszLine);
			if (FAILED(hRes))
				return hRes;
			file.WriteString(INI_CR);
		}
		return S_OK;
	}

	//******************************************************************

	CIniSection::CIniSection(const CIniSection& rSectionCopy)
		: CIniSectionData(rSectionCopy.isStripped())
		, m_sSectionTitle(rSectionCopy.get_SectionTitle())
	{
		//! copy constructor.
		SetLinesCopy(rSectionCopy);
	}

	bool GRAYCALL CIniSection::IsSectionTypeMatch(const IniChar_t* pszSection1, const IniChar_t* pszSection2) // static
	{
		bool bRoot1 = IsSectionTypeRoot(pszSection1);
		bool bRoot2 = IsSectionTypeRoot(pszSection2);
		if (bRoot1)
		{
			if (bRoot2)
				return true;
			return false;
		}
		if (bRoot2)
			return false;
		return(!StrT::CmpHeadI(pszSection1, pszSection2));
	}

	HRESULT CIniSection::WriteSection(CStreamOutput& file)
	{
		if (!CIniSection::IsSectionTypeRoot(this->m_sSectionTitle))
		{
			CIniWriter writer(&file);
			HRESULT hRes = writer.WriteSectionHead0(this->m_sSectionTitle);
			if (FAILED(hRes))
				return hRes;
		}
		return SUPER_t::WriteSectionData(file);
	}

	CStringI GRAYCALL CIniSection::GetSectionTitleParse(CStringI sSectionTitle, CStringI* psPropTag) // static
	{
		//! Parse [SectionTitle]. similar to GetLineParse2()
		if (psPropTag == nullptr)
		{
			return sSectionTitle;
		}

		const IniChar_t* pszSep = StrT::FindChar<IniChar_t>(sSectionTitle, ' ');
		if (pszSep == nullptr)
		{
			*psPropTag = sSectionTitle;
			return "";
		}

		*psPropTag = CStringI(sSectionTitle, StrT::Diff<IniChar_t>(pszSep, sSectionTitle));
		return CStringI(pszSep + 1);
	}
}

//**************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"

UNITTEST_CLASS(CIniSection)
{
	UNITTEST_METHOD(CIniSection)
	{
		// Test read a blob of data as a section.

		CIniSectionData section;

		static const IniChar_t k_SectionData[] =
			"DATA1\r\n" // 7
			"DATA2\r\n"	// 7
			"DATA3=3\r\n"	// 9
			"DATA4:4\r\n"	// 9
			"\r\n"
			"DATA6:5\r\n"	// ignored.
			"\r\n";

		StrLen_t iRead = section.SetLinesParse(k_SectionData, _countof(k_SectionData), nullptr, STRP_START_WHITE | STRP_MERGE_CRNL | STRP_END_WHITE | STRP_EMPTY_STOP);
		UNITTEST_TRUE(iRead == 33);
		UNITTEST_TRUE(section.get_LineQty() == 5);

		static const IniChar_t* k_tSent = "This is a sentence. And another. // comment";
		iRead = CIniReader::FindScriptLineEnd(k_tSent);	// k_tSent
		UNITTEST_TRUE(iRead == 32);

	}
};
UNITTEST_REGISTER(CIniSection, UNITTEST_LEVEL_Core);

#endif // USE_UNITTESTS
