//! @file cSpan.h
//! specify a sized block of memory.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cSpan_H
#define _INC_cSpan_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cMemSpan.h"
#include "cPtrFacade.h"
#include "cValSpan.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4275)  // non dll-interface class 'Gray::cMemSpan' used as base for dll-interface class 'Gray::cBlob'
#endif

namespace Gray {
/// <summary>
/// a span of some unknown type. Element size (stride) is known only at run time.
/// can be used with StrT::TableFind*
/// </summary>
struct cSpanUnk : public cMemSpan {
    const size_t _Stride;  /// each element is of this size. AKA pitch. Known at run time.

    cSpanUnk(const cMemSpan& m, size_t stride) noexcept : cMemSpan(m), _Stride(stride) {
        DEBUG_CHECK(stride > 0);  // do we allow misalignment ? extra bytes ? GetOverflow
    }
    cSpanUnk(const void* p, COUNT_t count, size_t stride) noexcept : cSpanUnk(cMemSpan(p, count * stride), stride) {}

    /// <summary>
    /// Is this array bigger that whole element of _Stride?
    /// </summary>
    /// <returns></returns>
    size_t get_Overflow() const noexcept {
        return this->get_SizeBytes() % _Stride;
    }

    constexpr COUNT_t get_Count() const noexcept {
        return get_SizeBytes() / _Stride;
    }
    constexpr ITERATE_t GetSize() const noexcept {  // for cArray
        return CastN(ITERATE_t, get_Count());       // AKA Count
    }

    constexpr size_t GetBytesOffset(ITERATE_t i) const noexcept {
        return i * _Stride;
    }
    inline bool IsValidIndex(ITERATE_t i) const noexcept {
        return IS_INDEX_GOOD(GetBytesOffset(i), get_SizeBytes());
    }

    inline const void* GetElemV(ITERATE_t i) const {
        ASSERT(!isNull());
        ASSERT(IsValidIndex(i));
        return GetTPtrC<BYTE>() + GetBytesOffset(i);
    }

    /// get type cast element. e.g. String searching. const
    /// @note sizeof(T) > _Stride is intentionally allowed!
    template <typename T>
    inline const T* GRAYCALL GetElemT(ITERATE_t i) const {
        return PtrCast<T>(GetElemV(i));
    }

    void ReverseSpan() {
        ASSERT(get_Overflow() == 0);  // MUST be aligned!
        cMemSpan::ReverseSpan(_Stride);
    }
};

/// <summary>
/// A Span of some known TYPE. Probably read only. like std::span.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <typename TYPE>
struct cSpan : public cMemSpan {
    typedef cMemSpan SUPER_t;
    typedef cSpan<TYPE> THIS_t;
    typedef TYPE ELEM_t;

    // static const size_t k_Stride = sizeof(TYPE);

    constexpr cSpan() noexcept {}
    constexpr cSpan(const TYPE* pData, COUNT_t nCount) noexcept : SUPER_t(pData, nCount * sizeof(TYPE)) {}
    constexpr cSpan(const TYPE* pData, const TYPE* pDataEnd) noexcept : SUPER_t(pData, cValSpan::Diff(pDataEnd, pData) * sizeof(TYPE)) {}

    constexpr explicit cSpan(const cMemSpan& span) noexcept : SUPER_t(span) {}

    constexpr COUNT_t get_Count() const noexcept {
        return get_SizeBytes() / sizeof(TYPE);
    }
    constexpr StrLen_t get_MaxLen() const noexcept {  // for StrT
        return CastN(StrLen_t, get_Count());          // AKA Count
    }
    constexpr ITERATE_t GetSize() const noexcept {  // for cArray
        return CastN(ITERATE_t, get_Count());       // AKA Count
    }

    constexpr TYPE* get_PtrW() noexcept {
        return GetTPtrW<TYPE>();
    }
    constexpr const TYPE* get_PtrConst() const noexcept {
        return GetTPtrC<TYPE>();
    }
    operator const TYPE*() const noexcept {
        return get_PtrConst();
    }

