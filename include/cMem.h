//! @file cMem.h
//! Move memory blocks and bytes around.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cMem_H
#define _INC_cMem_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"
#include "PtrCast.h"
#include <utility>  // std::move

namespace Gray {

/// <summary>
/// General return type from a compare.
/// Similar to _WIN#2 VARCMP_GT.
/// Similar to the C++ (Three-way comparison, spaceship operator) https://en.cppreference.com/w/cpp/language/operator_comparison
/// </summary>
typedef int COMPARE_t;  /// result of "Three-way" compare. 0=same, 1=a>b, -1=a<b, COMPRET_t
enum COMPRET_t {
    COMPARE_Less = -1,    /// like VARCMP_LT
    COMPARE_Equal = 0,    /// like VARCMP_EQ
    COMPARE_Greater = 1,  /// like VARCMP_GT
};

/// <summary>
/// static helpers for dealing with memory blocks. test bytes, Move memory bytes around.
/// May be on heap, const memory space or static in stack. do NOT assume. use cHeap.
/// Try to use cMemSpan intead of these calls directly.
/// </summary>
struct GRAYCORE_LINK cMem {                          // static cSingleton
    static VOLATILE uintptr_t sm_bDontOptimizeOut0;  /// static global byte to fool the optimizer into preserving this data.
    static VOLATILE uintptr_t sm_bDontOptimizeOutX;  /// static global byte to fool the optimizer into preserving this data.

    static const size_t k_PageSizeMin = 64;       /// Minimum page size for architecture. Usually More like 4K ?
    static const size_t k_ALLOC_MAX = 0x2000000;  /// (arbitrary) largest reasonable single malloc/object/span. e.g. Single frame allocation of big screen

    static const BYTE kFillAllocStack = 0xCC;   /// allocated on the stack in debug mode.
    static const BYTE kFillUnusedStack = 0xFE;  /// _DEBUG vsnprintf fills unused space with this.

#if !defined(UNDER_CE) && (!defined(_DEBUG) || !defined(_MSC_VER))
    static void __cdecl IsValidFailHandler(int nSig);  // JMP_t
#endif

    /// <summary>
    /// swap 2 values. similar to std::swap(). but uses the intrinsic = operator.
    /// dangerous for complex struct that has pointers and such. may not do a 'deep' copy.
    /// assume TYPE has a safe overloaded = operator. like std::swap()
    /// Overload this template for any specific TYPE Swaps.
    /// </summary>
    template <class TYPE>
    static constexpr void SwapT(TYPE& a, TYPE& b) noexcept {
#if 1
        std::swap<TYPE>(a, b);
#else
        TYPE tmp = std::move(a);  // use the std::move operator.
        a = std::move(b);
        b = std::move(tmp);
#endif
    }

    /// <summary>
    /// Get pointer difference in bytes. signed. Caller must check if its a reasonable sized block.
    /// </summary>
    static constexpr ptrdiff_t Diff(const void* pEnd, const void* pStart) noexcept {
        return PtrCast<BYTE>(pEnd) - PtrCast<BYTE>(pStart);  // like INT_PTR
    }

    /// <summary>
    /// Is this pointer in valid App data space? NOT nullptr. NOT kernel space. Kernel Space -lte- 1G or 2G for __linux__
    /// Can i plausibly read from this ? heap, stack, or static. Maybe code pointer ?
    /// Does not mean I have write permissions. May still page fault.
    /// Used to sanity check pointers. Ensure NOT offset from nullptr?
    /// </summary>
    /// <param name="pData"></param>
    /// <returns></returns>
    static bool constexpr IsValidApp(const void* pData) noexcept {
        const UINT_PTR u = CastPtrToNum(pData);
#if defined(_WIN32) && defined(USE_64BIT)
        // 1G or 2G ?
        if (u < 1024 * 1024 * 1024) return false;  // ASSUME memory in this range is never valid? Fail quickly. This is Kernel Space ONLY. <1G . PageSize ?
#else
        if (u < 16 * 1024) return false;  // ASSUME memory in this range is never valid? Fail quickly. This is Kernel Space ONLY. <1G . PageSize ?
#endif
        return true;
    }

    static bool GRAYCALL IsCorruptApp(const void* pData, size_t nSize, bool bWriteAccess = false) noexcept;

    /// <summary>
    /// Can i read or write to this ? nullptr => false.
    /// if DEBUG call IsCorruptApp(pData,1) ??
    /// </summary>
    /// <param name="pData"></param>
    /// <returns></returns>
    static bool constexpr inline IsValidPtr(const void* pData) noexcept {
        return IsValidApp(pData);
    }

