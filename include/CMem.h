//! @file cMem.h
//! Move memory blocks and bytes around.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cMem_H
#define _INC_cMem_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"
#include "StrConst.h"
#include "cDebugAssert.h"
#include "cValT.h"

namespace Gray {
/// <summary>
/// static helpers for dealing with memory blocks. test bytes, Move memory bytes around.
/// May be on heap, const memory space or static in stack. do NOT assume. use cHeap.
/// </summary>
struct GRAYCORE_LINK cMem {                          // static cSingleton
    static VOLATILE uintptr_t sm_bDontOptimizeOut0;  /// static global byte to fool the optimizer into preserving this data.
    static VOLATILE uintptr_t sm_bDontOptimizeOutX;  /// static global byte to fool the optimizer into preserving this data.
    static const size_t k_PageSizeMin = 64;          /// Minimum page size for architecture. Usually More like 4K ?

    static const BYTE kFillAllocStack = 0xCC;   /// allocated on the stack in debug mode.
    static const BYTE kFillUnusedStack = 0xFE;  /// _DEBUG vsnprintf fills unused space with this.

#if !defined(UNDER_CE) && (!defined(_DEBUG) || !defined(_MSC_VER))
    static void __cdecl IsValidFailHandler(int nSig);  // JMP_t
#endif

    /// <summary>
    /// Get pointer difference in bytes. signed. Caller must check if its a reasonable sized block.
    /// </summary>
    static constexpr ptrdiff_t Diff(const void* pEnd, const void* pStart) noexcept {
        return PtrCast<BYTE>(pEnd) - PtrCast<BYTE>(pStart);  // like INT_PTR
    }

    /// <summary>
    /// Is this pointer in valid App space? Not kernel space. Kernel Space -lte- 1G or 2G for __linux__
    /// Can i plausibly read from this ? heap, stack, or static.
    /// Does not mean I have write permissions. May still page fault.
    /// Used to sanity check pointers. Ensure NOT offset from nullptr?
    /// </summary>
    /// <param name="pData"></param>
    /// <returns></returns>
    static bool constexpr IsValidApp(const void* pData) noexcept {
        const UINT_PTR u = PtrCastToNum(pData);
#if defined(_WIN32) && defined(USE_64BIT)
        // 1G or 2G ?
        if (u < 1024 * 1024 * 1024)  // ASSUME memory in this range is never valid? Fail quickly. This is Kernel Space ONLY. <1G . PageSize ?
            return false;
#else
        if (u < 16 * 1024)  // ASSUME memory in this range is never valid? Fail quickly. This is Kernel Space ONLY. <1G . PageSize ?
            return false;   //
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
        if (!IsValidApp(pData))  // nullptr or corrupt?
            return true;
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
            register BYTE b1 = p1B[i];
            register BYTE b2 = p2B[i];
            if (b1 != b2) return b1 - b2;
        }
        return COMPARE_Equal;
#endif
    }

    static inline bool IsEqual(const void* p1, const void* p2, size_t nSizeBlock) noexcept {
        return Compare(p1, p2, nSizeBlock) == COMPARE_Equal;
    }

    static inline COMPARE_t CompareSecure(const void* p1, const void* p2, size_t nSizeBlock) noexcept {
        //! constant-time buffer comparison. NOT efficient. BUT Prevents timing based hacks.
        const BYTE* pB1 = PtrCast<BYTE>(p1);
        const BYTE* pB2 = PtrCast<BYTE>(p2);
        BYTE nDiff = 0;
        for (size_t i = 0; i < nSizeBlock; i++) {
            nDiff |= pB1[i] ^ pB2[i];
        }
        return CastN(COMPARE_t, nDiff);
    }

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

    static inline void Xor(BYTE* pDst, const BYTE* pSrc, size_t nBlockSize) noexcept {
        // Xor with self.
        for (size_t i = 0; i < nBlockSize; i++) pDst[i] ^= pSrc[i];
    }

    static inline void Xor2(BYTE* pDst, const BYTE* pSrc1, const BYTE* pSrc2, size_t nBlockSize) noexcept {
        for (size_t i = 0; i < nBlockSize; i++) pDst[i] = pSrc1[i] ^ pSrc2[i];
    }

