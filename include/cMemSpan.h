//! @file cMemSpan.h
//! specify a sized block of memory.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cMemSpan_H
#define _INC_cMemSpan_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrConst.h"  // StrLen_t
#include "cBits.h"     // BIT_ENUM_t
#include "cValT.h"     // cValT::Min

namespace Gray {
/// <summary>
/// A pointer to memory block/blob/span with known size but unknown ownership.
/// may be heap, stack, static or const based memory pointer (MEMTYPE_t). don't free on destruct. (although a derived class might. e.b. cBlob)
/// May be static init? or UN-INIT!
/// </summary>
class GRAYCORE_LINK cMemSpan {  // : public cMem
    typedef cMemSpan THIS_t;
    void* _pData = nullptr;  /// a block of memory of unknown ownership. Treat as MEMTYPE_t::_Temp
    size_t _nSizeBytes = 0;  /// size_t of _pData in bytes. upper byte is reserved.

 protected:
    static constexpr BIT_ENUM_t kUpperByteShift = (_SIZEOF_PTR - 1) * cBits::k_8;  // BIT_ENUM_t USE_64BIT
    static constexpr size_t kLowerMask = ~(CastN(size_t, 0xFF) << kUpperByteShift);

 public:
    constexpr cMemSpan() noexcept {}

    /// <summary>
    /// assume we don't modify it? read only like SetSpanConst.
    /// </summary>
    constexpr cMemSpan(const void* pData, size_t nSize) noexcept : _pData(nSize ? const_cast<void*>(pData) : nullptr), _nSizeBytes(nSize) {
        DEBUG_CHECK(isValid());
    }
    cMemSpan(BYTE* pStart, const BYTE* pEnd) noexcept : _pData(pStart), _nSizeBytes(pEnd - pStart) {
        ASSERT_NN(pStart);
        ASSERT_NN(pEnd);
        DEBUG_CHECK(isValid());
    }

    /// <summary>
    /// Init with shared pointers. This may be dangerous!
    /// </summary>
    explicit cMemSpan(const cMemSpan* pBlock) noexcept : _pData((pBlock == nullptr) ? nullptr : pBlock->_pData), _nSizeBytes((pBlock == nullptr) ? 0 : pBlock->_nSizeBytes) {
        DEBUG_CHECK(isValid());
    }

    /// <summary>
    /// Get size in bytes.
    /// </summary>
    constexpr inline size_t get_SizeBytes() const noexcept {
        return _nSizeBytes & kLowerMask;
    }
    constexpr inline BYTE get_UpperByte() const {
        return _nSizeBytes >> kUpperByteShift;
    }

    /// <summary>
    /// Is empty? assume NOT nullptr if not empty!
    /// </summary>
    constexpr inline bool isEmpty() const noexcept {
        return get_SizeBytes() <= 0;
    }

    inline const BYTE* get_BytePtrC() const noexcept {
        return PtrCast<BYTE>(_pData);
    }
    /// <summary>
    /// Get a writable byte pointer.
    /// </summary>
    inline BYTE* get_BytePtrW() const noexcept {
        return PtrCast<BYTE>(_pData);
    }

    /// <summary>
    /// Not exactly the same as empty? since nullptr and size are allowed for lockable types.
    /// </summary>
    constexpr inline bool isNull() const noexcept {
        return _pData == nullptr;
    }

    /// <summary>
    /// operator to auto cast to const pointer
    /// </summary>
    operator const void*() const noexcept {
        return _pData;
    }
    operator const BYTE*() const noexcept {
        return PtrCast<BYTE>(_pData);
    }

    /// <summary>
    /// get a read only arbitrary TYPE pointer. Might be nullptr. that's OK.
    /// </summary>
    template <typename TYPE2 = BYTE>
    inline const TYPE2* GetTPtrC() const noexcept {
        // DEBUG_CHECK(isEmpty() || get_SizeBytes() >= sizeof(TYPE2));
        return PtrCast<TYPE2>(_pData);
    }
    /// <summary>
    /// Get a writable arbitrary TYPE2 pointer.
    /// </summary>
    template <typename TYPE2 = BYTE>
    inline TYPE2* GetTPtrW() noexcept {
        // DEBUG_CHECK(isEmpty() || get_SizeBytes() >= sizeof(TYPE2));
        return PtrCast<TYPE2>(_pData);
    }

