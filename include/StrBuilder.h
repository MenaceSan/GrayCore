//! @file StrBuilder.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_StrBuilder_H
#define _INC_StrBuilder_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "StrConst.h"
#include "StrNum.h"
#include "StrT.h"
#include "cBlob.h"
#include "ITextWriter.h"

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
class GRAYCORE_LINK StrBuilder : protected cBlob, public ITextWriter {
    typedef cBlob SUPER_t;
    friend struct StrFormat<_TYPE_CH>;
    friend struct StrTemplate;

 public:
    static const StrLen_t k_nGrowSizeChunk = 1024;  /// -le- k_LEN_Default if isHeap()
    StrLen_t m_nWriteIndex;                         /// new items added/written here. end of readable. like cQueueIndex

 protected:
    inline StrLen_t get_AllocQty() const noexcept {
        return CastN(StrLen_t, SUPER_t::get_DataSize() / sizeof(_TYPE_CH));
    }

 public:
    inline _TYPE_CH* get_DataWork() {
        return SUPER_t::get_DataW<_TYPE_CH>();
    }
    inline void SetTerminated() noexcept {
        // always force terminate.
        if (!isValidPtr()) return;  // just estimating.
        DEBUG_CHECK(this->IsInSize(m_nWriteIndex));
        get_DataWork()[this->m_nWriteIndex] = '\0';
    }

    /// Build with a growing heap buffer.
    StrBuilder(StrLen_t nSizeChunk = k_nGrowSizeChunk) noexcept : SUPER_t(nSizeChunk), m_nWriteIndex(0) {
        SetTerminated();
    }
    /// Build with a non-growing static buffer.
    StrBuilder(const cSpanX<_TYPE_CH>& m) noexcept : SUPER_t(m, MEMTYPE_t::_Temp), m_nWriteIndex(0) {
        SetTerminated();
    }
    StrBuilder(SUPER_t& m) noexcept : SUPER_t(m), m_nWriteIndex(0) {
        SetTerminated();
    }

    /// <summary>
    /// How much space is avail for write into buffer? (given the current get_AllocQty() allocation size)
    /// Free space to write more. Not including '\0'
    /// </summary>
    /// <returns>Qty of TYPE</returns>
    inline StrLen_t get_WriteSpaceQty() const noexcept {
        return (get_AllocQty() - this->m_nWriteIndex) - 1;  // leave space for '\0'
    }
    inline StrLen_t get_WriteSpaceMax() const noexcept {
        return isHeap() ? ((cStrConst::k_LEN_MAX - this->m_nWriteIndex) - 1) : get_WriteSpaceQty();
    }

    /// <summary>
    /// Attempt to get more space for write iNeedCount of TYPE.
    /// ALWAYS check this->get_WriteSpaceQty() after this!
    /// Allocate as much as i can and truncate the rest.
    /// Paired with AdvanceWrite().
    /// </summary>
    /// <param name="iNeedCount"></param>
    /// <returns></returns>
    _TYPE_CH* GetWritePrep(StrLen_t iNeedCount) {
        if (isNull()) return nullptr;  // just estimating?
        if (isHeap()) {
            const StrLen_t nLenSpace = this->get_WriteSpaceQty();
            if (iNeedCount > nLenSpace) {  // Get more space ? or truncate?
                // grow = ReAlloc for more space.
                const StrLen_t nOldAlloc = this->get_AllocQty();
                if (nOldAlloc < cStrConst::k_LEN_MAX) {                         // can we grow?
                    StrLen_t nNewAlloc = nOldAlloc + (iNeedCount - nLenSpace);  // Min size for new alloc.
                    StrLen_t iRem = nNewAlloc % k_nGrowSizeChunk;
                    if (iRem <= 0) iRem = k_nGrowSizeChunk;  // grow a full block.
                    nNewAlloc += iRem;
                    if (nNewAlloc > cStrConst::k_LEN_MAX) {  // we hit the end! do what we can. truncate!
                        nNewAlloc = cStrConst::k_LEN_MAX;
                        // return nullptr;
                    }
                    nNewAlloc++;  // room for '\0'
                    if (!this->ReAllocSize(nNewAlloc * sizeof(_TYPE_CH))) return nullptr;
                }
            }
        }
        return get_DataWork() + this->m_nWriteIndex;
    }

