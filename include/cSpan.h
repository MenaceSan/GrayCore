//! @file cSpan.h
//! specify a sized block of memory.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cSpan_H
#define _INC_cSpan_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cMem.h"
#include "cPtrFacade.h"
#include "cValSpan.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4275)  // non dll-interface class 'Gray::cMemSpan' used as base for dll-interface class 'Gray::cBlob'
#endif

namespace Gray {
/// <summary>
/// A pointer to memory block/blob/span with known size and unknown ownership.
/// may be heap, stack, static or const based memory pointer. don't free on destruct. (although a derived class might)
/// May be static init? or UN-INIT!
/// </summary>
class GRAYCORE_LINK cMemSpan { // : public cMem 
    typedef cMemSpan THIS_t;
    void* m_pData;   /// a block of memory of unknown ownership. Treat as MEMTYPE_t::_Temp
    size_t m_nSize;  /// size_t of m_pData in bytes.

 public:
    /// <summary>
    /// Set/Adjust size in bytes but leave data pointer alone.
    /// </summary>
    void put_DataSize(size_t nSize) noexcept {
        m_nSize = nSize;
    }

    constexpr cMemSpan() noexcept : m_pData(nullptr), m_nSize(0) {}
    constexpr cMemSpan(const void* pData, size_t nSize) noexcept : m_pData(nSize ? const_cast<void*>(pData) : nullptr), m_nSize(nSize) {
        // just assume we don't modify it?
        // read only. SetSpanConst
        DEBUG_CHECK(isValid());
    }
    explicit cMemSpan(const cMemSpan* pBlock) noexcept {
        // Just shared pointers. This may be dangerous!
        m_pData = (pBlock == nullptr) ? nullptr : pBlock->m_pData;
        m_nSize = (pBlock == nullptr) ? 0 : pBlock->m_nSize;
        DEBUG_CHECK(isValid());
    }

    /// <summary>
    /// Get size in bytes.
    /// </summary>
    constexpr inline size_t get_DataSize() const noexcept {
        return m_nSize;
    }

    /// <summary>
    /// get a read only arbitrary TYPE pointer. Might be nullptr. that's OK.
    /// </summary>
    template <typename TYPE2 = BYTE>
    inline const TYPE2* get_DataC() const noexcept {
        // DEBUG_CHECK(m_nSize == 0 || m_nSize >= sizeof(TYPE2));
        return PtrCast<TYPE2>(m_pData);
    }
    /// <summary>
    /// Get a writable arbitrary TYPE2 pointer.
    /// </summary>
    template <typename TYPE2 = BYTE>
    inline TYPE2* get_DataW() noexcept {
        // DEBUG_CHECK(m_nSize == 0 || m_nSize >= sizeof(TYPE2));
        return PtrCast<TYPE2>(m_pData);
    }

    /// <summary>
    /// Get a Non-const pointer that we do not actually expect to write to ?! Some APIs seem to want this.
    /// </summary>
    /// <typeparam name="TYPE2"></typeparam>
    /// <returns></returns>
    template <typename TYPE2 = BYTE>
    inline TYPE2* get_DataNC() const noexcept {
        // DEBUG_CHECK(m_nSize == 0 || m_nSize >= sizeof(TYPE2));
        return PtrCast<TYPE2>(m_pData);
    }
    /// <summary>
    /// Is this (probably) valid to use/read/write. not nullptr.
    /// </summary>
    /// <returns></returns>
    constexpr inline bool isValidPtr() const noexcept {
        return cMem::IsValidPtr(m_pData);
    }
    /// <summary>
    /// Not exactly the same as empty? since nullptr and size are allowed for lockable types.
    /// </summary>
    constexpr inline bool isNull() const noexcept {
        return m_pData == nullptr;
    }

    /// <summary>
    /// operator to auto cast to const pointer
    /// </summary>
    operator const void*() const noexcept {
        return m_pData;
    }
    operator const BYTE*() const noexcept {
        return PtrCast<BYTE>(m_pData);
    }

    /// <summary>
    /// Is empty? assume NOT nullptr if not empty!
    /// </summary>
    constexpr inline bool isEmpty() const noexcept {
        return m_nSize <= 0;
    }

    /// <summary>
    /// Is in a valid state? nullptr = a valid state. Forbid: 0 sized isValidPtr !
    /// </summary>
    constexpr inline bool isValid() const noexcept {
        if (isNull()) return true;
        return isValidPtr() && m_nSize != 0;
    }

