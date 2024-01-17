//
//! @file cString.h
//! Create a UTF8/UNICODE interchangeable string.
//! ref counted string class.
//! STL C string emulating. NOT MFC.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cString_H
#define _INC_cString_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrT.h"
#include "cArrayT.h"

namespace Gray {
struct GRAYCORE_LINK cStreamInput;
struct GRAYCORE_LINK cStreamOutput;
class GRAYCORE_LINK cArchive;

/// <summary>
/// A dynamic allocation block to hold the characters of the string.
/// </summary>
/// <typeparam name="_TYPE_CH"></typeparam>
template <typename _TYPE_CH = char>
class GRAYCORE_LINK cStringHeadT final : public cArrayHeadT<_TYPE_CH> {
    typedef cArrayHeadT<_TYPE_CH> SUPER_t;
    typedef cStringHeadT<_TYPE_CH> THIS_t;
    CHEAPOBJECT_IMPL;

 public:
    // SUPER_t and THIS_t are interchangeable! Since we declare no extra data or virtual methods.
    static inline const THIS_t* Cvt(const void* p) noexcept {
        DEBUG_CHECK(p != nullptr);
        return reinterpret_cast<const THIS_t*>(p);
    }
    static inline THIS_t* Cvt(void* p) noexcept {
        DEBUG_CHECK(p != nullptr);
        return reinterpret_cast<THIS_t*>(p);
    }

    /// <summary>
    /// Allocate space for chars plus '\0'
    /// </summary>
    /// <param name="nCharCount"></param>
    /// <returns></returns>
    static THIS_t* CreateStringData(StrLen_t nCharCount) {
        ASSERT(nCharCount >= 0);
        return Cvt(SUPER_t::CreateHead(nCharCount + 1, false));
    }
    static THIS_t* CreateStringData2(StrLen_t nCharCount, const _TYPE_CH* pSrc) {
        THIS_t* p = CreateStringData(nCharCount);
        if (pSrc != nullptr) {  // init
            StrT::CopyLen(p->get_DataWork(), pSrc, nCharCount + 1);
        }
        return p;
    }

    inline const _TYPE_CH* get_Name() const noexcept {
        //! supported for cArraySort. NON hash sort.
        return get_DataConst();
    }
    inline StrLen_t get_CharCount() const noexcept {
        return CastN(StrLen_t, get_Count()) - 1;  // remove space for '\0';
    }
    bool isValidString() const noexcept {
        const StrLen_t iLen = get_CharCount();
        if (!IsValidInsideN(iLen * sizeof(_TYPE_CH))) return false;  // should never happen!
        if (get_RefCount() <= 0) return false;                       // should never happen!
        return get_Name()[iLen] == '\0';                             // terminated?
    }
    HASHCODE32_t get_HashCode() const noexcept {
        const StrLen_t iLen = get_CharCount();
        if (iLen <= 0) return k_HASHCODE_CLEAR;
        if (m_HashCode == k_HASHCODE_CLEAR) {
            // Lazy populate this value.
            const_cast<THIS_t*>(this)->m_HashCode = StrT::GetHashCode32(get_DataConst(), iLen);
        }
        return m_HashCode;
    }

    COMPARE_t CompareNoCase(const ATOMCHAR_t* pStr) const noexcept {
        return StrT::CmpI(get_DataConst(), pStr);
    }
    bool IsEqualNoCase(const ATOMCHAR_t* pStr) const noexcept {
        // TODO: Use HashCode for speed compares.
        return StrT::CmpI(get_DataConst(), pStr) == COMPARE_Equal;
    }
};

/// <summary>
/// Manage a reference counted pointer to cArrayHeadT _TYPE_CH. Is to a string array that is dynamically allocated.
/// Mimic the MFC ATL::CStringT<> functionality.
/// Unlike STL std::string this is shareable via reference count. No dynamic copied each time.
/// Use this for best string functionality.
/// NOTE cObject based ??
/// </summary>
/// <typeparam name="_TYPE_CH">char or wchar_t</typeparam>
template <typename _TYPE_CH = char>
class GRAYCORE_LINK cStringT {
    typedef cStringT<_TYPE_CH> THIS_t;

