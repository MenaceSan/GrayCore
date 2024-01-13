//
//! @file cQueue.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cQueue_H
#define _INC_cQueue_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "cArrayT.h"
#include "cHeap.h"
#include "cSpan.h"
#include "cStreamProgress.h"

namespace Gray {
/// <summary>
/// All types of queues have indexes in common. read index and write index.
/// Derived class might be grow-able, static vs dynamic memory, fixed size, wrappable, etc.
/// Base class for all cQueue*.
/// </summary>
class GRAYCORE_LINK cQueueIndex {
 protected:
    ITERATE_t m_nReadLast;   /// old items removed/read from here.
    ITERATE_t m_nWriteLast;  /// new items added/written here. end of readable.

 public:
    cQueueIndex(ITERATE_t iReadLast = 0, ITERATE_t iWriteLast = 0) noexcept : m_nReadLast(iReadLast), m_nWriteLast(iWriteLast) {
        DEBUG_CHECK(m_nWriteLast >= m_nReadLast && m_nReadLast >= 0);
    }

    void InitQ(ITERATE_t iReadLast = 0, ITERATE_t iWriteLast = 0) noexcept {
        m_nReadLast = iReadLast;
        m_nWriteLast = iWriteLast;  // put new data here.
        DEBUG_CHECK(m_nWriteLast >= m_nReadLast && m_nReadLast >= 0);
    }
    bool isEmptyQ() const noexcept {
        return m_nReadLast == m_nWriteLast;
    }
    /// <summary>
    /// Set empty. thread safe. single instruction operations cannot be time sliced.
    /// @note Should NOT be called by the Put thread !
    /// </summary>
    void SetEmptyQ() noexcept {
        m_nReadLast = m_nWriteLast = 0;
    }

    /// <summary>
    /// get Next read position.
    /// </summary>
    /// <returns></returns>
    ITERATE_t get_ReadIndex() const noexcept {
        return m_nReadLast;
    }
    /// <summary>
    /// get Next write position.
    /// </summary>
    ITERATE_t get_WriteIndex() const noexcept {
        return m_nWriteLast;
    }

    /// <summary>
    /// How much data is avail to read? // Assume will not will wrap to fill.
    /// </summary>
    /// <returns>Quantity of TYPE COUNT_t</returns>
    inline ITERATE_t get_ReadQty() const noexcept {
        DEBUG_CHECK(m_nWriteLast >= m_nReadLast);  // Assume will not will wrap to fill.
        return m_nWriteLast - m_nReadLast;
    }

    inline void AdvanceRead(ITERATE_t iCount = 1) noexcept {
        // Assume will not will wrap to fill.
        m_nReadLast += iCount;
        DEBUG_CHECK(m_nReadLast >= 0 && m_nReadLast <= m_nWriteLast);  // Assume will not will wrap to fill.
    }
    /// <summary>
    /// Assume caller allocated space is large enough.
    /// </summary>
    inline void AdvanceWrite(ITERATE_t iCount = 1) noexcept {
        this->m_nWriteLast += iCount;
        DEBUG_CHECK(m_nWriteLast >= 0);
    }

    /// <summary>
    /// move the current read start location.
    /// </summary>
    /// <param name="iOffset"></param>
    /// <param name="eSeekOrigin">SEEK_CUR, etc</param>
    /// <returns>the New stream/file position, -lte- 0=FAILED = INVALID_SET_FILE_POINTER</returns>
    HRESULT SeekQ(STREAM_OFFSET_t nOffset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) noexcept;  // support parents SeekX
};

//*********************************************************************

/// <summary>
/// a simple read only queue. un-managed memory
/// cMemSpan = NOT owned/managed block of memory I read from. not auto freed on destruct.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE = BYTE>
struct GRAYCORE_LINK cQueueRead : public cQueueIndex, protected cSpan<TYPE> {
    cQueueRead(const TYPE* pData = nullptr, ITERATE_t iReadLast = 0, ITERATE_t iWriteLast = 0) noexcept : cQueueIndex(iReadLast, iWriteLast), cSpan<TYPE>(pData, iWriteLast * sizeof(TYPE)) {}
    ~cQueueRead() noexcept {}

    /// reset data to be read.
    void SetQueueRead(const TYPE* pData, ITERATE_t iReadLast = 0, ITERATE_t iWriteLast = 0) {
        this->SetSpanConst(pData, iWriteLast * sizeof(TYPE));
        InitQ(iReadLast, iWriteLast);
    }