    /// <summary>
    /// Get a Non-const pointer that we do not actually expect to write to ?! Some APIs seem to want this.
    /// </summary>
    /// <typeparam name="TYPE2"></typeparam>
    /// <returns></returns>
    template <typename TYPE2>
    inline TYPE2* GetTPtrNC() const noexcept {
        // DEBUG_CHECK(isEmpty() || get_SizeBytes() >= sizeof(TYPE2));
        return PtrCast<TYPE2>(_pData);
    }
    /// <summary>
    /// Is this (probably) valid to use/read/write. not nullptr.
    /// </summary>
    /// <returns></returns>
    constexpr inline bool isValidPtr() const noexcept {
        return cMem::IsValidPtr(_pData);
    }

    /// <summary>
    /// Is in a valid state? nullptr = a valid state. Forbid: 0 sized isValidPtr !
    /// </summary>
    constexpr inline bool isValid() const noexcept {
        if (isNull()) return true;
        return isValidPtr() && !isEmpty();
    }

    /// <summary>
    /// Is byte offset inside the known valid range for the block? exclusive.
    /// I can write or read this?
    /// </summary>
    inline bool IsInSize(size_t i) const noexcept {
        return i < get_SizeBytes();  // IS_INDEX_GOOD
    }

    /// <summary>
    /// Is byte offset inside the known valid range for the block? or at end? inclusive.
    /// </summary>
    bool IsLTESize(size_t i) const noexcept {
        return i <= get_SizeBytes();  // at end is ok
    }

    /// <summary>
    /// ptrdiff_t
    /// </summary>
    inline ptrdiff_t GetOffset(const void* p) const noexcept {
        return cMem::Diff(p, _pData);
    }

    /// <summary>
    /// Is p inside the known valid range for the block? Exclusive = Cant be equal to end!
    /// </summary>
    inline bool IsInternalPtr(const void* p) const noexcept {
        return IsInSize(GetOffset(p));
    }

    /// <summary>
    /// Is p inside the known valid range for the block? Inclusive = Can be equal to end.
    /// </summary>
    inline bool IsInternalPtr2(const void* p) const noexcept {
        return IsLTESize(GetOffset(p));
    }

    bool IsZeros() const noexcept {
        return cMem::IsZeros(_pData, get_SizeBytes());
    }

    /// <summary>
    /// Exact same span? like IsEqual3()
    /// </summary>
    /// <param name="data"></param>
    bool IsSameSpan(const THIS_t& data) const noexcept {
        return _nSizeBytes == data._nSizeBytes && _pData == data._pData;
    }
    bool IsEqualData(const void* pData) const noexcept {
        return cMem::IsEqual(_pData, pData, get_SizeBytes());  // ASSUME pData is same size.
    }

    /// <summary>
    /// compare blocks of data.
    /// </summary>
    bool IsEqualSpan(const THIS_t& data) const noexcept {
        return get_SizeBytes() == data.get_SizeBytes() && IsEqualData(data._pData);
    }

    bool operator==(const cMemSpan& m2) const noexcept {
        return IsEqualSpan(m2);
    }
    COMPARE_t Compare(const cMemSpan& m2) const noexcept;

    /// <summary>
    /// get pointer that is good/valid for just one byte in the span.
    /// </summary>
    /// <param name="nOffset"></param>
    const BYTE* GetInternalPtr(size_t nOffset) const noexcept {
        if (!IsInSize(nOffset)) return nullptr;
        return GetTPtrC<BYTE>() + nOffset;
    }
    /// <summary>
    /// Get a pointer into the buffer as a byte pointer.
    /// Ensure the data is valid to size_t !
    /// </summary>
    /// <param name="nOffset"></param>
    /// <param name="size"></param>
    /// <returns></returns>
    const BYTE* GetInternal2(size_t nOffset, size_t size) const noexcept {
        ASSERT(size > 0);
        if (!IsInSize(nOffset) || !IsLTESize(nOffset + size)) {
            DEBUG_CHECK(false);
            return nullptr;  // not big enough!
        }
        return GetTPtrC<BYTE>() + nOffset;
    }

    /// <summary>
    /// Get a pointer to the end of the buffer. end().
    /// Never read/write to/past this pointer.
    /// </summary>
    /// <returns></returns>
    const BYTE* get_EndPtr() const noexcept {
        return GetTPtrC<BYTE>() + get_SizeBytes();
    }

    cMemSpan GetSkipBytes(size_t nSize) const {
        ASSERT(IsLTESize(nSize));
        return cMemSpan(GetTPtrC<BYTE>() + nSize, get_SizeBytes() - nSize);
    }

