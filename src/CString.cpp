//
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
const char cStringT<char>::m_Nil = '\0';  // Use this instead of nullptr. AKA cStrConst::k_Empty ?
template <>
const wchar_t cStringT<wchar_t>::m_Nil = '\0';  // Use this instead of nullptr. AKA cStrConst::k_Empty ?

/// <summary>
/// Dynamic allocate a buffer to hold the string. resize existing or create new.
/// </summary>
/// <typeparam name="_TYPE_CH"></typeparam>
/// <param name="iNewLength">chars not including null</param>
template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::AllocBuffer(StrLen_t iNewLength) {
    ASSERT(isValidString());
    ASSERT(IS_INDEX_GOOD(iNewLength, StrT::k_LEN_MAX + 1));  // reasonable arbitrary limit.

    if (iNewLength <= 0) {
        Empty();
        return;
    }

    HEAD_t* pHeadNew;
    if (m_pchData == &m_Nil) {
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
            if (nOldLen == iNewLength)  // no change.
                return;
            pHeadNew = HEAD_t::Cvt(pHeadOld->ResizeHead(iNewLength + 1, false));  // change.
            ASSERT_NN(pHeadNew);
        } else {
            // Make a new string. Copy from old. So we can change size.
            // NOTE: we maybe duping our self. (to change length)
            ASSERT(iRefCount > 1);
            pHeadNew = HEAD_t::Cvt(HEAD_t::CreateStringData(iNewLength));
            ASSERT_NN(pHeadNew);
            pHeadNew->IncRefCount();
            StrT::CopyLen(pHeadNew->get_DataWork(), m_pchData, cValT::Min(iNewLength, nOldLen) + 1);  // Copy from old
            pHeadOld->DecRefCount();                                                                  // release ref to previous string.
        }
    }

    ASSERT(cHeap::GetSize(pHeadNew) >= (sizeof(HEAD_t) + ((iNewLength + 1) * sizeof(_TYPE_CH))));

    m_pchData = pHeadNew->get_DataWork();
    m_pchData[iNewLength] = '\0';  // might just be trimming an existing string.
}

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::CloneBeforeWrite() {
    //! We are about to modify/change this. so make sure we don't interfere with other refs.
    //! Dupe this string.
    //! This might not be thread safe ?! if we start with 1 ref we may make another before we are done !
    if (IsEmpty()) return;
    HEAD_t* pHead = get_Head();
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
    //! Call this after using GetBuffer() or GetBufferSetLength();
    //! Reset size to actual used size.
    //! nNewLength = new (trimmed) count of chars, not including '\0' char at the end.
    if (m_pchData == &m_Nil) {
        ASSERT(nNewLength == 0);
        return;
    }
    HEAD_t* pHead = get_Head();
    ASSERT(pHead->get_RefCount() == 1);
    if (nNewLength <= k_StrLen_UNK) {  // default to current length
        nNewLength = StrT::Len(m_pchData);
    }
    if (nNewLength <= 0) {
        SetEmptyValid();  // make sure we all use m_Nil for empty. NOT nullptr
        return;
    }
    if (nNewLength != pHead->get_CharCount()) {
        // Shrink allocation ? or Leave allocation size the same ??
        ASSERT(nNewLength <= pHead->get_CharCount());
        m_pchData[nNewLength] = '\0';              // Can we assume this is already true ?
        pHead->ShrinkHead(nNewLength + 1, false);  // just shorten length.
    }
    ASSERT(isValidString());
}

