//! @file cArrayT.h
//! c++ Collections.  Simple.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cArrayT_H
#define _INC_cArrayT_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cDebugAssert.h"  // THROW_IF()
#include "cHeap.h"
#include "cHeapObject.h"
#include "cRefPtr.h"
#include "cSpan.h"

namespace Gray {
/// <summary>
/// a variable size allocated, reference counted array of some data _TYPE.
/// used with cStringT and cArrayT. similar to the MFC ATL:CStringData.
/// (ALWAYS) dynamically allocated cHeapObject
/// </summary>
/// <typeparam name="_TYPE"></typeparam>
template <class _TYPE>
class cArrayHeadT : public cRefBase, public cHeapObject {
    CHEAPOBJECT_IMPL;  /// Get the top level "new" pointer in the case of multiple inheritance.
    typedef cArrayHeadT<_TYPE> THIS_t;

 protected:
    ITERATE_t m_nCount = 0;  /// Number of _TYPE elements (upperBound - 1). m_pData MUST hold at least this many
    /// <summary>
    /// Some hash code of the data to follow. 0 = not yet calculated or empty. same as ATOMCODE_t.
    /// </summary>
    mutable HASHCODE32_t m_HashCode;
    // TYPE m_a[ m_nCount * sizeof(_TYPE) ];	/// dynamic heap allocated.

 public:
    inline static ITERATE_t GetHeapCountChunk(ITERATE_t i) {
        //! over allocate to allow room to grow.
        return i + (i / 16);
    }
    inline static size_t GetMallocSize(ITERATE_t i) {
        return sizeof(THIS_t) + (i * sizeof(_TYPE));
    }

 private:
    /// <summary>
    /// called by CreateHead. Will call private construct below.
    /// </summary>
    /// <param name="stAllocateBlock"></param>
    /// <param name="sizePayload">length in bytes</param>
    /// <returns></returns>
    static void* operator new(size_t stAllocateBlock, size_t sizePayload) {
        ASSERT(stAllocateBlock == sizeof(cArrayHeadT));
        return cHeap::AllocPtr(stAllocateBlock + sizePayload);
    }
    cArrayHeadT(ITERATE_t nCount) noexcept : m_nCount(nCount), m_HashCode(k_HASHCODE_CLEAR) {}

 public:
    static void operator delete(void* pObj, size_t sizePayload) {
        // called by cRefBase onZeroRefCount
        UNREFERENCED_PARAMETER(sizePayload);
        cHeap::FreePtr(pObj);
    }
    static void operator delete(void* pObj) {
        // called by cRefBase onZeroRefCount
        cHeap::FreePtr(pObj);
    }

    /// <summary>
    /// Get a pointer to the payload array of TYPE. Stored in the space allocated after this class.
    /// </summary>
    /// <returns></returns>
    const _TYPE* get_PtrConst() const noexcept {
        return reinterpret_cast<const _TYPE*>(this + 1);
    }
    _TYPE* get_PtrWork() noexcept {
        return reinterpret_cast<_TYPE*>(this + 1);
    }
    const _TYPE* get_DataEnd() const noexcept {
        return get_PtrConst() + get_Count();
    }
    _TYPE* get_DataEnd() noexcept {  // NEVER write past end ! for support end()
        return get_PtrWork() + get_Count();
    }

    /// <summary>
    /// get Number of array elements of _TYPE in payload.
    /// </summary>
    /// <returns></returns>
    inline ITERATE_t get_Count() const noexcept {
        return m_nCount;
    }
    inline size_t get_BytesSize() const noexcept {
        return m_nCount * sizeof(_TYPE);
    }

    /// Get the whole array as a span of memory.
    cSpan<_TYPE> get_Span() const noexcept {
        return ToSpan(get_PtrConst(), get_Count());
    }
    cSpanX<_TYPE> get_Span() noexcept {
        return ToSpan(get_PtrWork(), get_Count());
    }

    /// <summary>
    /// Get size of quantity of objects truly allocated. (may not be same as m_nSize or even properly aligned with TYPE)
    /// like STL capacity()
    /// </summary>
    /// <returns></returns>
    inline size_t get_BytesMalloc() const noexcept {
        return cHeap::GetSize(this) - sizeof(cArrayHeadT);
    }
    inline ITERATE_t get_HeapCount() const noexcept {
        return CastN(ITERATE_t, get_BytesMalloc() / sizeof(_TYPE));
    }

    inline bool IsHashCodeSet() const noexcept {
        return m_HashCode != k_HASHCODE_CLEAR && m_nCount > 0;
    }
    inline HASHCODE32_t get_HashCode() const {
        // hides get_HashCode() implemented by cRefBase
        return m_HashCode;
    }

