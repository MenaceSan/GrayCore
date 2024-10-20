//! @file cArray.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cArray_H
#define _INC_cArray_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"  // THROW_IF()
#include "cHeap.h"
#include "cPtrFacade.h"
#include "cSpan.h"

namespace Gray {
/// <summary>
/// Minimal/Base array template of elements. like MFC version.
/// @note MFC 8.0 uses INT_PTR for GetSize()
/// </summary>
/// <typeparam name="SPAN_TYPE">what is stored. cSpan</typeparam>
template <class SPAN_TYPE>
class cArrayImpl : public SPAN_TYPE {
    typedef SPAN_TYPE SUPER_t;
    typedef cArrayImpl<SPAN_TYPE> THIS_t;

 public:
    typedef typename SPAN_TYPE::ELEM_t ELEM_t;
    typedef typename SPAN_TYPE::ARG_t ARG_t;

 protected:
    // Don't allow public access to some cMemSpan methods.
    void SetSpanNull() = delete;
    void SetSpanConst(const void* pData, size_t nSize) = delete;
    void SetSpan2(void* pData, size_t nSize) = delete;
    void SetSpan(const SUPER_t& a) = delete;
    void SetSkipBytes(size_t nSize) = delete;

 public:
    cArrayImpl() noexcept {}

    /// <summary>
    /// copy constructor.
    /// </summary>
    cArrayImpl(const THIS_t& rCopy) : SUPER_t() {
        this->SetCopy(rCopy);
    }

    /// <summary>
    /// set Size to iSize empty entries.
    /// </summary>
    explicit cArrayImpl(ITERATE_t iSize) : SUPER_t() {
        this->SetSize(iSize);
    }

#if 0
    /// <summary>
    /// move constructor.
    /// </summary>
    cArrayImpl(THIS_t&& ref) noexcept : SUPER_t(ref) {
        ref.SUPER_t::SetSpanNull();
    }
#endif

    ~cArrayImpl() {
        RemoveAll();
    }

 public:
    void RemoveAll();
    void SetSize(ITERATE_t nSizeNew);
    void SetCopy(const THIS_t& aValues) {
        if (this == &aValues) return;
        this->SetSize(aValues.GetSize());  // This will call empty constructors.
        this->SetCopyAll(aValues);         // will call destruct on overwrite if needed.
    }

    //**************************
    // Base pointer

    /// <summary>
    /// Make sure the alloc is actually bigger than the declared/requested size.
    /// </summary>
    bool IsValidHeapSize() const noexcept;

    bool isValidCheck() const noexcept {
        // IsValidCast(THIS_t,this);
        if (!this->IsValidHeapSize()) return false;
        return true;
    }

    //**************************
    // Size

    /// <summary>
    /// Get sizeof all children alloc(s). not size of *this
    /// </summary>
    size_t CountHeapStats(OUT ITERATE_t& iAllocCount) const noexcept {
        if (this->isEmpty()) return 0;
        iAllocCount++;  // just the alloc for the array
        return cHeap::GetSize(this->get_PtrConst());
    }

    /// <summary>
    /// Get quantity of objects truly allocated. (may not be same as Size or even properly aligned with ELEM_t)
    /// like STL capacity()
    /// </summary>
    ITERATE_t get_HeapCount() const noexcept {
        return CastN(ITERATE_t, cHeap::GetSize(this->get_PtrConst()) / sizeof(ELEM_t));
    }

    /// <summary>
    /// over allocate to allow room to grow.
    /// </summary>
    constexpr static ITERATE_t GetHeapCountChunk(ITERATE_t i) {
        return i + (i / 16);
    }

    //********************************
    // elements

    /// Potentially growing the array
    void SetAtGrow(ITERATE_t nIndex, ARG_t newElement) {
        // ASSERT_VALID(this);
        if (nIndex >= this->GetSize()) SetSize(nIndex + 1);  // must grow.
        this->SetAt(nIndex, newElement);
    }

    /// <summary>
    /// Add to the end. AKA push_back(), Push()
    /// </summary>
    ITERATE_t Add(ARG_t newElement) {
        const ITERATE_t nIndex = this->GetSize();
        SetAtGrow(nIndex, newElement);
        return nIndex;
    }

