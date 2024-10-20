//! @file StrBuilder.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_StrBuilder_H
#define _INC_StrBuilder_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "ITextWriter.h"
#include "StrConst.h"
#include "StrNum.h"
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
class GRAYCORE_LINK StrBuilder : protected cBlob, public ITextWriter {
    typedef cBlob SUPER_t;
    friend struct StrFormat<_TYPE_CH>;
    friend struct StrTemplate;

 public:
    static const StrLen_t k_nGrowSizeChunk = 1024;  /// -le- k_LEN_Default if isHeap()
    static const StrLen_t k_ExtraNul = 1;           // '\0'

    StrLen_t _nWriteIndex = 0;  /// new items added/written here. end of readable. like cQueueIndex

 protected:
    inline StrLen_t get_AllocQty() const noexcept {
        return CastN(StrLen_t, SUPER_t::get_SizeBytes() / sizeof(_TYPE_CH));
    }

 public:
    inline _TYPE_CH* get_PtrWork() {
        return SUPER_t::GetTPtrW<_TYPE_CH>();
    }
    inline void SetTerminated() noexcept {
        // always force terminate.
        if (!isValidPtr()) return;  // just estimating.
        DEBUG_CHECK(this->IsInSize(_nWriteIndex));
        get_PtrWork()[this->_nWriteIndex] = '\0';
    }

    /// Build with a growing heap buffer.
    StrBuilder(StrLen_t nSizeChunk = k_nGrowSizeChunk) noexcept : SUPER_t(nSizeChunk) {
        SetTerminated();
    }
    /// Build with a non-growing static buffer.
    StrBuilder(cSpanX<_TYPE_CH> ret) noexcept : SUPER_t(ret, MEMTYPE_t::_Temp) {
        SetTerminated();
    }
    StrBuilder(SUPER_t& r) noexcept : SUPER_t(r) {
        SetTerminated();
    }

    /// <summary>
    /// How much space is avail for write into buffer? (given the current get_AllocQty() allocation size)
    /// Free space to write more. Not including '\0'
    /// </summary>
    /// <returns>Qty of TYPE</returns>
    inline StrLen_t get_WriteSpaceQty() const noexcept {
        return (get_AllocQty() - this->_nWriteIndex) - k_ExtraNul;  // leave space for '\0'
    }
    inline StrLen_t get_WriteSpaceMax() const noexcept {
        return isHeap() ? ((cStrConst::k_LEN_MAX - this->_nWriteIndex) - k_ExtraNul) : get_WriteSpaceQty();
    }

