//
//! @file cString.CPP
//! similar to the MFC CString
//! similar to the STL string and wstring, basic_string<char>
//! @note if a string is used by 2 threads then the usage count must be thread safe.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#include "pch.h"
#include "cString.h"
#include "cStream.h"
#include "StrU.h"
#include "StrChar.h"
#include "StrConst.h"
#include "HResult.h"
#include "cArchive.h"
#include "StrArg.h"

namespace Gray
{

#ifndef _MFC_VER

	//***************************************************************************
	// -cString, cStringT<>, cStringT

	template<>
	const char CStringT<char>::m_Nil = '\0';		// Use this instead of nullptr. AKA cStrConst::k_Empty ?
	template<>
	const wchar_t CStringT<wchar_t>::m_Nil = '\0';	// Use this instead of nullptr. AKA cStrConst::k_Empty ?

	template< typename _TYPE_CH>
	void CStringT<_TYPE_CH>::AllocBuffer(StrLen_t iNewLength)
	{
		//! Dynamic allocate a buffer to hold the string.
		//! @arg iNewLength = chars not including null

		ASSERT(isValidString());
		ASSERT(IS_INDEX_GOOD(iNewLength, StrT::k_LEN_MAX + 1)); // reasonable arbitrary limit.

		if (iNewLength <= 0)
		{
			Empty();
			return;
		}

		CStringData* pDataNew;
		const size_t iStringLengthBytes = (iNewLength + 1) * sizeof(_TYPE_CH);
		if (m_pchData == &m_Nil)
		{
			// Make a new string.
			pDataNew = new(iStringLengthBytes) CStringData;	// allocate extra space and call its constructor
			ASSERT(pDataNew != nullptr);
			pDataNew->IncRefCount();
		}
		else
		{
			auto pDataOld = GetData();
			StrLen_t nOldLen = pDataOld->get_CharCount();
			int iRefCounts = pDataOld->get_RefCount();
			if (iRefCounts == 1)
			{
				// just change the existing ref. or it may be the same size.
				if (nOldLen == iNewLength)	// no change.
					return;
				pDataNew = (CStringData*)cHeap::ReAllocPtr(pDataOld, sizeof(CStringData) + iStringLengthBytes);
				ASSERT_NN(pDataNew);
			}
			else
			{
				// Make a new string. Copy from old. So we can change size.
				// NOTE: we maybe duping our self. (to change length)
				ASSERT(iRefCounts > 1);
				pDataNew = new(iStringLengthBytes) CStringData;
				ASSERT_NN(pDataNew);
				pDataNew->IncRefCount();
				StrT::CopyLen(reinterpret_cast<_TYPE_CH*>(pDataNew->GetString()), m_pchData, MIN(iNewLength, nOldLen) + 1); // Copy from old
				pDataOld->DecRefCount();	// release ref to previous string.
			}
		}

		ASSERT(cHeap::GetSize(pDataNew) >= (sizeof(CStringData) + iStringLengthBytes));
		pDataNew->put_CharCount(iNewLength);

		m_pchData = reinterpret_cast<_TYPE_CH*>(pDataNew->GetString());
		m_pchData[iNewLength] = '\0';	// might just be trimming an existing string.
	}

	template< typename _TYPE_CH>
	void CStringT<_TYPE_CH>::CopyBeforeWrite()
	{
		//! We are about to modify/change this. so make sure we don't interfere with other refs.
		//! Dupe this string.
		//! This might not be thread safe ! if we start with 1 ref we may make another before we are done !
		if (IsEmpty())
			return;
		CStringData* pData = GetData();
		if (pData->get_RefCount() > 1)
		{
			// dupe if there are other viewers. .
			AllocBuffer(pData->get_CharCount());
		}
		else
		{
			// i own this and i can do as a please with it.
		}
		ASSERT(pData->get_RefCount() <= 1);
	}

	template< typename _TYPE_CH>
	void CStringT<_TYPE_CH>::ReleaseBuffer(StrLen_t nNewLength)
	{
		//! Call this after using GetBuffer() or GetBufferSetLength();
		//! Reset size to actual used size.
		//! nNewLength = count of chars, not including null char at the end.
		if (m_pchData == &m_Nil)
		{
			ASSERT(nNewLength == 0);
			return;
		}
		CStringData* pData = GetData();
		ASSERT(pData->get_RefCount() == 1);
		if (nNewLength <= k_StrLen_UNK)	// default to current length
		{
			nNewLength = StrT::Len(m_pchData);
		}
		if (nNewLength <= 0)
		{
			EmptyValid();	// make sure we all use m_Nil for empty. NOT nullptr
		}
		else
		{
			ASSERT(nNewLength <= pData->get_CharCount());
			pData->put_CharCount(nNewLength);	// just shorten length.
			m_pchData[nNewLength] = '\0';	// Can we assume this is already true ?
		}
		ASSERT(isValidString());
	}