    /// Get a dynamic/unknown span from this static/known span.
    cSpanUnk get_SpanUnk() const {
        return cSpanUnk(*this, sizeof(TYPE));
    }
    operator cSpanUnk() const {
        return get_SpanUnk();
    }

    constexpr size_t GetBytesOffset(ITERATE_t i) const noexcept {
        return i * sizeof(TYPE);
    }
    inline bool IsValidIndex(ITERATE_t i) const noexcept {
        return IS_INDEX_GOOD(GetBytesOffset(i), get_SizeBytes());
    }
    ptrdiff_t GetIndexIn(const TYPE* p) const noexcept {
        // ASSERT(IsInternalPtr(p));
        return GET_INDEX_IN(get_PtrConst(), p);
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
        return this->get_PtrConst()[nIndex];
    }
    const TYPE& GetAtSecure(ITERATE_t nIndex) const {  // throw
        //! throw an exception if we are out or range.
        ThrowIfInvalidIndex(nIndex);
        return this->get_PtrConst()[nIndex];
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
        return this->GetAt(nIndex);
    }

    typedef cIterator<const TYPE> const_iterator;  // like STL
    const_iterator begin() const noexcept {
        return const_iterator(GetTPtrC<TYPE>());
    }
    const_iterator end() const noexcept {
        return const_iterator(GetTPtrC<TYPE>() + get_Count());
    }

    /// Advance the span and shrink it.
    inline void SetSkip(ITERATE_t nSize) {
        SetSkipBytes(nSize * sizeof(TYPE));
    }
    THIS_t GetSkip(ITERATE_t nSize) const {
        ASSERT(nSize <= GetSize());
        return THIS_t(get_PtrConst() + nSize, GetSize() - nSize);
    }
};

/// <summary>
/// A span (of TYPE) we might also write to.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="ARG_TYPE">const ref, value or pointer to something contained by TYPE.</typeparam>
template <typename TYPE, typename ARG_TYPE = const TYPE&>
struct cSpanX : public cSpan<TYPE> {
    typedef cSpan<TYPE> SUPER_t;
    typedef cSpanX<TYPE, ARG_TYPE> THIS_t;
    typedef TYPE ELEM_t;
    typedef ARG_TYPE ARG_t;

    constexpr cSpanX(const cMemSpan& span) noexcept : SUPER_t(span) {}
    constexpr cSpanX(const TYPE* pData = nullptr, COUNT_t nCount = 0) noexcept : SUPER_t(pData, nCount) {}

    /// Find strict equal. Not sorted.
    ITERATE_t FindIFor3(ARG_TYPE arg) const noexcept {
        for (ITERATE_t i = this->GetSize(); i;) {
            if (IsEqual3<ARG_TYPE>(this->GetAt(--i), arg)) return i;  // operator == ??
        }
        return k_ITERATE_BAD;
    }

    /// <summary>
    /// Does the array contain this strict value? Not sorted.
    /// </summary>
    bool HasArg3(ARG_TYPE arg) const noexcept {
        return FindIFor3(arg) != k_ITERATE_BAD;
    }

    TYPE* get_PtrWork() noexcept {
        // return PtrCast<TYPE>(this->get_BytePtrW());
        return this->template GetTPtrW<TYPE>();
    }
    TYPE& ElementAt(ITERATE_t nIndex) {
        DEBUG_CHECK(IsValidIndex(nIndex));
        return get_PtrWork()[nIndex];
    }
    inline TYPE& operator[](ITERATE_t nIndex) {  // throw
        // Use ElementAtSecure?
        return this->ElementAt(nIndex);
    }
    inline const TYPE& operator[](ITERATE_t nIndex) const {  // throw
        // Use GetAtSecure?
        return this->GetAt(nIndex);
    }

    using SUPER_t::begin;
    using SUPER_t::end;

    typedef cIterator<TYPE> iterator;  // like STL
    iterator begin() noexcept {
        return iterator(get_PtrWork());
    }
    iterator end() noexcept {
        return iterator(get_PtrWork() + this->get_Count());
    }