    const THIS_t& operator=(const THIS_t& aValues) {  // otherwise this is considered deleted ?
        SetCopy(aValues);
        return *this;
    }

    // Operations that move elements around
    void InsertAt(ITERATE_t nIndex, ARG_t newElement);

    /// <summary>
    /// remove element at index.
    /// </summary>
    /// <param name="nIndex"></param>
    /// <returns>true = removed. false = was not here.</returns>
    bool RemoveAt(ITERATE_t nIndex);

    void RemoveAt(ITERATE_t nIndex, ITERATE_t iQty);

    /// <summary>
    /// Add to head. NOT like normal stack or queue. Adds are usually to the tail.
    /// </summary>
    /// <param name="newElement"></param>
    void AddHead(ARG_t newElement) {
        this->InsertAt(0, newElement);
    }

    /// <summary>
    /// Insert an array into this array. Like MFC CArray::Append() ? sort of. Emulate MFC. but also resolve overload conflict.
    /// </summary>
    /// <param name="i">point of insert</param>
    /// <param name="src"></param>
    void InsertArray(ITERATE_t i, const cSpan<ELEM_t>& src);

    void RemoveLast() {
        if (this->isEmpty()) return;
        this->RemoveAt(this->GetSize() - 1);
    }
    ELEM_t PopHead() {
        // pop from front of queue.
        ASSERT(!this->isEmpty());
        ELEM_t tmp = this->GetAt(0);  // copy it.
        this->RemoveAt(0);
        return tmp;
    }
    ELEM_t PopTail() {
        // pop from top of stack.
        // AKA Pop()
        ASSERT(!this->isEmpty());
        const ITERATE_t i = this->GetSize() - 1;
        ELEM_t tmp = this->GetAt(i);  // copy it.
        this->RemoveAt(i);
        return tmp;
    }
    bool RemoveArg(ARG_t arg) {
        return this->RemoveAt(this->FindIFor3(arg));
    }
};

//************************************************************************

template <class SPAN_TYPE>
void cArrayImpl<SPAN_TYPE>::InsertArray(ITERATE_t i, const cSpan<ELEM_t>& src) {
    if (src.isEmpty()) return;
    const ITERATE_t nCountPrev = this->GetSize();
    if (IS_INDEX_BAD(i, nCountPrev)) i = nCountPrev;  // to the end.

    ASSERT(!this->IsInternalPtr(src));  // append to self not supported. ReAlloc would destroy it.

    const ITERATE_t nSizeCopy = src.GetSize();
    const ITERATE_t nSizeNew = nCountPrev + nSizeCopy;  // new size.
    const ITERATE_t allocateCount = GetHeapCountChunk(nSizeNew);
    ELEM_t* pData = PtrCast<ELEM_t>(cHeap::ReAllocPtr(this->get_PtrWork(), allocateCount * sizeof(ELEM_t)));
    ASSERT_NN(pData);
    SUPER_t::SetSpan2(pData, nSizeNew * sizeof(ELEM_t));

    // Move existing elements.
    cMem::CopyOverlap(pData + i + nSizeCopy, pData + i, (nCountPrev - i) * sizeof(ELEM_t));
    // construct new elements
    cValSpan::ConstructElementsX<ELEM_t>(pData + i, nSizeCopy);
    if (!src.isNull()) cValSpan::CopyQty(pData + i, src.get_PtrConst(), nSizeCopy);  // Copy over new.
}

template <class SPAN_TYPE>
void cArrayImpl<SPAN_TYPE>::RemoveAll() {
    //! AKA RemoveAll, Empty
    //! @note SetSize(0) is slightly more efficient than RemoveAll() if u plan to re-use the array.

    ELEM_t* pData = this->get_PtrWork();
    if (pData != nullptr) {
        const ITERATE_t nSizeCur = this->GetSize();
        SUPER_t::SetSpanNull();
        cValSpan::DestructElementsX<ELEM_t>(pData, nSizeCur);
        cHeap::FreePtr(pData);
    }
}

template <class SPAN_TYPE>
void cArrayImpl<SPAN_TYPE>::SetSize(ITERATE_t nSizeNew) {
    //! @note SetSize(0) is slightly more efficient than RemoveAll() if u plan to re-use the array.
    // TODO What happens on alloc E_OUTOFMEMORY !?
    ASSERT(nSizeNew >= 0);
    const ITERATE_t nCountPrev = this->GetSize();
    ELEM_t* pData = this->get_PtrWork();
    if (nSizeNew <= this->get_HeapCount()) {
        // it fits. don't shrink the allocated array. just destroy unused entries. we may expand again some day.
        cValSpan::Resize<ELEM_t>(pData, nSizeNew, nCountPrev);
        SUPER_t::put_Count2(nSizeNew);
    } else {
        // otherwise, grow heap array
        // MFC will heuristically determine growth when nGrowBy == 0 (this avoids heap fragmentation in many situations)
        ASSERT(nSizeNew > nCountPrev);
        ITERATE_t allocateCount = nSizeNew;
        if (nCountPrev != 0) allocateCount = GetHeapCountChunk(allocateCount);  // not the first time we have done this.
        pData = PtrCast<ELEM_t>(cHeap::ReAllocPtr(pData, allocateCount * sizeof(ELEM_t)));
        ASSERT_NN(pData);
        // construct new elements
        cValSpan::ConstructElementsX<ELEM_t>(&pData[nCountPrev], nSizeNew - nCountPrev);
        SUPER_t::SetSpan2(pData, nSizeNew * sizeof(ELEM_t));
    }
}

template <class SPAN_TYPE>
void cArrayImpl<SPAN_TYPE>::InsertAt(ITERATE_t nIndex, ARG_t newElement) {
    //! Insert at this location, move anything after this.
    // newElement as interior pointer is ok.

    ASSERT(nIndex >= 0);  // will expand to meet need

    const ITERATE_t nCountPrev = this->GetSize();

    if (nIndex >= nCountPrev) {
        // adding after the end of the array
        SetSize(nIndex + 1);  // grow so nIndex is valid
    } else {
        // inserting in the middle of the array
        SetSize(nCountPrev + 1);  // grow it to new size
        // destroy initial data before copying over it (inefficient i know but is very convenient)
        this->ShiftElements(nCountPrev, nIndex);
    }

    // insert new value in the gap
    this->SetAt(nIndex, newElement);  // ASSUME copy constructor will be called!
}

template <class SPAN_TYPE>
bool cArrayImpl<SPAN_TYPE>::RemoveAt(ITERATE_t nIndex) {
    //! NOTE: Any destructor effecting the array MAY be reentrant ?!

    if (nIndex < 0) return false;
    const ITERATE_t nCountPrev = this->GetSize();
    const ITERATE_t nAfterCount = nCountPrev - (nIndex + 1);
    if (nAfterCount < 0) return false;

    ELEM_t* pData = this->get_PtrWork();
    cValSpan::DestructElementsX<ELEM_t>(&pData[nIndex], 1);
    if (nAfterCount > 0) {  // not last.
        cMem::CopyOverlap(&pData[nIndex], &pData[nIndex + 1], nAfterCount * sizeof(ELEM_t));
    }
    SUPER_t::put_Count2(nCountPrev - 1);
    return true;
}

template <class SPAN_TYPE>
void cArrayImpl<SPAN_TYPE>::RemoveAt(ITERATE_t nIndex, ITERATE_t iQty) {
    // NOTE: destructor effecting the array MAY be reentrant ?!
    if (iQty <= 0 || nIndex < 0) return;
    const ITERATE_t nCountPrev = this->GetSize();
    if (nIndex >= nCountPrev) return;
    ITERATE_t nAfterCount = nCountPrev - (nIndex + iQty);
    if (nAfterCount < 0) {  // iQty beyond the end!
        nAfterCount = 0;    // to Last element.
        iQty = nCountPrev - nIndex;
    }
    // remove a range
    ELEM_t* pData = this->get_PtrWork();
    cValSpan::DestructElementsX<ELEM_t>(pData + nIndex, iQty);
    if (nAfterCount > 0) {  // not last.
        cMem::CopyOverlap(pData + nIndex, pData + nIndex + iQty, nAfterCount * sizeof(ELEM_t));
    }
    SUPER_t::put_Count2(nCountPrev - iQty);
}

template <class SPAN_TYPE>
bool cArrayImpl<SPAN_TYPE>::IsValidHeapSize() const noexcept {
    const ITERATE_t nSizeCur = this->GetSize();
    const ELEM_t* pData = this->get_PtrConst();
    if (pData == nullptr) {
        if (nSizeCur != 0) return false;
    } else {
        // ASSERT(nSizeCur>0);
        if (nSizeCur > get_HeapCount()) return false;  // NOTE: get_HeapCount will check _pData
    }
    return true;
}

//*************************************************

/// <summary>
/// Base for array of some type of pointer using cPtrFacade. Allow dupes.
/// base for cArrayPtr, cArryNew, cArrayIUnk and cArrayRef
/// </summary>
/// <typeparam name="SPAN_TYPE">some cPtrFacade derived</typeparam>
template <class SPAN_TYPE>
struct cArrayFacade : public cArrayImpl<SPAN_TYPE> {
    typedef cArrayImpl<SPAN_TYPE> SUPER_t;
    typedef typename SPAN_TYPE::ELEM_t ELEM_t;
    typedef typename SPAN_TYPE::ARG_t ARG_t;