	template< typename _TYPE_CH>
	_TYPE_CH* CStringT<_TYPE_CH>::GetBuffer(StrLen_t iMinLength)
	{
		//! like MFC GetBufferSetLength also
		//! iMinLength = not including null
		ASSERT(iMinLength >= 0);
		if (iMinLength > GetLength())
		{
			AllocBuffer(iMinLength); // get brand new string.
		}
		else
		{
			CopyBeforeWrite();	// assume it is going to be changed.
		}
#ifdef _DEBUG
		if (iMinLength > 0)		// m_Nil is special. Cant call GetData()
		{
			CStringData* pData = GetData();
			ASSERT(pData->get_CharCount() >= iMinLength);
		}
#endif
		return m_pchData;
	}

	template< typename _TYPE_CH>
	void CStringT<_TYPE_CH>::AssignLenT(const _TYPE_CH* pszStr, StrLen_t iLenMax)
	{
		//! Copy pszStr into this string.
		//! iLenMax = max chars, not including null ! NOT sizeof() or _countof() like StrT::CopyLen
		//! DEBUG_MSG(( "cString:Assign" ));
		//! @note What if pszStr is in the current string?
		//! @note DO NOT ASSUME pszStr is terminated string!! DONT CALL StrT::Len

		if (StrT::IsNullOrEmpty(pszStr) || iLenMax <= 0)
		{
			Empty();
			return;
		}
		StrLen_t iLenCur = GetLength();	// current stated length of the string.
		if (pszStr == m_pchData)    // Same string.
		{
			if (iLenMax >= iLenCur)
				return;	// do nothing!
			Assign(Left(iLenMax));
			return;
		}
		if (pszStr >= m_pchData && pszStr <= m_pchData + iLenCur)
		{
			// Part of the same string so be safe !!
			THIS_t sTmp(pszStr, iLenMax);	// make a copy first!
			Assign(sTmp);
			return;
		}

		ASSERT(IS_INDEX_GOOD(iLenMax, StrT::k_LEN_MAX + 1));
		StrLen_t iLenStr = StrT::Len(pszStr, iLenMax);
		if (iLenMax > iLenStr)
			iLenMax = iLenStr;

		AllocBuffer(iLenMax);
		ASSERT(m_pchData != nullptr);
		ASSERT(m_pchData != &m_Nil);
		StrT::CopyLen(m_pchData, pszStr, iLenMax + 1);
		ASSERT(isValidString());
	}

	//*************************************************************
	template<>
	void CStringT<char>::AssignLen(const wchar_t* pwStr, StrLen_t iLenMax)
	{
		//! Convert UNICODE to UTF8
		//! iLenMax = _countof(StrT::k_LEN_MAX)-1 default
		//! iLenMax = max chars, not including null ! NOT sizeof() or _countof() like StrT::CopyLen

		char szTmp[StrT::k_LEN_MAX];
		StrLen_t iLenNew = StrU::UNICODEtoUTF8(szTmp, STRMAX(szTmp), pwStr, iLenMax);
		AssignLenT(szTmp, iLenNew);
	}
	template<>
	void CStringT<char>::AssignLen(const char* pszStr, StrLen_t iLenMax)
	{
		AssignLenT(pszStr, iLenMax);
	}
	template<>
	void CStringT<char>::Assign(const wchar_t* pwStr)
	{
		//! Convert unicode to UTF8
		AssignLen(pwStr, StrT::k_LEN_MAX - 1);
	}
	template<>
	void CStringT<char>::Assign(const char* pszStr)
	{
		AssignLenT(pszStr, StrT::k_LEN_MAX - 1);
	}

	//*************************************************************

