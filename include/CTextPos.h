//
//! @file cTextPos.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cTextPos_H
#define _INC_cTextPos_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cStreamProgress.h"
#include "StrT.h"

namespace Gray
{
	class GRAYCORE_LINK cTextPos
	{
		//! @class Gray::cTextPos
		//! Where inside of a cFileText, cFileTextReader (or other type of text buffer) are we ?
		//! Text files have line numbers that won't always correspond to offset when newlines have been transformed.
		//! ASSUME: we know which file it is in.
		//! We have no idea how long the file is.

	public:
		static const cTextPos k_Invalid;	//!< Set to invalid values
		static const cTextPos k_Zero;		//!< Top of file.

	protected:
		STREAM_POS_t m_lOffset;		//!< byte offset into the file. 0 based
		ITERATE_t m_iLineNum;		//!< 0 based row/line, for debug purposes if there is an error.
		StrLen_t m_iColNum;			//!< 0 based column number. if used. # of characters, not bytes. UTF can have multi bytes per char.
		// m_iLineNum = -1 or -2 can be used to indicate errors.

	public:
		cTextPos(STREAM_POS_t lOffset = k_STREAM_POS_ERR, ITERATE_t iLineNum = k_ITERATE_BAD, StrLen_t iColNum = k_StrLen_UNK) noexcept
			: m_lOffset(lOffset)
			, m_iLineNum(iLineNum)
			, m_iColNum(iColNum)
		{
		}

		void InitTop() noexcept
		{
			m_lOffset = 0;
			m_iLineNum = 0;
			m_iColNum = 0;
		}
		bool isTopLine() const noexcept
		{
			//! is it on the top line? k_Zero
			return(m_lOffset == 0 && m_iLineNum == 0);
		}

		bool isValidPos() const noexcept
		{
			//! is invalid values? Not k_Invalid
			return(m_iLineNum >= 0); // m_lOffset >= 0
		}
		STREAM_POS_t get_Offset() const noexcept
		{
			//! Offset in bytes into the stream.
			return m_lOffset;
		}
		ITERATE_t get_LineNum() const noexcept		//!< Get 0 based line.
		{
			return this->m_iLineNum;
		}
		ITERATE_t get_Line1() const	noexcept 	//!< Get 1 based line.
		{
			return this->m_iLineNum + 1;
		}
		StrLen_t get_Column1() const noexcept	//!< Get 1 based column.
		{
			return this->m_iColNum + 1;
		}

		void IncOffset(StrLen_t nLenOffsetSrc) noexcept
		{
			m_lOffset += nLenOffsetSrc;
			m_iColNum += nLenOffsetSrc;
		}
		void IncOffset(StrLen_t nLenOffsetSrc, StrLen_t nLenCol) noexcept
		{
			// nLenCol = 0 = invisible chars don't count.
			m_lOffset += nLenOffsetSrc;
			m_iColNum += nLenCol;
		}
		void IncChar(StrLen_t nLenChar = 1) noexcept
		{
			// Add one single char that is not a newline or tab.
			m_lOffset += nLenChar;	// UTF8 can span multiple bytes.
			m_iColNum++;
		}
		void IncLine(StrLen_t nLenChar = 1) noexcept
		{
			// CRLF or LF
			m_lOffset += nLenChar;	// UTF8 can span multiple bytes.
			++m_iLineNum;
			m_iColNum = 0;
		}

		StrLen_t GetStr2(OUT char* pszOut, StrLen_t nLenOut) const;
	};

	class GRAYCORE_LINK cTextReader : public cTextPos
	{
		//! @class Gray::cTextReader
		//! current File/cXmlReader/cJSONReader/Etc parsing position. include cTextPos
		//! similar to cStreamInput but for a memory buffer.
		//! cTextPos = Current cursor position for m_pszCursor in the file. used for error messages, etc.
	protected:
		const char* m_pszStart;	//!< starting read position in the data parsing stream/buffer. cTextPos cursor = m_pszStart + m_lOffset.
		StrLen_t m_nLenMax;		//!< don't advance cTextPos::m_lOffset past this.

	public:
		const StrLen_t m_iTabSize;		//!< for proper tracking of the column number on errors. and m_CursorPos. 0 = not used/don't care.

	public:
		cTextReader(const char* pszStart, StrLen_t nLenMax = StrT::k_LEN_MAX, StrLen_t nTabSize = cStrConst::k_TabSize) noexcept
			: cTextPos(0, 0, 0)
			, m_pszStart(pszStart)
			, m_nLenMax(nLenMax)
			, m_iTabSize(nTabSize)
		{
		}

		StrLen_t get_LenMax() const noexcept
		{
			return m_nLenMax;
		}
		StrLen_t get_LenRemaining() const noexcept
		{
			if (m_nLenMax < (StrLen_t)m_lOffset)
				return 0;
			return m_nLenMax - (StrLen_t)m_lOffset;
		}
		bool isValidIndex() const noexcept
		{
			return ((UINT)m_lOffset) < (UINT)m_nLenMax; // includes m_lOffset >= 0
		}
		bool isValidPos() const noexcept
		{
			//! is invalid values? Not k_Invalid
			return m_pszStart != nullptr && isValidIndex(); // includes m_lOffset >= 0
		}
		const char* get_CursorPtr() const noexcept
		{
			ASSERT(isValidPos());
			return m_pszStart + this->m_lOffset;
		}
		char get_CursorChar() const noexcept
		{
			ASSERT(m_pszStart != nullptr);
			if (!isValidIndex())
				return '\0';
			return m_pszStart[this->m_lOffset];
		}

		void IncToks(StrLen_t nLen = 1)
		{
			// Advance the cursor x.
			// Skip over some known token. It is not a new line. It has no tabs. It is not past the end of the data.
#ifdef _DEBUG
			ASSERT(isValidPos());
			char ch = get_CursorChar();
			ASSERT(!StrChar::IsSpaceX(ch) && ch != '\0');
#endif
			IncOffset(nLen); // eat chars
		}
		void IncTab(StrLen_t nLenChar = 1)
		{
			// Skip to next tab stop
			m_lOffset += nLenChar; // eat tab
			if (m_iTabSize <= 0)
				m_iColNum++;
			else
				m_iColNum = (m_iColNum / m_iTabSize + 1) * m_iTabSize;
		}
		StrLen_t IncLineCR(StrLen_t nLenChar = 1)
		{
			// Check for \r\n sequence, and treat this as a single character
			IncLine(nLenChar);	// bump down to the next line
			StrLen_t nLen = 1;
			if (get_CursorChar() == '\n')
			{
				m_lOffset += nLenChar; // eat combo char
				nLen++;
			}
			return nLen;
		}

		bool isEOF() const noexcept
		{
			return get_CursorChar() == '\0';
		}

		void SetStartPtr(const char* pszStart, StrLen_t nLenMax = StrT::k_LEN_MAX)
		{
			m_pszStart = pszStart;
			m_nLenMax = nLenMax;
			InitTop();
		}
	};
}

#endif