    /// <summary>
    /// allocate space and call _TYPE constructors. RefCount = 0.
    /// </summary>
    /// <param name="nCount"></param>
    /// <returns></returns>
    static THIS_t* CreateHead(ITERATE_t nCount, bool construct) {
        if (nCount == 0) return nullptr;
        THIS_t* pHead = new (nCount * sizeof(_TYPE)) THIS_t(nCount);  // allocate space and construct head only. HeapAlign ?? align_of ?
        ASSERT_NN(pHead);
        if (construct) {
            _TYPE* pData = pHead->get_PtrWork();
            cValSpan::ConstructElementsX(pData, nCount);
        }
        return pHead;
    }

    void ShrinkHead(ITERATE_t nCountNew, bool destruct) noexcept {
        // shrinking. it fits. don't shrink the allocated array. we may expand again some day.
        ASSERT(nCountNew < m_nCount);  // assume change.
        if (destruct) {
            // destruct the deleted elements.
            _TYPE* pData = get_PtrWork();
            cValSpan::DestructElementsX<_TYPE>(&pData[nCountNew], m_nCount - nCountNew);
        }
        m_nCount = nCountNew;
        m_HashCode = k_HASHCODE_CLEAR;  // invalidate hash.
    }

    /// <summary>
    /// Grow the array.
    /// DANGER set array size . assume allocation is valid. Single reference.
    /// MFC will heuristically determine growth when nGrowBy == 0 (this avoids heap fragmentation in many situations)
    /// </summary>
    THIS_t* GrowHead(ITERATE_t nCountNew, bool construct) noexcept {
        ASSERT(nCountNew > m_nCount);  // assume change.

        // allocate greater size if not the first time we have done this.
        const ITERATE_t allocateCount = (m_nCount != 0) ? GetHeapCountChunk(nCountNew) : nCountNew;
        const ITERATE_t nCountOld = m_nCount;
        m_nCount = nCountNew;
        m_HashCode = k_HASHCODE_CLEAR;  // invalidate hash.

        THIS_t* pHeadNew = PtrCast<THIS_t>(cHeap::ReAllocPtr(this, GetMallocSize(allocateCount)));
        ASSERT_NN(pHeadNew);

        if (construct) {
            // construct new elements
            cValSpan::ConstructElementsX<_TYPE>(pHeadNew->get_PtrWork() + nCountOld, nCountNew - nCountOld);
        }
        return pHeadNew;
    }

    THIS_t* ResizeHead(ITERATE_t nCountNew, bool construct) noexcept {
        if (nCountNew > m_nCount) {
            return GrowHead(nCountNew, construct);
        } else {
            ShrinkHead(nCountNew, construct);
            return this;
        }
    }
};

/// <summary>
/// An array that is contained in a single pointer similar to cString. Small empty size.
/// NO MFC compatibility.
/// NOTE: Danger! Since this is a reference, change to one is a change to all. CloneBeforeWrite if more than one ref.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cArrayT : public cRefPtr<cArrayHeadT<TYPE> > {
    typedef cRefPtr<cArrayHeadT<TYPE> > SUPER_t;
    typedef cArrayT<TYPE> THIS_t;
    typedef cArrayHeadT<TYPE> HEAD_t;

    /// <summary>
    /// deep copy (and resize) the array. private reference. like CloneBeforeWrite()
    /// </summary>
    /// <param name="pOld"></param>
    /// <param name="nCountNew"></param>
    /// <param name="nCountOld"></param>
    void SetCopyFrom(const HEAD_t* pOld, ITERATE_t nCountNew, ITERATE_t nCountOld) {
        HEAD_t* pNew = HEAD_t::CreateHead(nCountNew, true);
        ASSERT(pNew != nullptr || nCountNew == 0);
        if (pNew != nullptr && pOld != nullptr) {
            cValSpan::CopyQty(pNew->get_PtrWork(), pOld->get_PtrConst(), cValT::Min(nCountNew, nCountOld));  // Copy from pOld
        }
        this->put_Ptr(pNew);
    }

 public:
    cArrayT() noexcept {}
    explicit cArrayT(ITERATE_t nCount) : SUPER_t(HEAD_t::CreateHead(nCount, true)) {}

    /// <summary>
    /// get sizeof() all children alloc(s). not size of *this
    /// </summary>
    /// <param name="iAllocCount"></param>
    /// <returns></returns>
    size_t CountHeapStats(OUT ITERATE_t& iAllocCount) const noexcept {
        HEAD_t* pHead = this->get_Ptr();
        if (pHead == nullptr) return 0;
        return pHead->GetHeapStatsThis(iAllocCount);  // just the alloc for the array
    }

