//
//! @file StrBuilder.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_StrBuilder_H
#define _INC_StrBuilder_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "StrConst.h"
#include "StrT.h"
#include "cBlob.h"

namespace Gray {
template <typename _TYPE_CH>
struct GRAYCORE_LINK StrFormat;  // Forward declare.

/// <summary>
/// Build a string Similar to .NET StringBuilder
/// Fill a buffer with string stuff. Track how much space is left. Used with StrFormat.
/// Like cQueue or cStreamOutput ??
/// </summary>
/// <typeparam name="_TYPE_CH">char or wchar_t</typeparam>
template <typename _TYPE_CH = char>
class GRAYCORE_LINK StrBuilder : protected cBlob {
    typedef cBlob SUPER_t;
    friend struct StrFormat<_TYPE_CH>;
    friend struct StrTemplate;

 public:
    static const StrLen_t k_nGrowSizeChunk = 1024;  /// -le- k_LEN_Default if isHeap()
    StrLen_t m_nWriteLast;                          /// new items added/written here. end of readable. like cQueueIndex

 protected:
    inline _TYPE_CH* get_DataWork() {
        return SUPER_t::get_DataW<_TYPE_CH>();
    }

    inline StrLen_t get_AllocQty() const noexcept {
        return CastN(StrLen_t, SUPER_t::get_DataSize() / sizeof(_TYPE_CH));
    }

    inline void SetTerminated() noexcept {
        // always force terminate.
        if (!isValidPtr())  // just estimating.
            return;
        get_DataWork()[this->m_nWriteLast] = '\0';
    }

 public:
    /// <summary>
    /// How much space is avail for write into buffer? (given the current get_AllocQty() allocation size)
    /// Free space to write more. Not including '\0'
    /// </summary>
    /// <returns>Qty of TYPE</returns>
    inline StrLen_t get_WriteSpaceQty() const noexcept {
        return (get_AllocQty() - this->m_nWriteLast) - 1;  // leave space for '\0'
    }
    inline StrLen_t get_WriteSpaceMax() const noexcept {
        return ((isHeap() ? StrT::k_LEN_MAX : get_AllocQty()) - this->m_nWriteLast) - 1;
    }

    /// <summary>
    /// Do i have enough room to write iNeedCount of TYPE?
    /// Allocate as much as i can and truncate the rest. paired with AdvanceWrite().
    /// </summary>
    /// <param name="iNeedCount"></param>
    /// <returns></returns>
    _TYPE_CH* GetWritePrepared(StrLen_t iNeedCount) {
        if (isNull()) return nullptr;  // just estimating?
        if (isHeap()) {
            const StrLen_t nLenSpace = this->get_WriteSpaceQty();
            if (iNeedCount > nLenSpace) {  // Get more space ? or truncate?
                // grow = ReAlloc for more space.
                const StrLen_t nOldAlloc = this->get_AllocQty();
                if (nOldAlloc < StrT::k_LEN_MAX) {                              // can we grow?
                    StrLen_t nNewAlloc = nOldAlloc + (iNeedCount - nLenSpace);  // Min size for new alloc.
                    StrLen_t iRem = nNewAlloc % k_nGrowSizeChunk;
                    if (iRem <= 0) iRem = k_nGrowSizeChunk;  // grow a full block.
                    nNewAlloc += iRem;
                    if (nNewAlloc > StrT::k_LEN_MAX) {  // we hit the end! do what we can. truncate!
                        nNewAlloc = StrT::k_LEN_MAX;
                        // return nullptr;
                    }
                    nNewAlloc++;  // room for '\0'
                    if (!this->ReAllocSize(nNewAlloc * sizeof(_TYPE_CH))) return nullptr;
                }
            }
        }
        return get_DataWork() + this->m_nWriteLast;
    }

    /// <summary>
    /// advanced the used space.
    /// The estimated size in GetWritePrepared() might have been larger.
    /// </summary>
    /// <param name="nLen"></param>
    inline void AdvanceWrite(StrLen_t nLen) noexcept {
        DEBUG_CHECK(nLen <= get_WriteSpaceQty());
        this->m_nWriteLast += nLen;
        DEBUG_CHECK(this->m_nWriteLast >= nLen);
        SetTerminated();
    }

    /// Build with a growing heap buffer.
    StrBuilder(StrLen_t nSizeChunk = k_nGrowSizeChunk) noexcept : SUPER_t(nSizeChunk), m_nWriteLast(0) {
        SetTerminated();
    }
    /// Build with a non-growing static buffer.
    StrBuilder(_TYPE_CH* p, StrLen_t nSize) noexcept : SUPER_t(p, nSize * sizeof(_TYPE_CH), MEMTYPE_t::_Temp), m_nWriteLast(0) {
        SetTerminated();
    }
    StrBuilder(SUPER_t& m) noexcept : SUPER_t(m), m_nWriteLast(0) {
        SetTerminated();
    }

    void SetEmptyStr() noexcept {
        m_nWriteLast = 0;
        SetTerminated();
    }
    void SetTrimWhiteSpaceEnd() {
        m_nWriteLast = StrT::TrimWhitespaceEnd(get_DataWork(), m_nWriteLast);
    }