    /// <summary>
    /// Is byte offset inside the known valid range for the block? exclusive.
    /// I can write or read this?
    /// </summary>
    inline bool IsInSize(size_t i) const noexcept {
        return IS_INDEX_GOOD(i, m_nSize);
    }
    /// <summary>
    /// Is byte offset inside the known valid range for the block? or at end? inclusive.
    /// </summary>
    bool IsLTESize(size_t i) const noexcept {
        if (i == m_nSize)  // at end is ok
            return true;
        return IS_INDEX_GOOD(i, m_nSize);
    }

    inline size_t GetOffset(const void* p) const noexcept {
        return cMem::Diff(p, m_pData);
    }

    /// <summary>
    /// Is p inside the known valid range for the block? Exclusive = Cant be equal to end!
    /// </summary>
    inline bool IsInternalPtr(const void* p) const noexcept {
        return IsInSize(GetOffset(p));
    }

    /// <summary>
    /// Is p inside the known valid range for the block? Inclusive = Can be equal to end.
    /// </summary>
    inline bool IsInternalPtr2(const void* p) const noexcept {
        return IsLTESize(GetOffset(p));
    }

    bool IsZeros() const noexcept {
        return cMem::IsZeros(m_pData, m_nSize);
    }

    /// <summary>
    /// Exact same span?
    /// </summary>
    /// <param name="data"></param>
    bool IsSameSpan(const THIS_t& data) const noexcept {
        return m_nSize == data.m_nSize && m_pData == data.m_pData;
    }

    bool IsEqualData(const void* pData) const noexcept {
        return cMem::IsEqual(m_pData, pData, m_nSize);
    }

    /// <summary>
    /// compare spans of data.
    /// </summary>
    bool IsEqualSpan2(const void* pData, size_t nSize) const noexcept {
        return m_nSize == nSize && IsEqualData(pData);
    }
    /// <summary>
    /// compare blocks of data.
    /// </summary>
    bool IsEqualSpan(const THIS_t& data) const noexcept {
        return IsEqualSpan2(data.m_pData, data.m_nSize);
    }
 
    /// <summary>
    /// get pointer that is good/valid for just one byte in the span.
    /// </summary>
    /// <param name="nOffset"></param>
    const BYTE* GetInternalPtr(size_t nOffset) const noexcept {
        if (!IsInSize(nOffset)) return nullptr;
        return get_DataC<BYTE>() + nOffset;
    }
    /// <summary>
    /// Get a pointer into the buffer as a byte pointer.
    /// Ensure the data is valid to size_t !
    /// </summary>
    /// <param name="nOffset"></param>
    /// <param name="size"></param>
    /// <returns></returns>
    const BYTE* GetInternal2(size_t nOffset, size_t size) const noexcept {
        ASSERT(size > 0);
        if (!IsInSize(nOffset) || !IsLTESize(nOffset + size)) {
            DEBUG_CHECK(false);
            return nullptr;  // not big enough!
        }
        return get_DataC<BYTE>() + nOffset;
    }

    /// <summary>
    /// Get a pointer to the end of the buffer.
    /// Never read/write to/past this pointer.
    /// </summary>
    /// <returns></returns>
    const BYTE* get_SpanEnd() const noexcept {
        return get_DataC<BYTE>() + m_nSize;
    }

    void SetSpanNull() noexcept {
        m_nSize = 0;
        m_pData = nullptr;
    }
    /// set a read only span. nullptr ok.
    void SetSpanConst(const void* pData, size_t nSize) noexcept {
        m_pData = const_cast<void*>(pData);
        m_nSize = nSize;  // size does not apply if nullptr.
        DEBUG_CHECK(isValid());
    }
    /// set a writable span.
    void SetSpan2(void* pData, size_t nSize) noexcept {
        m_nSize = nSize;  // size does not apply if nullptr.
        m_pData = nSize ? pData : nullptr;
        DEBUG_CHECK(isValid());
    }
    /// make a copy of this span
    void SetSpan(const THIS_t& a) noexcept {
        m_nSize = a.m_nSize;  // size does not apply if nullptr.
        m_pData = a.m_pData;
        DEBUG_CHECK(isValid());
    }

    /// Advance the span and shrink it.
    inline void SetSpanSkip(size_t nSize) {
        ASSERT(nSize <= m_nSize);
        m_pData = get_DataW<BYTE>() + nSize;
        m_nSize -= nSize;
    }