    void SetAt(ITERATE_t nIndex, ARG_TYPE newElement) {  // throw
        //! @note Destructor is automatically called.
        DEBUG_CHECK(this->IsValidIndex(nIndex));
        this->get_PtrWork()[nIndex] = newElement;  // may call a copy constructor.
    }
    void SetCopyAll(const TYPE* pData) noexcept {
        if (this->isNull()) return;
        cValSpan::CopyQty(get_PtrWork(), pData, this->GetSize());
    }
    ITERATE_t SetCopyQty(const TYPE* pData, ITERATE_t nQty) {
        nQty = cValT::Min(this->GetSize(), nQty);
        if (this->isNull() || pData == nullptr) return nQty;  // how much might i write if not null.
        cValSpan::CopyQty(get_PtrWork(), pData, nQty);
        return nQty;
    }
    ITERATE_t SetCopySpan(const cSpan<TYPE>& src) {
        return SetCopyQty(src.get_PtrConst(), src.GetSize());
    }

    void Swap(ITERATE_t i, ITERATE_t j) {
        //! like cMem::SwapT(). dangerous for types that have pointers to themselves. self referenced.
        if (i == j) return;
        ASSERT(this->IsValidIndex(i));
        ASSERT(this->IsValidIndex(j));
        TYPE* p = get_PtrWork();
        cMem::Swap(PtrCast<BYTE>(p + i), PtrCast<BYTE>(p + j), sizeof(TYPE));
    }

    /// <summary>
    /// shift the whole array. move an index to another place.
    /// Similar to Swap() but only one element is moved. (sort of)
    /// dangerous for types that have internal pointers !
    /// </summary>
    void ShiftElements(ITERATE_t iFrom, ITERATE_t iTo) {  // throw
        ASSERT(this->IsValidIndex(iFrom));
        ASSERT(this->IsValidIndex(iTo));
        TYPE* p = this->get_PtrWork();
        cValSpan::ShiftElements(&p[iFrom], &p[iTo]);
    }

    /// <summary>
    /// reverse the order of an array of a TYPE. similar to cMem::ReverseArrayBlocks but iBlockSize==sizeof(TYPE)
    /// TYPE = BYTE = reverse bytes in a block. similar to htonl(), etc.
    /// Use ReverseBytes if TYPE = BYTE and nArraySizeBytes <= largest intrinsic type size?
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pArray"></param>
    /// <param name="nArraySizeBytes"></param>
    void ReverseSpan() noexcept {
        TYPE* pMemBS = this->get_PtrWork();
        TYPE* pMemBE = pMemBS + (this->GetSize() - 1);
        for (; pMemBS < pMemBE; pMemBS++, pMemBE--) {
            cMem::SwapT<TYPE>(*pMemBS, *pMemBE);
        }
    }

    /// <summary>
    /// Get reference to element and throw an exception if we are out of range.
    /// </summary>
    TYPE& ElementAtSecure(ITERATE_t nIndex) {  // throw
        this->ThrowIfInvalidIndex(nIndex);
        return this->get_PtrWork()[nIndex];
    }

    void put_Count2(COUNT_t count) noexcept {
        // Truncate this span.
        this->put_SizeBytes(count * sizeof(TYPE));
    }
    THIS_t GetSkip(ITERATE_t nSizeChange) const {
        ASSERT(nSizeChange <= this->GetSize());
        return THIS_t(this->get_PtrConst() + nSizeChange, this->GetSize() - nSizeChange);
    }
};

template <typename TYPE, typename ARG_TYPE = const TYPE&>
struct cSpanSearchable : public cSpanX<TYPE, ARG_TYPE> {
    typedef cSpanX<TYPE, ARG_TYPE> SUPER_t;

    cSpanSearchable() noexcept {}
    cSpanSearchable(const cMemSpan& m) noexcept : SUPER_t(m) {}

    /// <summary>
    /// Compare a data record to another data record. for QSort.
    /// derived class decides how (on what internal key) the span is sorted/compared.
    /// </summary>
    /// <param name="Data1"></param>
    /// <param name="Data2"></param>
    /// <returns></returns>
    virtual COMPARE_t CompareElems(ARG_TYPE data1, ARG_TYPE data2) const noexcept {
        return cValT::Compare(data1, data2);  // default type based compare.
    }