 protected:
    typedef cStringHeadT<_TYPE_CH> HEAD_t;
    _TYPE_CH* m_pchData;          /// points offset into HEAD_t/cStringHeadT[1] like cRefPtr<>. NOT a direct heap pointer!
    static const _TYPE_CH m_Nil;  /// '\0' Use this instead of nullptr. ala MFC. also like _afxDataNil. AKA cStrConst::k_Empty ?

 protected:
    void Init() noexcept {
        m_pchData = const_cast<_TYPE_CH*>(&m_Nil);
    }
    void SetEmptyValid() noexcept {
        // ASSUME NOT m_Nil. Use m_Nil for empty.
        DEBUG_CHECK(isValidString());
        get_Head()->DecRefCount();
        Init();
    }

    void AssignFirst(const THIS_t& s) noexcept {
        m_pchData = s.m_pchData;
        if (IsEmpty()) return;
        get_Head()->IncRefCount();
        DEBUG_CHECK(isValidString());
    }

    void AllocBuffer(StrLen_t iStrLength);
    void CloneBeforeWrite();

 public:
    typedef _TYPE_CH CharType_t;  /// ALA std::string::value_type

    cStringT() noexcept {
        Init();
    }

    /// Copy string
    cStringT(const wchar_t* pwText) {
        //! Init and convert UNICODE -> UTF8 if needed.
        Init();
        Assign(pwText);
    }
    cStringT(const wchar_t* pwText, StrLen_t iLenMax) {
        //! @arg iLenMax = STRMAX = _countof(wText)-1 default StrT::k_LEN_MAX
        //! @arg iLenMax = max chars, not including '\0' ! NOT sizeof() or _countof() like StrT::CopyLen
        Init();
        AssignLen(pwText, iLenMax);
    }
    cStringT(const char* pszStr) {
        //! Init and convert UNICODE is needed.
        Init();
        Assign(pszStr);
    }
    cStringT(const char* pszStr, StrLen_t iLenMax) {
        //! @arg iLenMax = _countof(StrT::k_LEN_MAX)-1 default
        //! @arg iLenMax = max chars, not including '\0' ! NOT sizeof() or _countof() like StrT::CopyLen
        Init();
        AssignLen(pszStr, iLenMax);
    }

    cStringT(const THIS_t& ref) noexcept {
        AssignFirst(ref);
    }
    /// <summary>
    /// Move constructor
    /// </summary>
    cStringT(THIS_t&& ref) noexcept {
        m_pchData = ref.m_pchData;
        ref.Init();
    }
    ~cStringT() {
        Empty();
    }

    explicit cStringT(HEAD_t* pHead) {
        // Assign internal data object directly. weird usage.
        DEBUG_CHECK(pHead != nullptr);
        pHead->IncRefCount();
        m_pchData = pHead->get_DataWork();
        DEBUG_CHECK(isValidString());
    }

    /// <summary>
    /// Is this string 0 length? like MFC.
    /// </summary>
    bool IsEmpty() const noexcept {
        return m_pchData == &m_Nil;
    }

    /// <summary>
    /// Is the string properly terminated?
    /// </summary>
    /// <returns></returns>
    bool isValidString() const noexcept {
        if (IsEmpty()) return true;
        const HEAD_t* const pHead = get_Head();
        if (pHead == nullptr) return false;  // should never happen!
        return pHead->isValidString();
    }

    /// <summary>
    /// Get my HEAD_t internal storage object pointer. like MFC
    /// </summary>
    /// <returns>NEVER nullptr</returns>
    const HEAD_t* get_Head() const noexcept {
        DEBUG_CHECK(!IsEmpty());
        return HEAD_t::Cvt(m_pchData) - 1;  // the block before this pointer.
    }
    HEAD_t* get_Head() noexcept {
        DEBUG_CHECK(!IsEmpty());
        return HEAD_t::Cvt(m_pchData) - 1;  // the block before this pointer.
    }
    const _TYPE_CH* get_CPtr() const noexcept {
        //! like MFC. GetString
        DEBUG_CHECK(isValidString());
        return m_pchData;
    }