    void SetZeros() noexcept {
        cMem::ZeroSecure(m_pData, m_nSize);
    }
    void SetCopyAll(const void* pData) noexcept {
        cMem::Copy(m_pData, pData, m_nSize);
    }
    size_t SetCopySpan(const cMemSpan& span2) noexcept {
        const size_t sizeMin = cValT::Min(m_nSize, span2.get_DataSize());
        if (!isNull() && !span2.isNull() && m_pData != span2.get_DataC()) {
            cMem::Copy(m_pData, span2, sizeMin);
        }
        return sizeMin;
    }

    void CopyTo(void* pData) const noexcept {
        cMem::Copy(pData, m_pData, m_nSize);
    }
    COMPARE_t Compare(const cMemSpan& m2) const;

    /// How much space does the hex digest need?
    static constexpr StrLen_t GetHexDigestSize(size_t nSize) noexcept {
        return CastN(StrLen_t, (nSize * 2) + 1);
    }
    /// <summary>
    /// Get the required size of the hex string.
    /// </summary>
    StrLen_t get_HexDigestSize() const {
        return GetHexDigestSize(get_DataSize());
    }
    /// ASSUME pszHexString output is big enough! GetHexDigestSize()
    StrLen_t GetHexDigest(OUT char* pszHexString) const;
    HRESULT SetHexDigest(const char* pszHexString, bool testEnd = true);

    // read/write a string of comma separated numbers.
    size_t ReadFromCSV(const char* pszSrc);
};

/// <summary>
/// a span of some unknown type. Element size (stride) is known only at run time.
/// can be used with StrT::TableFind*
/// </summary>
struct cSpanUnk : public cMemSpan {
    size_t _Stride;  /// each element is of this size. AKA pitch.

    cSpanUnk(const cMemSpan& m, size_t stride) : cMemSpan(m), _Stride(stride) {
        ASSERT(stride > 0);
    }

    constexpr COUNT_t get_Count() const noexcept {
        return get_DataSize() / _Stride;
    }
    constexpr ITERATE_t GetSize() const noexcept {  // for cArray
        return CastN(ITERATE_t, get_Count());       // AKA Count
    }
    inline bool IsValidIndex(COUNT_t i) const noexcept {
        return IS_INDEX_GOOD(i, get_Count());
    }

    inline const void* GetElemV(ITERATE_t i) const {
        ASSERT(!isNull());
        ASSERT(IsValidIndex(i));
        return get_DataC() + (i * _Stride);
    }

    // String searching. const
    template <typename T>
    inline const T* GRAYCALL GetElemT(ITERATE_t i) const {
        return PtrCast<T>(GetElemV(i));
    }
};

/// <summary>
/// A Span of some known TYPE. Probably read only.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <typename TYPE>
class cSpan : public cMemSpan {
    typedef cMemSpan SUPER_t;
    typedef cSpan<TYPE> THIS_t;

 public:
    constexpr cSpan() noexcept {}
    constexpr cSpan(const TYPE* pData, COUNT_t nCount) noexcept : SUPER_t(pData, nCount * sizeof(TYPE)) {}

    constexpr explicit cSpan(const cMemSpan& span) noexcept : SUPER_t(span) {}

    constexpr COUNT_t get_Count() const noexcept {
        return get_DataSize() / sizeof(TYPE);
    }
    constexpr StrLen_t get_MaxLen() const noexcept {  // for StrT
        return CastN(StrLen_t, get_Count());          // AKA Count
    }
    constexpr ITERATE_t GetSize() const noexcept {  // for cArray
        return CastN(ITERATE_t, get_Count());       // AKA Count
    }

    const TYPE* get_DataConst() const {
        return get_DataC<TYPE>();
    }
    operator const TYPE*() const noexcept {
        return get_DataConst();
    }

    cSpanUnk get_SpanUnk() const {
        return cSpanUnk(*this, sizeof(TYPE));
    }
    operator cSpanUnk() const {
        return get_SpanUnk();
    }

    inline bool IsValidIndex(COUNT_t i) const noexcept {
        return IS_INDEX_GOOD(i, get_Count());
    }
    ptrdiff_t GetIndexIn(const TYPE* p) const noexcept {
        // ASSERT(IsInternalPtr(p));
        return GET_INDEX_IN(get_DataConst(), p);
    }

