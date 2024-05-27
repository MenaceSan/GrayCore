//! @file cBits.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cBits_H
#define _INC_cBits_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"
#include "cTypes.h"

namespace Gray {
typedef unsigned int BIT_ENUM_t;  /// Enumerate number of bits or address a single bit in some array of bits. WORD?

/// <summary>
/// enumeration of bitwise actions. Take action on a bit.
/// Can also hold a tristate bool. similar to VB TriState ?
/// </summary>
enum class BITOP_t : signed char {
    _Toggle = -1,  /// XOR bit operation to flip bits. Also used for unknown bit state.
    _Clear = 0,    /// AND/NOT bit operation to clear bits.
    _Set = 1,      /// OR bit operation to set bits.
};

#ifdef USE_INT64  // native support for literal HEX constant UINT 64 bit.
#if defined(_MSC_VER) || defined(__WATCOMC__)
#define CUINT64(h, l) 0x##h##l##ui64
#else
#define CUINT64(h, l) 0x##h##l##ULL
#endif
#elif defined(USE_LITTLE_ENDIAN)
#define CUINT64(h, l) \
    { 0x##l, 0x##h }  // for use with cUInt64
#else
#define CUINT64(h, l) \
    { 0x##h, 0x##l }  // for use with cUInt64
#endif

/// <summary>
/// Bit mask type operations of all sorts. on various integral data types. static class.
/// </summary>
struct GRAYCORE_LINK cBits {          // static
    static const BIT_ENUM_t k_8 = 8;  /// represent the 8 bits in a byte. BIT_ENUM_t

    /// <summary>
    /// How many bytes to hold these bits. Round up to next byte.
    /// </summary>
    /// <param name="nBits"></param>
    /// <returns></returns>
    static constexpr size_t GetSizeBytes(BIT_ENUM_t nBits) noexcept {
        return (nBits + 7) / k_8;
    }

    /// <summary>
    /// Create a 1 bit mask of a given TYPE. Overflow/Underflow is just lost.
    /// </summary>
    /// <typeparam name="TYPE">default TYPE = size_t</typeparam>
    /// <param name="nBit">BIT_ENUM_t</param>
    template <typename TYPE>
    static constexpr TYPE Mask1(BIT_ENUM_t nBit) noexcept {
        return CastN(TYPE, 1) << nBit;
    }
    /// <summary>
    /// Create a mask of all bits less than this.
    /// </summary>
    template <typename TYPE>
    static constexpr TYPE MaskLT(BIT_ENUM_t nBitHigh) noexcept {
        return Mask1<TYPE>(nBitHigh) - 1;
    }

    /// <summary>
    /// Does this value just have a single bit set (and no more) ? Is power of 2 ? Count1Bits() == 1
    /// </summary>
    template <typename TYPE>
    static constexpr bool IsMask1(TYPE nVal) noexcept {
        return (nVal != 0) && ((nVal & (nVal - 1)) == 0);
    }

    /// <summary>
    /// Any nMask bits set in nVal? NOT require all bits!
    /// </summary>
    template <typename TYPE = UINT32>
    static constexpr bool HasAny(TYPE nVal, TYPE nMask) noexcept {
        return (nVal & nMask) != 0;
    }

    /// <summary>
    /// Test if a single bit is set.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="nVal"></param>
    /// <param name="nBit"></param>
    /// <returns></returns>
    template <typename TYPE>
    static constexpr bool IsSet(TYPE nVal, BIT_ENUM_t nBit) noexcept {
        return HasAny(nVal, Mask1<TYPE>(nBit));
    }
    template <typename TYPE>
    static constexpr bool IsClear(TYPE nVal, BIT_ENUM_t nBit) noexcept {
        return !IsSet(nVal, nBit);  // Test if a bit is NOT set.
    }
    template <typename TYPE>
    static constexpr TYPE SetBit(TYPE nVal, BIT_ENUM_t nBit) noexcept {
        return CastN(TYPE, nVal | Mask1<TYPE>(nBit));
    }
    template <typename TYPE>
    static constexpr TYPE ClearBit(TYPE nVal, BIT_ENUM_t nBit) noexcept {
        return CastN(TYPE, nVal & ~Mask1<TYPE>(nBit));
    }

    /// <summary>
    /// Cast from enum TYPE to UNDER_t
    /// </summary>
    template <typename UNDER_t = UINT, typename TYPE>
    static constexpr bool HasAnyT(TYPE nVal, TYPE nMask) noexcept {
        return HasAny<UNDER_t>(static_cast<UNDER_t>(nVal), static_cast<UNDER_t>(nMask));
    }
#if 0
    template <typename TYPE, typename UNDER_t>
    static constexpr bool IsSet(TYPE nVal, BIT_ENUM_t nBit) noexcept {
        return IsSet(static_cast<UNDER_t>(nVal), nBit);
    }
    template <typename TYPE, typename UNDER_t>
    static constexpr bool IsClear(TYPE nVal, BIT_ENUM_t nBit) noexcept {
        return !IsSet(static_cast<UNDER_t>(nVal), nBit);  // Test if a bit is NOT set.
    }
    template <typename TYPE, typename UNDER_t>
    static constexpr TYPE SetBit(TYPE nVal, BIT_ENUM_t nBit) noexcept {
        return CastN(TYPE, static_cast<UNDER_t>(nVal) | Mask1<UNDER_t>(nBit));
    }
    template <typename TYPE, typename UNDER_t>
    static constexpr TYPE ClearBit(TYPE nVal, BIT_ENUM_t nBit) noexcept {
        return CastN(TYPE, static_cast<UNDER_t>(nVal) & ~Mask1<UNDER_t>(nBit));
    }
#endif

    /// <summary>
    /// Or/Set mask of bits.
    /// If we are performing some operator on enums that are bitmasks. Cast back to enum
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="nValMask"></param>
    /// <returns></returns>
    template <typename UNDER_t = UINT, typename TYPE>
    static inline TYPE SetMask(TYPE nVal, TYPE nOrMask) {
        return CastN(TYPE, static_cast<UNDER_t>(nVal) | static_cast<UNDER_t>(nOrMask));
    }

    /// <summary>
    /// What is the highest set bit +1 in this primitive TYPE. 1 based. MSB.
    /// e.g. nVal=8 return=4
    /// similar to ffs() (POSIX) or __builtin_clz(), __builtin_clzll (__GNUC__) ?
    /// similar to _BitScanReverse, _BitScanReverse64. https://msdn.microsoft.com/en-us/library/fbxyd7zd.aspx
    /// x86 has BSR instruction. PowerPC there's a similar cntlz ("count leading zeros") instruction.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="nVal"></param>
    /// <returns>1 for value of 1. 0 = no bits.</returns>
    template <typename TYPE>
    static inline BIT_ENUM_t Highest1Bit(TYPE nVal) noexcept {
        BIT_ENUM_t nBit1 = 0;
        while (nVal != 0) {
            nVal >>= 1;
            nBit1++;
        }
        return nBit1;
    }

    /// <summary>
    /// Get lowest 1 bit. AKA Get alignment.
    /// similar to _BitScanForward, _BitScanForward64
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="nVal"></param>
    /// <returns></returns>
    template <typename TYPE>
    static inline BIT_ENUM_t Lowest1Bit(TYPE nVal) noexcept {
        BIT_ENUM_t nBit1 = 1;
        for (;;) {
            if (HasAny<TYPE>(nVal, 1)) return nBit1;
            nVal >>= 1;
            nBit1++;
        }
        return 0;  // no bits set.
    }

    /// <summary>
    /// Count total number of 1 bits.
    /// like: __builtin_popcount()
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="nVal"></param>
    /// <returns></returns>
    template <typename TYPE>
    static inline BIT_ENUM_t Count1Bits(TYPE nVal) noexcept {
        BIT_ENUM_t nBits = 0;  // accumulates the total bits set in nVal
        for (; nVal; nBits++) {
            nVal &= nVal - 1;  // clear the least significant bit set
        }
        return nBits;
    }

    /// <summary>
    /// Rotate bits left. increase value.
    /// If system doesn't have an inline rotate left function for X bits.
    /// </summary>
    template <typename TYPE>
    static inline TYPE Rotl(TYPE nVal, BIT_ENUM_t nBits) noexcept {
        return (nVal << nBits) | (nVal >> ((sizeof(nVal) * k_8) - nBits));
    }

    /// <summary>
    /// Rotate bits right. lower value.
    /// </summary>
    template <typename TYPE>
    static inline TYPE Rotr(TYPE nVal, BIT_ENUM_t nBits) noexcept {
        return (nVal >> nBits) | (nVal << ((sizeof(nVal) * k_8) - nBits));
    }

    /// <summary>
    /// Reverse the order of the bits. ASSUME not signed?
    /// </summary>
    template <typename TYPE>
    static inline TYPE Reverse(TYPE nVal) noexcept {
        TYPE nTemp = nVal;  // nTemp will have the reversed bits of nVal.
        for (size_t i = (sizeof(nVal) * k_8 - 1); i > 0; i--) {
            nTemp |= (nVal & 1);
            nTemp <<= 1;
            nVal >>= 1;
        }
        return nTemp | (nVal & 1);
    }

#if 0
    template <typename TYPE>
	static inline TYPE Op(TYPE nVal, BITOP_t eBitOp, TYPE nValMask = 1) {
		// TODO or,and,xor/not // 
	}
#endif
};

// Override implementations of templates.

#if defined(__GNUC__)
template <>
inline BIT_ENUM_t cBits::Count1Bits<UINT32>(UINT32 nVal) noexcept {  // static
    return ::__builtin_popcount(nVal);
}
template <>
inline BIT_ENUM_t cBits::Highest1Bit<UINT32>(UINT32 nVal) noexcept {  // static
    if (nVal == 0) return 0;
    return 32 - __builtin_clz(nVal);  // Use intrinsic function
}
template <>
inline BIT_ENUM_t cBits::Lowest1Bit<UINT32>(UINT32 nVal) noexcept {  // static
    if (nVal == 0) return 0;
    return __builtin_ctz(nVal);  // Use intrinsic function
}

#if defined(USE_INT64)
template <>
inline BIT_ENUM_t cBits::Count1Bits<UINT64>(UINT64 nVal) noexcept {  // static
    return ::__builtin_popcountll(nVal);
}
template <>
inline BIT_ENUM_t cBits::Highest1Bit<UINT64>(UINT64 nVal) noexcept {  // static
    if (nVal == 0) return 0;
    return 64 - __builtin_clzll(nVal);  // Use intrinsic function
}
template <>
inline BIT_ENUM_t cBits::Lowest1Bit<UINT64>(UINT64 nVal) noexcept {  // static
    if (nVal == 0) return 0;
    return __builtin_ctzll(nVal);  // Use intrinsic function
}
#endif

#else

template <>
constexpr BIT_ENUM_t cBits::Count1Bits<UINT32>(UINT32 nVal) noexcept {
    //! A math trick for counting 1 bits in 32 bit numbers.
    nVal = (nVal & 0x55555555) + ((nVal & 0xAAAAAAAA) >> 1);
    nVal = (nVal & 0x33333333) + ((nVal & 0xCCCCCCCC) >> 2);
    nVal = (nVal + (nVal >> 4)) & 0x0F0F0F0F;
    nVal = (nVal + (nVal >> 16));
    nVal = (nVal + (nVal >> 8)) & 0x3F;
    return CastN(BIT_ENUM_t, nVal);
}

#endif

#ifdef _MSC_VER
template <>
inline UINT32 cBits::Rotl<UINT32>(UINT32 nVal, BIT_ENUM_t nBits) noexcept {
    return ::_rotl(nVal, CastN(int, nBits));  /// use the _WIN32 intrinsic _rotl function.
}
template <>
inline UINT32 cBits::Rotr<UINT32>(UINT32 nVal, BIT_ENUM_t nBits) noexcept {
    return ::_rotr(nVal, CastN(int, nBits));  /// use the _WIN32 intrinsic _rotr function.
}

#if !defined(_MANAGED)
template <>
inline BIT_ENUM_t cBits::Highest1Bit<UINT32>(UINT32 nVal) noexcept {
    DWORD nRet;
    if (::_BitScanReverse(&nRet, nVal))  // Use intrinsic function
        return CastN(BIT_ENUM_t, nRet + 1);
    return 0;
}
template <>
inline BIT_ENUM_t cBits::Lowest1Bit<UINT32>(UINT32 nVal) noexcept {
    DWORD nRet;
    if (::_BitScanForward(&nRet, nVal))  // Use intrinsic function
        return CastN(BIT_ENUM_t, nRet + 1);
    return 0;
}
#endif

#if defined(USE_INT64) && !defined(UNDER_CE) && defined(_MSC_VER)  // _INTEGRAL_MAX_BITS >= 64
#ifdef USE_64BIT
template <>
inline BIT_ENUM_t cBits::Highest1Bit<UINT64>(UINT64 nVal) noexcept {
    DWORD nRet;
    if (::_BitScanReverse64(&nRet, nVal))  // Use intrinsic function
        return CastN(BIT_ENUM_t, nRet + 1);
    return 0;
}
template <>
inline BIT_ENUM_t cBits::Lowest1Bit<UINT64>(UINT64 nVal) noexcept {
    DWORD nRet;
    if (::_BitScanForward64(&nRet, nVal))  // Use intrinsic function
        return CastN(BIT_ENUM_t, nRet + 1);
    return 0;
}
#endif
template <>
inline UINT64 cBits::Rotl<UINT64>(UINT64 nVal, BIT_ENUM_t nBits) noexcept {
    return ::_rotl64(nVal, CastN(int, nBits));  /// use the _WIN32 intrinsic _rotl function.
}
template <>
inline UINT64 cBits::Rotr<UINT64>(UINT64 nVal, BIT_ENUM_t nBits) noexcept {
    return ::_rotr64(nVal, CastN(int, nBits));  /// use the _WIN32 intrinsic _rotr function.
}
#endif  // USE_INT64
#endif  // _MSC_VER

template <>
constexpr BYTE cBits::Reverse<BYTE>(BYTE nVal) noexcept {
    //! Reverse the order of the 8 bits. using 32 or 64 bit temporary.
    //! http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith32Bits
#ifdef USE_INT64
    return CastN(BYTE, ((nVal * CUINT64(2, 02020202)) & CUINT64(0108, 84422010)) % 1023);
#else
    return CastN(BYTE, (((nVal * 0x0802LU & 0x22110LU) | (nVal * 0x8020LU & 0x88440LU)) * 0x10101LU) >> 16);
#endif
}
template <>
inline UINT32 cBits::Reverse<UINT32>(UINT32 nVal) noexcept {
    //! Reverse the order of the 32 bits.
    nVal = (((nVal & 0xaaaaaaaa) >> 1) | ((nVal & 0x55555555) << 1));
    nVal = (((nVal & 0xcccccccc) >> 2) | ((nVal & 0x33333333) << 2));
    nVal = (((nVal & 0xf0f0f0f0) >> 4) | ((nVal & 0x0f0f0f0f) << 4));
    nVal = (((nVal & 0xff00ff00) >> 8) | ((nVal & 0x00ff00ff) << 8));
    return (nVal >> 16) | (nVal << 16);
}

template <>
inline ULONG cBits::Reverse<ULONG>(ULONG nVal) noexcept {  // static
    //! ULONG may be equiv to UINT32 or UINT64
#ifdef USE_LONG_AS_INT64
    return Reverse<UINT64>(nVal);
#else
    return Reverse<UINT32>(nVal);
#endif
}

/// <summary>
/// hold a mask of max MASK_t size bits. MASK_t or BIT1_t might be an enum!
/// like: std::bitset.
/// </summary>
/// <typeparam name="MASK_t"></typeparam>
template <typename MASK_t = UINT32, typename UNDER_t = MASK_t, typename BIT1_t = BIT_ENUM_t>
class cBitmask {
    UNDER_t _nMask;  /// mask of bits. "underlying" type.

 public:
    static constexpr MASK_t k_MASK_ALL = CastN(MASK_t, cTypeLimit<UNDER_t>::Max());  /// all bits set.

    cBitmask(MASK_t nMask = {}) noexcept : _nMask(static_cast<UNDER_t>(nMask)) {}
    MASK_t get_Mask() const noexcept {
        return CastN(MASK_t, _nMask);
    }
    operator MASK_t() const noexcept {
        return CastN(MASK_t, _nMask);
    }

    /// <summary>
    /// TODO Can we hold this bit?
    /// </summary>
    // static constexpr bool IsValidBit(BIT1_t i) noexcept { return IS_INDEX_GOOD(i, k_MASK_ALL); }

    inline bool IsSet(BIT1_t nBit) const noexcept {
        return cBits::IsSet(_nMask, nBit);
    }
    void SetBit(BIT1_t nBit) noexcept {
        _nMask = cBits::SetBit(_nMask, nBit);
    }
    void ClearBit(BIT1_t nBit) noexcept {
        _nMask = cBits::ClearBit(_nMask, nBit);
    }

    inline bool HasAny(MASK_t nBits) const noexcept {
        return cBits::HasAny(_nMask, CastN(UNDER_t, nBits));
    }
    /// <summary>
    /// Equiv of x |= y.
    /// </summary>
    void SetMask(MASK_t nBits) noexcept {
        _nMask = _nMask | static_cast<UNDER_t>(nBits);
    }
    void ClearMask(MASK_t nBits) noexcept {
        _nMask = _nMask & ~static_cast<UNDER_t>(nBits);
    }
    void ClearMask() noexcept {
        _nMask = 0;
    }
    void SetAll() noexcept {
        _nMask = k_MASK_ALL;
    }
};
}  // namespace Gray
#endif  // _INC_cBits_H
