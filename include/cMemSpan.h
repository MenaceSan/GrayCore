//! @file cMemSpan.h
//! specify a sized block of memory.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cMemSpan_H
#define _INC_cMemSpan_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "PtrCast.h"
#include "StrConst.h"  // StrLen_t
#include "cMem.h"

namespace Gray {
/// <summary>
/// A pointer to memory block/blob/span with known size and unknown ownership.
/// may be heap, stack, static or const based memory pointer. don't free on destruct. (although a derived class might)
/// May be static init? or UN-INIT!
/// </summary>
class GRAYCORE_LINK cMemSpan {  // : public cMem
    typedef cMemSpan THIS_t;
    void* m_pData = nullptr;  /// a block of memory of unknown ownership. Treat as MEMTYPE_t::_Temp
    size_t m_nSizeBytes = 0;  /// size_t of m_pData in bytes.

 public:
    /// <summary>
    /// Set/Adjust size in bytes but leave data pointer alone.
    /// </summary>
    void put_SizeBytes(size_t nSize) noexcept {
        m_nSizeBytes = nSize;
    }

    constexpr cMemSpan() noexcept : m_pData(nullptr), m_nSizeBytes(0) {}
    constexpr cMemSpan(const void* pData, size_t nSize) noexcept : m_pData(nSize ? const_cast<void*>(pData) : nullptr), m_nSizeBytes(nSize) {
        // just assume we don't modify it?
        // read only. SetSpanConst
        DEBUG_CHECK(isValid());
    }
    explicit cMemSpan(const cMemSpan* pBlock) noexcept {
        // Just shared pointers. This may be dangerous!
        m_pData = (pBlock == nullptr) ? nullptr : pBlock->m_pData;
        m_nSizeBytes = (pBlock == nullptr) ? 0 : pBlock->m_nSizeBytes;
        DEBUG_CHECK(isValid());
    }

    /// <summary>
    /// Get size in bytes.
    /// </summary>
    constexpr inline size_t get_SizeBytes() const noexcept {
        return m_nSizeBytes;
    }

    inline const BYTE* get_BytePtrC() const noexcept {
        return PtrCast<BYTE>(m_pData);
    }
    /// <summary>
    /// Get a writable byte pointer.
    /// </summary>
    inline BYTE* get_BytePtrW() const noexcept {
        return PtrCast<BYTE>(m_pData);
    }

    /// <summary>
    /// get a read only arbitrary TYPE pointer. Might be nullptr. that's OK.
    /// </summary>
    template <typename TYPE2 = BYTE>
    inline const TYPE2* GetTPtrC() const noexcept {
        // DEBUG_CHECK(m_nSizeBytes == 0 || m_nSizeBytes >= sizeof(TYPE2));
        return PtrCast<TYPE2>(m_pData);
    }
    /// <summary>
    /// Get a writable arbitrary TYPE2 pointer.
    /// </summary>
    template <typename TYPE2 = BYTE>
    inline TYPE2* GetTPtrW() noexcept {
        // DEBUG_CHECK(m_nSizeBytes == 0 || m_nSizeBytes >= sizeof(TYPE2));
        return PtrCast<TYPE2>(m_pData);
    }

    /// <summary>
    /// Get a Non-const pointer that we do not actually expect to write to ?! Some APIs seem to want this.
    /// </summary>
    /// <typeparam name="TYPE2"></typeparam>
    /// <returns></returns>
    template <typename TYPE2>
    inline TYPE2* GetTPtrNC() const noexcept {
        // DEBUG_CHECK(m_nSizeBytes == 0 || m_nSizeBytes >= sizeof(TYPE2));
        return PtrCast<TYPE2>(m_pData);
    }
    /// <summary>
    /// Is this (probably) valid to use/read/write. not nullptr.
    /// </summary>
    /// <returns></returns>
    constexpr inline bool isValidPtr() const noexcept {
        return cMem::IsValidPtr(m_pData);
    }
    /// <summary>
    /// Not exactly the same as empty? since nullptr and size are allowed for lockable types.
    /// </summary>
    constexpr inline bool isNull() const noexcept {
        return m_pData == nullptr;
    }

    /// <summary>
    /// operator to auto cast to const pointer
    /// </summary>
    operator const void*() const noexcept {
        return m_pData;
    }
    operator const BYTE*() const noexcept {
        return PtrCast<BYTE>(m_pData);
    }

    /// <summary>
    /// Is empty? assume NOT nullptr if not empty!
    /// </summary>
    constexpr inline bool isEmpty() const noexcept {
        return m_nSizeBytes <= 0;
    }

    /// <summary>
    /// Is in a valid state? nullptr = a valid state. Forbid: 0 sized isValidPtr !
    /// </summary>
    constexpr inline bool isValid() const noexcept {
        if (isNull()) return true;
        return isValidPtr() && m_nSizeBytes != 0;
    }