	template<>
	void CStringT<wchar_t>::AssignLen(const char* pszStr, StrLen_t iLenMax)
	{
		//! Convert UTF8 to unicode
		//! iLenMax = STRMAX = _countof(StrT::k_LEN_MAX)-1 default

		wchar_t wTmp[StrT::k_LEN_MAX];
		StrLen_t iLenNew = StrU::UTF8toUNICODE(wTmp, STRMAX(wTmp), pszStr, iLenMax);
		AssignLenT(wTmp, iLenNew);
	}
	template<>
	void CStringT<wchar_t>::AssignLen(const wchar_t* pwStr, StrLen_t iLenMax)
	{
		AssignLenT(pwStr, iLenMax);
	}
	template<>
	void CStringT<wchar_t>::Assign(const wchar_t* pwStr)
	{
		AssignLenT(pwStr, StrT::k_LEN_MAX - 1);
	}
	template<>
	void CStringT<wchar_t>::Assign(const char* pszStr)
	{
		//! Convert UTF8 to unicode
		AssignLen(pszStr, StrT::k_LEN_MAX - 1);
	}

	//*************************************************************

	template< typename _TYPE_CH>
	void CStringT<_TYPE_CH>::FormatV(const _TYPE_CH* pszFormat, va_list args)
	{
		//! _vsntprintf
		//! use the normal sprintf() style.
		_TYPE_CH szTemp[StrT::k_LEN_MAX];
		StrT::vsprintfN(OUT szTemp, STRMAX(szTemp), pszFormat, args);
		Assign(szTemp);
	}

	template< typename _TYPE_CH>
	StrLen_t CStringT<_TYPE_CH>::Insert(StrLen_t i, _TYPE_CH ch)
	{
		//! @return New length.
		if (i <= k_StrLen_UNK)
			i = 0;
		StrLen_t iLen = GetLength();
		if (i > iLen)
			i = iLen;
		AllocBuffer(iLen + 1);
		cMem::CopyOverlap(m_pchData + i + 1, m_pchData + i, iLen - i);
		m_pchData[i] = ch;
		return(GetLength());
	}

	template< typename _TYPE_CH>
	StrLen_t CStringT<_TYPE_CH>::Insert(StrLen_t i, const _TYPE_CH* pszStr, StrLen_t iLenCat)
	{
		//! insert in the middle.
		//! @return New length.
		ASSERT_NN(pszStr);
		if (iLenCat <= k_StrLen_UNK)
		{
			iLenCat = StrT::Len(pszStr);
		}
		if (iLenCat > 0)
		{
			if (i < 0)
				i = 0;
			StrLen_t iLen = GetLength();
			if (i > iLen)
				i = iLen;
			if (iLen + iLenCat > StrT::k_LEN_MAX)
			{
				DEBUG_ASSERT(0, "cString::Insert > StrT::k_LEN_MAX");
				return k_ITERATE_BAD;
			}
			AllocBuffer(iLen + iLenCat);
			cMem::CopyOverlap(m_pchData + i + iLenCat, m_pchData + i, (iLen - i) * sizeof(_TYPE_CH));
			cMem::Copy(m_pchData + i, pszStr, iLenCat * sizeof(_TYPE_CH));
		}
		return(GetLength());
	}

	template< typename _TYPE_CH>
	CStringT<_TYPE_CH> CStringT<_TYPE_CH>::Left(StrLen_t nCount) const
	{
		//! Get the left nCount chars. truncate.
		if (nCount >= GetLength())
			return *this;
		THIS_t sNew;
		if (nCount > 0)
		{
			_TYPE_CH* pData = sNew.GetBuffer(nCount);
			StrLen_t nCountNew = StrT::CopyLen(pData, GetString(), nCount + 1);
			sNew.ReleaseBuffer(nCountNew);
		}
		return sNew;
	}

	template< typename _TYPE_CH>
	CStringT<_TYPE_CH> CStringT<_TYPE_CH>::Right(StrLen_t nCount) const
	{
		//! Get the right nCount chars. skip leading chars.
		//! @return
		//!  a new string with the nCount right most chars in this string.

		if (nCount >= GetLength())
			return *this;
		THIS_t sNew;
		if (nCount > 0)
		{
			_TYPE_CH* pData = sNew.GetBuffer(nCount);
			StrLen_t nCountNew = StrT::CopyLen(pData, GetString() + (GetLength() - nCount), nCount + 1);
			sNew.ReleaseBuffer(nCountNew);
		}
		return sNew;
	}

	template< typename _TYPE_CH>
	CStringT<_TYPE_CH> CStringT<_TYPE_CH>::Mid(StrLen_t nFirst, StrLen_t nCount) const
	{
		//! Same as STL substr() function.
		if (nFirst >= GetLength())
			return "";
		return THIS_t(GetString() + nFirst, nCount);
	}

