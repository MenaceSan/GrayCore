//! @file cMem.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "HResult.h"
#include "cBlob.h"
#include "cDebugAssert.h"
#include "cExceptionSystem.h"
#include "cMem.h"
#include "cValSpan.h"

namespace Gray {

#if USE_CRT && !defined(UNDER_CE) && !defined(_CPPUNWIND)
#include <setjmp.h>
#include <signal.h>

static cExceptionJmp s_CMem_IsValidFailJmpBuf;
typedef void(__cdecl* SIGNAL_t)(int);

void __cdecl cMem::IsValidFailHandler(int nSig) {  // SIGNAL_t static
    //! @todo make s_CMem_IsValidFailJmpBuf thread safe.
    UNREFERENCED_PARAMETER(nSig);
    s_CMem_IsValidFailJmpBuf.Jump(1);
}
#endif

uintptr_t VOLATILE cMem::sm_bDontOptimizeOut0 = 0;  /// used to trick the optimizer. Always 0.
uintptr_t VOLATILE cMem::sm_bDontOptimizeOutX = 1;  /// used to trick the optimizer. Unknown value.

const cBlob cBlob::k_Empty;  // const static

/// <summary>
/// Is this pointer valid to read/write to ? On heap, stack or static const data space.
/// similar to _MFC_VER AfxIsValidAddress(), AtlIsValidAddress()
/// @note This IS slow. This should only ever be used in _DEBUG code. and only in an ASSERT.
/// IsBadWritePtr() is just doing exception handling under the hood.
/// _CrtIsValidPointer is just a null check in >= VS2010. Why bother?
/// @note This can cause problems with thread stack guard pages in theory. https://msdn.microsoft.com/en-us/library/bb288454.aspx
/// @todo make a faster (less thorough) version of this ?
/// </summary>
/// <param name="pData"></param>
/// <param name="nLen"></param>
/// <param name="bWriteAccess"></param>
/// <returns></returns>
bool GRAYCALL cMem::IsCorruptApp(const void* pData, size_t nLen, bool bWriteAccess) noexcept {  // static
    if (pData == nullptr) {                                                                     // nullptr is not technically bad unless nLen > 0.
        if (nLen == 0) return false;                                                            // This technically is ok.
        return true;
    }
    if (!IsValidApp(pData)) return true;  // ASSUME memory in this range is never valid? Fail quickly. This is Kernel Space. <1G

        // OS check for valid memory is not reliable??

#if defined(UNDER_CE)
    if (bWriteAccess) return ::IsBadWritePtr(const_cast<void*>(pData), nLen);
    return ::IsBadReadPtr(pData, nLen);
#elif defined(_CPPUNWIND)
    // They say IsBadReadPtr() isn't as good as exceptions. So don't use it.
    UNREFERENCED_PARAMETER(bWriteAccess);
    GRAY_TRY {  // GRAY_TRY or try
        // NOTE: Aligned blocks might make this faster.
        sm_bDontOptimizeOut0 = 0;  // Always 0
        for (size_t i = 0; i < nLen; i += k_PageSizeMin) {
            const BYTE bVal = ((const BYTE*)pData)[i];  // Read it.
            if (bWriteAccess) {                         // try to write it back.
                ((BYTE*)pData)[i] = bVal | (BYTE)sm_bDontOptimizeOut0;
            }
        }
        return false;
    }
    GRAY_TRY_CATCHALL {  // GRAY_TRY_CATCHALL // catch(...)
        return true;     // We failed.
    }
    GRAY_TRY_END;

#elif USE_CRT
    //! __linux__ / POSIX version of IsBadReadPtr()
    UNREFERENCED_PARAMETER(bWriteAccess);  // TODO bWriteAccess
    SIGNAL_t pPrevSignal = nullptr;
    if (s_CMem_IsValidFailJmpBuf.Init() != 0) {
        // We failed.
        ::signal(SIGSEGV, pPrevSignal);
        return true;
    }
    pPrevSignal = ::signal(SIGSEGV, IsValidFailHandler);

    // NOTE: Aligned blocks might make this faster.
    sm_bDontOptimizeOut0 = 0;  // Always 0
    for (size_t i = 0; i < nLen; i += k_PageSizeMin) {
        BYTE bVal = ((const BYTE*)pData)[i];  // try to Read it.
        if (bWriteAccess)                     // try to write it.
        {
            ((BYTE*)pData)[i] = bVal | (BYTE)sm_bDontOptimizeOut0;
        }
    }

    ::signal(SIGSEGV, pPrevSignal);  // undo signal.
    return false;                    // its good.
#else
    return false;
#endif
}

size_t GRAYCALL cMem::CompareIndex(const void* p1, const void* p2, size_t nSizeBlock) {  // static
    if (p1 == p2) return nSizeBlock;                                                     // is equal.
    if (p1 == nullptr || p2 == nullptr) return 0;

    size_t i = 0;

    if (nSizeBlock >= sizeof(uintptr_t)) {  // compare using max size registers.
        // TODO Alignment of pointers may be important?!? if not uintptr_t aligned this will fail on PPC?

        uintptr_t nDiffA = 0;
        size_t nSizeBlockA = nSizeBlock;
        if ((nSizeBlock & (sizeof(uintptr_t) - 1)) != 0) {
            nSizeBlockA &= ~(sizeof(uintptr_t) - 1);  // trim any unaligned part off the end for this loop.
        }

        for (; i < nSizeBlockA; i += sizeof(uintptr_t)) {
            nDiffA |= ((const uintptr_t*)p1)[i] ^ ((const uintptr_t*)p2)[i];
            if (nDiffA != 0)  // NOT the same.
                break;        // now find the exact byte in the block.
        }
    }

    // Do odd part at the end. BYTE aligned.
    BYTE nDiffB = 0;
    for (; i < nSizeBlock; i++) {
        nDiffB |= ((const BYTE*)p1)[i] ^ ((const BYTE*)p2)[i];
        if (nDiffB != 0)  // NOT the same.
            return i;
    }

    return nSizeBlock;  // is equal.
}

//**********************************************************

COMPARE_t cMemSpan::Compare(const cMemSpan& m2) const noexcept {
    //! @return COMPARE_Equal
    const size_t iLenMin = cValT::Min(get_SizeBytes(), m2.get_SizeBytes());
    const COMPARE_t iRet = cMem::Compare(GetTPtrC(), m2.GetTPtrC(), iLenMin);
    if (iRet != COMPARE_Equal) return iRet;
    return cValT::Compare(get_SizeBytes(), m2.get_SizeBytes());  // not same length? if otherwise equal the longer one wins
}

size_t cMemSpan::ReadFromCSV(const char* pszSrc) {
    //! Read/Parse bytes in from string as comma separated base 10 number values. opposite of StrT::ConvertToCSV().
    //! @return the number of bytes read.
    //! @note using hex or Base64 would be better.

    size_t i = 0;
    for (; i < get_SizeBytes();) {
        pszSrc = StrT::GetNonWhitespace(pszSrc);
        if (StrT::IsNullOrEmpty(pszSrc)) break;
        const char* pszSrcStart = pszSrc;
        GetTPtrW()[i++] = (BYTE)(StrNum::toU(pszSrc, &pszSrc, 10));
        if (pszSrcStart == pszSrc) break;  // must be the field terminator? ")},;". End.
        pszSrc = StrT::GetNonWhitespace(pszSrc);
        if (pszSrc[0] != ',') break;
        pszSrc++;
    }
    return i;
}

StrLen_t cMemSpan::GetHexDigest(cMemSpan hexString) const {
    char* pszHexString = hexString.GetTPtrW<char>();
    if (hexString.isEmpty()) return 0;
    const StrLen_t iLenMax = CastN(StrLen_t, cValT::Min(get_SizeBytes(), hexString.get_SizeBytes() - 1));
    StrLen_t iLen = 0;
    for (size_t i = 0; i < iLenMax; i++) {
        const BYTE ch = GetTPtrC()[i];
        pszHexString[iLen++] = StrChar::U2Hex(ch >> 4);
        pszHexString[iLen++] = StrChar::U2Hex(ch & 0x0F);
    }
    pszHexString[iLen] = '\0';
    return iLen;  // iLenMax
}

HRESULT cMemSpan::ReadHexDigest(const char* pszHexString, bool testEnd) {
    if (pszHexString == nullptr || isNull()) return E_POINTER;

    bool bNonZero = false;
    for (size_t i = 0; i < get_SizeBytes(); i++) {
        // 2 hex chars per byte.
        char ch = *pszHexString++;
        if (ch == '\0') return HRESULT_WIN32_C(RPC_X_BYTE_COUNT_TOO_SMALL);  // not long enough!
        const UINT bHex1 = StrChar::Hex2U(ch);
        if (bHex1 >= 0x10) return HRESULT_WIN32_C(ERROR_SXS_XML_E_INVALID_HEXIDECIMAL);
        ch = *pszHexString++;
        const UINT bHex2 = StrChar::Hex2U(ch);
        if (bHex2 >= 0x10) return HRESULT_WIN32_C(ERROR_SXS_XML_E_INVALID_HEXIDECIMAL);
        GetTPtrW()[i] = CastN(BYTE, (bHex1 * 0x10) + bHex2);
        if (GetTPtrW()[i] != 0) bNonZero = true;
    }

    if (testEnd) {
        if (StrChar::IsDigitRadix(*pszHexString, 0x10)) return HRESULT_WIN32_C(ERROR_BUFFER_OVERFLOW);  // too long! or not terminated.
    }
    if (bNonZero) return S_OK;
    return S_FALSE;  // zero
}

//**********************************************************

void cMemSpan::ReverseSpan(size_t stride) {
    // Optimize for the intrinsic sized blocks first.
    switch (stride) {
        case sizeof(BYTE):
            cSpanX<BYTE>(*this).ReverseSpan();
            return;
        case sizeof(WORD):
            cSpanX<WORD>(*this).ReverseSpan();
            return;
        case sizeof(UINT32):
            cSpanX<UINT32>(*this).ReverseSpan();
            return;
        default:
            break;
    }

    // array of arbitrary block size.
    BYTE* pMemBS = this->get_BytePtrW();
    BYTE* pMemBE = pMemBS + this->get_SizeBytes() - stride;  // end
    for (; pMemBS < pMemBE; pMemBS += stride, pMemBE -= stride) {
        cMem::Swap(pMemBS, pMemBE, stride);
    }
}
}  // namespace Gray