    /// <summary>
    /// Is byte offset inside the known valid range for the block? exclusive.
    /// I can write or read this?
    /// </summary>
    inline bool IsInSize(size_t i) const noexcept {
        return IS_INDEX_GOOD(i, m_nSizeBytes);
    }
    /// <summary>
    /// Is byte offset inside the known valid range for the block? or at end? inclusive.
    /// </summary>
    bool IsLTESize(size_t i) const noexcept {
        if (i == m_nSizeBytes) return true;  // at end is ok
        return IS_INDEX_GOOD(i, m_nSizeBytes);
    }

    /// <summary>
    /// ptrdiff_t ?
    /// </summary>
    inline size_t GetOffset(const void* p) const noexcept {
        return cMem::Diff(p, m_pData);
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
        return cMem::IsZeros(m_pData, m_nSizeBytes);
    }

    /// <summary>
    /// Exact same span? like IsEqual3()
    /// </summary>
    /// <param name="data"></param>
    bool IsSameSpam(const THIS_t& data) const noexcept {
        return m_nSizeBytes == data.m_nSizeBytes && m_pData == data.m_pData;
    }
    bool IsEqualData(const void* pData) const noexcept {
        return cMem::IsEqual(m_pData, pData, m_nSizeBytes);
    }

    /// <summary>
    /// compare blocks of data.
    /// </summary>
    bool IsEqualSpan(const THIS_t& data) const noexcept {
        return m_nSizeBytes == data.m_nSizeBytes && IsEqualData(data.m_pData);
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
    /// Get a pointer to the end of the buffer.
    /// Never read/write to/past this pointer.
    /// </summary>
    /// <returns></returns>
    const BYTE* get_EndPtr() const noexcept {
        return GetTPtrC<BYTE>() + m_nSizeBytes;
    }

    void SetSpanNull() noexcept {
        m_nSizeBytes = 0;
        m_pData = nullptr;
    }
    /// set a read only span. nullptr ok.
    void SetSpanConst(const void* pData, size_t nSize) noexcept {
        m_pData = const_cast<void*>(pData);
        m_nSizeBytes = nSize;  // size does not apply if nullptr.
        DEBUG_CHECK(isValid());
    }
    /// set a writable span.
    void SetSpan2(void* pData, size_t nSize) noexcept {
        m_nSizeBytes = nSize;  // size does not apply if nullptr.
        m_pData = nSize ? pData : nullptr;
        DEBUG_CHECK(isValid());
    }
    /// make a copy of this span
    void SetSpan(const THIS_t& a) noexcept {
        m_nSizeBytes = a.m_nSizeBytes;  // size does not apply if nullptr.
        m_pData = a.m_pData;
        DEBUG_CHECK(isValid());
    }

    /// Advance the span and shrink it.
    inline void SetSkipBytes(size_t nSize) {
        ASSERT(nSize <= m_nSizeBytes);
        m_pData = GetTPtrW<BYTE>() + nSize;
        m_nSizeBytes -= nSize;
    }
    cMemSpan GetSkipBytes(size_t nSize) const {
        ASSERT(nSize <= m_nSizeBytes);
        return cMemSpan(GetTPtrC<BYTE>() + nSize, m_nSizeBytes - nSize);
    }

    THIS_t GetSizeBytesMax(size_t sizeMax) const noexcept {
        return THIS_t(m_pData, cValT::Min(m_nSizeBytes, sizeMax));
    }

    void SetZeros() noexcept {
        cMem::ZeroSecure(m_pData, m_nSizeBytes);
    }
    void SetCopyN(const void* pData, size_t size) noexcept {
        if (isNull() || pData == nullptr) return;
        cMem::Copy(m_pData, pData, size);
    }
    void SetCopyAll(const void* pData) noexcept {
        SetCopyN(pData, m_nSizeBytes);
    }
    size_t SetCopySpan(const cMemSpan& span2) noexcept {
        const size_t sizeMin = cValT::Min(m_nSizeBytes, span2.get_SizeBytes());
        SetCopyN(span2, sizeMin);
        return sizeMin;
    }

    void CopyTo(void* pData) const noexcept {
        cMem::Copy(pData, m_pData, m_nSizeBytes);
    }

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
    /// ASSUME pszHexString output is big enough! GetHexDigestSize()
    StrLen_t GetHexDigest(OUT char* pszHexString) const;
    HRESULT SetHexDigest(const char* pszHexString, bool testEnd = true);

    /// read/write a string of comma separated numbers.
    size_t ReadFromCSV(const char* pszSrc);

    /// <summary>
    /// reverse the order of an array of blocks/objects.
    /// </summary>
    void ReverseSpan(size_t stride);
};
}  // namespace Gray
#endif
