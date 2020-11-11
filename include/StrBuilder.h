//
//! @file StrBuilder.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_StrBuilder_H
#define _INC_StrBuilder_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrT.h"
#include "cMem.h"

namespace Gray
{
	class StrBuilder : public cMemBlock // GRAYCORE_LINK
	{
		//! @class Gray::StrBuilder
		//! Similar to .NET StringBuilder
		//! Fill a buffer with stuff. Track how much space is left.
	public:
		char* m_pCur;
		StrLen_t m_nLenLeft;

	protected:
		void Advance(StrLen_t nLen)
		{
			if (((UINT)nLen) >= (UINT)m_nLenLeft)
			{
				m_pCur = ((char*)get_End()) - 1;
				m_nLenLeft = 0;
			}
			else
			{
				m_pCur += nLen;
				m_nLenLeft -= nLen;
			}
			m_pCur[0] = '\0';	// always force terminate.
		}

	public:
		StrBuilder(void* p, StrLen_t nSize)
			: cMemBlock(p, nSize)
			, m_pCur((char*)p)
			, m_nLenLeft(nSize)
		{
			// nSize = sizeof(*p);
			ASSERT(m_nLenLeft > 0);
			ASSERT(m_pCur != nullptr);
			*m_pCur = '\0';
		}
		StrBuilder(cMemBlock& m)
			: cMemBlock(m)
			, m_pCur((char*)(m.get_Start()))
			, m_nLenLeft((StrLen_t)m.get_Size())
		{
			// nSize = sizeof(*p);
			ASSERT(m_nLenLeft > 0);
			ASSERT(m_pCur != nullptr);
			*m_pCur = '\0';
		}

		void ResetStr()
		{
			m_pCur = (char*)get_Start();
			m_nLenLeft = (StrLen_t)get_Size();
			*m_pCur = '\0';
		}
		StrLen_t get_Length() const
		{
			//! get Length used.
			return StrT::Diff(m_pCur, (const char*)get_Start());
		}
		const char* get_Str() const
		{
			// get_StrA();
			return (const char*)get_Start();
		}
		StrLen_t get_LenLeft() const
		{
			return m_nLenLeft;
		}
		bool isOverflow() const
		{
			//! Assume truncation occurred. DISP_E_BUFFERTOOSMALL
			return m_nLenLeft <= 0;
		}
		StrLen_t _cdecl AddFormat(const char* pszFormat, ...)
		{
			// m_nLenLeft includes space for terminator '\0'
			va_list vargs;
			va_start(vargs, pszFormat);
			StrLen_t nLenRet = StrT::vsprintfN(m_pCur, m_nLenLeft, pszFormat, vargs);
			va_end(vargs);
			Advance(nLenRet);
			return nLenRet;
		}
		StrLen_t AddStr(const char* pszStr)
		{
			// m_nLenLeft includes space for terminator '\0'
			if (pszStr == nullptr)	// just add nothing.
				return 0;
			StrLen_t nLenRet = StrT::CopyLen(m_pCur, pszStr, m_nLenLeft);
			Advance(nLenRet);
			return nLenRet;
		}
		void AddCRLF()
		{
			// AKA CRNL
			AddStr(STR_CRLF);
		}
		void AddChar(char ch)
		{
			// m_nLenLeft includes space for terminator '\0'
			if (m_nLenLeft < 1)
				return;
			*m_pCur = ch;
			Advance(1);
		}
		void AddCharRepeat(char ch, int iRepeat)
		{
			// m_nLenLeft includes space for terminator '\0'
			for (int i = 0; i < iRepeat; i++)
			{
				AddChar(ch);
			}
		}
		void AddBytesRaw(const void* p, size_t nSize)
		{
			StrLen_t nLenRet = MIN(m_nLenLeft, (StrLen_t)nSize);
			::memcpy(m_pCur, p, nSize);
			Advance(nLenRet);
		}
		void AddBytes(const void* p, size_t nSize)
		{
			// Just add a string from void*. Don't assume terminated string. filter for printable characters.
			StrLen_t nLenRet = MIN(m_nLenLeft, (StrLen_t)nSize);
			for (StrLen_t i = 0; i < nLenRet; i++)
			{
				BYTE ch = ((BYTE*)p)[i];
				if (ch < 32 || ch == 127 || (ch > 128 && ch < 160))	// strip junk chars.
					m_pCur[i] = '?';
				else
					m_pCur[i] = ch;
			}
			Advance(nLenRet);
		}
	};

	class StrBuilderAlloc : public StrBuilder // GRAYCORE_LINK
	{
		//! @class Gray::StrBuilderAlloc
		//! Grow string on demand.
	public:
		StrBuilderAlloc(size_t nSizeStart = 1024)
			: StrBuilder(nullptr, 0)
		{
			UNREFERENCED_PARAMETER(nSizeStart);
		}
	};
}

#endif