    /// <summary>
    /// get start of data i could read directly. contiguous
    /// Peek into/read from the Queue's data.
    /// isEmptyQ() is OK, might be 0 length.
    /// </summary>
    inline const TYPE* get_ReadPtr() const noexcept {
        const TYPE* p = get_DataC<TYPE>() + m_nReadLast;
        DEBUG_CHECK(IsInternalPtr2(p));
        return p;
    }

    /// <summary>
    /// get a single TYPE element and advance index.
    /// </summary>
    TYPE Read1(void) {
        ASSERT(!isEmptyQ());
        return get_DataC<TYPE>()[m_nReadLast++];
    }

    /// <summary>
    /// Read but not advance.
    /// </summary>
    /// <param name="pData">nullptr = just return how much data i might get.</param>
    /// <param name="iDataMaxQty">max qty of TYPE units i can fit in pData.</param>
    /// <returns>quantity i actually read.</returns>
    HRESULT ReadPeek(TYPE* pData, ITERATE_t iDataMaxQty) noexcept {
        const ITERATE_t iQtyAvail = get_ReadQty();
        if (iDataMaxQty > iQtyAvail) iDataMaxQty = iQtyAvail;
        if (pData != nullptr) {
            cValArray::CopyQty(pData, get_ReadPtr(), iDataMaxQty);  // copy data out.
        }
        return CastN(HRESULT, iDataMaxQty);
    }

    /// <summary>
    /// Just read a block. like ReadX but for TYPE
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="iDataMaxQty">max qty of TYPE units i can fit in pData.</param>
    /// <returns>iQty i actually read.</returns>
    ITERATE_t ReadQty(TYPE* pData, ITERATE_t iDataMaxQty) noexcept {
        const ITERATE_t iQtyAvail = get_ReadQty();
        if (iDataMaxQty > iQtyAvail) iDataMaxQty = iQtyAvail;
        if (pData != nullptr) {
            cValArray::CopyQty(pData, get_ReadPtr(), iDataMaxQty);  // copy data out.
        }
        AdvanceRead(iDataMaxQty);  // advance m_nReadLast pointer.
        return iDataMaxQty;
    }

    /// <summary>
    /// Move (read) data down to not waste space. allow more space for writing.
    /// commit the read = can't get the data back. SeekX will fail.
    /// @note beware of the rollback that protocols like to do if they get a bad request or non atomic transactions.
    /// can't SeekX() back after this.
    /// pointers into this are now bad!
    /// </summary>
    /// <returns></returns>
    void ReadCommitNow() noexcept {
        if (this->m_nReadLast <= 0)  // next read is already at 0.
            return;
        const ITERATE_t iSize = this->get_ReadQty();
        if (iSize > 0) {  // there is data to move ?
            TYPE* pStart = this->get_DataW<TYPE>();
            const TYPE* pTmp = pStart + this->m_nReadLast;
            cMem::CopyOverlap(pStart, pTmp, iSize * sizeof(TYPE));
        }
        this->InitQ(0, iSize);
    }
};

//*********************************************************************

/// <summary>
/// Create an simple arbitrary queue of TYPE elements. can read and write.
/// @note This queue does NOT wrap. Does NOT grow or free. Non managed memory.
/// @note This is NOT thread safe
/// @note Does NOT free memory on destruct. Use cQueueBytes for that.
/// Does NOT auto expand buffer to hold more data if write past end.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE = BYTE>
class GRAYCORE_LINK cQueueRW : public cQueueRead<TYPE> {
    typedef cQueueRead<TYPE> SUPER_t;

 protected:
    ITERATE_t m_iAutoReadCommit;  /// Read data is destroyed once read more than this amount. make more room for writing. 0 = don't do this, just fail write if we run out of space.

 public:
    /// <summary>
    /// How much total space allocated for this?
    /// </summary>
    inline ITERATE_t get_AllocQty() const noexcept {
        return CastN(ITERATE_t, this->get_Count());
    }

    /// <summary>
    /// Cant fit any more. would have to grow buffer.
    /// </summary>
    inline bool isFullQ() const noexcept {
        return this->m_nWriteLast >= get_AllocQty();
    }

    /// <summary>
    /// How much TYPE space is avail for write into buffer? (given the current get_AllocQty() allocation size)
    /// </summary>
    /// <returns>Qty of TYPE</returns>
    inline ITERATE_t get_WriteSpaceQty() const noexcept {
        DEBUG_CHECK(this->m_nWriteLast <= get_AllocQty());
        return get_AllocQty() - this->m_nWriteLast;
    }

