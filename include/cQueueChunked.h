//! @file cQueueChunked.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cQueueChunked_H
#define _INC_cQueueChunked_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cQueue.h"

namespace Gray {
/// <summary>
/// a list of Q chunks to use as a large queue.
/// Delete chunks when fully read. Infinitely growing.
/// allocate new chunks when we need to write.
/// No attempt to pool allocated chunks.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <COUNT_t _CHUNKGROW = 1024, typename TYPE = BYTE>
class cQueueChunked : public cQueueIndex {
    /// A single chunk in cQueueChunked list.
    struct Chunk_t : public cSpanStatic<_CHUNKGROW, TYPE> {
        Chunk_t* _pNext = nullptr;
    };
    Chunk_t* _pFirst = nullptr;
    Chunk_t* _pLast = nullptr;
    ITERATE_t _nFirstIndex = 0;  /// Arbitrary read index of the _pFirst.
    ITERATE_t _nLastIndex = 0;   /// Arbitrary write index of the _pLast

    /// <summary>
    /// Get offset of _nReadIndex inside _pFirst
    /// </summary>
    ITERATE_t get_FirstReadIndex() const {
        ASSERT(_nReadIndex >= _nFirstIndex);
        return _nReadIndex - _nFirstIndex;
    }
    ITERATE_t GetReadChunkAvail(const ITERATE_t firstReadIndex) const {
        ASSERT(firstReadIndex <= _CHUNKGROW);
        const ITERATE_t firstWriteIndex = cValT::Min<ITERATE_t>(_nWriteIndex - _nFirstIndex, _CHUNKGROW);
        ASSERT(firstWriteIndex >= firstReadIndex && firstWriteIndex <= _CHUNKGROW);
        return firstWriteIndex - firstReadIndex;  // might be 0.
    }
    const TYPE* GetReadPtr(const ITERATE_t firstReadIndex) const {
        ASSERT(firstReadIndex <= _CHUNKGROW);
        ASSERT_NN(_pFirst);
        return _pFirst->GetPtrC() + firstReadIndex;
    }

    /// <summary>
    /// Get offset of _nWriteIndex inside _pLast
    /// </summary>
    ITERATE_t get_LastWriteIndex() const {
        ASSERT(_nWriteIndex >= _nLastIndex);
        return _nWriteIndex - _nLastIndex;
    }
    ITERATE_t get_WriteChunkAvail() const {
        const ITERATE_t lastWriteIndex = get_LastWriteIndex();
        ASSERT(lastWriteIndex <= _CHUNKGROW);
        ASSERT_NN(_pLast);
        return _CHUNKGROW - lastWriteIndex;
    }

    /// <summary>
    /// Use get_WriteChunkAvail() to check actual size to write.
    /// </summary>
    /// <returns></returns>
    TYPE* GetWritePrep() {
        if (_pLast == nullptr) {
            _pFirst = _pLast = new Chunk_t();
            return _pLast->GetPtrW();
        }
        const ITERATE_t lastWriteIndex = get_LastWriteIndex();
        ASSERT(lastWriteIndex <= _CHUNKGROW);
        if (lastWriteIndex >= _CHUNKGROW) {  // no space left.
            Chunk_t* pNext = new Chunk_t();
            _pLast->_pNext = pNext;
            _pLast = pNext;
            _nLastIndex += _CHUNKGROW;
            return _pLast->GetPtrW();
        }
        return _pLast->GetPtrW() + lastWriteIndex;
    }

    cSpanX<TYPE> get_SpanWrite() {
        TYPE* p = GetWritePrep();   // Call this first.
        return ToSpan(p, get_WriteChunkAvail());
    }

    void DeleteFirst() {
        Chunk_t* pPrev = _pFirst;
        _pFirst = _pFirst->_pNext;
        delete pPrev;
    }

    /// Write is complete so advance the write pointer. Called AFTER GetWritePrep();
    void AdvanceWrite(ITERATE_t iCount = 1) {
        // iCount of TYPE
        ASSERT_NN(_pLast);
        ASSERT(iCount <= get_WriteChunkAvail());
        cQueueIndex::AdvanceWrite(iCount);
    }

 public:
    ~cQueueChunked() {
        SetEmptyQ();
    }

    cSpan<TYPE> get_SpanRead() const {
        if (this->isEmptyQ()) return cSpan<TYPE>();
        const ITERATE_t firstReadIndex = get_FirstReadIndex();
        return ToSpan(GetReadPtr(firstReadIndex), GetReadChunkAvail(firstReadIndex));
    }

    /// <summary>
    /// Destructive read. called after we read actual data.
    /// </summary>
    void AdvanceRead(ITERATE_t iCount = 1) {
        while (iCount > 0) {
            ASSERT(!isEmptyQ());
            ASSERT_NN(_pFirst);
            const ITERATE_t firstReadIndex = get_FirstReadIndex();
            const ITERATE_t countChunk = GetReadChunkAvail(firstReadIndex);
            const ITERATE_t countChunkRead = cValT::Min(countChunk, iCount);
            ASSERT(countChunkRead);
            cQueueIndex::AdvanceRead(countChunkRead);
            iCount -= countChunkRead;
            if (firstReadIndex + countChunkRead >= _CHUNKGROW) {  // chunk is used up?
                DeleteFirst();                                   // delete chunk only when completely used up. not just isEmptyQ.
                _nFirstIndex += _CHUNKGROW;
            }
        }
        if (_pFirst == nullptr) {
            _pLast = nullptr;
            ASSERT(this->isEmptyQ());
        }
    }

    TYPE Read1() {
        const ITERATE_t firstReadIndex = get_FirstReadIndex();
        TYPE val = *GetReadPtr(firstReadIndex);
        AdvanceRead(1);
        return val;
    }

    /// copy data out.
    ITERATE_t ReadSpanQ(cSpanX<TYPE> ret) {
        ITERATE_t i = 0;
        TYPE* pBuf = ret.get_PtrWork();
        for (; !isEmptyQ() && i < ret.GetSize(); i++) {
            pBuf[i] = Read1();
        }
        return i;
    }

    void SetEmptyQ() {
        cQueueIndex::SetEmptyQ();
        while (_pFirst != nullptr) {  // Full destruction.
            DeleteFirst();
        }
        _pLast = nullptr;
    }

    void Write1(TYPE val) {
        *GetWritePrep() = val;  // ALWAY has at least 1. will grow.
        AdvanceWrite(1);
    }

    /// copy all data into Q.
    void WriteSpanQ(const cSpan<TYPE>& src) {
        ITERATE_t nCount = src.GetSize();
        const TYPE* pSrc = src.get_PtrConst();
        while (nCount > 0) {
            const auto chunkQty = get_SpanWrite().SetCopyQty(pSrc, nCount);
            AdvanceWrite(chunkQty);
            pSrc += chunkQty;
            nCount -= chunkQty;
        }
    }
};
}  // namespace Gray
#endif  // _INC_cQueueChunked_H