    /// <summary>
    /// get number of chars. (not bytes)
    /// </summary>
    /// <returns></returns>
    StrLen_t GetLength() const noexcept {
        if (IsEmpty()) return 0;
        const HEAD_t* pHead = get_Head();
        if (pHead == nullptr) {
            DEBUG_CHECK(pHead != nullptr);
            return 0;
        }
        return pHead->get_CharCount();
    }
    /// <summary>
    /// AKA SetEmpty
    /// </summary>
    void Empty() noexcept {
        if (m_pchData == nullptr)  // certain off instances where it could be nullptr. arrays
            return;
        if (IsEmpty()) return;
        SetEmptyValid();
    }
    /// <summary>
    /// Clear this more thoroughly for security reasons. passwords, etc. ZeroSecure ?
    /// </summary>
    void SetErase() {
        this->Empty();
    }

    const _TYPE_CH& ReferenceAt(StrLen_t nIndex) const {  // 0 based
        // AKA ElementAt()
        ASSERT(nIndex <= GetLength());
        return m_pchData[nIndex];
    }

    /// <summary>
    /// Get a character.
    /// </summary>
    /// <param name="nIndex"></param>
    /// <returns></returns>
    _TYPE_CH GetAt(StrLen_t nIndex) const {  // 0 based
        ASSERT(nIndex <= GetLength());       // allow to get the '\0' char
        return m_pchData[nIndex];
    }

    /// <summary>
    /// Set a character.
    /// </summary>
    /// <param name="nIndex"></param>
    /// <param name="ch"></param>
    void SetAt(StrLen_t nIndex, _TYPE_CH ch) {
        ASSERT(IS_INDEX_GOOD(nIndex, GetLength()));
        CloneBeforeWrite();
        m_pchData[nIndex] = ch;
        ASSERT(isValidString());
    }

    _TYPE_CH* GetBuffer(StrLen_t iMinLength);
    void ReleaseBuffer(StrLen_t nNewLength = k_StrLen_UNK);

    /// <summary>
    /// Copy assignment
    /// </summary>
    const THIS_t& operator=(const THIS_t& ref) {
        Assign(ref);
        return *this;
    }
    /// <summary>
    /// Move assignment
    /// </summary>
    const THIS_t& operator=(THIS_t&& ref) {
        m_pchData = ref.m_pchData;
        ref.Init();
        return *this;
    }

    void AssignLenT(const _TYPE_CH* pszStr, StrLen_t iLenMax);

    // UTF8. auto converted
    void AssignLen(const char* pszStr, StrLen_t iSizeMax = StrT::k_LEN_MAX);
    const THIS_t& operator=(const char* pStr) {
        Assign(pStr);
        return *this;
    }

    // UNICODE. auto converted
    void AssignLen(const wchar_t* pwText, StrLen_t iSizeMax = StrT::k_LEN_MAX);
    const THIS_t& operator=(const wchar_t* pStr) {
        Assign(pStr);
        return *this;
    }

    void FormatV(const _TYPE_CH* pszFormat, va_list args);
    void _cdecl Format(const _TYPE_CH* pszFormat, ...) {
        //! format a string using the sprintf() style.
        //! @note Use StrArg<_TYPE_CH>(s) for safe "%s" args.
        va_list vargs;
        va_start(vargs, pszFormat);
        FormatV(pszFormat, vargs);
        va_end(vargs);
    }

    COMPARE_t Compare(const _TYPE_CH* pszStr) const noexcept {
        return StrT::Cmp(get_CPtr(), pszStr);
    }
    COMPARE_t CompareNoCase(const _TYPE_CH* pszStr) const noexcept {
        return StrT::CmpI(get_CPtr(), pszStr);
    }
    bool IsEqualNoCase(const _TYPE_CH* pszStr) const noexcept {
        // TODO: Use HashCode for speed compares.
        return StrT::CmpI(get_CPtr(), pszStr) == 0;
    }

    void MakeUpper();  // MFC like not .NET like.
    void MakeLower();
    THIS_t Left(StrLen_t nCount) const;
    THIS_t Right(StrLen_t nCount) const;
    THIS_t Mid(StrLen_t nFirst, StrLen_t nCount = StrT::k_LEN_MAX) const;

    StrLen_t Find(_TYPE_CH ch, StrLen_t nPosStart = 0) const;

    _TYPE_CH operator[](StrLen_t nIndex) const {  // same as GetAt
        return GetAt(nIndex);
    }
    const _TYPE_CH& operator[](StrLen_t nIndex) {
        return ReferenceAt(nIndex);
    }
    operator const _TYPE_CH*() const noexcept {  // as a C string
        return get_CPtr();
    }