    static inline bool IsZeros(const void* pData, size_t nSize) noexcept {
        //! Is all zeros ? nSize = 0 = true.
        if (!IsValidApp(pData)) return true;  // nullptr or corrupt?
        const BYTE* pB = static_cast<const BYTE*>(pData);
        for (size_t i = 0; i < nSize; i++) {
            if (pB[i] != 0) return false;
        }
        return true;
    }

    /// <summary>
    /// Compare two blocks of memory. ASSUME both are at least nSizeBlock sized.
    /// Does not assume memory alignment.
    /// @note 0 nSizeBlock is always equal. return 0. pointers should not be nullptr if nSizeBlock > 0
    /// </summary>
    static inline COMPARE_t Compare(const void* p1, const void* p2, size_t nSizeBlock) noexcept {
#if USE_CRT
        return ::memcmp(p1, p2, nSizeBlock);
#elif defined(__GNUC__)
        return ::__builtin_memcmp(p1, p2, nSizeBlock);
#else
        const BYTE* p1B = PtrCast<BYTE>(p1);
        const BYTE* p2B = PtrCast<BYTE>(p2);
        for (size_t i = 0; i < nSizeBlock; i++) {
            const BYTE b1 = p1B[i];
            const BYTE b2 = p2B[i];
            if (b1 != b2) return b1 - b2;
        }
        return COMPARE_Equal;
#endif
    }

    static inline bool IsEqual(const void* p1, const void* p2, size_t nSizeBlock) noexcept {
        return Compare(p1, p2, nSizeBlock) == COMPARE_Equal;
    }

    /// <summary>
    /// a constant-time buffer comparison. NOT efficient. BUT Prevents timing based hacks.
    /// </summary>
    /// <param name="p1"></param>
    /// <param name="p2"></param>
    /// <param name="nSizeBlock"></param>
    /// <returns>COMPARE_t</returns>
    static inline COMPARE_t CompareSecure(const void* p1, const void* p2, size_t nSizeBlock) noexcept {
        const BYTE* pB1 = PtrCast<BYTE>(p1);
        const BYTE* pB2 = PtrCast<BYTE>(p2);
        if (pB1 == nullptr || pB2 == nullptr) return CastN(COMPARE_t, pB1 - pB2);
        BYTE nDiff = 0;
        for (size_t i = 0; i < nSizeBlock; i++) {
            nDiff |= pB1[i] ^ pB2[i];
        }
        return CastN(COMPARE_t, nDiff);
    }

    /// <summary>
    /// Compare two buffers and return at what point they differ.
    /// Does not assume memory alignment for uintptr_t block compares.
    /// </summary>
    /// <param name="p1"></param>
    /// <param name="p2"></param>
    /// <param name="nSizeBlock"></param>
    /// <returns>nSizeBlock = equal.</returns>
    static size_t GRAYCALL CompareIndex(const void* p1, const void* p2, size_t nSizeBlock);

    /// <summary>
    /// Same as memset(). but with argument order change.
    /// </summary>
    static inline void Fill(void* pDst, size_t nSize, BYTE bVal) noexcept {
#if USE_CRT
        ::memset(pDst, bVal, nSize);
#elif defined(__GNUC__)
        ::__builtin_memset(pDst, bVal, nSize);
#elif defined(_WIN32)
        ::FillMemory(pDst, nSize, bVal);
#else
        BYTE* pDstB = PtrCast<BYTE>(pDst);
        for (size_t i = 0; i < nSize; i++) {
            pDstB[j] = bVal;
        }
#endif
    }

    /// <summary>
    /// set a block/blob of memory to Zeros.
    /// same as RtlZeroMemory() but maybe not the same as SecureZeroMeory() to make sure it is not optimized out. (for password clearing)
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="nSizeBlock"></param>
    static inline void Zero(void* pData, size_t nSizeBlock) noexcept {
#ifdef _WIN32
        ::ZeroMemory(pData, nSizeBlock);
#else
        Fill(pData, nSizeBlock, 0);
#endif
    }
    /// <summary>
    /// This is for security purposes. Not for initialization. Zero destructed values so they leave no trace.
    /// like SecureZeroMeory() and RtlSecureZeroMemory(). ensure it doesn't get optimized out. (like in an inline destructor)
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="nSizeBlock"></param>
    static inline void ZeroSecure(void* pData, size_t nSizeBlock) noexcept {
        VOLATILE BYTE* p2 = PtrCast<BYTE>(pData);  // 'volatile' will ensure it doesn't get optimized out.
        while (nSizeBlock--) *p2++ = 0;
    }