	template< typename _TYPE_CH>
	StrLen_t CStringT<_TYPE_CH>::Find(_TYPE_CH ch, StrLen_t nPosStart) const
	{
		StrLen_t iLen = GetLength();
		if (nPosStart > iLen)	// ch might be '\0' ?
			return k_ITERATE_BAD;
		StrLen_t nIndex = StrT::FindCharN(m_pchData + nPosStart, ch);
		if (nIndex < 0)
			return k_ITERATE_BAD;
		return nIndex + nPosStart;
	}

	template< typename _TYPE_CH>
	void CStringT<_TYPE_CH>::MakeUpper()
	{
		//! replaces _strupr
		//! No portable __linux__ equiv to _strupr()?
		//! Like MFC CString::MakeUpper(), Similar to .NET String.ToUpper(). BUT NOT THE SAME.
		CopyBeforeWrite();
		StrT::MakeUpperCase(m_pchData, GetLength());
		ASSERT(isValidString());
	}

	template< typename _TYPE_CH>
	void CStringT<_TYPE_CH>::MakeLower()
	{
		//! replaces strlwr()
		//! No portable __linux__ equiv to strlwr()?
		//! Like MFC CString::MakeLower(), Similar to .NET String.ToLower() BUT NOT THE SAME.
		CopyBeforeWrite();
		StrT::MakeLowerCase(m_pchData, GetLength());
		ASSERT(isValidString());
	}

	template< typename _TYPE_CH>
	void CStringT<_TYPE_CH>::TrimRight()
	{
		//! Like MFC CString::TrimRight(), Similar to .NET String.TrimEnd()
		StrLen_t iLenChars = GetLength();
		StrLen_t iLenNew = StrT::GetWhitespaceEnd(m_pchData, iLenChars);
		if (iLenNew == iLenChars)
			return;
		AssignLen(m_pchData, iLenNew);
	}

	template< typename _TYPE_CH>
	void CStringT<_TYPE_CH>::TrimLeft()
	{
		//! Like MFC CString::TrimLeft(), Similar to .NET String.TrimStart()
		_TYPE_CH* pszTrim = StrT::GetNonWhitespace(m_pchData);
		if (pszTrim == m_pchData)
			return;
		Assign(pszTrim);
	}

#endif // ! _MFC_VER

	//***********************************************************************************************

	template< typename _TYPE_CH>
	HRESULT cStringT<_TYPE_CH>::ReadZ(cStreamInput& stmIn, StrLen_t iLenMax)
	{
		//! Read in a new string from an open binary file. No length prefix.
		//! @arg
		//!  pFile = the open file.
		//!  iLenMax = The length of the string to read. NOT THE '\0' !
		//! @return
		//!  <= 0 = error or no valid string.
		//!  length of the string.

		_TYPE_CH* pBuffer = SUPER_t::GetBuffer(iLenMax); // ASSUME extra alloc for null is made.
		HRESULT hRes = stmIn.ReadX(pBuffer, iLenMax);
		SUPER_t::ReleaseBuffer(iLenMax);

		if (hRes != (HRESULT)iLenMax)
		{
			if (FAILED(hRes))
				return hRes;
			return HRESULT_WIN32_C(ERROR_READ_FAULT) ;
		}
		return iLenMax ;
	}

	template< typename _TYPE_CH>
	bool cStringT<_TYPE_CH>::WriteZ(cStreamOutput& File) const
	{
		//! Write a string AND '\0' out to the file. No length prefix.
		//! @arg
		//!  File = the open file.
		//! @note
		//!  Standard RIFF strings are '\0' terminated !

		File.WriteX(get_CPtr(), THIS_t::GetLength() + 1);
		return true;
	}

