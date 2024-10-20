//! @file cQueue.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cQueue_H
#define _INC_cQueue_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "cArrayT.h"
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
    ITERATE_t _nReadIndex = 0;   /// old items removed/read from here. unspecified sized elements.
    ITERATE_t _nWriteIndex = 0;  /// new items added/written here. end of readable. like cRange

protected:
    cQueueIndex(ITERATE_t iReadIndex = 0, ITERATE_t iWriteIndex = 0) noexcept : _nReadIndex(iReadIndex), _nWriteIndex(iWriteIndex) {
        DEBUG_CHECK(isNormal());
    }

    void InitQ(ITERATE_t iReadIndex = 0, ITERATE_t iWriteIndex = 0) noexcept {
        _nReadIndex = iReadIndex;
        _nWriteIndex = iWriteIndex;  // put new data here.
        DEBUG_CHECK(isNormal());
    }

    inline void AdvanceRead(ITERATE_t iCount = 1) noexcept {
        // Assume will not will wrap to fill.
        _nReadIndex += iCount;
        DEBUG_CHECK(isNormal());  // Assume will not will wrap to fill.
    }
    /// <summary>
    /// Assume caller allocated space is large enough.
    /// </summary>
    inline void AdvanceWrite(ITERATE_t iCount = 1) noexcept {
        this->_nWriteIndex += iCount;
        DEBUG_CHECK(_nWriteIndex >= 0);
    }

    /// <summary>
    /// move the current read start location.
    /// </summary>
    /// <param name="iOffset"></param>
    /// <param name="eSeekOrigin">SEEK_CUR, etc</param>
    /// <returns>the New stream/file position, -lte- 0=FAILED = INVALID_SET_FILE_POINTER</returns>
    HRESULT SeekQ(STREAM_OFFSET_t nOffset, SEEK_t eSeekOrigin = SEEK_t::_Set) noexcept;  // support parents SeekX

 public:
    inline bool isNormal() const noexcept {
        return _nReadIndex <= _nWriteIndex && _nReadIndex >= 0;
    }
    inline bool isEmptyQ() const noexcept {
        return _nReadIndex == _nWriteIndex;
    }
    /// <summary>
    /// get Next read position.
    /// </summary>
    /// <returns></returns>
    inline ITERATE_t get_ReadIndex() const noexcept {
        return _nReadIndex;
    }
    /// <summary>
    /// get Next write position.
    /// </summary>
    inline ITERATE_t get_WriteIndex() const noexcept {
        return _nWriteIndex;
    }
    /// <summary>
    /// How much data is avail to read? // Assume will not will wrap to fill.
    /// </summary>
    /// <returns>Quantity of TYPE COUNT_t</returns>
    inline ITERATE_t get_ReadQty() const noexcept {
        DEBUG_CHECK(isNormal());  // Assume will not will wrap to fill.
        return _nWriteIndex - _nReadIndex;
    }

    /// <summary>
    /// Set empty. thread safe. single instruction operations cannot be time sliced.
    /// @note Should NOT be called by the Put thread !
    /// </summary>
    void SetEmptyQ() noexcept {
        _nReadIndex = _nWriteIndex = 0;
    }
};

//*********************************************************************

/// <summary>
/// a simple read only queue. un-managed memory
/// cMemSpan = NOT owned/managed block of memory I read from. not auto freed on destruct.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE = BYTE>
struct GRAYCORE_LINK cQueueRead : public cQueueIndex, protected cSpan<TYPE> {
    cQueueRead() {}
    cQueueRead(const cSpan<TYPE>& span, ITERATE_t iReadIndex, ITERATE_t iWriteIndex) noexcept : cQueueIndex(iReadIndex, iWriteIndex), cSpan<TYPE>(span) {}
    cQueueRead(const cMemSpan& span) noexcept : cQueueIndex(0, (ITERATE_t)(span.get_SizeBytes() / sizeof(TYPE))), cSpan<TYPE>(span) {}
    ~cQueueRead() noexcept {}

