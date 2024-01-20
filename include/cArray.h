//! @file cArray.h
//! c++ Collections. MFC compatible.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cArray_H
#define _INC_cArray_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"  // THROW_IF()
#include "cHeap.h"
#include "cSpan.h"
#include "cObject.h"
#include "cPtrFacade.h"
#include "cValArray.h"

namespace Gray {
/// <summary>
/// Minimal array template of elements. like MFC version.
/// @note MFC 8.0 uses INT_PTR for GetSize()
/// </summary>
/// <typeparam name="TYPE">what is stored.</typeparam>
/// <typeparam name="ARG_TYPE">const TYPE ref or TYPE depending on what makes sense for SetAt() and Add operations.</typeparam>
template <class TYPE, class ARG_TYPE = const TYPE&>
class cArray : public cObject, public cSpan2<TYPE, ARG_TYPE> {
    typedef cArray<TYPE, ARG_TYPE> THIS_t;
    typedef cSpan2<TYPE, ARG_TYPE> SUPER_t;

 protected:
    // Don't allow public access to some cMemSpan methods.
    void SetSpanNull() = delete;
    void SetSpanConst(const void* pData, size_t nSize) = delete;
    void SetSpanSkip(size_t nSize) = delete;
    void SetSpan(void* pData, size_t nSize) = delete;
    void SetSpan(const SUPER_t& a) = delete;

 public:
    cArray() noexcept {}

    void RemoveAll();
    void SetSize(ITERATE_t nSizeNew);
    void SetCopy(const THIS_t& aValues) {
        if (this == &aValues) return;
        // this->RemoveAll();  // destruct any previous data?
        this->SetSize(aValues.GetSize());  // This will call empty constructors.
        cValArray::CopyQty(this->get_DataWork(), aValues.get_DataConst(), this->GetSize());
    }

    /// <summary>
    /// copy constructor.
    /// </summary>
    cArray(const THIS_t& rCopy) {
        this->SetCopy(rCopy);
    }

    /// <summary>
    /// set Size to iSize empty entries.
    /// </summary>
    explicit cArray(ITERATE_t iSize) {
        this->SetSize(iSize);
    }

    /// <summary>
    /// move constructor.
    /// </summary>
    cArray(THIS_t&& ref) noexcept : SUPER_t(ref.get_DataConst(), ref.get_Count()) {
        ref.SUPER_t::SetSpanNull();
    }
    ~cArray() override {
        RemoveAll();
    }

    //**************************
    // Base pointer

    bool IsValidMallocSize() const noexcept;
    bool isValidCheck() const noexcept {
        if (!cObject::isValidCheck()) return false;
        // IsValidCast(cArray<TYPE, ARG_TYPE>,this);
        if (!this->IsValidMallocSize()) return false;
        return true;
    }

    //**************************
    // Size
    size_t CountHeapStats(OUT ITERATE_t& iAllocCount) const noexcept {
        //! @return sizeof all children alloc(s). not size of *this
        if (this->isEmpty()) return 0;
        iAllocCount++;  // just the alloc for the array
        return cHeap::GetSize(this->get_DataConst());
    }

    /// <summary>
    /// Get quantity of objects truly allocated. (may not be same as Size or even properly aligned with TYPE)
    /// like STL capacity()
    /// </summary>
    ITERATE_t get_CountMalloc() const noexcept {
        return CastN(ITERATE_t, cHeap::GetSize(this->get_DataConst()) / sizeof(TYPE));
    }

    /// <summary>
    /// over allocate to allow room to grow.
    /// </summary>
    constexpr static ITERATE_t GetCountMalloc(ITERATE_t i) {
        return i + (i / 16);
    }

    //********************************
    // elements

    /// Potentially growing the array
    void SetAtGrow(ITERATE_t nIndex, ARG_TYPE newElement) {
        ASSERT_VALID(this);
        if (nIndex >= GetSize()) {  // must grow.
            SetSize(nIndex + 1);
        }
        SetAt(nIndex, newElement);
    }