	template<>
	StrLen_t cStringT<char>::SetCodePage(const wchar_t* pwStr, CODEPAGE_t uCodePage)
	{
		//! Convert UNICODE to CODEPAGE_t ASCII type string.
		//! @arg uCodePage = CP_UTF8
		//! similar to StrU::UNICODEtoUTF8()
		char szTmp[StrT::k_LEN_MAX];
#ifdef _WIN32
		StrLen_t iStrLen = ::WideCharToMultiByte(uCodePage, 0, pwStr, -1, 0, 0, 0, 0); // get length first.
		if (iStrLen > STRMAX(szTmp))
			iStrLen = STRMAX(szTmp);
		::WideCharToMultiByte(uCodePage, 0, pwStr, -1, szTmp, iStrLen, 0, 0);
		Assign(szTmp);
#else
		// Convert to UTF8
		StrU::UNICODEtoUTF8(szTmp, STRMAX(szTmp), pwStr, k_StrLen_UNK);
		Assign(szTmp);
#endif
		return GetLength();
	}
	template<>
	StrLen_t cStringT<wchar_t>::SetCodePage(const wchar_t* pwStr, CODEPAGE_t uCodePage)
	{
		//! Just copy. No conversion since its already UNICODE.
		UNREFERENCED_PARAMETER(uCodePage);
		Assign(pwStr);
		return GetLength();
	}
	template<>
	StrLen_t cStringT<char>::GetCodePage(OUT wchar_t* pwText, StrLen_t iLenMax, CODEPAGE_t uCodePage) const
	{
		//! Convert char with CODEPAGE_t to UNICODE.
		//! @arg uCodePage = CP_UTF8
		//! similar to StrU::UTF8toUNICODE
#ifdef _WIN32
		return ::MultiByteToWideChar(
			uCodePage,      // code page CP_ACP
			0,				// character-type options
			get_CPtr(),		// address of string to map
			GetLength(),    // number of bytes in string
			pwText,			// address of wide-character buffer
			iLenMax			// size of buffer
		);
#else
		return StrU::UTF8toUNICODE(pwText, iLenMax, get_CPtr(), GetLength());	// true size is variable and < iLen
#endif
	}
	template<>
	StrLen_t cStringT<wchar_t>::GetCodePage(OUT wchar_t* pwText, StrLen_t iLenMax, CODEPAGE_t uCodePage) const
	{
		//! Just copy. No conversion if already UNICODE.
		UNREFERENCED_PARAMETER(uCodePage);
		return StrT::CopyLen(pwText, get_CPtr(), iLenMax);
	}

	template< typename _TYPE_CH>
	cStringT<_TYPE_CH> cStringT<_TYPE_CH>::GetTrimWhitespace() const
	{
		//! Trim whitespace from both ends.
		cStringT<_TYPE_CH> sTmp(*this);	// copy of this.
		sTmp.TrimRight();
		sTmp.TrimLeft();
		return sTmp;
	}

	template< typename _TYPE_CH>
	cStringT<_TYPE_CH> _cdecl cStringT<_TYPE_CH>::Join(const _TYPE_CH* psz1, ...) // static
	{
		cStringT<_TYPE_CH> sTmp;
		va_list vargs;
		va_start(vargs, psz1);
		for (int i = 0; i < k_ARG_ARRAY_MAX; i++)
		{
			if (StrT::IsNullOrEmpty(psz1))
				break;
			sTmp += psz1;
			psz1 = va_arg(vargs, const _TYPE_CH*); // next
		}
		va_end(vargs); 
		return sTmp;
	}

	template< typename _TYPE_CH>
	cStringT<_TYPE_CH> _cdecl cStringT<_TYPE_CH>::GetFormatf(const _TYPE_CH* pszFormat, ...) // static
	{
		//! Make a formatted string.
		cStringT<_TYPE_CH> sTmp;
		va_list vargs;
		va_start(vargs, pszFormat);
		sTmp.FormatV(pszFormat, vargs);
		va_end(vargs);
		return sTmp;
	}

	template< typename _TYPE_CH>
	cStringT<_TYPE_CH> cStringT<_TYPE_CH>::GetErrorStringV(HRESULT hResError, void* pSource, va_list vargs) // static
	{
		//! Describe a system error code as a string.
		//! hResError = HRESULT_WIN32_C(ERROR_INTERNAL_ERROR) etc.
		//! @note Must use HResult::FromWin32(x) instead of raw "::GetLastError()"

		if (hResError == S_OK)
			return StrT::Cast<_TYPE_CH>(CSTRCONST("OK"));

		if (FAILED(hResError))
		{
			// Put the system error message here.
			GChar_t szTmp[StrT::k_LEN_MAX];
			StrLen_t iLen = HResult::GetTextV(hResError, szTmp, STRMAX(szTmp), pSource, vargs);
			UNREFERENCED_PARAMETER(iLen);
			return szTmp;
		}
		return StrArg<_TYPE_CH>((UINT32)hResError, (RADIX_t)0x10); // its not an error just a number/code.
	}