    /// <summary>
    /// Attempt to get more space for write iNeedCount of TYPE.
    /// ALWAYS check this->get_WriteSpaceQty() after this!
    /// Allocate as much as i can and truncate the rest.
    /// Paired with AdvanceWrite().
    /// </summary>
    _TYPE_CH* GetWritePrep(StrLen_t iNeedCount) {
        if (isNull()) return nullptr;  // just estimating?
        if (isHeap()) {                // can grow?
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
        return get_PtrWork() + this->_nWriteIndex;
    }

    cSpanX<_TYPE_CH> get_SpanWrite() {
        return ToSpan(get_PtrWork() + this->_nWriteIndex, get_WriteSpaceQty());
    }
    cSpanX<_TYPE_CH> GetSpanWrite(StrLen_t iNeedCount) {
        _TYPE_CH* p = GetWritePrep(iNeedCount);  // MUST call this first to grow.
        return ToSpan(p, get_WriteSpaceQty());   // May be greater than i asked for!
    }

    /// <summary>
    /// advanced the used space.
    /// The estimated size in GetWritePrep() might have been larger.
    /// </summary>
    /// <param name="nLen"></param>
    inline void AdvanceWrite(StrLen_t nLen) noexcept {
        DEBUG_CHECK(nLen <= get_WriteSpaceQty());
        this->_nWriteIndex += nLen;
        DEBUG_CHECK(this->_nWriteIndex >= nLen);
        SetTerminated();
    }

    void SetEmptyStr() noexcept {
        _nWriteIndex = 0;
        SetTerminated();
    }

    /// <summary>
    /// get used/filled Length. not including '\0';
    /// </summary>
    inline StrLen_t get_Length() const noexcept {
        return _nWriteIndex;
    }
    /// <summary>
    /// Get raw pointer to the string value.
    /// </summary>
    /// <returns></returns>
    inline const _TYPE_CH* get_CPtr() const noexcept {
        return SUPER_t::GetTPtrC<_TYPE_CH>();
    }

    /// <summary>
    /// operator to auto cast to const pointer
    /// </summary>
    operator const _TYPE_CH*() const noexcept {
        return get_CPtr();
    }

    cSpan<_TYPE_CH> get_SpanStr() const noexcept {
        return ToSpan(get_CPtr(), get_Length());  // NOT the '\0'
    }
    cSpanX<_TYPE_CH> get_SpanEdit() noexcept {
        return ToSpan(get_PtrWork(), get_Length());  // NOT the '\0'
    }
    void SetTrimWhiteSpaceEnd() {
        _nWriteIndex = StrT::GetWhitespaceEnd(get_SpanStr());
        SetTerminated();
    }

    /// <summary>
    /// Assume truncation occurred? DISP_E_BUFFERTOOSMALL
    /// </summary>
    inline bool isOverflow() const noexcept {
        return get_WriteSpaceQty() <= 0;
    }

    void AddChar(_TYPE_CH ch) {
        // includes space for terminator '\0'
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
        if (_nWriteIndex > 0) AddChar(ch);
    }

    void AddCharRepeat(_TYPE_CH ch, StrLen_t iRepeat) {
        cSpanX<_TYPE_CH> spanWrite = GetSpanWrite(iRepeat);
        const StrLen_t nLenWrite = cValT::Min(iRepeat, spanWrite.get_MaxLen());
        if (!spanWrite.isNull()) {
            cValSpan::FillQty<_TYPE_CH>(spanWrite.get_PtrWork(), nLenWrite, ch);
        }
        AdvanceWrite(nLenWrite);
    }

    void AddSpan(const cSpan<_TYPE_CH>& span) {
        // span = not including space for '\0'
        if (span.isEmpty()) return;  // just add nothing.
        cSpanX<_TYPE_CH> spanWrite = GetSpanWrite(span.GetSize());
        const StrLen_t nLenWrite = cValT::Min(spanWrite.get_MaxLen(), span.GetSize());
        if (!spanWrite.isNull()) {
            cSpanX<_TYPE_CH> spanWrite2 = ToSpan(spanWrite.get_PtrWork(), nLenWrite + k_ExtraNul);  // add 1 more for '\0'. GetSpanWrite() allows k_ExtraNul!
            StrT::CopyPtr(spanWrite2, span.get_PtrConst());
        }
        AdvanceWrite(nLenWrite);
    }

    /// AKA WriteString()
    void AddStr(const _TYPE_CH* pszStr) {
        if (pszStr == nullptr) return;
        AddSpan(StrT::ToSpanStr(pszStr));
    }

    /// Add quoted string. NOT escaped! EscSeqAddQ() ?
    void AddSpanQ(const cSpan<_TYPE_CH>& span, STR_BLOCK_t eBlockType = STR_BLOCK_t::_QUOTE) {
        if (eBlockType != STR_BLOCK_t::_NONE) {
            AddChar(StrT::GetBlockStart(eBlockType));
        }
        AddSpan(span);
        if (eBlockType != STR_BLOCK_t::_NONE) {
            AddChar(StrT::GetBlockEnd(eBlockType));
        }
    }
    void AddCrLf() {
        // AKA CRNL
        AddSpan(ToSpanStr<_TYPE_CH>(cStrConst::k_CrLf));
    }

    /// Add a nullptr terminated list of strings. AKA Concat
    void _cdecl Join(const _TYPE_CH* psz1, ...) {
        ::va_list vargs;
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
    void AddBytesRaw(const cMemSpan& data) {
        const StrLen_t nLenSrc = CastN(StrLen_t, data.get_SizeBytes() / sizeof(_TYPE_CH));
        AdvanceWrite(GetSpanWrite(nLenSrc).SetCopyQty(data.GetTPtrC<_TYPE_CH>(), nLenSrc));
    }

    void AddBytesFiltered(const cMemSpan& data) {
        // Just add a string from void*. Don't assume terminated string. filter for printable characters.
        const StrLen_t nLenSrc = CastN(StrLen_t, data.get_SizeBytes() / sizeof(_TYPE_CH));
        auto spanWrite = GetSpanWrite(nLenSrc);
        const StrLen_t nLenRet = cValT::Min(spanWrite.get_MaxLen(), nLenSrc);
        if (!spanWrite.isNull() && !data.isNull()) {
            _TYPE_CH* pWrite = spanWrite.get_PtrWork();
            const BYTE* pSrc = data;
            for (StrLen_t i = 0; i < nLenRet; i++) {
                const BYTE ch = pSrc[i];
                pWrite[i] = StrChar::IsPrintA(ch) ? ch : '?';  // strip unprintable chars.
            }
        }
        AdvanceWrite(nLenRet);
    }

    void AddInt(INT64 uVal);
    void AddUInt(UINT64 uVal, RADIX_t nRadix = 10);
    void AddFloat(double dVal, char chE = -'e');

    void AddFormatV(const _TYPE_CH* pszFormat, ::va_list vargs);
    void _cdecl AddFormat(const _TYPE_CH* pszFormat, ...);

    HRESULT WriteString(const char* pszStr) override {  // ITextWriter
        const StrLen_t nWritePrev = _nWriteIndex;
        AddStr(StrArg<_TYPE_CH>(pszStr));
        return CastN(HRESULT, _nWriteIndex - nWritePrev);
    }
    HRESULT WriteString(const wchar_t* pszStr) override {  // ITextWriter
        const StrLen_t nWritePrev = _nWriteIndex;
        AddStr(StrArg<_TYPE_CH>(pszStr));
        return CastN(HRESULT, _nWriteIndex - nWritePrev);
    }

    /// <summary>
    /// Write values out to a string as comma separated base 10 numbers.
    /// opposite of cMemSpan::ReadFromCSV().
    /// For Bytes Try to use ReadHexDigest() or Base64 instead?
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