    size_t GetSizeLimit(size_t sizeMax) const noexcept {
        return cValT::Min(get_SizeBytes(), sizeMax);
    }
    THIS_t GetSpanLimit(size_t sizeMax) const noexcept {
        return THIS_t(_pData, GetSizeLimit(sizeMax));
    }

    void CopyTo(void* pDest) const noexcept {
        // ASSUME the caller knows pDest is big enough!
        cMem::Copy(pDest, _pData, get_SizeBytes());
    }

    //**********************************
    // can Modify

    /// How much space does the hex digest need?
    static constexpr StrLen_t GetHexDigestSize(size_t nSize) noexcept {
        return CastN(StrLen_t, (nSize * 2) + 1);
    }
    /// <summary>
    /// Get the required size of the hex string.
    /// </summary>
    StrLen_t get_HexDigestSize() const {
        return GetHexDigestSize(get_SizeBytes());
    }

    /// <summary>
    /// Get the final hash as a pre-formatted string of hex digits. opposite of SetHexDigest
    /// ASSUME sizeof(pszHexString) >= GetHexDigestSize()
    /// @note using Base64 would be better.
    /// </summary>
    /// <param name="hexStr">ASSUME hexStr output is big enough! GetHexDigestSize(). MUST include space for '\0'</param>
    /// <returns></returns>
    StrLen_t GetHexDigest(cMemSpan hexStr) const;

    /// <summary>
    /// Set binary pDigest from string pszHexString
    /// opposite of cMemSpan::GetHexDigest
    /// @note using Base64 would be better.
    /// </summary>
    /// <param name="pszHexString">input hex string.</param>
    /// <param name="testEnd"></param>
    /// <returns>S_FALSE = was zero value.</returns>
    HRESULT ReadHexDigest(const char* pszHexString, bool testEnd = true);

    /// read/write a string of comma separated numbers.
    size_t ReadFromCSV(const char* pszSrc);

    /// <summary>
    /// reverse the order of an array of blocks/objects inside.
    /// </summary>
    void ReverseSpan(size_t stride);

    void SetZeros() noexcept {
        cMem::ZeroSecure(_pData, get_SizeBytes());
    }

    /// Copy data but do not change current span size.
    void SetCopyN(const void* pSrc, size_t nSize) noexcept {
        if (isNull() || pSrc == nullptr) return;
        ASSERT(IsLTESize(nSize));
        cMem::Copy(_pData, pSrc, nSize);
    }
    void SetCopyAll(const void* pSrc) noexcept {
        SetCopyN(pSrc, get_SizeBytes());
    }
    /// <summary>
    ///  Copy data but do not change current span size.
    /// </summary>
    /// <param name="span2"></param>
    /// <returns></returns>
    size_t SetCopySpan(const cMemSpan& span2) noexcept {
        const size_t sizeMin = GetSizeLimit(span2.get_SizeBytes());
        SetCopyN(span2, sizeMin);
        return sizeMin;
    }

    //************************************
    // Setters.

    /// <summary>
    /// Set/Adjust size in bytes but leave data pointer alone.
    /// </summary>
    void put_SizeBytes(size_t nSize) noexcept {
        _nSizeBytes = nSize;
    }

    void SetSpanNull() noexcept {
        _nSizeBytes = 0;
        _pData = nullptr;
    }
    /// set a read only span. nullptr ok.
    void SetSpanConst(const void* pData, size_t nSize) noexcept {
        _pData = const_cast<void*>(pData);
        _nSizeBytes = nSize;  // size ignored if nullptr.
        DEBUG_CHECK(isValid());
    }
    /// make a dupe of this span
    void SetSpan(const THIS_t& a) noexcept {
        _nSizeBytes = a._nSizeBytes;  // size ignored if nullptr.
        _pData = a._pData;
        DEBUG_CHECK(isValid());
    }
    /// set a writable span.
    void SetSpan2(void* pData, size_t nSize) noexcept {
        _nSizeBytes = nSize;  // size ignored if nullptr.
        _pData = nSize ? pData : nullptr;
        DEBUG_CHECK(isValid());
    }

    /// Advance the span and shrink it.
    inline void SetSkipBytes(size_t nSize) {
        // ASSUME NOT cBlob heap.
        ASSERT(IsLTESize(nSize));
        _pData = GetTPtrW<BYTE>() + nSize;
        _nSizeBytes -= nSize;
    }
};
}  // namespace Gray
#endif