    cSpanX<_TYPE_CH> get_SpanWrite() {
        return ToSpan(get_DataWork() + this->m_nWriteIndex, get_WriteSpaceQty());
    }
    cSpanX<_TYPE_CH> GetSpanWrite(StrLen_t iNeedCount) {
        _TYPE_CH* p = GetWritePrep(iNeedCount);  // MUST call this first to grow.
        return ToSpan(p, get_WriteSpaceQty());  // May be greater than i asked for!
    }

    /// <summary>
    /// advanced the used space.
    /// The estimated size in GetWritePrep() might have been larger.
    /// </summary>
    /// <param name="nLen"></param>
    inline void AdvanceWrite(StrLen_t nLen) noexcept {
        DEBUG_CHECK(nLen <= get_WriteSpaceQty());
        this->m_nWriteIndex += nLen;
        DEBUG_CHECK(this->m_nWriteIndex >= nLen);
        SetTerminated();
    }

    void SetEmptyStr() noexcept {
        m_nWriteIndex = 0;
        SetTerminated();
    }
    void SetTrimWhiteSpaceEnd() {
        m_nWriteIndex = StrT::TrimWhitespaceEnd(get_DataWork(), m_nWriteIndex);
    }

    /// <summary>
    /// get used/filled Length. not including '\0';
    /// </summary>
    inline StrLen_t get_Length() const noexcept {
        return m_nWriteIndex;
    }
    /// <summary>
    /// Get raw pointer to the string value.
    /// </summary>
    /// <returns></returns>
    inline const _TYPE_CH* get_CPtr() const noexcept {
        return SUPER_t::get_DataC<_TYPE_CH>();
    }

    /// <summary>
    /// operator to auto cast to const pointer
    /// </summary>
    operator const _TYPE_CH*() const noexcept {
        return get_CPtr();
    }

    cSpan<_TYPE_CH> get_SpanStr() const noexcept {
        return ToSpan(get_CPtr(), get_Length()); // NOT the '\0'
    }

    /// <summary>
    /// Assume truncation occurred? DISP_E_BUFFERTOOSMALL
    /// </summary>
    inline bool isOverflow() const noexcept {
        return get_WriteSpaceQty() <= 0;
    }

    void AddChar(_TYPE_CH ch) {
        // m_nLenLeft includes space for terminator '\0'
        _TYPE_CH* pszWrite = GetWritePrep(1);
        if (isOverflow()) return;  // no space.
        if (pszWrite != nullptr) *pszWrite = ch;
        AdvanceWrite(1);
    }
    /// Add newline.
    void AddNl() {
        AddChar('\n');
    }
    /// Add separator (newline) ONLY if there is already text.
    void AddSep(char ch = '\n') {
        if (m_nWriteIndex > 0) AddChar(ch);
    }

    void AddCharRepeat(_TYPE_CH ch, StrLen_t iRepeat) {
        cSpanX<_TYPE_CH> spanWrite = GetSpanWrite(iRepeat);
        const StrLen_t nLenWrite = cValT::Min(iRepeat, spanWrite.get_MaxLen());
        if (!spanWrite.isNull()) {
            cValSpan::FillQty<_TYPE_CH>(spanWrite.get_DataWork(), nLenWrite, ch);
        }
        AdvanceWrite(nLenWrite);
    }

    void AddStrLen(const _TYPE_CH* pszStr, StrLen_t nLen) {
        // nLen = not including space for '\0'
        if (nLen <= 0) return;  // just add nothing.
        cSpanX<_TYPE_CH> spanWrite = GetSpanWrite(nLen);
        StrLen_t nLenWrite = cValT::Min(spanWrite.get_MaxLen(), nLen);
        if (!spanWrite.isNull()) {
            StrT::CopyLen(spanWrite.get_DataWork(), pszStr, nLenWrite + 1);  // add 1 more for '\0'
        }
        AdvanceWrite(nLenWrite);
    }