    /// Just return nullptr if index out of bounds. AKA Safe. GetAtSafe()
    ARG_t GetAtCheck(ITERATE_t index) const noexcept {
        if (!SUPER_t::IsValidIndex(index)) return nullptr;
        return SUPER_t::GetAt(index);
    }
    /// <summary>
    /// NOT the same as IsValidIndex()
    /// </summary>
    bool IsValidAt(ITERATE_t index) const noexcept {
        return GetAtCheck(index) != nullptr;
    }

    ELEM_t PopHead() {
        if (SUPER_t::isEmpty()) return nullptr;
        return SUPER_t::PopHead();
    }
    ELEM_t PopTail() {
        if (SUPER_t::isEmpty()) return nullptr;
        return SUPER_t::PopTail();
    }
};

/// <summary>
/// An array of some sort of dumb pointer. Pointer memory ownership is unknown. Does not free it automatically.
/// allow dupes. NOT sorted.
/// </summary>
/// <typeparam name="SPAN_TYPE">is allowed to be const X</typeparam>
template <class TYPE>
struct cArrayPtr : public cArrayFacade<cSpanSearchable<TYPE*, TYPE*>> {
    typedef cArrayFacade<cSpanSearchable<TYPE*, TYPE*>> SUPER_t;

    /// <summary>
    /// Manually delete Dynamic heap allocated object.
    /// </summary>
    void DeleteAt(ITERATE_t i) {
        if (!this->IsValidIndex(i)) return;
        TYPE* pObj = this->GetAt(i);  // make copy.
        this->RemoveAt(i);
        delete pObj;
    }