    friend bool operator==(const THIS_t& str1, const _TYPE_CH* str2) THROW_DEF {
        return str1.Compare(str2) == COMPARE_Equal;
    }
    friend bool operator!=(const THIS_t& str1, const _TYPE_CH* str2) THROW_DEF {
        return str1.Compare(str2) != COMPARE_Equal;
    }

    // insert character at zero-based index; concatenates
    // if index is past end of string
    StrLen_t Insert(StrLen_t nIndex, _TYPE_CH ch);
    const THIS_t& operator+=(_TYPE_CH ch) {
        Insert(GetLength(), ch);
        return *this;
    }

    /// <summary>
    /// insert substring at zero-based index;
    /// concatenates if index is past end of string
    /// </summary>
    /// <param name="nIndex"></param>
    /// <param name="pszStr"></param>
    /// <param name="iLenCat"></param>
    /// <returns>New length.</returns>
    StrLen_t Insert(StrLen_t nIndex, const _TYPE_CH* pszStr, StrLen_t iLenCat);
    StrLen_t Insert(StrLen_t nIndex, const _TYPE_CH* pszStr) {
        return Insert(nIndex, pszStr, k_StrLen_UNK);
    }
    const THIS_t& operator+=(const _TYPE_CH* psz) {  // like strcat()
        Insert(GetLength(), psz, k_StrLen_UNK);
        return *this;
    }

    void Assign(const THIS_t& str) {
        if (m_pchData == str.get_CPtr())  // already same.
            return;
        Empty();
        AssignFirst(str);
    }
    void Assign(const wchar_t* pwText);
    void Assign(const char* pszStr);

    bool isPrintableString() const noexcept {
        if (IsEmpty()) return true;
        const HEAD_t* pHead = this->get_Head();
        const StrLen_t iLen = pHead->get_CharCount();
        ASSERT(pHead->IsValidInsideN(iLen * sizeof(_TYPE_CH)));
        ASSERT(pHead->get_RefCount() > 0);
        return StrT::IsPrintable(m_pchData, iLen) && (m_pchData[iLen] == '\0');
    }
    bool isValidCheck() const noexcept {
        return isValidString();
    }
    bool IsWhitespace() const {
        //! Like .NET String.IsNullOrWhitespace
        return StrT::IsWhitespace(this->get_CPtr(), this->length());
    }

    /// <summary>
    /// Read in a new string from an open binary file. No length prefix.
    /// </summary>
    /// <param name="File">the open file</param>
    /// <param name="iLenMax">The length of the string to read. NOT THE '\0' !</param>
    /// <returns>length of the string. -le- 0 = error or no valid string.</returns>
    HRESULT ReadZ(cStreamInput& File, StrLen_t iLenMax);

    /// <summary>
    /// Write a string AND '\0' out to the file. No length prefix.
    /// @note Standard RIFF strings are '\0' terminated !
    /// </summary>
    /// <typeparam name="_TYPE_CH"></typeparam>
    /// <param name="File">the open file.</param>
    /// <returns></returns>
    bool WriteZ(cStreamOutput& File) const;

    HASHCODE32_t get_HashCode() const noexcept {
        if (this->IsEmpty()) return 0;
        return this->get_Head()->get_HashCode();
    }
    size_t CountHeapStats(OUT ITERATE_t& iAllocCount) const {
        //! Get data allocations for all children. does not include sizeof(*this)
        if (this->IsEmpty()) return 0;
        return this->get_Head()->GetHeapStatsThis(iAllocCount);
    }
    int get_RefCount() const {
        // expose internal ref count. ASSUME NOT _Nil ?
        return this->get_Head()->get_RefCount();
    }
    void SetStringStatic() {
        //! Make this string permanent. never removed from memory.
        this->get_Head()->IncRefCount();
    }

    StrLen_t SetCodePage(const wchar_t* pwText, CODEPAGE_t uCodePage = CP_UTF8);
    StrLen_t GetCodePage(OUT wchar_t* pwText, StrLen_t iLenMax, CODEPAGE_t uCodePage = CP_UTF8) const;

    THIS_t GetTrimWhitespace() const;

    HRESULT SerializeInput(cStreamInput& File, StrLen_t iLenMax = StrT::k_LEN_MAX);
    HRESULT SerializeOutput(cStreamOutput& File) const;
    HRESULT SerializeOutput(cArchive& a) const;
    HRESULT Serialize(cArchive& a);