    /// AKA WriteString()
    void AddStr(const _TYPE_CH* pszStr) {
        if (pszStr == nullptr) return;
        AddStrLen(pszStr, StrT::Len(pszStr));
    }
    /// Add quoted string. NOT escaped! EscSeqAddQ() ?
    void AddStrQ(const _TYPE_CH* pszStr, STR_BLOCK_t eBlockType = STR_BLOCK_t::_QUOTE) {
        if (eBlockType != STR_BLOCK_t::_NONE) {
            AddChar(StrT::GetBlockStart(eBlockType));
        }
        AddStr(pszStr);
        if (eBlockType != STR_BLOCK_t::_NONE) {
            AddChar(StrT::GetBlockEnd(eBlockType));
        }
    }
    void AddCrLf() {
        // AKA CRNL
        AddStrLen(cStrConst::k_CrLf, 2);
    }

    /// Add a nullptr terminated list of strings. AKA Concat
    void _cdecl Join(const _TYPE_CH* psz1, ...) {
        va_list vargs;
        va_start(vargs, psz1);
        for (int i = 0; i < k_ARG_ARRAY_MAX; i++) {
            if (StrT::IsNullOrEmpty(psz1)) break;
            AddStr(psz1);
            psz1 = va_arg(vargs, const _TYPE_CH*);  // next
        }
        va_end(vargs);
    }

    /// <summary>
    /// add raw bytes as chars with no filtering.
    /// </summary>
    void AddBytesRaw(const void* pSrc, size_t nSize) {
        const StrLen_t nLen = CastN(StrLen_t, nSize / sizeof(_TYPE_CH));
        AdvanceWrite(GetSpanWrite(nLen).SetCopyQty(PtrCast<_TYPE_CH>(pSrc), nLen));
    }

    void AddBytesFiltered(const void* pSrc, size_t nSize) {
        // Just add a string from void*. Don't assume terminated string. filter for printable characters.
        auto spanWrite = GetSpanWrite((StrLen_t)nSize);
        StrLen_t nLenRet = cValT::Min(spanWrite.get_MaxLen(), (StrLen_t)nSize);
        if (!spanWrite.isNull() && pSrc != nullptr) {
            _TYPE_CH* pWrite = spanWrite.get_DataWork();
            for (StrLen_t i = 0; i < nLenRet; i++) {
                BYTE ch = ((BYTE*)pSrc)[i];
                if (ch < 32 || ch == 127 || (ch > 128 && ch < 160))  // strip junk chars.
                    pWrite[i] = '?';
                else
                    pWrite[i] = ch;
            }
        }
        AdvanceWrite(nLenRet);
    }

    void AddInt(INT64 uVal);
    void AddUInt(UINT64 uVal, RADIX_t nRadix = 10);
    void AddFloat(double dVal, char chE = -'e');

    void AddFormatV(const _TYPE_CH* pszFormat, va_list vargs);
    void _cdecl AddFormat(const _TYPE_CH* pszFormat, ...);

    HRESULT WriteString(const char* pszStr) override {  // ITextWriter
        const StrLen_t nWritePrev = m_nWriteIndex;
        AddStr(StrArg<_TYPE_CH>(pszStr));
        return CastN(HRESULT, m_nWriteIndex - nWritePrev);
    }
    HRESULT WriteString(const wchar_t* pszStr) override {  // ITextWriter
        const StrLen_t nWritePrev = m_nWriteIndex;
        AddStr(StrArg<_TYPE_CH>(pszStr));
        return CastN(HRESULT, m_nWriteIndex - nWritePrev);
    }

    /// <summary>
    /// Write values out to a string as comma separated base 10 numbers.
    /// opposite of cMemSpan::ReadFromCSV().
    /// For Bytes Try to use SetHexDigest() or Base64 instead?
    /// </summary>
    /// <typeparam name="_TYPE_VAL"></typeparam>
    /// <param name="src"></param>
    template <typename _TYPE_VAL>
    void AddCSV(const cSpan<_TYPE_VAL>& src) {  // static
        //! Similar to StrT::ConvertToCSV?
        for (_TYPE_VAL v : src) {
            AddSep(',');
            const StrLen_t iLenVal = StrNum::ValueToA<_TYPE_VAL>(GetSpanWrite(StrNum::k_LEN_MAX_DIGITS_INT), v);
            if (iLenVal <= 0) break;
            AdvanceWrite(iLenVal);
        }
    }
};
}  // namespace Gray
#endif
