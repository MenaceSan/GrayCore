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
    cArrayT<TYPE> _aData;  /// dynamic sized storage for cQueueRW. Maps into cSpan. TODO USE cBlob !!

 protected:
    ITERATE_t _nGrowSizeChunk = 64;  /// number of TYPE elements to grow by in a single re-alloc chunk. 0 = never grow.
    ITERATE_t _nGrowSizeMax = cMem::k_ALLOC_MAX / sizeof(TYPE);  /// Total arbitrary max allowed for get_AllocQty(). 0 = never grow.

 protected:
    bool AllocSizeMaxQ(ITERATE_t iDataAlloc) {
        //! (re)Allocate the total size we will need.
        if (iDataAlloc > _nGrowSizeMax) return false;  // too big !

        if (this->get_AllocQty() != iDataAlloc) {
            _aData.put_Count(iDataAlloc);  // realloc
            this->SetSpan(_aData->get_Span());
        }
        return true;
    }

 public:
    cQueueDyn(ITERATE_t nGrowSizeChunk = 64, ITERATE_t nGrowSizeMax = (cMem::k_ALLOC_MAX / sizeof(TYPE))) noexcept : _nGrowSizeChunk(nGrowSizeChunk), _nGrowSizeMax(nGrowSizeMax) {
        //! @arg nGrowSizeMax = 0 = not used. write only ?

        DEBUG_CHECK(_nGrowSizeChunk >= 0);
        DEBUG_CHECK(_nGrowSizeMax >= 0);
        if (_nGrowSizeMax > 0) {
            if (_nGrowSizeChunk < 64)  // reasonable MIN chunk size.
                _nGrowSizeChunk = 64;
            if (_nGrowSizeChunk > _nGrowSizeMax)  // Not allowed.
                _nGrowSizeMax = _nGrowSizeChunk;
        } else {
            _nGrowSizeChunk = 0;  // Must both be 0.
        }
        AllocSizeMaxQ(_nGrowSizeChunk);
        this->put_AutoReadCommit(_nGrowSizeChunk / 2);  // default = half buffer.
    }
    virtual ~cQueueDyn() {
        //! _aData is freed
    }

    /// <summary>
    /// How big are the chunks if we need to grow.
    /// </summary>
    void put_GrowSizeChunk(ITERATE_t nGrowSizeChunk) noexcept {
        if (nGrowSizeChunk > _nGrowSizeMax) _nGrowSizeMax = nGrowSizeChunk;  // Must not be greater!            
        _nGrowSizeChunk = nGrowSizeChunk;
    }
    inline ITERATE_t get_GrowSizeChunk() const noexcept {
        return _nGrowSizeChunk;
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
            if (iOldAllocQty < _nGrowSizeMax) {                                     // I cant get enough room. I cant write.
                const ITERATE_t iNewAllocQty = iOldAllocQty + (iNeedCount - iRoom);  // new total needed.
                ASSERT(iNewAllocQty > 0);
                ASSERT(_nGrowSizeChunk > 0);                                                               // must grow by rounded up chunk size.
                const ITERATE_t nChunksAlloc = (iNewAllocQty + (_nGrowSizeChunk - 1)) / _nGrowSizeChunk;  // round up to next _nGrowSizeChunk.
                ASSERT(nChunksAlloc > 0);
                AllocSizeMaxQ(cValT::Min(nChunksAlloc * _nGrowSizeChunk, _nGrowSizeMax));
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
    explicit cQueueBytes(size_t nGrowSizeChunk = 8 * 1024, size_t nGrowSizeMax = cMem::k_ALLOC_MAX) noexcept : SUPER_t(CastN(ITERATE_t, nGrowSizeChunk), CastN(ITERATE_t, nGrowSizeMax)) {
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
    bool InsertDataHead(const cMemSpan& m) {
        const ITERATE_t iSize = CastN(ITERATE_t, m.get_SizeBytes());
        BYTE* const pWriteSpace = GetWritePrep(iSize);
        UNREFERENCED_PARAMETER(pWriteSpace);
        if (get_WriteSpaceQty() < iSize) return false;
        BYTE* pDataRead = GetTPtrW() + get_ReadIndex();
        cMem::CopyOverlap(pDataRead + iSize, pDataRead, get_ReadQty());
        cMem::Copy(pDataRead, m, iSize);
        AdvanceWrite(CastN(ITERATE_t, iSize));
        return true;
    }
    /// <summary>
    /// Replace with new data. Toss any previous data.
    /// just set the data in the queue. erase any previous data
    /// </summary>
    bool SetAllData(const cMemSpan& m) {
        const ITERATE_t iSize = CastN(ITERATE_t, m.get_SizeBytes());
        if (m.get_SizeBytes() > get_SizeBytes()) {  // need to grow ?
            if (!AllocSizeMaxQ(iSize)) return false;
        }
        cMem::CopyOverlap(GetTPtrW(), m, iSize);  // may be from overlapped buffer?
        InitQ(0, iSize);
        return true;
    }
};
}  // namespace Gray
#endif