    /// <summary>
    /// get Length used/filled. not including '\0';
    /// </summary>
    inline StrLen_t get_Length() const noexcept {
        return m_nWriteLast;
    }
    inline const _TYPE_CH* get_Str() const noexcept {
        return SUPER_t::get_DataC<_TYPE_CH>();
    }
    cSpan<_TYPE_CH> get_SpanStr() const {
        return cSpan<_TYPE_CH>(get_Str(), get_Length());
    }

    /// <summary>
    /// Assume truncation occurred? DISP_E_BUFFERTOOSMALL
    /// </summary>
    inline bool isOverflow() const noexcept {
        return get_WriteSpaceQty() <= 0;
    }

    bool AddChar(_TYPE_CH ch) {
        // m_nLenLeft includes space for terminator '\0'
        _TYPE_CH* pszWrite = GetWritePrepared(1);
        const StrLen_t nLenRet = get_WriteSpaceQty();
        if (nLenRet < 1) return false;  // no space.
        if (pszWrite != nullptr) *pszWrite = ch;
        AdvanceWrite(1);
        return true;
    }
    /// Add newline.
    bool AddNl() {
        return AddChar('\n');
    }
    /// Add newline ONLY if there is already text.
    bool AddNl2() {
        if (m_nWriteLast <= 0) return false;
        return AddChar('\n');
    }

    StrLen_t AddCharRepeat(_TYPE_CH ch, StrLen_t iRepeat) {
        _TYPE_CH* pszWrite = GetWritePrepared(iRepeat);
        const StrLen_t nLenSpace = get_WriteSpaceQty();
        const StrLen_t nLenRet = cValT::Min(iRepeat, nLenSpace);
        if (pszWrite != nullptr) {
            cValArray::FillQty<_TYPE_CH>(pszWrite, nLenRet, ch);
        }
        AdvanceWrite(nLenRet);
        return nLenRet;
    }

    StrLen_t AddStrLen(const _TYPE_CH* pszStr, StrLen_t nLen) {
        // nLen = not including space for '\0'
        if (nLen <= 0) return 0;  // just add nothing.
        _TYPE_CH* pszWrite = GetWritePrepared(nLen);
        const StrLen_t nLenSpace = get_WriteSpaceQty();
        StrLen_t nLenRet = cValT::Min(nLenSpace, nLen);
        if (pszWrite != nullptr) {
            nLenRet = StrT::CopyLen(pszWrite, pszStr, nLenRet + 1);  // add 1 more for '\0'
        }
        AdvanceWrite(nLenRet);
        return nLenRet;
    }

    /// AKA WriteString()
    StrLen_t AddStr(const _TYPE_CH* pszStr) {
        return AddStrLen(pszStr, StrT::Len(pszStr));
    }
    void AddCrLf() {
        // AKA CRNL
        AddStr(cStrConst::k_CrLf);
    }

    /// Add a nullptr terminated list of strings. AKA Concat
    StrLen_t _cdecl Join(const _TYPE_CH* psz1, ...) {
        StrLen_t len = 0;
        va_list vargs;
        va_start(vargs, psz1);
        for (int i = 0; i < k_ARG_ARRAY_MAX; i++) {
            if (StrT::IsNullOrEmpty(psz1)) break;
            const StrLen_t lenStr = AddStr(psz1);
            len += lenStr;
            psz1 = va_arg(vargs, const _TYPE_CH*);  // next
        }
        va_end(vargs);
        return len;
    }

    /// <summary>
    /// add raw bytes as chars with no filtering.
    /// </summary>
    void AddBytesRaw(const void* pSrc, size_t nSize) {
        _TYPE_CH* pszWrite = GetWritePrepared((StrLen_t)nSize);
        StrLen_t nLenRet = get_WriteSpaceQty();
        nLenRet = cValT::Min(nLenRet, (StrLen_t)nSize);
        if (pszWrite != nullptr) {
            cMem::Copy(pszWrite, pSrc, nLenRet);
        }
        AdvanceWrite(nLenRet);
    }

    void AddBytesFiltered(const void* pSrc, size_t nSize) {
        // Just add a string from void*. Don't assume terminated string. filter for printable characters.
        _TYPE_CH* pszWrite = GetWritePrepared((StrLen_t)nSize);
        StrLen_t nLenRet = get_WriteSpaceQty();
        nLenRet = cValT::Min(nLenRet, (StrLen_t)nSize);
        if (pszWrite != nullptr) {
            for (StrLen_t i = 0; i < nLenRet; i++) {
                BYTE ch = ((BYTE*)pSrc)[i];
                if (ch < 32 || ch == 127 || (ch > 128 && ch < 160))  // strip junk chars.
                    pszWrite[i] = '?';
                else
                    pszWrite[i] = ch;
            }
        }
        AdvanceWrite(nLenRet);
    }

    void AddUInt(UINT64 uVal, const _TYPE_CH* pszPrefix = nullptr, RADIX_t nRadix = 10, char chRadixA = 'A');
    void AddFloat(double dVal, char chE = 0);

    void AddFormatV(const _TYPE_CH* pszFormat, va_list vargs);
    void _cdecl AddFormat(const _TYPE_CH* pszFormat, ...);
};
}  // namespace Gray
#endif