    /// <summary>
    /// Copy a block of memory. same as CopyMemory(), RtlCopyMemory, memcpy().
    /// @note: Some older architectures needed versions of this to do 'huge' memory copies.
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

    /// Does overlap require a reverse copy?
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
            register BYTE* pDstB = PtrCast<BYTE>(pDst) + nSizeBlock - 1;
            register const BYTE* pSrcB = PtrCast<BYTE>(pSrc) + nSizeBlock - 1;
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
    static inline void ReverseBytes(void* pDst, size_t nSizeBlock) noexcept {
        register BYTE* pSrcB = PtrCast<BYTE>(pDst);
        register BYTE* pDstB = pSrcB + nSizeBlock - 1;
        nSizeBlock /= 2;
        while (nSizeBlock--) {
            cValT::Swap(*pSrcB++, *pDstB--);
        }
    }

    /// <summary>
    /// Copy a block of memory BYTEs reversed. e.g. {3,2,1} = {1,2,3}, nSizeBlock = 3
    /// </summary>
    static inline void CopyReverse(void* pDst, const void* pSrc, size_t nSizeBlock) noexcept {
        if (pDst == pSrc) {
            ReverseBytes(pDst, nSizeBlock);
        } else {
            register BYTE* pDstB = PtrCast<BYTE>(pDst) + nSizeBlock - 1;
            register const BYTE* pSrcB = PtrCast<BYTE>(pSrc);
            while (nSizeBlock--) {
                *pDstB-- = *pSrcB++;
            }
        }
    }