    /// <summary>
    /// get a valid index.
    /// </summary>
    /// <param name="i">-1 = empty array.</param>
    /// <returns></returns>
    inline ITERATE_t ClampValidIndex(ITERATE_t i) const noexcept {
        if (i < 0) i = 0;
        const ITERATE_t nSize = GetSize();
        if (i >= nSize) return nSize - 1;
        return i;
    }
    void ThrowIfInvalidIndex(ITERATE_t nIndex) const {  // throw
        THROW_IF(!IsValidIndex(nIndex));
    }

    // Accessing elements
    const TYPE& GetAt(ITERATE_t nIndex) const noexcept {
        DEBUG_CHECK(IsValidIndex(nIndex));
        return this->get_DataConst()[nIndex];
    }
    const TYPE& GetAtSecure(ITERATE_t nIndex) const {  // throw
        //! throw an exception if we are out or range.
        ThrowIfInvalidIndex(nIndex);
        return this->get_DataConst()[nIndex];
    }
    const TYPE& GetAtHead() const {
        // AKA front()
        return this->GetAt(0);
    }
    const TYPE& GetAtTail() const {
        // AKA back()
        return this->GetAt(this->GetSize() - 1);
    }
 
    // overloaded operator helpers
    inline const TYPE& operator[](ITERATE_t nIndex) const {  // throw
        // Use GetAtSecure?
        return GetAt(nIndex);
    }

    typedef cIterator<const TYPE> const_iterator;  // like STL
    const_iterator begin() const noexcept {
        return const_iterator(get_DataC<TYPE>());
    }
    const_iterator end() const noexcept {
        return const_iterator(get_DataC<TYPE>() + get_Count());
    }
};

/// <summary>
/// A span (of TYPE) we might also write to.
/// </summary>
template <typename TYPE, class ARG_TYPE = const TYPE&>
class cSpanX : public cSpan<TYPE> {
    typedef cSpan<TYPE> SUPER_t;

 protected:
    /// <summary>
    /// Compare a data record to another data record. QSort
    /// ASSUME this is the same as comparing keys. Otherwise you must overload this.
    /// Default implementation. Override this for proper implementation. This probably won't work for most cases.
    /// </summary>
    /// <param name="Data1"></param>
    /// <param name="Data2"></param>
    /// <returns></returns>
    virtual COMPARE_t CompareData(ARG_TYPE data1, ARG_TYPE data2) const noexcept {
        // return cValT::Compare(Data1,Data2);
        return cMem::Compare(&data1, &data2, sizeof(data2));  // should we use cValT::Compare()??
    }

    ITERATE_t QSortPartition(ITERATE_t iLeft, ITERATE_t iRight);

    /// <summary>
    /// Sort a span.
    /// Re-sort- might have become unsorted for some reason.
    /// similar to std::sort()
    /// </summary>
    /// <param name="iLeft"></param>
    /// <param name="iRight"></param>
    void QSort(ITERATE_t iLeft, ITERATE_t iRight);

 public:
    constexpr cSpanX(const cMemSpan& span) noexcept : SUPER_t(span) {}
    constexpr cSpanX(const TYPE* pData = nullptr, COUNT_t nCount = 0) noexcept : SUPER_t(pData, nCount) {}

    TYPE* get_DataWork() {
        return get_DataW<TYPE>();
    }
    TYPE& ElementAt(ITERATE_t nIndex) {
        DEBUG_CHECK(IsValidIndex(nIndex));
        return get_DataWork()[nIndex];
    }
    inline TYPE& operator[](ITERATE_t nIndex) {  // throw
        // Use ElementAtSecure?
        return ElementAt(nIndex);
    }
    inline const TYPE& operator[](ITERATE_t nIndex) const {  // throw
        // Use GetAtSecure?
        return GetAt(nIndex);
    }

    using SUPER_t::begin;
    using SUPER_t::end;

    typedef cIterator<TYPE> iterator;  // like STL
    iterator begin() noexcept {
        return iterator(get_DataWork());
    }
    iterator end() noexcept {
        return iterator(get_DataWork() + get_Count());
    }

