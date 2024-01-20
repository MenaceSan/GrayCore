//
//! @file cSpan.h
//! specify a sized block of memory.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cSpan_H
#define _INC_cSpan_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cMem.h"
#include "cPtrFacade.h"
#include "cValArray.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4275)  // non dll-interface class 'Gray::cMemSpan' used as base for dll-interface class 'Gray::cBlob'
#endif

namespace Gray {
/// <summary>
/// A pointer to memory block/blob/span with known size and unknown ownership.
/// may be heap, stack, static or const based memory pointer. don't free on destruct. (although a derived class might)
/// May be static init? or UN-INIT!
/// </summary>
class cMemSpan {
    typedef cMemSpan THIS_t;
    size_t m_nSize;  /// size_t of m_pData in bytes.
    void* m_pData;   /// a block of memory of unknown ownership. Treat as MEMTYPE_t::_Temp

 protected:
    /// <summary>
    /// Set size in bytes but leave data pointer alone.
    /// </summary>
    void put_DataSize(size_t nSize) noexcept {
        m_nSize = nSize;
    }

 public:
    cMemSpan() noexcept : m_nSize(0), m_pData(nullptr) {}
    constexpr cMemSpan(const void* pData, size_t nSize) noexcept : m_nSize(nSize), m_pData(nSize ? const_cast<void*>(pData) : nullptr) {
        // just assume we don't modify it?
        // read only. SetSpanConst
        DEBUG_CHECK(isValid());
    }
    cMemSpan(const cMemSpan& spn) noexcept {
        // Just shared pointers. This may be dangerous!
        m_nSize = spn.m_nSize;
        m_pData = spn.m_pData;
        DEBUG_CHECK(isValid());
    }
    explicit cMemSpan(const cMemSpan* pBlock) noexcept {
        // Just shared pointers. This may be dangerous!
        m_nSize = (pBlock == nullptr) ? 0 : pBlock->m_nSize;
        m_pData = (pBlock == nullptr) ? nullptr : pBlock->m_pData;
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
        return PtrCast<TYPE2>(m_pData);
    }
    /// <summary>
    /// Get a writable arbitrary TYPE2 pointer.
    /// </summary>
    template <typename TYPE2 = BYTE>
    inline TYPE2* get_DataW() noexcept {
        return PtrCast<TYPE2>(m_pData);
    }