    /// reset data to be read.
    void SetQueueRead(const cSpan<TYPE>& span, ITERATE_t iReadIndex = 0) {
        this->SetSpan(span);
        this->InitQ(iReadIndex, span.GetSize());
    }

    /// <summary>
    /// get start of data i could read directly. contiguous
    /// Peek into/read from the Queue's data.
    /// isEmptyQ() is OK, might be 0 length.
    /// </summary>
    inline const TYPE* get_ReadPtr() const noexcept {
        const TYPE* p = this->template GetTPtrC<TYPE>() + this->_nReadIndex;
        DEBUG_CHECK(this->IsInternalPtr2(p));
        return p;
    }

    cSpan<TYPE> get_SpanRead() const {
        return ToSpan(this->get_ReadPtr(), this->get_ReadQty());
    }

    /// <summary>
    /// get a single TYPE element and advance index.
    /// </summary>
    TYPE Read1() {
        ASSERT(!this->isEmptyQ());
        return this->template GetTPtrC<TYPE>()[this->_nReadIndex++];
    }

    /// <summary>
    /// Read but not advance.
    /// </summary>
    /// <param name="ret">nullptr = just return how much data i might get.</param>
    /// <returns>quantity i actually read.</returns>
    HRESULT ReadPeek(cSpanX<TYPE> ret) noexcept {
        const ITERATE_t iQtyAvail = ret.SetCopySpan(get_SpanRead());
        return CastN(HRESULT, iQtyAvail);
    }