    /// <summary>
    /// Find the index of a specified entry arg. brute force. Not sorted.
    /// </summary>
    /// <param name="arg"></param>
    /// <returns>index. -1 = k_ITERATE_BAD = not found.</returns>
    ITERATE_t FindIForN(ARG_TYPE arg) const noexcept {
        for (ITERATE_t i = this->GetSize(); i;) {
            if (COMPARE_Equal == CompareElems(this->GetAt(--i), arg)) return i;
        }
        return k_ITERATE_BAD;
    }

    /// <summary>
    /// Does the array contain this value? Not sorted.
    /// </summary>
    bool HasArgN(ARG_TYPE arg) const noexcept {
        return FindIForN(arg) != k_ITERATE_BAD;
    }
};

/// <summary>
/// A span that MAY be sorted. base class.
/// </summary>
template <typename TYPE, typename ARG_TYPE = const TYPE&>
class cSpanSorted : public cSpanSearchable<TYPE, ARG_TYPE> {
 protected:
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
    /// <summary>
    /// Do a binary search for the elements key. Sorted Array.
    /// </summary>
    /// <param name="argFind"></param>
    /// <param name="riCompareRes">COMPARE_t
    ///		 0 = match with index. we may allow duplicates?
    ///		-1 = key is less than index. COMPARE_Less
    ///		+1 = key is greater than index</param>
    /// <returns>ITERATE_t</returns>
    ITERATE_t FindINearS(ARG_TYPE argFind, OUT COMPARE_t& riCompareRes) const noexcept {
        if (this->isEmpty()) {
            riCompareRes = COMPARE_Less;
            return 0;
        }

        ITERATE_t iHigh = this->GetSize() - 1;
        ITERATE_t iLow = 0;
        ITERATE_t i = 0;
        COMPARE_t iCompareRes = COMPARE_Less;
        while (iLow <= iHigh) {
            i = (iHigh + iLow) / 2;
            iCompareRes = this->CompareElems(argFind, this->GetAt(i));  // virtual call.
            if (iCompareRes == COMPARE_Equal) break;
            if (iCompareRes > 0) {
                iLow = i + 1;
            } else {
                iHigh = i - 1;
            }
        }
        riCompareRes = iCompareRes;
        return i;
    }

    /// <summary>
    /// Does the array contain this value? sorted.
    /// </summary>
    bool HasArgS(ARG_TYPE argFind) const noexcept {
        COMPARE_t iCompareRes;
        const ITERATE_t i = FindINearS(argFind, OUT iCompareRes);
        return iCompareRes == COMPARE_Equal && i >= 0;
    }

    /// <summary>
    /// is sorted? Allow dupes
    /// </summary>
    bool isSpanSorted() const;

    /// <summary>
    /// is sorted? Allow NO dupes!
    /// </summary>
    bool isSpanSortedND() const;