    /// <summary>
    /// get start of data i could write directly. contiguous
    /// </summary>
    /// <param name="iNeedCount"></param>
    /// <returns></returns>
    virtual TYPE* GetWritePrepared(ITERATE_t iNeedCount) {
        UNREFERENCED_PARAMETER(iNeedCount);
        if (!this->isValidPtr()) return nullptr;
        return this->get_DataW<TYPE>() + this->m_nWriteLast;
    }

    /// <summary>
    /// Advance index. paired with GetWritePrepared
    /// </summary>
    /// <param name="iCount"> -lt- 0 is ok.</param>
    inline void AdvanceWrite(ITERATE_t iCount = 1) {
        ASSERT(iCount <= get_WriteSpaceQty());
        SUPER_t::AdvanceWrite(iCount);
        ASSERT(this->m_nWriteLast <= get_AllocQty());
    }

 public:
    cQueueRW() noexcept : m_iAutoReadCommit(0) {}  // empty.

    explicit cQueueRW(TYPE* pData, ITERATE_t iDataAlloc, ITERATE_t iReadLast, ITERATE_t iWriteLast, ITERATE_t iAutoReadCommit = 0) noexcept : SUPER_t(pData, iReadLast, iWriteLast), m_iAutoReadCommit(iAutoReadCommit) {
        // Read / Write.
        this->put_Count(iDataAlloc);
    }
    explicit cQueueRW(const TYPE* pData, ITERATE_t iDataMax) noexcept : SUPER_t(const_cast<TYPE*>(pData), 0, iDataMax), m_iAutoReadCommit(0) {
        // Read Only iDataMax.
    }

    //***************************************************
    // Reader functions.

    /// <summary>
    /// is it time to attempt to reclaim space in the queue. (So we can write more)
    /// @note beware of the roll back that protocols like to do if they get a bad request/underflow.
    /// can't SeekX() back now !
    /// </summary>
    void ReadCommitCheck() noexcept {
        if (m_iAutoReadCommit != 0 && this->m_nReadLast >= m_iAutoReadCommit) {  // (ITERATE_t) m_nGrowSizeChunk
            this->ReadCommitNow();
        }
    }
    ITERATE_t get_AutoReadCommit() const noexcept {
        return m_iAutoReadCommit;
    }
    /// <summary>
    /// For SetSeekSizeMin. For SetSeekSizeMin
    /// </summary>
    /// <param name="iAutoReadCommit">the size at which we 'commit' contents and erase already read data. to make room for more writing.
    /// 0 = never do auto commit. we are reading and we may need to SeekX back.</param>
    void put_AutoReadCommit(ITERATE_t iAutoReadCommit = 8 * 1024) noexcept {
        m_iAutoReadCommit = iAutoReadCommit;
        if (iAutoReadCommit != 0) {
            this->ReadCommitNow();
        }
    }

    void put_ReadIndex(ITERATE_t iReadLo) {
        //! Reset the read index back to some new place.
        ASSERT(iReadLo >= 0);
        ASSERT(iReadLo <= this->m_nWriteLast);
        this->m_nReadLast = iReadLo;
        ReadCommitCheck();
    }

    // Act as a read/write stream

    /// <summary>
    /// move the current read start location.
    /// </summary>
    /// <param name="iOffset">quantity of TYPE</param>
    /// <param name="eSeekOrigin">SEEK_CUR, etc</param>
    /// <returns>the New position,  -lt- 0=FAILED = INVALID_SET_FILE_POINTER </returns>
    HRESULT SeekQ(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) noexcept {
        SUPER_t::SeekQ(iOffset, eSeekOrigin);
        ReadCommitCheck();
        return (HRESULT)this->m_nReadLast;
    }

    /// <summary>
    /// Just read a block. like ReadX but for TYPE.
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="iDataMaxQty"></param>
    /// <returns>iQty i actually read.</returns>
    ITERATE_t ReadQty(TYPE* pData, ITERATE_t iDataMaxQty) noexcept {
        const ITERATE_t nReadQty = SUPER_t::ReadQty(pData, iDataMaxQty);
        ReadCommitCheck();
        return nReadQty;
    }

    HRESULT ReadX(void* pData, size_t nDataSize) noexcept {
        //! read bytes
        const ITERATE_t nReadQty = ReadQty((TYPE*)pData, CastN(ITERATE_t, nDataSize / sizeof(TYPE)));
        return CastN(HRESULT, nReadQty * sizeof(TYPE));
    }

    //***************************************************
    // Writer functions.

