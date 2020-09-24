//
//! @file CFileText.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CFileText_H
#define _INC_CFileText_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CFile.h"
#include "CTextPos.h"

UNITTEST_PREDEF(CFileText)

namespace Gray
{
#if defined(UNDER_CE) || defined(__linux__)
	typedef HANDLE FILEDESC_t;		//!< Posix file descriptor id. _fileno is same as HANDLE in __linux__ and UNDER_CE
#else
	typedef int FILEDESC_t;		//!< Posix file descriptor id for std C. used for separate _fileno in FILE*
#endif

	class GRAYCORE_LINK CFileText : public cFile	// Try to be compatible with MFC CStdioFile
	{
		//! @class Gray::CFileText
		//! A text file with special processing for detecting and converting text "\r\n" chars
		//! like MFC CStdioFile. Compatible with C standard FILE,stdin,stdout,stderr.
		//! use OF_TEXT_A or OF_TEXT_W for format ??

		typedef cFile SUPER_t;

	protected:
		FILE* m_pStream;	//!< the current open script/text type file. named as MFC CStdioFile.
	private:
		ITERATE_t m_iCurLineNum;	//!< track the line number we are on currently. (0 based) (for CTextPos)

	private:
		const FILECHAR_t* get_ModeCPtr() const;
		HRESULT OpenCreate(CStringF sFilePath, OF_FLAGS_t nOpenFlags = OF_CREATE | OF_WRITE, _SECURITY_ATTRIBUTES* pSa = nullptr)
		{
			//! Don't use this.
			ASSERT(0);
			UNREFERENCED_PARAMETER(sFilePath);
			UNREFERENCED_PARAMETER(nOpenFlags);
			UNREFERENCED_PARAMETER(pSa);
			return E_NOTIMPL;
		}

	public:
		CFileText();
		CFileText(CStringF sFilePath, OF_FLAGS_t nOpenFlags);
		virtual ~CFileText();

		virtual bool isFileOpen() const override
		{
			return m_pStream != nullptr;
		}
		bool isEOF() const;
		HRESULT GetStreamError() const;

		// NOT OF_TEXT since \n processing is weird.
		virtual HRESULT OpenX(CStringF sFilePath = "", OF_FLAGS_t nOpenFlags = OF_READ | OF_SHARE_DENY_NONE) override;
		virtual void Close(void) override;
		HRESULT OpenFileHandle(HANDLE h, OF_FLAGS_t nOpenFlags);

		FILE* DetachFileStream()
		{
			FILE* pStream = m_pStream;
			DetachFileHandle();
			m_pStream = nullptr;
			return pStream;
		}

		virtual STREAM_SEEKRET_t Seek(STREAM_OFFSET_t offset = 0, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) override;
		virtual STREAM_POS_t GetPosition() const override;

		virtual HRESULT FlushX() override;
		virtual HRESULT ReadX(void* pBuffer, size_t nSizeMax) override;
		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override;

		virtual HRESULT WriteString(const char* pszStr) override;
		virtual HRESULT WriteString(const wchar_t* pszStr) override
		{
			return SUPER_t::WriteString(pszStr);
		}
		virtual HRESULT ReadStringLine(char* pBuffer, StrLen_t nSizeMax) override;
		virtual HRESULT ReadStringLine(wchar_t* pszBuffer, StrLen_t iSizeMax) override
		{
			return SUPER_t::ReadStringLine(pszBuffer, iSizeMax);
		}

		HRESULT ReadStringLineA(cStringA& r);
		// HRESULT ReadStringLineW(cStringW &r);

#ifndef _MFC_VER
		BOOL ReadString(CString& rString)
		{
			// Emulate MFC
			HRESULT hRes = ReadStringLineA(rString);
			return SUCCEEDED(hRes);
		}
#endif

		bool put_TextPos(const CTextPos& rPos);
		CTextPos get_TextPos() const
		{
			return CTextPos(GetPosition(), m_iCurLineNum, 0);
		}
		ITERATE_t get_CurrentLineNumber() const
		{
			return m_iCurLineNum;
		}

		FILE* get_FileStream() const
		{
			return m_pStream;
		}
		// operator FILE* () const { return m_pStream; }	// Dangerous.

		UNITTEST_FRIEND(CFileText);
	};
}

#ifndef _MFC_VER
typedef Gray::CFileText CStdioFile;		// emulate  _MFC_VER
#endif

#endif