    void QSort() {
        if (this->isEmpty()) return;
        QSort(0, this->GetSize() - 1);
    }
};

template <class TYPE, class ARG_TYPE>
ITERATE_t cSpanSorted<TYPE, ARG_TYPE>::QSortPartition(ITERATE_t iLeft, ITERATE_t iRight) {
    ASSERT(iLeft < iRight);
    for (;;) {
        // Do right side.
        while (iLeft < iRight && CompareElems(this->ElementAt(iLeft), this->ElementAt(iRight)) <= COMPARE_Equal) iRight--;  // skip stuff already in order.
        if (iLeft >= iRight) break;
        this->Swap(iRight, iLeft);

        // Do left side.
        while (iLeft < iRight && CompareElems(this->ElementAt(iLeft), this->ElementAt(iRight)) <= COMPARE_Equal) iLeft++;
        if (iLeft >= iRight) break;
        this->Swap(iLeft, iRight);
    }
    return iLeft;  // Next mid point.
}

template <class TYPE, class ARG_TYPE>
void cSpanSorted<TYPE, ARG_TYPE>::QSort(ITERATE_t iLeft, ITERATE_t iRight) {
    ITERATE_t iMid = QSortPartition(iLeft, iRight);
    if (iLeft < iMid - 1) QSort(iLeft, iMid - 1);
    if (iMid + 1 < iRight) QSort(iMid + 1, iRight);
}

template <class TYPE, class ARG_TYPE>
bool cSpanSorted<TYPE, ARG_TYPE>::isSpanSorted() const {
    if (this->isEmpty()) return true;
    const ITERATE_t iQty = this->GetSize() - 1;
    for (ITERATE_t i = 0; i < iQty; i++) {
        const TYPE& a = this->GetAt(i);
        const TYPE& b = this->GetAt(i + 1);
        if (CompareElems(a, b) > 0) return false;  // allow dupes
    }
    return true;
}

template <class TYPE, class ARG_TYPE>
bool cSpanSorted<TYPE, ARG_TYPE>::isSpanSortedND() const {
    if (this->isEmpty()) return true;
    const ITERATE_t iQty = this->GetSize() - 1;
    for (ITERATE_t i = 0; i < iQty; i++) {
        const TYPE& a = this->GetAt(i);
        const TYPE& b = this->GetAt(i + 1);
        if (CompareElems(a, b) >= 0) return false;  // no dupes.
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
constexpr cSpanX<T*, T*> ToSpanSize(T** p, size_t size) noexcept {
    return cSpanX<T*, T*>(cMemSpan(p, size));
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
constexpr cSpan<T> ToSpanStr(const cStrConst& c) noexcept {
    return cSpan<T>(c.get_T<T>(), c._Len);
}
template <typename T>
constexpr cSpan<T> ToSpanZ(const cStrConst& c) noexcept {
    return cSpan<T>(c.get_T<T>(), c._Len + 1);
}

#define TOSPAN(s) ToSpanSize((s), sizeof(s))  // Assume an array. AKA TOSPANA() ? NOT cSpanUnk.
#define TOSPANT(v) cMemSpan(&(v), sizeof(v))  // Assume typed value NOT an array.

/// Convert a literal string to (read only) cMemSpan at compile time.
/// @note ONLY works for literal "static string", or BYTE[123] you cannot use a 'BYTE* x' here!
#define TOSPAN_LIT(s) ToSpan(s, STRMAX(s))

/// <summary>
/// Store an inline/static blob/block/span of memory of a specific known (at compile time) size. _COUNT in qty.
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
    static const size_t k_SizeBytes = _COUNT * sizeof(TYPE);  /// const size in bytes = sizeof(*this)
    static const COUNT_t k_Count = _COUNT;                    /// const _COUNT of TYPE

    constexpr const TYPE* GetPtrC() const noexcept {
        return _Data;
    }
    constexpr TYPE* GetPtrNC() const noexcept {
        return const_cast<TYPE*>(_Data);
    }
    constexpr TYPE* GetPtrW() noexcept {
        return _Data;
    }
    operator const TYPE*() const noexcept {
        return _Data;  // default = read pointer.
    }
    constexpr cSpanX<TYPE> get_SpanMax() const noexcept {
        return cSpanX<TYPE>(_Data, _COUNT);
    }
    void SetZero() {
        cMem::Zero(_Data, k_SizeBytes);
    }
    void SetZeroSecure() {
        cMem::ZeroSecure(_Data, k_SizeBytes);
    }

    /// <summary>
    /// Get hex string. ASSUME pszHexString output is big enough! GetHexDigestSize()
    /// </summary>
    /// <param name="hexStr"></param>
    /// <returns></returns>
    StrLen_t GetHexDigest(cMemSpan hexStr) const {
        return this->get_SpanMax().GetHexDigest(hexStr);
    }
    HRESULT ReadHexDigest(const char* pszHexString) {
        return this->get_SpanMax().ReadHexDigest(pszHexString);
    }
};

#ifndef GRAY_STATICLIB                          // force implementation/instantiate for DLL/SO.
template struct GRAYCORE_LINK cSpanX<BYTE>;     // force implementation/instantiate for DLL/SO.
template struct GRAYCORE_LINK cSpanX<char>;     // force implementation/instantiate for DLL/SO.
template struct GRAYCORE_LINK cSpanX<wchar_t>;  // force implementation/instantiate for DLL/SO.
#endif

}  // namespace Gray
#endif