    /// <summary>
    /// Xor pDst with self.
    /// </summary>
    /// <param name="pDst"></param>
    /// <param name="pSrc"></param>
    /// <param name="nBlockSize"></param>
    /// <returns></returns>
    static inline void Xor(BYTE* pDst, const BYTE* pSrc, size_t nBlockSize) noexcept {
        for (size_t i = 0; i < nBlockSize; i++) pDst[i] ^= pSrc[i];
    }

    static inline void Xor2(BYTE* pDst, const BYTE* pSrc1, const BYTE* pSrc2, size_t nBlockSize) noexcept {
        for (size_t i = 0; i < nBlockSize; i++) pDst[i] = pSrc1[i] ^ pSrc2[i];
    }

    /// <summary>
    /// Copy a block of memory. same as CopyMemory(), RtlCopyMemory, memcpy().
    /// @note: Some older architectures needed versions of this to do 'huge' memory copies.
    /// @note: exact duplicate is fine but overlap is not.
    /// </summary>
    /// <param name="pDst"></param>
    /// <param name="pSrc"></param>
    /// <param name="nSizeBlock">if 0 then nullptr is ok.</param>
    static inline void Copy(void* pDst, const void* pSrc, size_t nSizeBlock) noexcept {
        ASSERT(pSrc != nullptr || nSizeBlock == 0);
        ASSERT(pDst != nullptr || nSizeBlock == 0);
#if USE_CRT
        ::memcpy(pDst, pSrc, nSizeBlock);
#elif defined(__GNUC__)
        ::__builtin_memcpy(pDst, pSrc, nSizeBlock);
#elif defined(_WIN32)
        ::CopyMemory(pDst, pSrc, nSizeBlock);
#else
        if (pDst == pSrc) return;
        register BYTE* pDstB = PtrCast<BYTE>(pDst);
        register const BYTE* pSrcB = PtrCast<BYTE>(pSrc);
        for (size_t i = 0; i < nSizeBlock; i++) {
            pDstB[i] = pSrcB[i];
        }
#endif
    }

    /// Does overlap require a reverse copy? NOT same as IsOverlap
    static constexpr bool IsOverlapRev(const void* pDst, const void* pSrc, size_t nSizeBlock) noexcept {
        const auto diff = cMem::Diff(pDst, pSrc);
        return diff > 0 && diff < CastN(ptrdiff_t, nSizeBlock);
    }

    /// <summary>
    /// Copy possibly overlapping blocks of memory. start from end or beginning if needed.
    /// same as MoveMemory RtlMoveMemory, memmove, hmemcpy(),
    /// @note: Some older architectures needed versions of this to do 'huge' memory moves.
    /// </summary>
    /// <param name="pDst"></param>
    /// <param name="pSrc"></param>
    /// <param name="nSizeBlock">bytes</param>
    static inline void CopyOverlap(void* pDst, const void* pSrc, size_t nSizeBlock) noexcept {
#if USE_CRT
        ::memmove(pDst, pSrc, nSizeBlock);
#elif defined(_WIN32)
        ::MoveMemory(pDst, pSrc, nSizeBlock);
#else
        if (IsOverlapRev(pDst, pSrc, nSizeBlock)) {  // reverse copy overlap
            BYTE* pDstB = PtrCast<BYTE>(pDst) + nSizeBlock - 1;
            const BYTE* pSrcB = PtrCast<BYTE>(pSrc) + nSizeBlock - 1;
            while (nSizeBlock--) {
                *pDstB = *pSrcB;
                pDstB--;
                pSrcB--;
            }
        } else {
            Copy(pDst, pSrc, nSizeBlock);  // forward overlap or direction doesn't matter. (no overlap)
        }
#endif
    }

    /// <summary>
    /// like ReverseSpan
    /// </summary>
    static constexpr void ReverseBytes(void* pDst, size_t nSizeBlock) noexcept {
        if (pDst == nullptr) return;
        BYTE* pSrcB = PtrCast<BYTE>(pDst);
        BYTE* pDstB = pSrcB + nSizeBlock - 1;
        nSizeBlock /= 2;
        while (nSizeBlock--) {
            SwapT<BYTE>(*pSrcB++, *pDstB--);
        }
    }