    /// <summary>
    /// Write up to iQty * TYPE data to the q. like WriteX but for TYPE
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="iQty"></param>
    /// <returns>How much can did actually write in to the queue, before it gets full. 0 = was full.</returns>
    ITERATE_t WriteQty(const TYPE* pData, ITERATE_t iQty, bool atomic) {
        TYPE* pWrite = GetWritePrepared(iQty);
        const ITERATE_t iWriteSpace = get_WriteSpaceQty();  // less is ok ??
        const ITERATE_t iWriteQty = cValT::Min(iWriteSpace, iQty);
        ASSERT(this->m_nWriteLast + iWriteQty <= get_AllocQty());  // assume enough space.
        if (atomic && iQty > iWriteQty) return 0;

        if (pData != nullptr && pWrite != nullptr) {
            cValArray::CopyQty(pWrite, pData, iWriteQty);
        }
        AdvanceWrite(iWriteQty);
        return iWriteQty;
    }

    /// <summary>
    /// Write a buffer/array of bytes into the Q.
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="nDataSize">bytes NOT instances of TYPE</param>
    /// <returns>size of data added in bytes.</returns>
    HRESULT WriteX(const void* pData, size_t nDataSize) {
        ITERATE_t iWriteQty = WriteQty((TYPE*)pData, CastN(ITERATE_t, nDataSize / sizeof(TYPE)), false);
        return CastN(HRESULT, iWriteQty * sizeof(TYPE));
    }

    /// <summary>
    /// Write a single TYPE value into the Q.
    /// </summary>
    bool Write1(TYPE val) {
        TYPE* pWrite = GetWritePrepared(1);
        const ITERATE_t iWriteSpace = get_WriteSpaceQty();
        if (iWriteSpace <= 0) return false;
        if (pWrite != nullptr) {
            *pWrite = val;
        }
        AdvanceWrite(1);
        return true;
    }
    /// <summary>
    /// Write a Q into this Q
    /// </summary>
    /// <param name="queue"></param>
    /// <returns>false = full.</returns>
    bool WriteQ(cQueueRead<TYPE>& queue) {
        if (!WriteQty(queue.get_ReadPtr(), queue.get_ReadQty(), true)) return false;
        queue.SetEmptyQ();
        return true;
    }
};

//*********************************************************************

/// <summary>
/// Create a generic (dynamic sized) contiguous queue.
/// Does NOT wrap! just grows as more is written.
/// @note Needs to SetEmptyQ() or AutoReadCommit() periodically so it doesn't grow > nGrowSizeMax and FAIL!
/// freed on destruct.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE = BYTE>
class cQueueDyn : public cQueueRW<TYPE> {
    typedef cQueueRW<TYPE> SUPER_t;

 private:
    cArrayT<TYPE> m_aData;  /// dynamic sized m_pData for Queue. Maps into cSpan

 protected:
    ITERATE_t m_nGrowSizeChunk;  /// number of TYPE elements to grow by in a single re-alloc chunk. 0 = never grow.
    ITERATE_t m_nGrowSizeMax;    /// Total arbitrary max allowed for get_AllocQty(). 0 = never grow.

 protected:
    bool AllocSizeMaxQ(ITERATE_t iDataAlloc) {
        //! (re)Allocate the total size we will need.
        if (iDataAlloc > m_nGrowSizeMax) return false;  // too big !

        if (this->get_AllocQty() != iDataAlloc) {
            m_aData.put_Count(iDataAlloc);  // realloc
            this->SetSpan(m_aData->get_Span());
        }
        return true;
    }

 public:
    cQueueDyn(ITERATE_t nGrowSizeChunk = 64, ITERATE_t nGrowSizeMax = (cHeap::k_ALLOC_MAX / sizeof(TYPE))) noexcept : m_nGrowSizeChunk(nGrowSizeChunk), m_nGrowSizeMax(nGrowSizeMax) {
        //! @arg nGrowSizeMax = 0 = not used. write only ?

        DEBUG_CHECK(m_nGrowSizeChunk >= 0);
        DEBUG_CHECK(m_nGrowSizeMax >= 0);
        if (m_nGrowSizeMax > 0) {
            if (m_nGrowSizeChunk < 64)  // reasonable size MIN.
                m_nGrowSizeChunk = 64;
            if (m_nGrowSizeChunk > m_nGrowSizeMax)  // Not allowed.
                m_nGrowSizeMax = m_nGrowSizeChunk;
        } else {
            m_nGrowSizeChunk = 0;  // Must both be 0.
        }
        AllocSizeMaxQ(m_nGrowSizeChunk);
        this->put_AutoReadCommit(m_nGrowSizeChunk / 2);  // default = half buffer.
    }
    virtual ~cQueueDyn() {
        //! m_aData is freed
    }