    /// <summary>
    /// Get a Non-const pointer that we do not actually expect to write to ?! Some APIs seem to want this.
    /// </summary>
    /// <typeparam name="TYPE2"></typeparam>
    /// <returns></returns>
    template <typename TYPE2 = BYTE>
    inline TYPE2* get_DataNC() const noexcept {
        return PtrCast<TYPE2>(m_pData);
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
    /// Is in a valid state? nullptr = a valid state. Forbid: 0 sized isValidPtr !
    /// </summary>
    constexpr inline bool isValid() const noexcept {
        if (isNull()) return true;
        return isValidPtr() && m_nSize != 0;
    }

    /// <summary>
    /// Is byte offset inside the known valid range for the block? exclusive.
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

    /// <summary>
    /// Is p inside the known valid range for the block? Exclusive = Cant be equal to end!
    /// </summary>
    inline bool IsInternalPtr(const void* p) const noexcept {
        return IsInSize(cMem::Diff(p, m_pData));
    }

    /// <summary>
    /// Is p inside the known valid range for the block? Inclusive = Can be equal to end.
    /// </summary>
    inline bool IsInternalPtr2(const void* p) const noexcept {
        return IsLTESize(cMem::Diff(p, m_pData));
    }

    bool IsZeros() const noexcept {
        return cMem::IsZeros(m_pData, m_nSize);
    }

    /// <summary>
    /// compare spans of data.
    /// </summary>
    bool IsEqualSpan(const void* pData, size_t nSize) const noexcept {
        return m_nSize == nSize && cMem::IsEqual(m_pData, pData, nSize);
    }
    /// <summary>
    /// compare blocks of data.
    /// </summary>
    bool IsEqualSpan(const THIS_t* pData) const noexcept {
        if (pData == nullptr) return false;  // isValidPtr() ?
        return IsEqualSpan(pData->m_pData, pData->m_nSize);
    }
    /// <summary>
    /// compare blocks of data.
    /// </summary>
    bool IsEqualSpan(const THIS_t& data) const noexcept {
        return IsEqualSpan(data.m_pData, data.m_nSize);
    }

    /// <summary>
    /// get pointer that is good/valid for just one byte in the span.
    /// </summary>
    /// <param name="nOffset"></param>
    const BYTE* GetSpan1(size_t nOffset) const noexcept {
        if (!IsInSize(nOffset)) {
            DEBUG_CHECK(false);
            return nullptr;
        }
        return get_DataC<BYTE>() + nOffset;
    }
    /// <summary>
    /// Get a pointer into the buffer as a byte pointer.
    /// Ensure the data is valid to size_t !
    /// </summary>
    /// <param name="nOffset"></param>
    /// <param name="size"></param>
    /// <returns></returns>
    const BYTE* GetSpan(size_t nOffset, size_t size) const noexcept {
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
    const void* get_SpanEnd() const noexcept {
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
    /// Advance the span and shrink it.
    void SetSpanSkip(size_t nSize) {
        ASSERT(nSize <= m_nSize);
        m_pData = get_DataW<BYTE>() + nSize;
        m_nSize -= nSize;
    }
    /// set a writable span.
    void SetSpan(void* pData, size_t nSize) noexcept {
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

    void SetZeros() noexcept {
        cMem::ZeroSecure(m_pData, m_nSize);
    }
    void SetCopyAll(const void* pData) {
        cMem::Copy(m_pData, pData, m_nSize);
    }
};
#define TOSPAN(s) cMemSpan(s, sizeof(s))
#define TOSPANR(v) cMemSpan(&v, sizeof(v))

template <typename TYPE>
class cSpan : public cMemSpan {
    typedef cSpan<TYPE> THIS_t;

 protected:
    void put_Count(COUNT_t count) {
        put_DataSize(count * sizeof(TYPE));
    }

 public:
    cSpan(const TYPE* pData = nullptr, COUNT_t nCount = 0) noexcept : cMemSpan(pData, nCount * sizeof(TYPE)) {}

    inline COUNT_t get_Count() const noexcept {
        return get_DataSize() / sizeof(TYPE);
    }
    inline ITERATE_t GetSize() const noexcept {
        return CastN(ITERATE_t, get_Count());  // AKA Count
    }

    const TYPE* get_DataConst() const {
        return get_DataC<TYPE>();
    }
    TYPE* get_DataWork() {
        return get_DataW<TYPE>();
    }
    operator const TYPE*() const noexcept {
        return get_DataC<TYPE>();
    }

    inline bool IsValidIndex(COUNT_t i) const noexcept {
        return IS_INDEX_GOOD(i, get_Count());
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

    TYPE& ElementAt(ITERATE_t nIndex) {
        DEBUG_CHECK(IsValidIndex(nIndex));
        return this->get_DataWork()[nIndex];
    }
    /// <summary>
    /// Get reference to element and throw an exception if we are out or range.
    /// </summary>
    TYPE& ElementAtSecure(ITERATE_t nIndex) {  // throw
        ThrowIfInvalidIndex(nIndex);
        return this->get_DataWork()[nIndex];
    }

    // overloaded operator helpers
    inline TYPE& operator[](ITERATE_t nIndex) {  // throw
        // Use ElementAtSecure?
        return ElementAt(nIndex);
    }
    inline const TYPE& operator[](ITERATE_t nIndex) const {  // throw
        // Use GetAtSecure?
        return GetAt(nIndex);
    }

    typedef cIterator<TYPE> iterator;              // like STL
    typedef cIterator<const TYPE> const_iterator;  // like STL
    iterator begin() noexcept {
        return iterator(get_DataW<TYPE>());
    }
    iterator end() noexcept {
        return iterator(get_DataW<TYPE>() + get_Count());
    }
    const_iterator begin() const noexcept {
        return const_iterator(get_DataC<TYPE>());
    }
    const_iterator end() const noexcept {
        return const_iterator(get_DataC<TYPE>() + get_Count());
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
        cValArray::MoveElement1(&p[iFrom], &p[iTo]);
    }
};

// Convert a literal string to cMemSpan at compile time.
//! @note ONLY works for literal "static string", or BYTE[123] you cannot use a 'BYTE* x' here!
#define TOSPAN_LIT(s) cSpan<char>(s, STRMAX(s))

/// <summary>
/// A span (of TYPE) we might also write to.
/// </summary>
template <typename TYPE, class ARG_TYPE = const TYPE&>
class cSpan2 : public cSpan<TYPE> {
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
    cSpan2(const TYPE* pData = nullptr, COUNT_t nCount = 0) noexcept : SUPER_t(pData, nCount) {}

    void SetAt(ITERATE_t nIndex, ARG_TYPE newElement) {  // throw
        //! @note Destructor is automatically called.
        DEBUG_CHECK(IsValidIndex(nIndex));
        this->get_DataWork()[nIndex] = newElement;  // may call a copy constructor.
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
ITERATE_t cSpan2<TYPE, ARG_TYPE>::QSortPartition(ITERATE_t iLeft, ITERATE_t iRight) {
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
void cSpan2<TYPE, ARG_TYPE>::QSort(ITERATE_t iLeft, ITERATE_t iRight) {
    ITERATE_t iMid = QSortPartition(iLeft, iRight);
    if (iLeft < iMid - 1) QSort(iLeft, iMid - 1);
    if (iMid + 1 < iRight) QSort(iMid + 1, iRight);
}

template <class TYPE, class ARG_TYPE>
bool cSpan2<TYPE, ARG_TYPE>::isSpanSorted() const {
    ITERATE_t iQty = this->GetSize() - 1;
    for (ITERATE_t i = 0; i < iQty; i++) {
        const TYPE& a = this->GetAt(i);
        const TYPE& b = this->GetAt(i + 1);
        if (CompareData(a, b) > 0) return false;
    }
    return true;
}

template <class TYPE, class ARG_TYPE>
bool cSpan2<TYPE, ARG_TYPE>::isSpanSortedND() const {
    ITERATE_t iQty = this->GetSize() - 1;
    for (ITERATE_t i = 0; i < iQty; i++) {
        const TYPE& a = this->GetAt(i);
        const TYPE& b = this->GetAt(i + 1);
        if (CompareData(a, b) >= 0) return false;
    }
    return true;
}

/// <summary>
/// Store an inline/static blob/block/span of memory of a specific known size. _COUNT in qty.
/// Act as union placeholder for the proper size of the dynamic element. (like arrays,blobs,etc). So we won't engage construct/destruct logic in union.
/// Like: stl::array
/// </summary>
template <COUNT_t _COUNT, typename TYPE = BYTE>
class cSpanStatic {
 protected:
    TYPE _Data[_COUNT];  /// All objects of this type are this size. Maybe construction or un-init data.
 public:
    static const size_t k_DataSize = _COUNT * sizeof(TYPE);  /// const size in bytes
    static const COUNT_t k_Count = _COUNT;                   /// const _COUNT

    inline const TYPE* get_DataC() const noexcept {
        return _Data;
    }
    inline TYPE* get_DataW() noexcept {
        return _Data;
    }
    operator const void*() const noexcept {
        return _Data;
    }
    operator const TYPE*() const noexcept {
        return _Data;
    }
    cSpan<TYPE> get_Span() const noexcept {
        return cSpan<TYPE>(_Data, _COUNT);
    }
};

#ifndef GRAY_STATICLIB                        // force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cSpan<BYTE>;     // force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cSpan<char>;     // force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cSpan<wchar_t>;  // force implementation/instantiate for DLL/SO.
#endif

}  // namespace Gray
#endif