    inline ITERATE_t get_Count() const noexcept {
        HEAD_t* pHead = this->get_Ptr();
        if (pHead == nullptr) return 0;  // shortcut to 0.
        return pHead->get_Count();
    }
    inline ITERATE_t GetSize() const noexcept {
        // ALIAS for MFC compat. AKA count
        return get_Count();
    }
    inline bool IsValidIndex(ITERATE_t i) const noexcept {
        return IS_INDEX_GOOD(i, this->get_Count());
    }
    inline bool isEmpty() const noexcept {
        return get_Count() == 0;
    }

    /// Get a pointer to the array.
    inline TYPE* get_PtrWork() const noexcept {
        HEAD_t* pHead = this->get_Ptr();
        if (pHead == nullptr) return nullptr;
        return pHead->get_PtrWork();
    }
    inline const TYPE* get_PtrConst() const noexcept {
        HEAD_t* pHead = this->get_Ptr();
        if (pHead == nullptr) return nullptr;
        return pHead->get_PtrConst();
    }

    cSpanX<TYPE> get_Span() const noexcept {
        HEAD_t* pHead = this->get_Ptr();
        if (pHead == nullptr) return {};
        return pHead->get_Span();
    }
    operator cSpan<TYPE>() const noexcept {  // auto convert to cSpan
        return get_Span();
    }

    inline const TYPE& GetAt(ITERATE_t nIndex) const noexcept {
        // I can make a const pointer from this and know what its life span is. NOT just a copy.
        DEBUG_CHECK(IsValidIndex(nIndex));
        return this->get_Ptr()->get_PtrConst()[nIndex];
    }
    inline TYPE& ElementAt(ITERATE_t nIndex) noexcept {
        DEBUG_CHECK(IsValidIndex(nIndex));
        return this->get_Ptr()->get_PtrWork()[nIndex];
    }
    inline void SetAt(ITERATE_t nIndex, const TYPE& newElement) noexcept {
        // If multiple refs to this then we should copy/split it ?
        // @note DANGER - this is a reference counted object. Any changes to it will make changes to all references !! Make a deep copy if required.
        DEBUG_CHECK(IsValidIndex(nIndex));
        this->get_Ptr()->get_PtrWork()[nIndex] = newElement;
    }

    void ThrowIfInvalidIndex(ITERATE_t nIndex) const {  // throw
        THROW_IF(!IsValidIndex(nIndex));                // may be part of test throw?
    }
    inline TYPE& operator[](ITERATE_t nIndex) {  // throw
        //! throw an exception if we are out of range.
        ThrowIfInvalidIndex(nIndex);
        return this->get_Ptr()->get_PtrWork()[nIndex];
    }
    inline const TYPE& operator[](ITERATE_t nIndex) const {  // throw
        //! throw an exception if we are out of range.
        ThrowIfInvalidIndex(nIndex);
        return this->get_Ptr()->get_PtrConst()[nIndex];
    }

    /// <summary>
    /// Get quantity of objects truly allocated. (may not be same as m_nSize or even properly aligned with TYPE)
    /// like STL capacity()
    /// </summary>
    ITERATE_t get_HeapCount() const noexcept {
        if (!this->isValidPtr()) return 0;
        return this->get_Ptr()->get_HeapCount();
    }

    void SetCopy(const cArrayT<TYPE>& a) {
        // deep copy the array. private reference.
        const ITERATE_t nCount = get_Count();
        SetCopyFrom(a.get_Ptr(), nCount, nCount);
    }

    void SetEmpty() {
        this->put_Ptr(nullptr);  // just release ref.
    }

    /// <summary>
    /// change size of array. realloc. Other refs will NOT see this change.
    /// </summary>
    void put_Count(ITERATE_t nCountNew) {
        ASSERT(IS_INDEX_GOOD(nCountNew, cMem::k_ALLOC_MAX));  // reasonable arbitrary limit.

        HEAD_t* pOld = this->get_Ptr();
        if (pOld == nullptr) {
            // Make a new array.
            if (nCountNew == 0) return;
            this->put_Ptr(HEAD_t::CreateHead(nCountNew, true));
            return;
        }

        const ITERATE_t nCountOld = pOld->get_Count();
        if (nCountNew == nCountOld) return;  // no change.

        if (nCountNew == 0) {
            SetEmpty();
            return;
        }

        const REFCOUNT_t iRefCounts = pOld->get_RefCount();
        if (iRefCounts != 1) {  // other refs exist. so i must make a private copy.
            // Make a new array. Copy from old. So we can change size.
            // NOTE: we may be duping our self. (to change length)
            ASSERT(iRefCounts > 1);
            SetCopyFrom(pOld, nCountNew, nCountOld);
            return;
        }

        // I have an exclusive copy that no other is using.
        // just change/resize the existing heap alloc instance.
        this->AttachPtr(pOld->ResizeHead(nCountNew, true));  // replace reallocated pointer. (if it changed at all) No ref count change.
    }