    /// <summary>
    /// Find the index of a specified entry arg. brute force.
    /// </summary>
    /// <param name="arg"></param>
    /// <returns>index. -1 = k_ITERATE_BAD = not found.</returns>
    ITERATE_t FindIFor(ARG_TYPE arg) const noexcept {
        const TYPE* p = this->get_DataConst();
        for (ITERATE_t nIndex = 0; nIndex < this->GetSize(); nIndex++) {
            if (p[nIndex] == arg) return nIndex;
        }
        return k_ITERATE_BAD;
    }
    /// <summary>
    /// Does the array contain this value?
    /// </summary>
    bool HasArg(ARG_TYPE arg) const noexcept {
        return FindIFor(arg) != k_ITERATE_BAD;
    }

    void put_Count2(COUNT_t count) noexcept {
        // Truncate this span.
        put_DataSize(count * sizeof(TYPE));
    }

    void SetAt(ITERATE_t nIndex, ARG_TYPE newElement) {  // throw
        //! @note Destructor is automatically called.
        DEBUG_CHECK(IsValidIndex(nIndex));
        this->get_DataWork()[nIndex] = newElement;  // may call a copy constructor.
    }
    void SetCopyAll(const TYPE* pData) noexcept {
        if (!isNull()) {
            cValSpan::CopyQty(get_DataWork(), pData, GetSize());
        }
    }
    ITERATE_t SetCopyQty(const TYPE* pData, ITERATE_t nQty) {
        nQty = cValT::Min(GetSize(), nQty);
        if (!isNull()) {
            cValSpan::CopyQty(get_DataWork(), pData, nQty);
        }
        return nQty;
    }
    ITERATE_t SetCopySpan(const cSpan<TYPE>& src) {
        return SetCopyQty(src.get_DataConst(), src.GetSize());
    }

    void Swap(ITERATE_t i, ITERATE_t j) {
        //! like cMemT::Swap(). dangerous for types that have pointers to themselves. self referenced.
        if (i == j) return;
        ASSERT(IsValidIndex(i));
        ASSERT(IsValidIndex(j));
        TYPE* p = get_DataWork();
        cMem::Swap(PtrCast<BYTE>(p + i), PtrCast<BYTE>(p + j), sizeof(TYPE));
    }

    /// <summary>
    /// shift the whole array. move an index to another place.
    /// Similar to Swap() but only one element is moved. (sort of)
    /// dangerous for types that have internal pointers !
    /// </summary>
    void MoveElement(ITERATE_t iFrom, ITERATE_t iTo) {  // throw
        ASSERT(IsValidIndex(iFrom));
        ASSERT(IsValidIndex(iTo));
        TYPE* p = get_DataWork();
        cValSpan::MoveElement1(&p[iFrom], &p[iTo]);
    }

    /// <summary>
    /// Get reference to element and throw an exception if we are out of range.
    /// </summary>
    TYPE& ElementAtSecure(ITERATE_t nIndex) {  // throw
        ThrowIfInvalidIndex(nIndex);
        return this->get_DataWork()[nIndex];
    }

    /// <summary>
    /// test is sorted? Allow dupes
    /// </summary>
    bool isSpanSorted() const;

    /// <summary>
    /// test is sorted? Allow NO dupes!
    /// </summary>
    bool isSpanSortedND() const;