/// <summary>
/// Allow direct manipulation of the string buffer. ASSUME ReleaseBuffer() will be called.
/// like MFC GetBufferSetLength also
/// </summary>
/// <typeparam name="_TYPE_CH">char or wchar_t</typeparam>
/// <param name="iMinLength">not including null</param>
/// <returns></returns>
template <typename _TYPE_CH>
_TYPE_CH* cStringT<_TYPE_CH>::GetBuffer(StrLen_t iMinLength) {
    ASSERT(iMinLength >= 0);
    if (iMinLength > GetLength()) {
        AllocBuffer(iMinLength);  // get brand new string.
    } else {
        CloneBeforeWrite();  // assume it is going to be changed.
    }
#ifdef _DEBUG
    if (iMinLength > 0) {  // m_Nil is special. Cant call GetData()
        HEAD_t* pHead = get_Head();
        ASSERT(pHead->get_CharCount() >= iMinLength);
    }
#endif
    return m_pchData;
}

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::AssignLenT(const _TYPE_CH* pszStr, StrLen_t iLenMax) {
    //! Copy pszStr into this string. nullptr = just allocate and leave blank.
    //! iLenMax = max chars, not including null ! NOT sizeof() or _countof() like StrT::CopyLen
    //! DEBUG_MSG(( "cString:Assign" ));
    //! @note What if pszStr is in the current string?
    //! @note DO NOT ASSUME pszStr is terminated string!! DONT CALL StrT::Len

    if (StrT::IsNullOrEmpty(pszStr) || iLenMax <= 0) {
        Empty();
        return;
    }
    StrLen_t iLenCur = GetLength();      // current stated length of the string.
    if (pszStr == m_pchData) {           // Same string.
        if (iLenMax >= iLenCur) return;  // do nothing!
        Assign(Left(iLenMax));
        return;
    }

    ASSERT(IS_INDEX_GOOD(iLenMax, StrT::k_LEN_MAX + 1));
    if (pszStr == nullptr) {
        // just allocate the space and leave it blank.
        AllocBuffer(iLenMax);
        ASSERT(m_pchData != nullptr);
        ASSERT(m_pchData != &m_Nil);
        m_pchData[0] = '\0';
        return;
    }

    if (pszStr >= m_pchData && pszStr <= m_pchData + iLenCur) {
        // Part of the same string so be safe !!
        THIS_t sTmp(pszStr, iLenMax);  // make a copy first!
        Assign(sTmp);
        return;
    }

    StrLen_t iLenStr = StrT::Len(pszStr, iLenMax);
    if (iLenMax > iLenStr) iLenMax = iLenStr;

    AllocBuffer(iLenMax);
    ASSERT_NN(m_pchData);
    ASSERT(m_pchData != &m_Nil);
    StrT::CopyLen(m_pchData, pszStr, iLenMax + 1);
    ASSERT(isValidString());
}

//*************************************************************
template <>
void cStringT<char>::AssignLen(const wchar_t* pwStr, StrLen_t iLenMax) {
    //! Convert UNICODE to UTF8
    //! iLenMax = _countof(StrT::k_LEN_MAX)-1 default
    //! iLenMax = max chars, not including null ! NOT sizeof() or _countof() like StrT::CopyLen

    char szTmp[StrT::k_LEN_Default];
    StrLen_t iLenNew = StrU::UNICODEtoUTF8(szTmp, STRMAX(szTmp), pwStr, iLenMax);
    AssignLenT(szTmp, iLenNew);
}
template <>
void cStringT<char>::AssignLen(const char* pszStr, StrLen_t iLenMax) {
    AssignLenT(pszStr, iLenMax);
}
template <>
void cStringT<char>::Assign(const wchar_t* pwStr) {
    //! Convert UNICODE to UTF8
    AssignLen(pwStr, StrT::k_LEN_MAX - 1);
}
template <>
void cStringT<char>::Assign(const char* pszStr) {
    AssignLenT(pszStr, StrT::k_LEN_MAX - 1);
}

//*************************************************************

template <>
void cStringT<wchar_t>::AssignLen(const char* pszStr, StrLen_t iLenMax) {
    //! Convert UTF8 to UNICODE
    //! iLenMax = STRMAX = _countof(StrT::k_LEN_MAX)-1 default

    wchar_t wTmp[StrT::k_LEN_Default];
    StrLen_t iLenNew = StrU::UTF8toUNICODE(wTmp, STRMAX(wTmp), pszStr, iLenMax);
    AssignLenT(wTmp, iLenNew);
}
template <>
void cStringT<wchar_t>::AssignLen(const wchar_t* pwStr, StrLen_t iLenMax) {
    AssignLenT(pwStr, iLenMax);
}
template <>
void cStringT<wchar_t>::Assign(const wchar_t* pwStr) {
    AssignLenT(pwStr, StrT::k_LEN_MAX - 1);
}
template <>
void cStringT<wchar_t>::Assign(const char* pszStr) {
    //! Convert UTF8 to UNICODE
    AssignLen(pszStr, StrT::k_LEN_MAX - 1);
}

//*************************************************************

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::FormatV(const _TYPE_CH* pszFormat, va_list args) {
    //! _vsntprintf
    //! use the normal sprintf() style.
    _TYPE_CH szTemp[StrT::k_LEN_Default];
    StrT::vsprintfN(OUT szTemp, STRMAX(szTemp), pszFormat, args);
    Assign(szTemp);
}