    /// <summary>
    /// Add to the end. AKA push_back(), Push()
    /// </summary>
    ITERATE_t Add(ARG_TYPE newElement) {
        const ITERATE_t nIndex = GetSize();
        SetAtGrow(nIndex, newElement);
        return nIndex;
    }

    const cArray<TYPE, ARG_TYPE>& operator=(const cArray<TYPE, ARG_TYPE>& aValues) {  // otherwise this is considered deleted ?
        SetCopy(aValues);
        return *this;
    }

    // Operations that move elements around
    void InsertAt(ITERATE_t nIndex, ARG_TYPE newElement);
    void RemoveAt(ITERATE_t nIndex);
    void RemoveAt(ITERATE_t nIndex, ITERATE_t iQty);

    void AddHead(ARG_TYPE newElement) {
        // NOT a normal stack or queue. Adds are usually to the tail.
        this->InsertAt(0, newElement);
    }

    /// <summary>
    /// Insert an array into this array. Like MFC CArray::Append() ? sort of.
    /// </summary>
    /// <param name="i">point of insert</param>
    /// <param name="pCopy"></param>
    /// <param name="countCopy"></param>
    void InsertArray(ITERATE_t i, const TYPE* pCopy, ITERATE_t countCopy);

    void InsertArray(ITERATE_t i, const THIS_t& src) {
        // Emulate MFC. but also resolve overload conflict.
        InsertArray(i, src.get_DataConst(), src.GetSize());
    }

