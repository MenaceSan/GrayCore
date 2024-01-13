//
//! @file cStreamQueue.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cStreamQueue_H
#define _INC_cStreamQueue_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "StrBuilder.h"
#include "cHeap.h"
#include "cQueue.h"
#include "cStream.h"

namespace Gray {
/// <summary>
/// Read and write to/from a dynamic memory cStream.
/// Grow the cQueueBytes memory allocation as needed.
/// similar to cFileMem, System.IO.MemoryStream
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
    ~cStreamQueue() noexcept {}

    HRESULT WriteX(const void* pData, size_t nDataSize) override {
        return SUPER_t::WriteX(pData, nDataSize);
    }
    HRESULT ReadX(void* pData, size_t nDataSize) noexcept override {
        return SUPER_t::ReadX(pData, nDataSize);
    }
    HRESULT ReadPeek(void* pData, size_t nDataSize) override {
        return SUPER_t::ReadPeek(static_cast<BYTE*>(pData), CastN(ITERATE_t, nDataSize));
    }
    size_t SetSeekSizeMin(size_t nSizeMin = k_FILE_BLOCK_SIZE) override {
        //! similar to ReadCommit (put_AutoReadCommitSize) size. Used by cStreamTransaction.
        //! @arg nSizeMin = 0 = turn off auto read commit. Allow SeekX() back.
        //! @return previous value for get_AutoReadCommit.
        const ITERATE_t iAutoReadCommit = this->get_AutoReadCommit();
        this->put_AutoReadCommit(CastN(ITERATE_t, nSizeMin));
        return CastN(size_t, iAutoReadCommit);
    }
    HRESULT SeekX(STREAM_OFFSET_t offset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) noexcept override {
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
 
    explicit cStreamStatic() noexcept : cQueueRW<BYTE>(nullptr, 0, 0, 0, 0) {
        //! Empty
        //! Read Only 0.
    }
    explicit cStreamStatic(void* pData, size_t iDataMax, size_t iReadLast, size_t iWriteLast, size_t iAutoReadCommit = 0) noexcept
        : cQueueRW<BYTE>(PtrCast<BYTE>(pData), (ITERATE_t)iDataMax, (ITERATE_t)iReadLast, (ITERATE_t)iWriteLast, (ITERATE_t)iAutoReadCommit) {
        //! Read/Write to provided pData cMemSpan
    }
    explicit cStreamStatic(const void* pData, size_t iDataMax) : cQueueRW<BYTE>(static_cast<const BYTE*>(pData), (ITERATE_t)iDataMax) {
        //! Used to serve a memory string as a cStream. AKA StringStream or StreamString.
        //! Read Only iDataMax.
    }
    explicit cStreamStatic(const cMemSpan& m) : cQueueRW<BYTE>(m.get_DataC<BYTE>(), (ITERATE_t)m.get_DataSize()) {
        //! Used to serve a memory string as a cStream. AKA StringStream or StreamString.
        //! Read Only m.get_Size().
    }
    cStreamStatic(const cStreamStatic& a) {
        //! do nothing! copy ??
        UNREFERENCED_REFERENCE(a);
    }
    ~cStreamStatic() {}
 
    HRESULT SeekX(STREAM_OFFSET_t offset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) noexcept override {
        return this->SeekQ(offset, eSeekOrigin);
    }
    STREAM_POS_t GetPosition() const override {
        return this->get_ReadIndex();
    }
    STREAM_POS_t GetLength() const override {
        //! Get the full Seek-able length. not just get_ReadQty() left to read. Assume Seek(0) read length.
        return this->get_WriteIndex();
    }

    HRESULT WriteX(const void* pData, size_t nDataSize) override {
        return SUPER_t::WriteX(pData, nDataSize);
    }
    HRESULT ReadX(void* pData, size_t nDataSize) noexcept override {
        return SUPER_t::ReadX(pData, nDataSize);
    }
    HRESULT ReadPeek(void* pData, size_t nDataSize) override {
        return SUPER_t::ReadPeek(static_cast<BYTE*>(pData), (ITERATE_t)nDataSize);
    }
};

/// <summary>
/// Build a string as a stream based on stack allocated/inline memory. cStreamOutput
/// Similar to StrBuilder .  cStreamQueue
/// equiv to STL string builder. std::stringstream
/// </summary>
struct GRAYCORE_LINK cStreamStringA : public cStreamOutput, public StrBuilder<char> {
    typedef StrBuilder<char> SUPER_t;
 
    cStreamStringA() noexcept : SUPER_t(k_nGrowSizeChunk) {
        // Write only
    }
    char* get_PtrA() {
        return SUPER_t::get_DataWork();
    }
    operator char*() {
        return get_PtrA();
    }

    STREAM_POS_t GetLength() const override {
        return SUPER_t::get_Length();
    }
    HRESULT WriteString(const char* pszStr) override {
        // cStreamOutput
        return SUPER_t::AddStr(pszStr);
    }
};
}  // namespace Gray

#endif  // _INC_cStreamQueue_H