	template< typename _TYPE_CH>
	cStringT<_TYPE_CH> cStringT<_TYPE_CH>::GetErrorString(HRESULT hResError, void* pSource) // static
	{
		//! hResError = HRESULT_WIN32_C(ERROR_INTERNAL_ERROR) etc.
		//! @note use HResult::FromWin32(x) instead of raw "::GetLastError()"
		return GetErrorStringV(hResError, pSource, nullptr);
	}

	template< typename _TYPE_CH>
	cStringT<_TYPE_CH> _cdecl cStringT<_TYPE_CH>::GetErrorStringf(HRESULT hResError, void* pSource, ...) // static
	{
		//! hResError = HRESULT_WIN32_C(ERROR_INTERNAL_ERROR) etc.
		//! @note Must use HResult::FromWin32(x) instead of raw "::GetLastError()"
		va_list vargs;
		va_start(vargs, pSource);
		cStringT<_TYPE_CH> sTmp = GetErrorStringV(hResError, pSource, vargs);
		va_end(vargs);
		return sTmp;
	}

	template< typename _TYPE_CH>
	cStringT<_TYPE_CH> cStringT<_TYPE_CH>::GetSizeK(UINT64 uVal, UINT nKUnit, bool bSpace) // static
	{
		_TYPE_CH szTmp[StrT::k_LEN_MAX_KEY];	// StrT::k_LEN_MAX_KEY
		StrLen_t nLen = StrT::ULtoAK(uVal, szTmp, STRMAX(szTmp), nKUnit, bSpace);
		return cStringT<_TYPE_CH>(szTmp, nLen);
	}

	template< typename _TYPE_CH>
	HRESULT cStringT<_TYPE_CH>::SerializeInput(cStreamInput& File, StrLen_t iLenMax)
	{
		//! Read in a new string from an open cStreamInput. CString
		//! Assume a size prefix. (in bytes not chars)
		//! @arg
		//!  File = the open file.
		//!  iLenMax = Truncate to MAX length of the string to read. NOT THE '\0' !
		//! @return
		//!  < 0 = error or no valid string.
		//!  0 = empty string.
		//!  length of the string.

		size_t nSizeRead;
		HRESULT hRes = File.ReadSize(nSizeRead);	// bytes
		if (FAILED(hRes))
		{
			return HResult::GetDef(hRes, HRESULT_WIN32_C(ERROR_IO_INCOMPLETE));
		}
		StrLen_t iLen = (StrLen_t)(nSizeRead / sizeof(_TYPE_CH));
		if (iLen > iLenMax || iLen >= StrT::k_LEN_MAX)
		{
			return HRESULT_WIN32_C(RPC_S_STRING_TOO_LONG);
		}

		_TYPE_CH* pBuffer = this->GetBuffer(iLen); // ASSUME extra alloc for null is made.

		hRes = File.ReadT(pBuffer, nSizeRead);	// all or nothing.
		if (FAILED(hRes))
		{
			SUPER_t::ReleaseBuffer(0);
			return(hRes);
		}
		this->ReleaseBuffer(StrT::Len(pBuffer, iLen));	// may be shorter. (pre-terminated)
		return iLen;
	}

	template< typename _TYPE_CH>
	HRESULT cStringT<_TYPE_CH>::SerializeOutput(cStreamOutput& File) const
	{
		//!  Write a string AND length (in bytes not chars) out to the file.
		//! @arg
		//!  File = the open file.
		//! @note
		//!  This is NOT '\0' term. though Standard RIFF strings are!

		return File.WriteN(get_CPtr(), SUPER_t::GetLength() * sizeof(_TYPE_CH));
	}

	template< typename _TYPE_CH>
	HRESULT cStringT<_TYPE_CH>::SerializeOutput(cArchive& a) const
	{
		return SerializeOutput(a.ref_Out());
	}

	template< typename _TYPE_CH>
	HRESULT cStringT<_TYPE_CH>::Serialize(cArchive& a)
	{
		//! Serialize in either direction.
		if (a.IsStoring())
			return SerializeOutput(a.ref_Out());
		else
			return SerializeInput(a.ref_Inp());
	}

	// force implementation/instantiate for DLL/SO.
#ifndef _MFC_VER
	template class GRAYCORE_LINK CStringT < char >;		// force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK CStringT < wchar_t >;	// force implementation/instantiate for DLL/SO.
#endif
	template class GRAYCORE_LINK cStringT < char >;
	template class GRAYCORE_LINK cStringT < wchar_t >;

}