    /// <summary>
    /// Just read a block. like ReadX but for TYPE
    /// </summary>
    /// <param name="ret">qty of TYPE units i can fit.</param>
    /// <returns>iQty i actually read.</returns>
    ITERATE_t ReadSpanQ(cSpanX<TYPE> ret) noexcept {
        const ITERATE_t iQtyAvail = ret.SetCopySpan(get_SpanRead());
        this->AdvanceRead(iQtyAvail);  // advance _nReadIndex pointer.
        return CastN(HRESULT, iQtyAvail);
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
        if (this->_nReadIndex <= 0) return;  // next read is already at 0.
        const ITERATE_t iSize = this->get_ReadQty();
        if (iSize > 0) {  // there is data to move ?
            TYPE* pStart = this->template GetTPtrW<TYPE>();
            const TYPE* pTmp = pStart + this->_nReadIndex;
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
    ITERATE_t _nAutoReadCommit = 0;  /// Read data is destroyed once read more than this amount. make more room for writing. 0 = don't do this, just fail write if we run out of space.

 public:
    cQueueRW() noexcept {}  // empty.

    explicit cQueueRW(const cSpan<TYPE>& span, ITERATE_t iReadIndex, ITERATE_t iWriteIndex, ITERATE_t iAutoReadCommit = 0) noexcept : SUPER_t(span, iReadIndex, iWriteIndex), _nAutoReadCommit(iAutoReadCommit) {
        // Read / Write.
    }
    explicit cQueueRW(const cMemSpan& span) noexcept : SUPER_t(span) {
        // Read Only iDataMax.
    }

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
        return this->_nWriteIndex >= get_AllocQty();
    }

    /// <summary>
    /// How much TYPE space is avail for write into buffer? (given the current get_AllocQty() allocation size)
    /// </summary>
    /// <returns>Qty of TYPE</returns>
    inline ITERATE_t get_WriteSpaceQty() const noexcept {
        DEBUG_CHECK(this->_nWriteIndex <= get_AllocQty());
        return get_AllocQty() - this->_nWriteIndex;
    }

    /// <summary>
    /// get start of data i could write directly. contiguous
    /// </summary>
    /// <param name="iNeedCount"></param>
    /// <returns></returns>
    virtual TYPE* GetWritePrep(ITERATE_t iNeedCount) {
        UNREFERENCED_PARAMETER(iNeedCount);  // no resize available here.
        if (!this->isValidPtr()) return nullptr;
        return this->template GetTPtrW<TYPE>() + this->_nWriteIndex;
    }

    /// Must also call AdvanceWrite()
    cSpanX<TYPE> GetSpanWrite(ITERATE_t iNeedCount) {
        TYPE* p = GetWritePrep(iNeedCount);  // call this before get_WriteSpaceQty
        return ToSpan(p, get_WriteSpaceQty());
    }

    /// <summary>
    /// Advance index. paired with GetWritePrep
    /// </summary>
    /// <param name="iCount"> -lt- 0 is ok.</param>
    inline void AdvanceWrite(ITERATE_t iCount = 1) {
        ASSERT(iCount <= get_WriteSpaceQty());
        SUPER_t::AdvanceWrite(iCount);
        ASSERT(this->_nWriteIndex <= get_AllocQty());
    }

    //***************************************************
    // Reader functions.

    /// <summary>
    /// is it time to attempt to reclaim space in the queue. (So we can write more)
    /// @note beware of the roll back that protocols like to do if they get a bad request/underflow.
    /// can't SeekX() back now !
    /// </summary>
    void ReadCommitCheck() noexcept {
        if (_nAutoReadCommit != 0 && this->_nReadIndex >= _nAutoReadCommit) {  // (ITERATE_t) _nGrowSizeChunk
            this->ReadCommitNow();
        }
    }
    ITERATE_t get_AutoReadCommit() const noexcept {
        return _nAutoReadCommit;
    }
    /// <summary>
    /// For SetReadCommitSize.
    /// </summary>
    /// <param name="iAutoReadCommit">the size at which we 'commit' contents and erase already read data. to make room for more writing.
    /// 0 = never do auto commit. we are reading and we may need to SeekX back.</param>
    void put_AutoReadCommit(ITERATE_t iAutoReadCommit = 8 * 1024) noexcept {
        if (_nAutoReadCommit == iAutoReadCommit) {
            ReadCommitCheck();
            return;
        }
        _nAutoReadCommit = iAutoReadCommit;
        if (iAutoReadCommit != 0) {
            this->ReadCommitNow();
        }
    }

    /// <summary>
    /// Reset the read index back to some new place.
    /// </summary>
    void put_ReadIndex(ITERATE_t iReadLo) {
        ASSERT(iReadLo >= 0);
        ASSERT(iReadLo <= this->get_WriteIndex());
        this->_nReadIndex = iReadLo;
        ReadCommitCheck();
    }

    // Act as a read/write stream

    /// <summary>
    /// move the current read start location.
    /// </summary>
    /// <param name="iOffset">quantity of TYPE</param>
    /// <param name="eSeekOrigin">SEEK_CUR, etc</param>
    /// <returns>the New position,  -lt- 0=FAILED = INVALID_SET_FILE_POINTER </returns>
    HRESULT SeekQ(STREAM_OFFSET_t iOffset, SEEK_t eSeekOrigin = SEEK_t::_Set) noexcept {
        SUPER_t::SeekQ(iOffset, eSeekOrigin);
        ReadCommitCheck();
        return CastN(HRESULT, this->get_ReadIndex());
    }

    /// <summary>
    /// Just read a block. like ReadX but for TYPE.
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="iDataMaxQty"></param>
    /// <returns>iQty i actually read.</returns>
    ITERATE_t ReadSpanQ(cSpanX<TYPE> ret) noexcept {
        const ITERATE_t nReadQty = SUPER_t::ReadSpanQ(ret);
        ReadCommitCheck();
        return nReadQty;
    }

    /// <summary>
    /// Read some bytes
    /// </summary>
    HRESULT ReadX(cMemSpan ret) noexcept {
        const ITERATE_t nReadQty = ReadSpanQ(cSpanX<TYPE>(ret));
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
    ITERATE_t WriteSpanQ(const cSpan<TYPE>& src, bool atomic) {
        auto spanWrite = GetSpanWrite(src.GetSize());  // less is ok ??
        if (atomic && src.GetSize() > spanWrite.GetSize()) return 0;
        const ITERATE_t iWriteQty = spanWrite.SetCopySpan(src);
        AdvanceWrite(iWriteQty);
        return iWriteQty;
    }

    /// <summary>
    /// Write a buffer/array of bytes into the Q.
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="nDataSize">bytes NOT instances of TYPE</param>
    /// <returns>size of data added in bytes.</returns>
    HRESULT WriteX(const cMemSpan& m) {
        const ITERATE_t iWriteQty = WriteSpanQ(cSpan<TYPE>(m), false);
        return CastN(HRESULT, iWriteQty * sizeof(TYPE));
    }

    /// <summary>
    /// Write a single TYPE value into the Q.
    /// </summary>
    bool Write1(TYPE val) {
        cSpanX<TYPE> spanWrite = GetSpanWrite(1);
        if (spanWrite.isEmpty()) return false;
        if (!spanWrite.isNull()) {
            *spanWrite.get_PtrWork() = val;
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
        if (!WriteSpanQ(queue.get_SpanRead(), true)) return false;
        queue.SetEmptyQ();
        return true;
    }
};

//*********************************************************************

/// <summary>
/// Create a generic thread/multi process safe (static sized) wrapping queue.
/// similar to std::istringstream except NOT on Heap. static allocations.
/// @note Get and Put are NOT reentrant safe against themselves, just each other.
/// _nWriteIndex and _nReadIndex will wrap to fill.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <ITERATE_t _QTY = 1024, class TYPE = BYTE>
class GRAYCORE_LINK cQueueStatic : public cQueueIndex, public cSpanStatic<_QTY, TYPE> {
    inline ITERATE_t get_ReadQty() const {  // Don't use this if we wrap.
        ASSERT(false);
        return 0;
    }

 public:
    cQueueStatic() noexcept {
        STATIC_ASSERT(_QTY > 0, _QTY);
#if defined(_DEBUG)
        cMem::Zero(_Data, sizeof(_Data));
#endif
    }
    bool isFullQ() const noexcept {
        return this->GetWrapIndex(_nWriteIndex + 1) == this->_nReadIndex;
    }
    /// <summary>
    /// How much Total data is in the Queue ? may be wrapped. Thread safe.
    /// </summary>
    ITERATE_t get_ReadQtyT() const noexcept {
        const ITERATE_t iRead = this->_nReadIndex;
        ITERATE_t iWrite = this->_nWriteIndex;
        if (iRead > iWrite) {  // wrap
            iWrite += _QTY;
            DEBUG_ASSERT(iWrite > iRead, "get_ReadQtyT");  // sanity check. should never happen!
        }
        return iWrite - iRead;
    }
    /// <summary>
    /// get Max we can get in a single CONTIGUOUS block peek/read. For use with get_ReadPtr().
    /// </summary>
    ITERATE_t get_ReadQtyC() const noexcept {
        const ITERATE_t iTop = (this->_nWriteIndex >= this->_nReadIndex) ? this->_nWriteIndex : _QTY;
        return iTop - this->_nReadIndex;
    }
    /// <summary>
    /// use get_ReadQtyC() to get the allowed size.
    /// </summary>
    const TYPE* get_ReadPtr() const {
        ASSERT(!isEmptyQ());  // ONLY call this if there is data to read.
        return &this->_Data[this->_nReadIndex];
    }
    void AdvanceRead(ITERATE_t iCount = 1) {
        const ITERATE_t iReadCount = get_ReadQtyT();
        if (iCount > iReadCount) iCount = iReadCount;
        this->_nReadIndex = this->GetWrapIndex(this->_nReadIndex + iCount);
    }

    /// <summary>
    /// total available space to write - not contiguous.
    /// @note since read=write=empty we can only use QTY-1 to write.
    /// </summary>
    ITERATE_t get_WriteQtyT() const noexcept {
        return (_QTY - 1) - get_ReadQtyT();
    }

    /// <summary>
    /// Read a single TYPE element. Thread safe against Write.
    /// @note This is NOT reentrant safe. i.e. multi calls to Read().
    /// @note ASSERT if empty!
    /// </summary>
    /// <returns></returns>
    TYPE Read1() {
        ASSERT(!isEmptyQ());
        const ITERATE_t iRead = this->_nReadIndex;
        ASSERT(IS_INDEX_GOOD(iRead, _QTY));
        const ITERATE_t iReadNext = this->GetWrapIndex(iRead + 1);
        const TYPE val = this->_Data[iRead];
        this->_nReadIndex = iReadNext;
        return val;
    }

    /// <summary>
    /// copy TYPE data out. NOT thread safe.
    /// @todo use cValSpan::CopyQty ?
    /// </summary>
    ITERATE_t ReadSpanQ(cSpanX<TYPE> ret) {
        ITERATE_t i = 0;
        TYPE* pBuf = ret.get_PtrWork();
        for (; !isEmptyQ() && i < ret.GetSize(); i++) {
            pBuf[i] = Read1();
        }
        return i;
    }

    /// <summary>
    /// Add a single TYPE element. Thread Safe against Read.
    /// @note This is NOT reentrant safe. i.e. multi calls to Write()
    /// </summary>
    /// <param name="val"></param>
    /// <returns>false = full.</returns>
    bool Write1(TYPE val) {
        const ITERATE_t iWrite = this->_nWriteIndex;
        ASSERT(IS_INDEX_GOOD(iWrite, _QTY));
        ITERATE_t iWriteNext = this->GetWrapIndex(iWrite + 1);
        if (iWriteNext == _nReadIndex) return false;  // isFullQ() ?
        this->_Data[iWrite] = val;
        this->_nWriteIndex = iWriteNext;
        return true;
    }

    /// <summary>
    /// Add several TYPE items to the Q using cValSpan::CopyQty. NOT thread safe.
    /// @note This is NOT reentrant/thread safe.
    /// </summary>
    /// <param name="src"></param>
    /// <returns>length put. 0 = full. I cant write anything.</returns>
    ITERATE_t WriteSpanQ(const cSpan<TYPE>& src) {
        const ITERATE_t iRoom = get_WriteQtyT();
        ASSERT(iRoom >= 0 && iRoom <= _QTY);
        const ITERATE_t iWrite = this->_nWriteIndex;
        ASSERT(iWrite >= 0 && iWrite < _QTY);
        const ITERATE_t iLengthMin = cValT::Min(iRoom, src.GetSize());  // max i can write and hold.
        if (iWrite + iLengthMin > _QTY) {                               // will overflow/wrap?
            const ITERATE_t iTmp1 = _QTY - iWrite;
            cValSpan::CopyQty(this->_Data + iWrite, src.get_PtrConst(), iTmp1);  // Write to end of buffer.
            const ITERATE_t iTmp2 = iLengthMin - iTmp1;
            cValSpan::CopyQty(this->_Data, src.get_PtrConst() + iTmp1, iTmp2);  // Wrap back from beginning.
            this->_nWriteIndex = iTmp2;
        } else {
            cValSpan::CopyQty(this->_Data + iWrite, src.get_PtrConst(), iLengthMin);
            this->_nWriteIndex = iWrite + iLengthMin;
        }
        return iLengthMin;
    }
};

#ifndef GRAY_STATICLIB  // force implementation/instantiate for DLL/SO.
template struct GRAYCORE_LINK cQueueRead<BYTE>;
template class GRAYCORE_LINK cQueueRW<BYTE>;
#endif
}  // namespace Gray
#endif  // _INC_cQueue_H