    bool Contains(const _TYPE_CH* pSubStr) {
        // Like .NET
        return StrT::FindStr(get_CPtr(), pSubStr) != nullptr;
    }
    bool ContainsI(const _TYPE_CH* pSubStr) {
        // Like .NET
        return StrT::FindStrI(get_CPtr(), pSubStr) != nullptr;
    }
    bool StartsWithI(const _TYPE_CH* pSubStr) {
        // Like .NET
        return StrT::StartsWithI(get_CPtr(), pSubStr);
    }
    bool EndsWithI(const _TYPE_CH* pSubStr) const {
        // Like .NET
        return StrT::EndsWithI<_TYPE_CH>(get_CPtr(), pSubStr, this->GetLength());
    }

    //! emulate STL string operators.
    //! Is any of this needed ??
    static const int npos = -1;  // k_ITERATE_BAD. same name as STL.

    const _TYPE_CH* c_str() const {
        return this->get_CPtr();  // STL
    }
    StrLen_t size() const {
        // basic_string::size
        return this->GetLength();
    }
    StrLen_t length() const {
        // char_traits::length
        return this->GetLength();
    }
    bool empty() const {
        return this->IsEmpty();
    }
    StrLen_t find(_TYPE_CH ch) const {
        //! @return npos = -1 = not found.
        return this->Find(ch, 0);
    }
    void assign(const _TYPE_CH* pszStr, StrLen_t iLenCat) {
        *this = THIS_t(pszStr, iLenCat);
    }
    void append(const _TYPE_CH* pszStr, StrLen_t iLenCat) {
        this->Insert(this->GetLength(), pszStr, iLenCat);
    }
    void push_back(_TYPE_CH ch) {
        this->Insert(this->GetLength(), ch);
    }

    void resize(StrLen_t iSize) {
        this->AllocBuffer(iSize);
    }
    void reserve(StrLen_t iSize) {
        // optimize that this is the end length.
        UNREFERENCED_PARAMETER(iSize);
    }
    THIS_t substr(StrLen_t nFirst, StrLen_t nCount = StrT::k_LEN_MAX) const {
        // Like return this->Mid(nFirst, nCount)
        if (nFirst >= this->GetLength()) return cStrConst::k_Empty.Get<_TYPE_CH>();
        return THIS_t(this->get_CPtr() + nFirst, nCount);
    }

    static THIS_t _cdecl Join(const _TYPE_CH* s1, ...);
    static THIS_t _cdecl GetFormatf(const _TYPE_CH* pszFormat, ...);

    /// <summary>
    /// Describe a system error code as a string.
    /// @note Must use HResult::FromWin32(x) instead of raw "::GetLastError()"
    /// </summary>
    /// <param name="nFormatID">HRESULT_WIN32_C(ERROR_INTERNAL_ERROR) etc.</param>
    /// <param name="pSource"></param>
    /// <returns></returns>
    static THIS_t GRAYCALL GetErrorStr(HRESULT nFormatID, const void* pSource = nullptr);

    static THIS_t GRAYCALL GetSizeK(UINT64 uVal, UINT nKUnit = 1024, bool bSpace = false);
};

//***********************************************************************************************************

typedef cStringT<wchar_t> cStringW;
typedef cStringT<char> cStringA;
typedef cStringT<GChar_t> cString;

#if !defined(_MFC_VER) && !defined(__CLR_VER)
typedef cString CString;
#endif

cStringA inline operator+(const char* pStr1, const cStringA& s2) {  // static global
    cStringA s1(pStr1);
    s1 += s2;
    return s1;
}
cStringW inline operator+(const wchar_t* pStr1, const cStringW& s2) {  // static global
    cStringW s1(pStr1);
    s1 += s2;
    return s1;
}

template <typename _TYPE_CH>  // "= char" error C4519: default template arguments are only allowed on a class template
void inline operator>>(cArchive& ar, cStringT<_TYPE_CH>& pOb) {
    pOb.Serialize(ar);
}
template <typename _TYPE_CH>
void inline operator<<(cArchive& ar, const cStringT<_TYPE_CH>& pOb) {
    pOb.SerializeOutput(ar);
}
}  // namespace Gray
#endif  // _INC_cString_H