    void RemoveLast() {
        this->RemoveAt(this->GetSize() - 1);
    }
    TYPE PopHead() {
        // pop from front of queue.
        ASSERT(!this->isEmpty());
        TYPE tmp = this->GetAt(0);  // copy it.
        this->RemoveAt(0);
        return tmp;
    }
    TYPE PopTail() {
        // pop from top of stack.
        // AKA Pop()
        ASSERT(!this->isEmpty());
        const ITERATE_t i = this->GetSize() - 1;
        TYPE tmp = this->GetAt(i);  // copy it.
        this->RemoveAt(i);
        return tmp;
    }
    bool RemoveArg(ARG_TYPE arg) {
        //! @return true = removed. false = was not here.
        const ITERATE_t nIndex = FindIFor(arg);
        if (nIndex < 0) return false;
        this->RemoveAt(nIndex);
        return true;
    }
};

//************************************************************************

template <class TYPE, class ARG_TYPE>
void cArray<TYPE, ARG_TYPE>::InsertArray(ITERATE_t i, const TYPE* pCopy, ITERATE_t countCopy) {
    ASSERT_VALID(this);

    if (countCopy <= 0) return;
    const ITERATE_t nSizePrev = this->GetSize();
    if (i < 0 || i > nSizePrev) i = nSizePrev;

    ASSERT(!cMem::IsInsideBlock(pCopy, this->get_DataConst() + i, nSizePrev - i));  // append to self not supported.

    const ITERATE_t nSizeNew = nSizePrev + countCopy;  // new size.
    const ITERATE_t allocateCount = GetCountMalloc(nSizeNew);
    TYPE* pData = PtrCast<TYPE>(cHeap::ReAllocPtr(get_DataWork(), allocateCount * sizeof(TYPE)));
    ASSERT_NN(pData);
    SUPER_t::SetSpan(pData, nSizeNew * sizeof(TYPE));

    // Move existing elements.
    cMem::CopyOverlap(pData + i + countCopy, pData + i, (nSizePrev - i) * sizeof(TYPE));
    // construct new elements
    cValArray::ConstructElementsX<TYPE>(pData + i, countCopy);
    if (pCopy != nullptr) {  // Copy over new.
        cValArray::CopyQty(pData + i, pCopy, countCopy);
    }
}

template <class TYPE, class ARG_TYPE>
void cArray<TYPE, ARG_TYPE>::RemoveAll() {
    //! AKA RemoveAll, Empty
    //! @note SetSize(0) is slightly more efficient than RemoveAll() if u plan to re-use the array.
#ifdef _DEBUG
    // AssertSize();
#endif
    const ITERATE_t nSizePrev = GetSize();
    TYPE* pData = get_DataWork();
    SUPER_t::SetSpanNull();
    if (pData != nullptr) {
        cValArray::DestructElementsX<TYPE>(pData, nSizePrev);
        cHeap::FreePtr(pData);  // const_cast
    }
}

template <class TYPE, class ARG_TYPE>
void cArray<TYPE, ARG_TYPE>::SetSize(ITERATE_t nSizeNew) {
    //! @note SetSize(0) is slightly more efficient than RemoveAll() if u plan to re-use the array.
    ASSERT_VALID(this);
    ASSERT(nSizeNew >= 0);
    const ITERATE_t nSizePrev = GetSize();
    TYPE* pData = get_DataWork();
    if (nSizeNew <= get_CountMalloc()) {
        // it fits. don't shrink the allocated array. just destroy unused entries. we may expand again some day.
        cValArray::Resize<TYPE>(pData, nSizeNew, nSizePrev);
        SUPER_t::put_Count(nSizeNew);
    } else {
        // otherwise, grow heap array
        // MFC will heuristically determine growth when nGrowBy == 0 (this avoids heap fragmentation in many situations)
        ASSERT(nSizeNew > nSizePrev);
        ITERATE_t allocateCount = nSizeNew;
        if (nSizePrev != 0) {  // not the first time we have done this.
            allocateCount = GetCountMalloc(allocateCount);
        }
        pData = PtrCast<TYPE>(cHeap::ReAllocPtr(pData, allocateCount * sizeof(TYPE)));
        ASSERT_NN(pData);
        // construct new elements
        cValArray::ConstructElementsX<TYPE>(&pData[nSizePrev], nSizeNew - nSizePrev);
        SUPER_t::SetSpan(pData, nSizeNew * sizeof(TYPE));
    }
}

template <class TYPE, class ARG_TYPE>
void cArray<TYPE, ARG_TYPE>::InsertAt(ITERATE_t nIndex, ARG_TYPE newElement) {
    //! Insert at this location, move anything after this.
    // newElement as interior pointer is ok.

    ASSERT_VALID(this);
    ASSERT(nIndex >= 0);  // will expand to meet need

    const ITERATE_t nSizePrev = GetSize();

    if (nIndex >= nSizePrev) {
        // adding after the end of the array
        SetSize(nIndex + 1);  // grow so nIndex is valid
    } else {
        // inserting in the middle of the array
        SetSize(nSizePrev + 1);  // grow it to new size
        // destroy initial data before copying over it (inefficient i know but is very convenient)
        MoveElement(nSizePrev, nIndex);
    }

    // insert new value in the gap
    SetAt(nIndex, newElement);  // ASSUME copy constructor will be called!
}

template <class TYPE, class ARG_TYPE>
void cArray<TYPE, ARG_TYPE>::RemoveAt(ITERATE_t nIndex) {
    //! NOTE: Any destructor effecting the array MAY be reentrant ?!
    ASSERT_VALID(this);
    if (nIndex < 0) return;
    const ITERATE_t nSizePrev = GetSize();
    const ITERATE_t nMoveCount = nSizePrev - (nIndex + 1);
    if (nMoveCount < 0) return;

    TYPE* pData = get_DataWork();
    cValArray::DestructElementsX<TYPE>(&pData[nIndex], 1);
    if (nMoveCount > 0) {  // not last.
        cMem::CopyOverlap(&pData[nIndex], &pData[nIndex + 1], nMoveCount * sizeof(TYPE));
    }
    SUPER_t::put_Count(nSizePrev - 1);
}

template <class TYPE, class ARG_TYPE>
void cArray<TYPE, ARG_TYPE>::RemoveAt(ITERATE_t nIndex, ITERATE_t iQty) {
    // NOTE: Any destructor effecting the array will be reentrant ?!
    ASSERT_VALID(this);
    if (iQty <= 0 || nIndex < 0) return;
    const ITERATE_t nSizePrev = GetSize();
    ITERATE_t nMoveCount = nSizePrev - (nIndex + iQty);
    if (nMoveCount < 0) {  // iQty beyond the end!
        ASSERT(nMoveCount >= 0);
        nMoveCount = 0;  // Last element.
        iQty = nSizePrev - nIndex;
    }
    if (iQty >= nSizePrev) {
        // ASSERT(nIndex == 0);	// assumed.
        RemoveAll();
        return;
    }
    // just remove a range
    TYPE* pData = get_DataWork();
    cValArray::DestructElementsX<TYPE>(&pData[nIndex], iQty);
    if (nMoveCount > 0) {  // not last.
        cMem::CopyOverlap(&pData[nIndex], &pData[nIndex + iQty], nMoveCount * sizeof(TYPE));
    }
    SUPER_t::put_Count(nSizePrev - iQty);
}

template <class TYPE, class ARG_TYPE>
bool cArray<TYPE, ARG_TYPE>::IsValidMallocSize() const noexcept {
    // Make sure the alloc is actually bigger than the declared size.
    const ITERATE_t nSizePrev = GetSize();
    const TYPE* pData = get_DataConst();
    if (pData == nullptr) {
        if (nSizePrev != 0) return false;
    } else {
        // ASSERT(nSizePrev>0);
        if (nSizePrev > get_CountMalloc()) return false;  // NOTE: get_CountMalloc will check m_pData
    }
    return true;
}

//*************************************************

/// <summary>
/// An array of some type of pointer using cPtrFacade. Allow dupes.
/// base for cArrayPtr, cArryNew, cArrayIUnk and cArrayRef
/// </summary>
/// <typeparam name="TYPE">some cPtrFacade derived</typeparam>
/// <typeparam name="ARG_TYPE"></typeparam>
template <class TYPE, class ARG_TYPE = TYPE*>
struct cArrayFacade : public cArray<TYPE, ARG_TYPE> {
    typedef cArray<TYPE, ARG_TYPE> SUPER_t;
    typedef cArrayFacade<TYPE, ARG_TYPE> THIS_t;