    /// <summary>
    /// Copy a block of memory BYTEs reversed. e.g. {3,2,1} = {1,2,3}, nSizeBlock = 3
    /// </summary>
    static constexpr void CopyReverse(void* pDst, const void* pSrc, size_t nSizeBlock) noexcept {
        if (pDst == pSrc) {
            ReverseBytes(pDst, nSizeBlock);
        } else {
            if (pDst == nullptr) return;
            BYTE* pDstB = PtrCast<BYTE>(pDst) + nSizeBlock - 1;
            if (pSrc == nullptr) return;
            const BYTE* pSrcB = PtrCast<BYTE>(pSrc);
            while (nSizeBlock--) {
                *pDstB-- = *pSrcB++;
            }
        }
    }

    /// <summary>
    /// Fill pDst with repeating copies of pSrc. Wrap back to start if nSrcSize is too small.
    /// </summary>
    static inline size_t CopyRepeat(void* pDst, size_t nDstSize, const void* pSrc, size_t nSrcSize, size_t nSrcStart = 0) noexcept {
        BYTE* pDstB = PtrCast<BYTE>(pDst);
        const BYTE* pSrcB = PtrCast<BYTE>(pSrc);
        for (size_t i = 0; i < nDstSize; i++) {
            pDstB[i] = pSrcB[nSrcStart];
            if (++nSrcStart > nSrcSize) nSrcStart = 0;
        }
        return nSrcStart;
    }

    /// <summary>
    /// Copy from Host (Local Native) Order into Network Order (Big Endian)
    /// </summary>
    static inline void CopyHtoN(BYTE* pDst, const void* pSrc, size_t nSizeBlock) noexcept {
#ifdef USE_LITTLE_ENDIAN
        cMem::CopyReverse(pDst, pSrc, nSizeBlock);
#else
        cMem::Copy(pDst, pSrc, nSizeBlock);
#endif
    }

    /// <summary>
    /// Copy from Network Order (Big Endian) to Host Order (Local Native)
    /// </summary>
    static inline void CopyNtoH(void* pDst, const BYTE* pSrc, size_t nSizeBlock) noexcept {
#ifdef USE_LITTLE_ENDIAN
        cMem::CopyReverse(pDst, pSrc, nSizeBlock);
#else
        cMem::Copy(pDst, pSrc, nSizeBlock);
#endif
    }

    /// <summary>
    /// swap copy 2 blocks of memory by bytes. like cMem::SwapT() but for 2 arbitrary sized blocks.
    /// swap them byte by byte.
    /// use cMem::SwapT instead if possible for intrinsic types.
    /// @arg nBlockSize = size in bytes.
    /// @note
    ///  DO NOT Use this for complex types that need special copiers. have internal pointers and such. use cMem::SwapT()
    /// </summary>
    /// <param name="pvMem1"></param>
    /// <param name="pvMem2"></param>
    /// <param name="nBlockSize"></param>
    static inline void Swap(void* pvMem1, void* pvMem2, size_t nBlockSize) noexcept {
        if (pvMem1 == pvMem2) return;  // no change.
        BYTE* pMem1 = PtrCast<BYTE>(pvMem1);
        BYTE* pMem2 = PtrCast<BYTE>(pvMem2);
        for (; nBlockSize--; pMem1++, pMem2++) {
            SwapT(*pMem1, *pMem2);
        }
    }
};

/// <summary>
/// attach debug data for validating the heap/memory.
/// uses an unlikely pattern of bytes. These may be scattered about any place. _SIGVALID = e.g. 0xFEEDF00D, 0xCA11AB1E
/// @note May not be allocated on the system heap.
/// @note This is somewhat redundant with some built in _MSC_VER _DEBUG heap code. heap has its own mechanism to do this.
/// </summary>
template <UINT32 _SIGVALID>
class DECLSPEC_NOVTABLE cMemSignature {
    UINT32 _nSignature = _SIGVALID;  /// Hold bytes of known value.
 public:
    static const UINT32 k_INVALID = 0xDEADBEA7;  /// Mark as NOT valid when freed! Don't use this pattern for anything else.
 public:
    void put_Valid() {
        _nSignature = _SIGVALID;
    }
    void put_Invalid() {
        _nSignature = k_INVALID;
    }
    cMemSignature() noexcept {
        static_assert(k_INVALID != _SIGVALID && 0 != _SIGVALID, "cMemSignature");
    }
    ~cMemSignature() {
        //! @note calling virtual doesn't work in constructor or destructor ! so ASSERT( isValidCheck()); not possible here !
        put_Invalid();  // Mark as invalid.
    }
    bool inline isValidSig() const noexcept {
        // assumes pointer is good.
        return _nSignature == _SIGVALID;
    }
    bool inline isValidSignature() const noexcept {
        return cMem::IsValidApp(this) && isValidSig();
    }
};
}  // namespace Gray
#endif