    /// <summary>
    /// Similar to RemoveAll(), DisposeAll() except it calls 'delete' to try to dereference all the entries.
    /// @note often delete has the effect of removing itself from the list. Beware of this.
    /// </summary>
    void DeleteAll() {
        ITERATE_t iSize = this->GetSize();
        for (ITERATE_t i = iSize - 1; i >= 0; i--) {  // reverse order they got added. might be faster?
            TYPE* pObj = this->GetAt(i);
            if (pObj != nullptr) {
                this->SetAt(i, nullptr);
                delete pObj;
            }
            // delete removes itself from the list?
            const ITERATE_t iSize2 = this->GetSize();
            if (iSize2 != iSize) {
                iSize = iSize2;
                if (i != iSize) i = iSize;  // start over.
            }
        }
        SUPER_t::RemoveAll();
    }
};

/// <summary>
/// An array of some simple value type that is easy to copy.
/// Using a Reference is a waste if the object is small. Just use a copy for small objects.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
struct cArrayVal : public cArrayImpl<cSpanSearchable<TYPE, TYPE>> {
    cArrayVal() {}
    explicit cArrayVal(ITERATE_t iSize) : cArrayImpl<cSpanSearchable<TYPE, TYPE>>(iSize) {}
};
template <class TYPE>
struct cArrayStruct : public cArrayImpl<cSpanX<TYPE, const TYPE&>> {
    cArrayStruct() {}
    explicit cArrayStruct(ITERATE_t iSize) : cArrayImpl<cSpanX<TYPE, const TYPE&>>(iSize) {}
};
}  // namespace Gray
#endif  // _INC_cArray_H