    void SetAtGrow(ITERATE_t nIndex, const TYPE& newElement) {
        const ITERATE_t nCountOld = get_Count();
        if (nIndex >= nCountOld) {  // must grow.
            put_Count(nIndex + 1);
        }
        SetAt(nIndex, newElement);
    }

    /// <summary>
    /// Add to the end. AKA push_back()
    /// </summary>
    ITERATE_t Add(const TYPE& newElement) {
        const ITERATE_t nCountOld = get_Count();
        put_Count(nCountOld + 1);  // Grow.
        SetAt(nCountOld, newElement);
        return nCountOld;
    }

    void InsertAt(ITERATE_t nIndex, const TYPE& newElement) {
        // newElement as interior pointer is ok.
        ASSERT(nIndex >= 0);  // will expand to meet need
        const ITERATE_t nCountOld = get_Count();
        if (nIndex >= nCountOld) {  // add at end.
            put_Count(nIndex + 1);  // grow it to new size
        } else {
            put_Count(nCountOld + 1);  // grow it to new size
            TYPE* pData = this->get_Ptr()->get_PtrWork();
            cValSpan::MoveElement1(pData + nCountOld, pData + nIndex);  // make space.
        }

        // insert new value in the gap
        SetAt(nIndex, newElement);
    }

    /// <summary>
    /// Add array to the end. Like MFC CArray::Append() ?
    /// </summary>
    void InsertArray(ITERATE_t i, const cSpan<TYPE>& src) {
        if (src.isEmpty()) return;
        const ITERATE_t nSizeCopy = src.GetSize();

        HEAD_t* pNew = nullptr;
        HEAD_t* pOld = this->get_Ptr();
        if (pOld == nullptr) {
            // assume i = 0; or it doesnt matter anyhow.
            this->put_Ptr(pNew = HEAD_t::CreateHead(nSizeCopy, true));
        } else {
            const ITERATE_t nSizePrev = pOld->get_Count();
            if (IS_INDEX_BAD(i, nSizePrev)) i = nSizePrev;

            ASSERT(!pOld->get_Span().IsInternalPtr(src));  // append to self not supported.

            const REFCOUNT_t iRefCounts = pOld->get_RefCount();
            if (iRefCounts != 1) {
                // TODO MUST MAKE PRIVATE COPY !!!
                ASSERT(0);
            }

            const ITERATE_t nCountNew = nSizePrev + nSizeCopy;  // new size.
            pNew = pOld->GrowHead(nCountNew, false);
            this->AttachPtr(pNew);  // replace re-alloc-ed pointer. (if it changed at all) No ref count change.

            TYPE* pData = pNew->get_PtrWork();
            // Move existing elements.
            cMem::CopyOverlap(pData + i + nSizeCopy, pData + i, (nSizePrev - i) * sizeof(TYPE));
            // construct new elements at insert point.
            cValSpan::ConstructElementsX<TYPE>(pData + i, nSizeCopy);
        }

        if (!src.isNull()) cValSpan::CopyQty(pNew->get_PtrWork() + i, src.get_PtrConst(), nSizeCopy);  // Copy over new.
    }

    void RemoveAt(ITERATE_t nIndex) {
        //! NOTE: Any destructor effecting the array MAY be reentrant ?!
        if (nIndex < 0) return;
        HEAD_t* pHead = this->get_Ptr();
        if (pHead == nullptr) return;
        const ITERATE_t nCount = pHead->get_Count();
        ITERATE_t nMoveCount = nCount - (nIndex + 1);
        if (nMoveCount < 0) return;  // nIndex is out of range!
        TYPE* pData = pHead->get_PtrWork();
        cValSpan::DestructElementsX<TYPE>(pData + nIndex, 1);
        if (nMoveCount > 0) {  // not last.
            cMem::CopyOverlap(pData + nIndex, pData + nIndex + 1, nMoveCount * sizeof(TYPE));
        }
        pHead->ShrinkHead(nCount - 1, false);
    }

    typedef cIterator<TYPE> iterator;              // like STL
    typedef cIterator<const TYPE> const_iterator;  // like STL
    iterator begin() noexcept {
        return iterator(get_PtrWork());
    }
    iterator end() noexcept {
        HEAD_t* pHead = this->get_Ptr();
        return iterator((pHead == nullptr) ? nullptr : pHead->get_DataEnd());
    }
    const_iterator begin() const noexcept {
        return const_iterator(get_PtrWork());
    }
    const_iterator end() const noexcept {
        const HEAD_t* pHead = this->get_Ptr();
        return const_iterator((pHead == nullptr) ? nullptr : pHead->get_DataEnd());
    }
};
}  // namespace Gray
#endif