    /// <summary>
    /// Make sure the virtual get destroyed correctly.
    /// </summary>
    ~cArrayFacade() override {
        this->RemoveAll();
    }

    /// <summary>
    /// Compare a data record to another data record. Use cValT::Compare()??
    /// </summary>
    /// <param name="pData1"></param>
    /// <param name="pData2"></param>
    /// <returns></returns>
    COMPARE_t CompareData(ARG_TYPE pData1, ARG_TYPE pData2) const noexcept override {
        // return cValT::Compare(Data1,Data2);
        return cMem::Compare(pData1, pData2, sizeof(*pData2));
    }

    /// Just return nullptr if index out of bounds. AKA Safe. GetAtSafe()
    ARG_TYPE GetAtCheck(ITERATE_t index) const {
        if (!SUPER_t::IsValidIndex(index)) return nullptr;
        return SUPER_t::GetAt(index);
    }

    TYPE PopHead() {
        if (SUPER_t::isEmpty()) return nullptr;
        return SUPER_t::PopHead();
    }
    TYPE PopTail() {
        if (SUPER_t::isEmpty()) return nullptr;
        return SUPER_t::PopTail();
    }
};

/// <summary>
/// An array of some sort of dumb pointer. Pointer memory ownership is unknown. Does not free it automatically.
/// </summary>
/// <typeparam name="TYPE">is allowed to be const X</typeparam>
template <class TYPE>
struct cArrayPtr : public cArrayFacade<TYPE*, TYPE*> {
    typedef cArrayFacade<TYPE*, TYPE*> SUPER_t;

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
struct cArrayVal : public cArray<TYPE, TYPE> {
    typedef cArray<TYPE, TYPE> SUPER_t;
    cArrayVal() {}
    explicit cArrayVal(ITERATE_t iSize) : SUPER_t(iSize) {}
};
}  // namespace Gray
#endif  // _INC_cArray_H