template <typename _TYPE_CH>
StrLen_t cStringT<_TYPE_CH>::Insert(StrLen_t i, _TYPE_CH ch) {
    //! @return New length.
    if (i <= k_StrLen_UNK) i = 0;
    const StrLen_t lenOld = GetLength();
    if (i > lenOld) i = lenOld;
    AllocBuffer(lenOld + 1);
    cMem::CopyOverlap(m_pchData + i + 1, m_pchData + i, lenOld - i);
    m_pchData[i] = ch;
    return GetLength();
}

template <typename _TYPE_CH>
StrLen_t cStringT<_TYPE_CH>::Insert(StrLen_t i, const _TYPE_CH* pszStr, StrLen_t iLenCat) {
    ASSERT_NN(pszStr);
    if (iLenCat <= k_StrLen_UNK) iLenCat = StrT::Len(pszStr);

    if (iLenCat > 0) {
        if (i < 0) i = 0;
        const StrLen_t lenOld = GetLength();
        if (i > lenOld) i = lenOld;
        if (lenOld + iLenCat > StrT::k_LEN_MAX) {
            DEBUG_ASSERT(0, "cString::Insert > StrT::k_LEN_MAX");
            return k_ITERATE_BAD;
        }
        AllocBuffer(lenOld + iLenCat);
        cMem::CopyOverlap(m_pchData + i + iLenCat, m_pchData + i, (lenOld - i) * sizeof(_TYPE_CH));
        cMem::Copy(m_pchData + i, pszStr, iLenCat * sizeof(_TYPE_CH));
    }
    return GetLength();
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> cStringT<_TYPE_CH>::Left(StrLen_t nCount) const {
    //! Get the left nCount chars. truncate.
    const StrLen_t lenOld = GetLength();
    if (nCount >= lenOld) return *this;
    if (nCount <= 0) return THIS_t();
    return THIS_t(get_CPtr(), nCount);
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> cStringT<_TYPE_CH>::Right(StrLen_t nCount) const {
    //! Get the right nCount chars. skip leading chars.
    //! @return
    //!  a new string with the nCount right most chars in this string.
    const StrLen_t lenOld = GetLength();
    if (nCount >= lenOld) return *this;
    if (nCount <= 0) return THIS_t();
    return THIS_t(get_CPtr() + (lenOld - nCount), nCount);
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> cStringT<_TYPE_CH>::Mid(StrLen_t nFirst, StrLen_t nCount) const {
    //! Same as STL substr() function.
    const StrLen_t lenOld = GetLength();
    if (nFirst >= lenOld) return THIS_t();
    if (nFirst + nCount >= lenOld) nCount = lenOld - nFirst;
    return THIS_t(get_CPtr() + nFirst, nCount);
}

template <typename _TYPE_CH>
StrLen_t cStringT<_TYPE_CH>::Find(_TYPE_CH ch, StrLen_t nPosStart) const {
    const StrLen_t iLen = GetLength();
    if (nPosStart > iLen)  // ch might be '\0' ?
        return k_ITERATE_BAD;
    const StrLen_t nIndex = StrT::FindCharN(m_pchData + nPosStart, ch);
    if (nIndex < 0) return k_ITERATE_BAD;
    return nIndex + nPosStart;
}

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::MakeUpper() {
    //! replaces _strupr
    //! No portable __linux__ equiv to _strupr()?
    //! Like MFC CString::MakeUpper(), Similar to .NET String.ToUpper(). BUT NOT THE SAME.
    CloneBeforeWrite();
    StrT::MakeUpperCase(m_pchData, GetLength());
    ASSERT(isValidString());
}

template <typename _TYPE_CH>
void cStringT<_TYPE_CH>::MakeLower() {
    //! replaces strlwr()
    //! No portable __linux__ equiv to strlwr()?
    //! Like MFC CString::MakeLower(), Similar to .NET String.ToLower() BUT NOT THE SAME.
    CloneBeforeWrite();
    StrT::MakeLowerCase(m_pchData, GetLength());
    ASSERT(isValidString());
}

//***********************************************************************************************

template <typename _TYPE_CH>
HRESULT cStringT<_TYPE_CH>::ReadZ(cStreamInput& stmIn, StrLen_t iLenMax) {
    _TYPE_CH* pBuffer = this->GetBuffer(iLenMax);  // ASSUME extra alloc for null is made.
    const HRESULT hRes = stmIn.ReadX(pBuffer, iLenMax);
    this->ReleaseBuffer(iLenMax);

    if (hRes != CastN(HRESULT, iLenMax)) {
        if (FAILED(hRes)) return hRes;
        return HRESULT_WIN32_C(ERROR_READ_FAULT);
    }
    return iLenMax;
}

template <typename _TYPE_CH>
bool cStringT<_TYPE_CH>::WriteZ(cStreamOutput& File) const {
    File.WriteX(get_CPtr(), THIS_t::GetLength() + 1);
    return true;
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
    StrU::UNICODEtoUTF8(szTmp, STRMAX(szTmp), pwStr, k_StrLen_UNK);
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
StrLen_t cStringT<char>::GetCodePage(OUT wchar_t* pwText, StrLen_t iLenMax, CODEPAGE_t uCodePage) const {
    //! Convert char with CODEPAGE_t to UNICODE.
    //! @arg uCodePage = CP_UTF8
    //! similar to StrU::UTF8toUNICODE
#ifdef _WIN32
    return ::MultiByteToWideChar(uCodePage,    // code page CP_ACP
                                 0,            // character-type options
                                 get_CPtr(),   // address of string to map
                                 GetLength(),  // number of bytes in string
                                 pwText,       // address of wide-character buffer
                                 iLenMax       // size of buffer
    );
#else
    return StrU::UTF8toUNICODE(pwText, iLenMax, get_CPtr(), GetLength());  // true size is variable and < iLen
#endif
}
template <>
StrLen_t cStringT<wchar_t>::GetCodePage(OUT wchar_t* pwText, StrLen_t iLenMax, CODEPAGE_t uCodePage) const {
    //! Just copy. No conversion if already UNICODE.
    UNREFERENCED_PARAMETER(uCodePage);
    return StrT::CopyLen(pwText, get_CPtr(), iLenMax);
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> cStringT<_TYPE_CH>::GetTrimWhitespace() const {
    // Trim whitespace from both ends.
    const StrLen_t lenOld = GetLength();
    const StrLen_t left = StrT::GetNonWhitespaceI(m_pchData, lenOld);
    const StrLen_t right = StrT::GetWhitespaceEnd(m_pchData, lenOld);
    if (left == 0 && right == lenOld) return *this;
    return THIS_t(m_pchData + left, right - left);
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
        StrBuilder<GChar_t> sb(szTmp, STRMAX(szTmp));
        HResult::GetTextV(hResError, sb, pSource, k_va_list_empty);  // TODO va_list
        return THIS_t(szTmp, sb.get_Length());
    }
    if (hResError == S_OK) return CSTRCONST("OK").Get<_TYPE_CH>();
    return StrArg<_TYPE_CH>((UINT32)hResError, (RADIX_t)0x10);  // its not an error just a hex number/code.
}

template <typename _TYPE_CH>
cStringT<_TYPE_CH> GRAYCALL cStringT<_TYPE_CH>::GetSizeK(UINT64 uVal, UINT nKUnit, bool bSpace) {  // static
    _TYPE_CH szTmp[StrNum::k_LEN_MAX_DIGITS];
    StrLen_t nLen = StrT::ULtoAK(uVal, szTmp, STRMAX(szTmp), nKUnit, bSpace);
    return THIS_t(szTmp, nLen);
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
    if (FAILED(hRes)) {
        return HResult::GetDef(hRes, HRESULT_WIN32_C(ERROR_IO_INCOMPLETE));
    }
    StrLen_t iLen = (StrLen_t)(nSizeRead / sizeof(_TYPE_CH));
    if (iLen > iLenMax || iLen >= StrT::k_LEN_MAX) {
        return HRESULT_WIN32_C(RPC_S_STRING_TOO_LONG);
    }

    _TYPE_CH* pBuffer = this->GetBuffer(iLen);  // ASSUME extra alloc for null is made.

    hRes = File.ReadT(pBuffer, nSizeRead);  // all or nothing.
    if (FAILED(hRes)) {
        this->ReleaseBuffer(0);
        return hRes;
    }
    this->ReleaseBuffer(StrT::Len(pBuffer, iLen));  // may be shorter. (pre-terminated)
    return iLen;
}

template <typename _TYPE_CH>
HRESULT cStringT<_TYPE_CH>::SerializeOutput(cStreamOutput& File) const {
    //!  Write a string AND length (in bytes not chars) out to the file.
    //! @arg
    //!  File = the open file.
    //! @note
    //!  This is NOT '\0' term. though Standard RIFF strings are!
    return File.WriteBlob(get_CPtr(), this->GetLength() * sizeof(_TYPE_CH));
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
