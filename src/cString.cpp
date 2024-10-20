//! @file cString.cpp
//! similar to the MFC CString
//! similar to the STL string and wstring, basic_string<char>
//! @note if a string is used by 2 threads then the usage count must be thread safe.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "HResult.h"
#include "StrArg.h"
#include "StrBuilder.h"
#include "StrChar.h"
#include "StrConst.h"
#include "StrU.h"
#include "cArchive.h"
#include "cHeap.h"
#include "cStream.h"
#include "cString.h"

namespace Gray {
template <>
const char cStringT<char>::k_Nil = '\0';  // Use this instead of nullptr. AKA cStrConst::k_Empty ?
template <>
const wchar_t cStringT<wchar_t>::k_Nil = '\0';  // Use this instead of nullptr. AKA cStrConst::k_Empty ?

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::AllocBuffer(StrLen_t iNewLength) {
    ASSERT(isValidString());
    ASSERT(IS_INDEX_GOOD(iNewLength, cStrConst::k_LEN_MAX + 1));  // reasonable arbitrary limit.

    if (iNewLength <= 0) {
        Empty();
        return;
    }

    HEAD_t* pHeadNew;
    if (_pchData == &k_Nil) {
        // Make a new string.
        pHeadNew = HEAD_t::CreateStringData(iNewLength);  // allocate extra space and call its constructor
        ASSERT_NN(pHeadNew);
        pHeadNew->IncRefCount();
    } else {
        auto pHeadOld = get_Head();
        StrLen_t nOldLen = pHeadOld->get_CharCount();
        const REFCOUNT_t iRefCount = pHeadOld->get_RefCount();
        if (iRefCount == 1) {
            // just change the existing ref. or it may be the same size.
            if (nOldLen == iNewLength) return;                                    // no change.
            pHeadNew = HEAD_t::Cvt(pHeadOld->ResizeHead(iNewLength + 1, false));  // change.
            ASSERT_NN(pHeadNew);
        } else {
            // Make a new string. Copy from old. So we can change size.
            // NOTE: we maybe duping our self. (to change length)
            ASSERT(iRefCount > 1);
            pHeadNew = HEAD_t::Cvt(HEAD_t::CreateStringData(iNewLength));
            ASSERT_NN(pHeadNew);
            pHeadNew->IncRefCount();
            StrT::CopyLen(pHeadNew->get_PtrWork(), _pchData, cValT::Min(iNewLength, nOldLen) + 1);  // Copy from old
            pHeadOld->DecRefCount();                                                                 // release ref to previous string.
        }
    }

    ASSERT(cHeap::GetSize(pHeadNew) >= (sizeof(HEAD_t) + ((iNewLength + 1) * sizeof(_TYPE_CH))));

    _pchData = pHeadNew->get_PtrWork();
    _pchData[iNewLength] = '\0';  // might just be trimming an existing string.
}

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::CloneBeforeWrite() {
    // This might not be thread safe ?! if we start with 1 ref we may make another before we are done!
    if (IsEmpty()) return;
    const HEAD_t* pHead = get_Head();
    if (pHead->get_RefCount() > 1) {
        // dupe if there are other viewers.
        AllocBuffer(pHead->get_CharCount());
    } else {
        // i solely own this and i can do as a please with it.
    }
    ASSERT(pHead->get_RefCount() <= 1);
}

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::ReleaseBuffer(StrLen_t nNewLength) {
    if (_pchData == &k_Nil) {
        ASSERT(nNewLength == 0);
        return;
    }
    HEAD_t* pHead = get_Head();
    ASSERT(pHead->get_RefCount() == 1);
    if (nNewLength <= k_StrLen_UNK) nNewLength = StrT::Len(_pchData);  // default to current length

    if (nNewLength <= 0) {
        SetEmptyValid();  // make sure we all use k_Nil for empty. NOT nullptr
        return;
    }
    if (nNewLength != pHead->get_CharCount()) {
        // Shrink allocation ? or Leave allocation size the same ??
        ASSERT(nNewLength <= pHead->get_CharCount());
        _pchData[nNewLength] = '\0';              // Can we assume this is already true ?
        pHead->ShrinkHead(nNewLength + 1, false);  // just shorten length.
    }
    ASSERT(isValidString());
}

template <typename _TYPE_CH>
_TYPE_CH* cStringT<_TYPE_CH>::GetBuffer(StrLen_t iMinLength) {
    ASSERT(iMinLength >= 0);
    if (iMinLength > GetLength()) {
        AllocBuffer(iMinLength);  // get brand new string.
    } else {
        CloneBeforeWrite();  // assume it is going to be changed.
    }
#ifdef _DEBUG
    if (iMinLength > 0) {  // k_Nil is special. Cant call GetData()
        HEAD_t* pHead = get_Head();
        ASSERT(pHead->get_CharCount() >= iMinLength);
    }
#endif
    return _pchData;
}

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::AssignSpanT(const cSpan<_TYPE_CH>& src) {
    if (src.isEmpty()) {
        Empty();
        return;
    }
    const StrLen_t iLenCur = GetLength();         // current stated length of the string.
    if (src.get_PtrConst() == _pchData) {        // Same string.
        if (src.get_MaxLen() >= iLenCur) return;  // do nothing!
        Assign(Left(src.get_MaxLen()));           // truncate length.
        return;
    }

    ASSERT(IS_INDEX_GOOD(src.get_MaxLen(), cStrConst::k_LEN_MAX + 1));
    if (src.isNull()) {
        // just allocate the space and leave it blank.
        AllocBuffer(src.get_MaxLen());
        ASSERT_NN(_pchData);
        ASSERT(_pchData != &k_Nil);
        _pchData[0] = '\0';
        return;
    }

    const _TYPE_CH* pszStr = src.get_PtrConst();
    if (pszStr >= _pchData && pszStr <= _pchData + iLenCur) {  // overlap?
        // Part of the same string so be safe !!
        THIS_t sTmp(src);  // make a copy first!
        Assign(sTmp);
        return;
    }

    const StrLen_t lenNew = src.get_MaxLen();
    AllocBuffer(lenNew);
    ASSERT_NN(_pchData);
    ASSERT(_pchData != &k_Nil);
    StrT::CopyLen(_pchData, pszStr, lenNew + 1);
    ASSERT(isValidString());
}

//*************************************************************
template <>
void cStringT<char>::AssignSpan(const cSpan<wchar_t>& src) {
    // Convert UNICODE to UTF8
    const StrLen_t iLenNew = StrU::UNICODEtoUTF8Size(src);
    AllocBuffer(iLenNew);
    StrU::UNICODEtoUTF8(cSpanX<char>(_pchData, iLenNew + 1), src);
}
template <>
void cStringT<char>::AssignSpan(const cSpan<char>& src) {
    AssignSpanT(src);
}
template <>
void cStringT<char>::Assign(const wchar_t* pwStr) {
    //! Convert UNICODE to UTF8
    AssignSpan(StrT::ToSpanStr(pwStr));
}
template <>
void cStringT<char>::Assign(const char* pszStr) {
    AssignSpanT(StrT::ToSpanStr(pszStr));
}

//*************************************************************

template <>
void cStringT<wchar_t>::AssignSpan(const cSpan<char>& src) {
    // Convert UTF8 to UNICODE
    const StrLen_t iLenNew = StrU::UTF8toUNICODELen(src);
    AllocBuffer(iLenNew);
    StrU::UTF8toUNICODE(cSpanX<wchar_t>(_pchData, iLenNew + 1), src);
}
template <>
void cStringT<wchar_t>::AssignSpan(const cSpan<wchar_t>& src) {
    AssignSpanT(src);
}
template <>
void cStringT<wchar_t>::Assign(const wchar_t* pwStr) {
    AssignSpanT(StrT::ToSpanStr(pwStr));
}
template <>
void cStringT<wchar_t>::Assign(const char* pszStr) {
    //! Convert UTF8 to UNICODE
    AssignSpan(StrT::ToSpanStr(pszStr));
}

//*************************************************************

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::FormatV(const _TYPE_CH* pszFormat, va_list args) {
    //! _vsntprintf
    //! use the normal sprintf() style.
    _TYPE_CH szTemp[StrT::k_LEN_Default];
    StrT::vsprintfN(TOSPAN(szTemp), pszFormat, args);
    Assign(szTemp);
}

template <typename _TYPE_CH>
StrLen_t cStringT<_TYPE_CH>::Insert(StrLen_t i, _TYPE_CH ch) {
    //! @return New length.
    if (i <= k_StrLen_UNK) i = 0;
    const StrLen_t lenOld = GetLength();
    if (i > lenOld) i = lenOld;
    AllocBuffer(lenOld + 1);
    cMem::CopyOverlap(_pchData + i + 1, _pchData + i, lenOld - i);
    _pchData[i] = ch;
    return GetLength();
}

template <typename _TYPE_CH>
StrLen_t cStringT<_TYPE_CH>::InsertSpan(StrLen_t i, const cSpan<_TYPE_CH>& src) {
    const StrLen_t lenCat = src.get_MaxLen();
    if (!src.isNull() && lenCat) {
        if (i < 0) i = 0;
        const StrLen_t lenOld = GetLength();
        if (i > lenOld) i = lenOld;
        if (lenOld + lenCat > cStrConst::k_LEN_MAX) {
            DEBUG_ASSERT(0, "cString::Insert > cStrConst::k_LEN_MAX");
            return k_ITERATE_BAD;
        }
        AllocBuffer(lenOld + lenCat);
        cMem::CopyOverlap(_pchData + i + lenCat, _pchData + i, (lenOld - i) * sizeof(_TYPE_CH));
        cMem::Copy(_pchData + i, src, lenCat * sizeof(_TYPE_CH));
    }
    return GetLength();
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> cStringT<_TYPE_CH>::Left(StrLen_t nCount) const {
    //! Get the left nCount chars. truncate. Same as substr(0,nCount);
    const StrLen_t lenOld = GetLength();
    if (nCount >= lenOld) return *this;
    if (nCount <= 0) return THIS_t();
    return ToSpan(get_CPtr(), nCount);
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> cStringT<_TYPE_CH>::Right(StrLen_t nCount) const {
    //! Get the right nCount chars. skip leading chars.
    //! @return
    //!  a new string with the nCount right most chars in this string.
    const StrLen_t lenOld = GetLength();
    if (nCount >= lenOld) return *this;
    if (nCount <= 0) return THIS_t();
    return ToSpan(get_CPtr() + (lenOld - nCount), nCount);
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> cStringT<_TYPE_CH>::Mid(StrLen_t nFirst, StrLen_t nCount) const {
    //! Same as STL substr() function.
    const StrLen_t lenOld = GetLength();
    if (nFirst >= lenOld) return THIS_t();
    if (nFirst + nCount >= lenOld) nCount = lenOld - nFirst;
    return ToSpan(get_CPtr() + nFirst, nCount);
}

template <typename _TYPE_CH>
StrLen_t cStringT<_TYPE_CH>::Find(_TYPE_CH ch, StrLen_t nPosStart) const {
    const StrLen_t iLen = GetLength();
    if (nPosStart > iLen) return k_ITERATE_BAD;  // ch might be '\0' ?
    const StrLen_t nIndex = StrT::FindCharN(_pchData + nPosStart, ch);
    if (nIndex < 0) return k_ITERATE_BAD;
    return nIndex + nPosStart;
}

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::MakeUpper() {
    //! replaces _strupr
    //! No portable __linux__ equiv to _strupr()?
    //! Like MFC CString::MakeUpper(), Similar to .NET String.ToUpper(). BUT NOT THE SAME.
    CloneBeforeWrite();
    StrT::MakeUpperCase(_pchData, GetLength());
    ASSERT(isValidString());
}

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::MakeLower() {
    //! replaces strlwr()
    //! No portable __linux__ equiv to strlwr()?
    //! Like MFC CString::MakeLower(), Similar to .NET String.ToLower() BUT NOT THE SAME.
    CloneBeforeWrite();
    StrT::MakeLowerCase(_pchData, GetLength());
    ASSERT(isValidString());
}

//***********************************************************************************************

template <typename _TYPE_CH>
HRESULT cStringT<_TYPE_CH>::ReadZ(cStreamInput& stmIn, StrLen_t iLenMax) {
    _TYPE_CH* pBuffer = this->GetBuffer(iLenMax);  // ASSUME extra alloc for null is made.
    const HRESULT hRes = stmIn.ReadX(cMemSpan(pBuffer, iLenMax));
    this->ReleaseBuffer(iLenMax);  // or less ?

    if (hRes != CastN(HRESULT, iLenMax)) {
        if (FAILED(hRes)) return hRes;
        return HRESULT_WIN32_C(ERROR_READ_FAULT);
    }
    return iLenMax;
}

template <typename _TYPE_CH>
bool cStringT<_TYPE_CH>::WriteZ(cStreamOutput& file) const {
    const HRESULT hRes = file.WriteSpan(get_SpanZ());
    return SUCCEEDED(hRes);
}

template <>
StrLen_t cStringT<char>::SetCodePage(const wchar_t* pwStr, CODEPAGE_t uCodePage) {
    //! Convert UNICODE to CODEPAGE_t ASCII type string.
    //! @arg uCodePage = CP_UTF8
    //! similar to StrU::UNICODEtoUTF8()
    char szTmp[StrT::k_LEN_Default];
#ifdef _WIN32
    StrLen_t iStrLen = ::WideCharToMultiByte(uCodePage, 0, pwStr, -1, 0, 0, 0, 0);  // get length first.
    if (iStrLen > STRMAX(szTmp)) iStrLen = STRMAX(szTmp);
    ::WideCharToMultiByte(uCodePage, 0, pwStr, -1, szTmp, iStrLen, 0, 0);
    Assign(szTmp);
#else
    // Convert to UTF8
    StrU::UNICODEtoUTF8(TOSPAN(szTmp), StrT::ToSpanStr(pwStr));
    Assign(szTmp);
#endif
    return GetLength();
}
template <>
StrLen_t cStringT<wchar_t>::SetCodePage(const wchar_t* pwStr, CODEPAGE_t uCodePage) {
    //! Just copy. No conversion since its already UNICODE.
    UNREFERENCED_PARAMETER(uCodePage);
    Assign(pwStr);
    return GetLength();
}
template <>
StrLen_t cStringT<char>::GetCodePage(cSpanX<wchar_t> ret, CODEPAGE_t uCodePage) const {
    //! Convert char with CODEPAGE_t to UNICODE.
    //! @arg uCodePage = CP_UTF8
    //! similar to StrU::UTF8toUNICODE
#ifdef _WIN32
    return ::MultiByteToWideChar(uCodePage,          // code page CP_ACP
                                 0,                  // character-type options
                                 get_CPtr(),         // address of string to map
                                 GetLength(),        // number of bytes in string
                                 ret.get_PtrWork(),  // address of wide-character buffer
                                 ret.GetSize()       // size of buffer
    );
#else
    return StrU::UTF8toUNICODE(ret, get_SpanStr());  // true size is variable and < iLen
#endif
}
template <>
StrLen_t cStringT<wchar_t>::GetCodePage(cSpanX<wchar_t> ret, CODEPAGE_t uCodePage) const {
    //! Just copy. No conversion if already UNICODE.
    UNREFERENCED_PARAMETER(uCodePage);
    return StrT::CopyPtr(ret, get_CPtr());
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> cStringT<_TYPE_CH>::GetTrimWhitespace() const {
    // Trim whitespace from both ends.
    const StrLen_t lenOld = GetLength();
    const StrLen_t left = StrT::GetNonWhitespaceN(_pchData, lenOld);
    const StrLen_t right = StrT::GetWhitespaceEnd(ToSpan(_pchData, lenOld));
    if (left == 0 && right == lenOld) return *this;  // no whitespace.
    return ToSpan(_pchData + left, right - left);
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> _cdecl cStringT<_TYPE_CH>::Join(const _TYPE_CH* psz1, ...) {  // static
    THIS_t sTmp;
    va_list vargs;
    va_start(vargs, psz1);
    for (int i = 0; i < k_ARG_ARRAY_MAX; i++) {
        if (StrT::IsNullOrEmpty(psz1)) break;
        sTmp += psz1;
        psz1 = va_arg(vargs, const _TYPE_CH*);  // next
    }
    va_end(vargs);
    return sTmp;
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> _cdecl cStringT<_TYPE_CH>::GetFormatf(const _TYPE_CH* pszFormat, ...) {  // static
    //! Make a formatted string.
    THIS_t sTmp;
    va_list vargs;
    va_start(vargs, pszFormat);
    sTmp.FormatV(pszFormat, vargs);
    va_end(vargs);
    return sTmp;
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> GRAYCALL cStringT<_TYPE_CH>::GetErrorStr(HRESULT hResError, const void* pSource) {  // static
    if (FAILED(hResError)) {
        GChar_t szTmp[StrT::k_LEN_Default];
        StrBuilder<GChar_t> sb(TOSPAN(szTmp));
        HResult::GetTextV(hResError, sb, pSource, k_va_list_empty);  // TODO va_list
        return sb.get_SpanStr();
    }
    if (hResError == S_OK) return CSTRCONST("OK").get_T<_TYPE_CH>();
    return StrArg<_TYPE_CH>((UINT32)hResError, (RADIX_t)0x10);  // its not an error just a hex number/code.
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> GRAYCALL cStringT<_TYPE_CH>::GetSizeK(UINT64 uVal, UINT nKUnit, bool bSpace) {  // static
    _TYPE_CH szTmp[StrNum::k_LEN_MAX_DIGITS];
    const StrLen_t nLen = StrT::ULtoAK(uVal, TOSPAN(szTmp), nKUnit, bSpace);
    return ToSpan(szTmp, nLen);
}

template <typename _TYPE_CH>
HRESULT cStringT<_TYPE_CH>::SerializeInput(cStreamInput& File, StrLen_t iLenMax) {
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
    HRESULT hRes = File.ReadSize(nSizeRead);  // bytes
    if (FAILED(hRes)) return HResult::GetDef(hRes, HRESULT_WIN32_C(ERROR_IO_INCOMPLETE));

    const StrLen_t iLen = (StrLen_t)(nSizeRead / sizeof(_TYPE_CH));
    if (iLen > iLenMax || iLen >= cStrConst::k_LEN_MAX) {
        return HRESULT_WIN32_C(RPC_S_STRING_TOO_LONG);
    }

    _TYPE_CH* pBuffer = this->GetBuffer(iLen);  // ASSUME extra alloc for null is made.

    hRes = File.ReadSpan(cMemSpan(pBuffer, nSizeRead));  // read all or nothing.
    if (FAILED(hRes)) {
        this->ReleaseBuffer(0);
        return hRes;
    }
    this->ReleaseBuffer(StrT::Len2(pBuffer, iLen));  // may be shorter. (pre-terminated)
    return iLen;
}

template <typename _TYPE_CH>
HRESULT cStringT<_TYPE_CH>::SerializeOutput(cStreamOutput& File) const {
    //!  Write a string AND length (in bytes not chars) out to the file.
    //! @arg
    //!  File = the open file.
    //! @note
    //!  This is NOT '\0' term. though Standard RIFF strings are!
    return File.WriteBlob(get_SpanStr());
}

template <typename _TYPE_CH>
HRESULT cStringT<_TYPE_CH>::SerializeOutput(cArchive& a) const {
    return SerializeOutput(a.ref_Out());
}

template <typename _TYPE_CH>
HRESULT cStringT<_TYPE_CH>::Serialize(cArchive& a) {
    //! Serialize in either direction.
    if (a.IsStoring())
        return SerializeOutput(a.ref_Out());
    else
        return SerializeInput(a.ref_Inp());
}

template class GRAYCORE_LINK cStringT<char>;     // force implementation/instantiate for DLL/SO. AND GRAY_STATICLIB
template class GRAYCORE_LINK cStringT<wchar_t>;  // force implementation/instantiate for DLL/SO. AND GRAY_STATICLIB

}  // namespace Gray
