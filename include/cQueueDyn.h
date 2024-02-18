//! @file cQueueDyn.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cQueueDyn_H
#define _INC_cQueueDyn_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cHeap.h"
#include "cQueue.h"

namespace Gray {
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
    cArrayT<TYPE> m_aData;  /// dynamic sized storage for cQueueRW. Maps into cSpan. TODO USE cBlob !!

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
            if (m_nGrowSizeChunk < 64)  // reasonable MIN chunk size.
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
    TYPE* GetWritePrep(ITERATE_t iNeedCount) override {
        const ITERATE_t iRoom = this->get_WriteSpaceQty();
        if (iNeedCount > iRoom) {  // no room?
            const ITERATE_t iOldAllocQty = this->get_AllocQty();
            if (iOldAllocQty < m_nGrowSizeMax) {                                     // I cant get enough room. I cant write.
                const ITERATE_t iNewAllocQty = iOldAllocQty + (iNeedCount - iRoom);  // new total needed.
                ASSERT(iNewAllocQty > 0);
                ASSERT(m_nGrowSizeChunk > 0);                                                               // must grow by rounded up chunk size.
                const ITERATE_t nChunksAlloc = (iNewAllocQty + (m_nGrowSizeChunk - 1)) / m_nGrowSizeChunk;  // round up to next m_nGrowSizeChunk.
                ASSERT(nChunksAlloc > 0);
                AllocSizeMaxQ(cValT::Min(nChunksAlloc * m_nGrowSizeChunk, m_nGrowSizeMax));
            }
        }
        return SUPER_t::GetWritePrep(iNeedCount);
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
        BYTE* pWriteSpace = GetWritePrep(CastN(ITERATE_t, iLen));
        UNREFERENCED_PARAMETER(pWriteSpace);
        if ((size_t)get_WriteSpaceQty() < iLen) return false;
        BYTE* pDataRead = get_DataW() + get_ReadIndex();
        cMem::CopyOverlap(pDataRead + iLen, pDataRead, get_ReadQty());
        cMem::Copy(pDataRead, pDataSrc, iLen);
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
}  // namespace Gray
#endif
