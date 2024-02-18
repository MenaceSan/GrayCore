//! @file cStreamQueue.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cStreamQueue_H
#define _INC_cStreamQueue_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "StrBuilder.h"
#include "cHeap.h"
#include "cQueueDyn.h"
#include "cStream.h"

namespace Gray {
/// <summary>
/// Read and write to/from a dynamic memory cStream.
/// Grow the cQueueBytes memory allocation as needed.
/// similar to StrBuilder, cFileMem, System.IO.MemoryStream
/// </summary>
class GRAYCORE_LINK cStreamQueue : public cStream, public cQueueBytes {
    typedef cQueueBytes SUPER_t;

 protected:
    cStreamQueue(const cStreamQueue& a) noexcept {
        //! do nothing!
        UNREFERENCED_REFERENCE(a);
    }

 public:
    cStreamQueue(size_t nGrowSizeChunk = 4 * 1024, size_t nGrowSizeMax = cHeap::k_ALLOC_MAX) noexcept : SUPER_t(nGrowSizeChunk, nGrowSizeMax) {
        //! @arg nGrowSizeMax = 0 = not used. write only ?
    }

    HRESULT WriteX(const void* pData, size_t nDataSize) override {
        return SUPER_t::WriteX(pData, nDataSize);
    }
    HRESULT ReadX(void* pData, size_t nDataSize) noexcept override {
        return SUPER_t::ReadX(pData, nDataSize);
    }
    HRESULT ReadPeek(cMemSpan& ret) override {
        return SUPER_t::ReadPeek(cSpanX<BYTE>(ret));
    }
    /// <summary>
    /// Set the Auto Read commit size. Allow SeekX back for incomplete transactions to a certain size.
    /// similar to ReadCommit (put_AutoReadCommitSize) size. Used by cStreamTransaction.
    /// </summary>
    /// <param name="nSizeMin">AutoReadCommit size. 0 = turn off auto read commit. Allow SeekX() back.</param>
    /// <returns>previous value for get_AutoReadCommit.</returns>
    size_t SetReadCommitSize(size_t nSizeMin = k_FILE_BLOCK_SIZE) override {
        const ITERATE_t prevReadCommit = SUPER_t::get_AutoReadCommit();
        SUPER_t::put_AutoReadCommit(CastN(ITERATE_t, nSizeMin));
        return CastN(size_t, prevReadCommit);
    }
    HRESULT SeekX(STREAM_OFFSET_t offset, SEEK_t eSeekOrigin = SEEK_t::_Set) noexcept override {
        return SUPER_t::SeekQ(offset, eSeekOrigin);
    }
    STREAM_POS_t GetPosition() const override {
        return this->get_ReadIndex();
    }
    STREAM_POS_t GetLength() const override {
        //! Get the full Seek-able length. line Seek(SEEK_END).
        return this->get_WriteIndex();  // get_ReadQty()
    }
};

/// <summary>
/// Read and write to a single preallocated memory block as cStream.
/// Data block is pre-allocated and provided.
/// DO NOT grow the memory allocation if needed.
/// </summary>
struct GRAYCORE_LINK cStreamStatic : public cStream, public cQueueRW<BYTE> {
    typedef cQueueRW<BYTE> SUPER_t;

    /// <summary>
    /// Empty. Read Only 0.
    /// </summary>
    explicit cStreamStatic() noexcept {}
    /// Read/Write to provided pData cMemSpan
    explicit cStreamStatic(const cSpan<BYTE>& span, size_t iReadLast, size_t iWriteLast, size_t iAutoReadCommit = 0) noexcept : SUPER_t(span, (ITERATE_t)iReadLast, (ITERATE_t)iWriteLast, (ITERATE_t)iAutoReadCommit) {}
    /// <summary>
    /// serve a memory string as a cStream. AKA StringStream? Read Only.
    /// </summary>
    explicit cStreamStatic(const cMemSpan& span) : SUPER_t(cSpan<BYTE>(span)) {}

    /// <summary>
    /// do nothing! copy ??
    /// </summary>
    cStreamStatic(const cStreamStatic& a) {
        UNREFERENCED_REFERENCE(a);
    }

    HRESULT WriteX(const void* pData, size_t nDataSize) override {
        return SUPER_t::WriteX(pData, nDataSize);
    }
    HRESULT ReadX(void* pData, size_t nDataSize) noexcept override {
        return SUPER_t::ReadX(pData, nDataSize);
    }
    HRESULT ReadPeek(cMemSpan& ret) override {
        return SUPER_t::ReadPeek(cSpanX<BYTE>(ret));
    }

    HRESULT SeekX(STREAM_OFFSET_t offset, SEEK_t eSeekOrigin = SEEK_t::_Set) noexcept override {
        return this->SeekQ(offset, eSeekOrigin);
    }
    STREAM_POS_t GetPosition() const override {
        return this->get_ReadIndex();
    }
    STREAM_POS_t GetLength() const override {
        //! Get the full Seek-able length. not just get_ReadQty() left to read. Assume Seek(0) read length.
        return this->get_WriteIndex();
    }
};
}  // namespace Gray

#endif  // _INC_cStreamQueue_H