    /// <summary>
    /// Fill pDst with repeating copies of pSrc. Wrap back to start if nSrcSize is too small.
    /// </summary>
    static inline size_t CopyRepeat(void* pDst, size_t nDstSize, const void* pSrc, size_t nSrcSize, size_t nSrcStart = 0) noexcept {
        register BYTE* pDstB = PtrCast<BYTE>(pDst);
        register const BYTE* pSrcB = PtrCast<BYTE>(pSrc);
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
    /// swap copy 2 blocks of memory by bytes. like cMemT::Swap() but for 2 arbitrary sized blocks.
    /// swap them byte by byte.
    /// use cMemT::Swap instead if possible for intrinsic types.
    /// @arg nBlockSize = size in bytes.
    /// @note
    ///  DO NOT Use this for complex types that need special copiers. have internal pointers and such. use cMemT::Swap()
    /// </summary>
    /// <param name="pvMem1"></param>
    /// <param name="pvMem2"></param>
    /// <param name="nBlockSize"></param>
    static inline void Swap(void* pvMem1, void* pvMem2, size_t nBlockSize) noexcept {
        if (pvMem1 == pvMem2)  // no change.
            return;
        BYTE* pMem1 = PtrCast<BYTE>(pvMem1);
        BYTE* pMem2 = PtrCast<BYTE>(pvMem2);
        for (; nBlockSize--; pMem1++, pMem2++) {
            cValT::Swap(*pMem1, *pMem2);
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
 public:
    static const UINT32 k_INVALID = 0xDEADBEA7;  /// Mark as NOT valid when freed!
 private:
    UINT32 m_nSignature;  /// Hold bytes of known value.
 public:
    cMemSignature() noexcept : m_nSignature(_SIGVALID) {
        //! @note calling virtuals doesn't work in constructors or destructors !
        static_assert(k_INVALID != _SIGVALID && 0 != _SIGVALID, "cMemSignature");
    }
    ~cMemSignature() {
        //! @note calling virtuals doesn't work in constructors or destructors !
        //! so ASSERT( isValidCheck()); not possible here !
        ASSERT(isValidSignature());
        m_nSignature = k_INVALID;  // Mark as invalid.
    }
    bool inline isValidSignature() const noexcept {
        if (!cMem::IsValidApp(this)) return false;
        if (m_nSignature != _SIGVALID) return false;
        return true;
    }
};

/// <summary>
/// collection of templates to handle An arbitrary value type in memory. cValT in heap, const or stack.
/// Deal with it as an array of bytes.
/// </summary>
struct GRAYCORE_LINK cMemT : public cValT {  // Value of some type in memory.
    /// <summary>
    /// Reverse the byte order in an intrinsic type.
    /// Like __GNUC__ __builtin_bswap16(), __builtin_bswap32, etc
    /// Similar to: ntohl(), htonl(), ntohs(), htons().
    /// </summary>
    template <class TYPE>
    static inline TYPE ReverseType(TYPE nVal) noexcept {
        cMem::CopyReverse(&nVal, &nVal, sizeof(nVal));
        return nVal;
    }
    template <typename TYPE>
    static inline TYPE HtoN(TYPE nVal) noexcept {
        //! Host byte order to network order (big endian). like htonl() htons()
#ifdef USE_LITTLE_ENDIAN
        //! Assume bytes are correct bit order but larger types are not ordered correctly.
        return ReverseType(nVal);
#else
        return nVal;  // no change needed.
#endif
    }
    template <typename TYPE>
    static inline TYPE NtoH(TYPE nVal) noexcept {
        //! Network byte order (big endian) to host order. like ntohl() ntohs()
        //! Network order = BigEndian = High order comes first = Not Intel.
#ifdef USE_LITTLE_ENDIAN
        //! Assume bytes are correct bit order but larger types are not ordered correctly.
        return ReverseType(nVal);
#else
        return nVal;  // no change needed.
#endif
    }

    template <typename TYPE>
    static inline TYPE HtoLE(TYPE nVal) noexcept {
        //! Host byte order to little endian. (Intel)
#ifdef USE_LITTLE_ENDIAN
        return nVal;  // no change needed.
#else
        //! Assume bytes are correct bit order but larger types are not ordered correctly.
        return ReverseType(nVal);
#endif
    }
    template <typename TYPE>
    static inline TYPE LEtoH(TYPE nVal) noexcept {
        //! Little Endian (Intel) to host byte order.
#ifdef USE_LITTLE_ENDIAN
        return nVal;  // no change needed.
#else
        //! Assume bytes are correct bit order but larger types are not ordered correctly.
        return ReverseType(nVal);
#endif
    }

    /// <summary>
    /// Get a data TYPE value from an unaligned TYPE pointer.
    /// Like the _WIN32 UNALIGNED macro.
    /// @note some architectures will crash if you try to access unaligned data. (PowerPC)
    /// In this case we need to memcpy() to a temporary buffer first.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pData"></param>
    /// <returns></returns>
    template <typename TYPE>
    static inline TYPE GetUnaligned(const void* pData) noexcept {
#ifdef _MSC_VER
        return *((const UNALIGNED TYPE*)pData);
#else
        return *((const TYPE*)pData);
#endif
    }

    /// <summary>
    /// Set data value from an unknown unaligned TYPE pointer.
    /// Like the _WIN32 UNALIGNED macro.
    /// @note some architectures will crash if you try to access unaligned data. (PowerPC)
    /// In this case we need to memcpy() to a temporary buffer first.
    /// </summary>
    template <typename TYPE>
    static inline void SetUnaligned(void* pData, TYPE nVal) noexcept {
#ifdef _MSC_VER
        *((UNALIGNED TYPE*)pData) = nVal;
#else
        *((TYPE*)pData) = nVal;
#endif
    }

    /// <summary>
    /// Get bytes packed as LE (Intel). ( Not "Network order" which is big endian.)
    /// </summary>
    template <typename TYPE>
    static inline TYPE GetLEtoH(const void* pData) noexcept {
        return LEtoH(GetUnaligned<TYPE>(pData));
    }
    /// <summary>
    /// Set bytes packed as LE (Intel). ( Not "Network order" which is big endian.)
    /// </summary>
    template <typename TYPE>
    static inline void SetHtoLE(void* pData, TYPE nVal) noexcept {
        return SetUnaligned(pData, HtoLE(nVal));
    }
    /// <summary>
    /// Get bytes packed as BE (Network order, Not Intel). similar to CopyNtoH()
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pData"></param>
    /// <returns></returns>
    template <typename TYPE>
    static inline TYPE GetNtoH(const void* pData) noexcept {
        return NtoH(GetUnaligned<TYPE>(pData));
    }
    /// <summary>
    /// Set bytes packed as BE (Network order, Not Intel). similar to CopyHtoN()
    /// </summary>
    template <typename TYPE>
    static inline void SetHtoN(void* pData, TYPE nVal) noexcept {
        return SetUnaligned(pData, HtoN(nVal));
    }

    /// <summary>
    /// Get 3 (Network order, Big Endian) BYTEs as a host value.
    /// opposite of SetHtoN3()
    /// </summary>
    static inline UINT32 GetNtoH3(const BYTE* p) noexcept {
        return ((UINT32)p[0]) << 16 | ((UINT32)p[1]) << 8 | p[2];
    }

    /// <summary>
    /// Set 3 (Network order, Big Endian) BYTEs from host value.
    /// opposite of GetNtoH3()
    /// </summary>
    static inline void SetHtoN3(BYTE* p, UINT nVal) noexcept {
        p[0] = (BYTE)((nVal >> 16) & 0xFF);
        p[1] = (BYTE)((nVal >> 8) & 0xFF);  // HIBYTE
        p[2] = (BYTE)((nVal)&0xFF);         // LOBYTE
    }
};

template <>
inline WORD cMemT::ReverseType<WORD>(WORD nVal) noexcept {  // static
    //! Reverse the bytes in an intrinsic 16 bit type WORD. e.g. 0x1234 = 0x3412
    //! like ntohs(),htons(), MAKEWORD()
    return (WORD)((nVal >> 8) | (nVal << 8));
}
template <>
inline UINT32 cMemT::ReverseType<UINT32>(UINT32 nVal) noexcept {  // static
    //! Reverse the bytes in an intrinsic 32 bit type UINT32.
    //! like ntohl(),htonl()
    nVal = (nVal >> 16) | (nVal << 16);
    return ((nVal & 0xff00ff00UL) >> 8) | ((nVal & 0x00ff00ffUL) << 8);
}

#ifdef USE_INT64
template <>
inline UINT64 cMemT::ReverseType<UINT64>(UINT64 nVal) noexcept {  // static
    //! Reverse the bytes in an intrinsic 64 bit type UINT64.
    nVal = (nVal >> 32) | (nVal << 32);
    nVal = ((nVal & 0xff00ff00ff00ff00ULL) >> 8) | ((nVal & 0x00ff00ff00ff00ffULL) << 8);
    return ((nVal & 0xffff0000ffff0000ULL) >> 16) | ((nVal & 0x0000ffff0000ffffULL) << 16);
}
#endif
#ifndef USE_LONG_AS_INT64
template <>
inline ULONG cMemT::ReverseType<ULONG>(ULONG nVal) noexcept {  // static
    //! ULONG may be equiv to UINT32 or UINT64
    // return ReverseType<UINT64>(nVal);
    return ReverseType<UINT32>(nVal);
}
#endif

#if 0  // USE_LITTLE_ENDIAN
	template <>
	inline UINT32 cMemT::GetNtoH<UINT32>(const void* pData)	{
		const BYTE* p = (const BYTE*)pData;
		return ((UINT32)p[0] << 24)
			| ((UINT32)p[1] << 16)
			| ((UINT32)p[2] << 8)
			| ((UINT32)p[3]);
	}
	template <>
	inline void cMemT::SetHtoN<UINT32>(void* pData, UINT32 nVal) {
		BYTE* p = (BYTE*)pData;
		p[0] = (BYTE)(nVal >> 24);
		p[1] = (BYTE)(nVal >> 16);
		p[2] = (BYTE)(nVal >> 8);
		p[3] = (BYTE)(nVal);
	}

	template <>
	inline UINT64 cMemT::GetNtoH<UINT64>(const void* pData)	{
		const BYTE* p = (const BYTE*)pData;
		return ((UINT64)p[0] << 56)
			| ((UINT64)p[1] << 48)
			| ((UINT64)p[2] << 40)
			| ((UINT64)p[3] << 32)
			| ((UINT64)p[4] << 24)
			| ((UINT64)p[5] << 16)
			| ((UINT64)p[6] << 8)
			| ((UINT64)p[7]);
	}
	template <>
	inline void cMemT::SetHtoN<UINT64>(void* pData, UINT64 nVal) {
		BYTE* p = (BYTE*)pData;
		p[0] = (BYTE)(nVal >> 56);
		p[1] = (BYTE)(nVal >> 48);
		p[2] = (BYTE)(nVal >> 40);
		p[3] = (BYTE)(nVal >> 32);
		p[4] = (BYTE)(nVal >> 24);
		p[5] = (BYTE)(nVal >> 16);
		p[6] = (BYTE)(nVal >> 8);
		p[7] = (BYTE)(nVal);
	}
#endif
}  // namespace Gray
#endif