    void QSort() {
        ITERATE_t iSize = this->GetSize() - 1;
        if (iSize <= 0) return;
        QSort(0, iSize);
    }
};

template <class TYPE, class ARG_TYPE>
ITERATE_t cSpanX<TYPE, ARG_TYPE>::QSortPartition(ITERATE_t iLeft, ITERATE_t iRight) {
    ASSERT(iLeft < iRight);
    for (;;) {
        // Do right side.
        while (iLeft < iRight && CompareData(this->ElementAt(iLeft), this->ElementAt(iRight)) <= COMPARE_Equal)  // skip stuff already in order.
            iRight--;
        if (iLeft >= iRight) break;
        this->Swap(iRight, iLeft);

        // Do left side.
        while (iLeft < iRight && CompareData(this->ElementAt(iLeft), this->ElementAt(iRight)) <= COMPARE_Equal) iLeft++;
        if (iLeft >= iRight) break;
        this->Swap(iLeft, iRight);
    }
    return iLeft;  // Next mid point.
}

template <class TYPE, class ARG_TYPE>
void cSpanX<TYPE, ARG_TYPE>::QSort(ITERATE_t iLeft, ITERATE_t iRight) {
    ITERATE_t iMid = QSortPartition(iLeft, iRight);
    if (iLeft < iMid - 1) QSort(iLeft, iMid - 1);
    if (iMid + 1 < iRight) QSort(iMid + 1, iRight);
}

template <class TYPE, class ARG_TYPE>
bool cSpanX<TYPE, ARG_TYPE>::isSpanSorted() const {
    ITERATE_t iQty = this->GetSize() - 1;
    for (ITERATE_t i = 0; i < iQty; i++) {
        const TYPE& a = this->GetAt(i);
        const TYPE& b = this->GetAt(i + 1);
        if (CompareData(a, b) > 0) return false;
    }
    return true;
}

template <class TYPE, class ARG_TYPE>
bool cSpanX<TYPE, ARG_TYPE>::isSpanSortedND() const {
    ITERATE_t iQty = this->GetSize() - 1;
    for (ITERATE_t i = 0; i < iQty; i++) {
        const TYPE& a = this->GetAt(i);
        const TYPE& b = this->GetAt(i + 1);
        if (CompareData(a, b) >= 0) return false;
    }
    return true;
}

/// <summary>
/// C++ can ONLY infer type as arg to function but NOT via constructor! no idea why.
/// </summary>
template <typename T>
constexpr cSpan<T> ToSpanSize(const T* p, size_t size) noexcept {
    return cSpan<T>(cMemSpan(p, size));
}
template <typename T>
constexpr cSpanX<T> ToSpanSize(T* p, size_t size) noexcept {
    return cSpanX<T>(cMemSpan(p, size));
}
template <typename T>
constexpr cSpan<T> ToSpan(const T* p, ITERATE_t count) noexcept {
    return cSpan<T>(p, count);
}
template <typename T>
constexpr cSpanX<T> ToSpan(T* p, ITERATE_t count) noexcept {
    return cSpanX<T>(p, count);
}
template <typename T>
constexpr cSpan<T> ToSpan(const cStrConst& c) noexcept {
    return cSpan<T>(c.GetT<T>(), c._Len);
}

#define TOSPAN(s) ToSpanSize((s), sizeof(s))    // Assume an array. AKA TOSPANA() ?
#define TOSPANT(v) ToSpanSize(&(v), sizeof(v))  // Assume typed value NOT an array.

// Convert a literal string to (read only) cMemSpan at compile time.
//! @note ONLY works for literal "static string", or BYTE[123] you cannot use a 'BYTE* x' here!
#define TOSPAN_LIT(s) ToSpan(s, STRMAX(s))

/// <summary>
/// Store an inline/static blob/block/span of memory of a specific known size. _COUNT in qty.
/// Act as union placeholder for the proper size of the dynamic element. (like arrays,blobs,etc). So we won't engage construct/destruct logic in union.
/// Like: stl::array
/// </summary>
template <COUNT_t _COUNT, typename TYPE = BYTE>
class cSpanStatic {
 protected:
    TYPE _Data[_COUNT];  /// All objects of this type are this size. Maybe construction or un-init data.

 protected:
    static inline ITERATE_t GetWrapIndex(ITERATE_t i) noexcept {
        return i % _COUNT;
    }

 public:
    static const size_t k_DataSize = _COUNT * sizeof(TYPE);  /// const size in bytes
    static const COUNT_t k_Count = _COUNT;                   /// const _COUNT

    constexpr const TYPE* get_DataC() const noexcept {
        return _Data;
    }
    constexpr TYPE* get_DataW() noexcept {
        return _Data;
    }
    operator const TYPE*() const noexcept {
        return _Data;  // default = read pointer.
    }
    constexpr cSpanX<TYPE> get_SpanMax() const noexcept {
        return cSpanX<TYPE>(_Data, _COUNT);
    }
    void SetZero() {
        cMem::Zero(_Data, k_DataSize);
    }
    void SetZeroSecure() {
        cMem::ZeroSecure(_Data, k_DataSize);
    }

    /// <summary>
    /// Get hex string. ASSUME pszHexString output is big enough! GetHexDigestSize()
    /// </summary>
    /// <param name="pszHexString"></param>
    /// <returns></returns>
    StrLen_t GetHexDigest(OUT char* pszHexString) const {
        return this->get_SpanMax().GetHexDigest(pszHexString);
    }
    HRESULT SetHexDigest(const char* pszHexString) {
        return this->get_SpanMax().SetHexDigest(pszHexString);
    }
};

#ifndef GRAY_STATICLIB                        // force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cSpanX<BYTE>;     // force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cSpanX<char>;     // force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cSpanX<wchar_t>;  // force implementation/instantiate for DLL/SO.
#endif

}  // namespace Gray
#endif