    void put_GrowSizeChunk(ITERATE_t nGrowSizeChunk) noexcept {
        //! How big are the chunks if we need to grow.
        if (nGrowSizeChunk > m_nGrowSizeMax)  // Must not be greater!
            m_nGrowSizeMax = nGrowSizeChunk;
        m_nGrowSizeChunk = nGrowSizeChunk;
    }
    inline ITERATE_t get_GrowSizeChunk() const noexcept {
        return m_nGrowSizeChunk;
    }

    /// <summary>
    /// Try to get enough room to write iNeedCount of TYPE.
    /// Use get_WriteSpaceQty() to check size avail. Grow buffer if i need to/can.
    /// paired with AdvanceWrite()
    /// </summary>
    /// <param name="iNeedCount"></param>
    /// <returns></returns>
    TYPE* GetWritePrepared(ITERATE_t iNeedCount) override {
        const ITERATE_t iRoom = this->get_WriteSpaceQty();
        if (iRoom < iNeedCount) {  // all set?
            // need more.
            const ITERATE_t iOldAllocQty = this->get_AllocQty();
            if (iOldAllocQty < m_nGrowSizeMax) {                    // I cant get enough room. I cant write.
                const ITERATE_t iGrowRequest = iNeedCount - iRoom;  // how much MORE do i need?
                ASSERT(iGrowRequest > 0);

                // must grow by rounded up chunk size.
                ASSERT(m_nGrowSizeChunk > 0);
                ITERATE_t iChunksGrow = iGrowRequest / m_nGrowSizeChunk;
                const ITERATE_t iRem = iGrowRequest % m_nGrowSizeChunk;
                if (iRem != 0) iChunksGrow++;
                ASSERT(iChunksGrow > 0);

                ITERATE_t iNewAllocQty = iOldAllocQty + iChunksGrow * m_nGrowSizeChunk;
                if (iNewAllocQty > m_nGrowSizeMax) {  // Chunks too big !
                    iNewAllocQty = m_nGrowSizeMax;
                }

                AllocSizeMaxQ(iNewAllocQty);
            }
        }
        return SUPER_t::GetWritePrepared(iNeedCount);
    }
};

//*********************************************************************

/// <summary>
/// a dynamic BYTE queue that grows the memory allocation as needed.
/// free on destruct.
/// </summary>
class GRAYCORE_LINK cQueueBytes : public cQueueDyn<BYTE> {
    typedef cQueueDyn<BYTE> SUPER_t;

 public:
    explicit cQueueBytes(size_t nGrowSizeChunk = 8 * 1024, size_t nGrowSizeMax = cHeap::k_ALLOC_MAX) noexcept : SUPER_t(CastN(ITERATE_t, nGrowSizeChunk), CastN(ITERATE_t, nGrowSizeMax)) {
        //! @arg nGrowSizeMax = 0 = not used. write only ? total size < nGrowSizeMax.
    }
    virtual ~cQueueBytes() {
        // Memory is freed
    }

    /// <summary>
    /// Insert data at the head of the queue. first out.
    /// </summary>
    /// <param name="pDataSrc"></param>
    /// <param name="iLen"></param>
    /// <returns></returns>
    bool InsertDataHead(const BYTE* pDataSrc, size_t iLen) {
        BYTE* pWriteSpace = GetWritePrepared(CastN(ITERATE_t, iLen));
        UNREFERENCED_PARAMETER(pWriteSpace);
        if ((size_t)get_WriteSpaceQty() < iLen) return false;
        BYTE* pDataHead = get_DataW();
        cMem::CopyOverlap(pDataHead + m_nReadLast + iLen, pDataHead + m_nReadLast, get_ReadQty());
        cMem::Copy(pDataHead + m_nReadLast, pDataSrc, iLen);
        AdvanceWrite(CastN(ITERATE_t, iLen));
        return true;
    }
    /// <summary>
    /// Replace with new data. Toss any previous data.
    /// just set the data in the queue. erase any previous data
    /// </summary>
    bool SetAllData(const BYTE* pData, size_t iLen) {
        if (iLen > get_DataSize()) {  // need to grow ?
            if (!AllocSizeMaxQ(CastN(ITERATE_t, iLen))) return false;
        }
        cMem::CopyOverlap(get_DataW(), pData, iLen);  // may be from the same buffer!? use memmove to be safe.
        InitQ(0, CastN(ITERATE_t, iLen));
        return true;
    }
};

#ifndef GRAY_STATICLIB  // force implementation/instantiate for DLL/SO.
template struct GRAYCORE_LINK cQueueRead<BYTE>;
template class GRAYCORE_LINK cQueueRW<BYTE>;
#endif
}  // namespace Gray
#endif  // _INC_cQueue_H
