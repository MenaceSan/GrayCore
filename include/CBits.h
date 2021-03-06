//
//! @file cBits.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cBits_H
#define _INC_cBits_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray
{
	typedef unsigned short BIT_SIZE_t;	//!< number of bits in some intrinsic type. <= 256 ?
	typedef unsigned int BIT_ENUM_t;	//!< Enumerate number of bits or address a single bit in some array of bits.
	enum BITOP_TYPE
	{
		//! @enum Gray::BITOP_TYPE
		//! Take action on a bit.
		//! Hold a tristate bool. similar to VB TriState ?
		BITOP_TOGGLE = -1,	//!< XOR bit operation to flip bits. Also used for unknown bit state.
		BITOP_CLR = 0,		//!< AND/NOT bit operation to clear bits.
		BITOP_SET = 1,		//!< OR bit operation to set bits.
	};

#ifndef _1BITMASK
#define _1BITMASK(b) 	(((size_t)1)<<(b))	//!< default bitmask type = size_t. Use cBits::Mask1() for other type.
#endif

#ifdef USE_INT64		// native support for literal HEX constant UINT 64 bit.
#if defined(_MSC_VER) || defined(__WATCOMC__) 
#define CUINT64(h,l)	0x##h##l##ui64
#else
#define CUINT64(h,l)	0x##h##l##ULL
#endif
#elif defined(USE_LITTLE_ENDIAN)
#define CUINT64(h,l)	{ 0x##l, 0x##h }		// for use with cUInt64
#else
#define CUINT64(h,l)	{ 0x##h, 0x##l }		// for use with cUInt64
#endif

	struct GRAYCORE_LINK cBits	// static
	{
		//! @struct Gray::cBits
		//! Bit mask type operations of all sorts. on various integral data types.

		static const BYTE k_8 = 8;	//!< represent the 8 bits in a byte. BIT_ENUM_t

		static constexpr size_t GetSizeBytes(BIT_ENUM_t nBits) noexcept
		{
			//! How many bytes to hold these bits. Round up to next byte.
#define GETSIZEBYTES(nBits) (((nBits)+7)/8)		// equiv for use in const
			return (nBits + 7) / k_8;
		}

		template <typename TYPE>
		static constexpr TYPE Mask1(BIT_ENUM_t nBit) noexcept
		{
			//! Create a 1 bit mask of a given TYPE. Overflow/Underflow is just lost.
			//! _1BITMASK(nBit) = cBits::Mask1<size_t>(nBit).
			//! default TYPE = size_t
			return ((TYPE)1) << nBit;
		}
		template <typename TYPE>
		static constexpr bool IsMask1(TYPE nVal) noexcept
		{
			//! Does this just have a single bit on ? Is power of 2 ?
			return (nVal != 0) && ((nVal & (nVal - 1)) == 0) ;
		}

		template <typename TYPE>
		static constexpr bool IsSet(TYPE nVal, BIT_ENUM_t nBit) noexcept
		{
			//! Test if a bit is set.
			return (nVal & Mask1<TYPE>(nBit)) ? true : false ;
		}
		template <typename TYPE>
		static constexpr bool IsClear(TYPE nVal, BIT_ENUM_t nBit) noexcept
		{
			//! Test if a bit is NOT set.
			return (nVal & Mask1<TYPE>(nBit)) ? false : true ;
		}

		template <typename TYPE>
		static constexpr TYPE SetBit(TYPE nVal, BIT_ENUM_t nBit) noexcept
		{
			return nVal | Mask1<TYPE>(nBit);
		}
		template <typename TYPE>
		static constexpr TYPE ClearBit(TYPE nVal, BIT_ENUM_t nBit) noexcept
		{
			return nVal & ~Mask1<TYPE>(nBit);
		}

		template <typename TYPE>
		static constexpr bool HasMask(TYPE nVal, TYPE nMask) noexcept
		{
			//! Any nMask bits set ?
			return (nVal & nMask) != 0;
		}

		template <typename TYPE>
		static inline BIT_ENUM_t Highest1Bit(TYPE nMask) noexcept
		{
			//! What is the highest set bit in this primitive TYPE. 1 based. MSB.
			//! @return 1 for value of 1. 0 = no bits.
			//! e.g. nMask=8 return=4
			//! similar to ffs() (POSIX) or __builtin_clz(), __builtin_clzll (__GNUC__) ? 
			//! similar to _BitScanReverse, _BitScanReverse64. https://msdn.microsoft.com/en-us/library/fbxyd7zd.aspx
			//! x86 has BSR instruction. PowerPC there's a similar cntlz ("count leading zeros") instruction.

			BIT_ENUM_t nBitsHighest = 0;
			while (nMask != 0)
			{
				nMask >>= 1;
				nBitsHighest++;
			}
			return nBitsHighest;
		}

		template <typename TYPE>
		static inline BIT_ENUM_t Count1Bits(TYPE nMask) noexcept
		{
			//! Count total number of 1 bits.
			//! like: __builtin_popcount()

			BIT_ENUM_t nBits = 0; // accumulates the total bits set in nVal
			for (; nMask; nBits++)
			{
				nMask &= nMask - 1; // clear the least significant bit set
			}
			return nBits;
		}

		template <typename TYPE>
		static inline TYPE Rotl(TYPE nVal, BIT_ENUM_t nBits) noexcept
		{
			//! Rotate bits left.
			//! If system doesn't have an inline rotate left function for X bits.
			return (nVal << nBits) | (nVal >> ((sizeof(nVal) * k_8) - nBits));
		}
		template <typename TYPE>
		static inline TYPE Rotr(TYPE nVal, BIT_ENUM_t nBits) noexcept
		{
			//! Rotate bits right.
			return (nVal >> nBits) | (nVal << ((sizeof(nVal) * k_8) - nBits));
		}

		template <typename TYPE>
		static inline TYPE Reverse(TYPE nVal) noexcept
		{
			//! Reverse the order of the bits. ASSUME not signed?

			TYPE nTemp = nVal; // nTemp will have the reversed bits of nVal.
			for (size_t i = (sizeof(nVal) * k_8 - 1); i > 0; i--)
			{
				nTemp |= (nVal & 1);
				nTemp <<= 1;
				nVal >>= 1;
			}
			return nTemp | (nVal & 1);
		}

#if 0
		template <typename TYPE>
		static inline TYPE Op(TYPE nVal, BITOP_TYPE eBitOp, TYPE nValMask = 1)
		{
			// TODO or,and,xor/not // 
		}
#endif
	};

	// Override implementations of templates.

#if defined(__GNUC__)
	template <>
	inline BIT_ENUM_t cBits::Count1Bits<UINT32>(UINT32 nMask) // static
	{
		return ::__builtin_popcount(nMask);
	}
	template <>
	inline BIT_ENUM_t cBits::Highest1Bit<UINT32>(UINT32 nMask) // static
	{
		// Use intrinsic function
		if (nMask == 0)
			return 0;
		return 32 - __builtin_clz(nMask);
	}

#if defined(USE_INT64)
	template <>
	inline BIT_ENUM_t cBits::Count1Bits<UINT64>(UINT64 nMask) // static
	{
		return ::__builtin_popcountll(nMask);
	}
	template <>
	inline BIT_ENUM_t cBits::Highest1Bit<UINT64>(UINT64 nMask) // static
	{
		// Use intrinsic function
		if (nMask == 0)
			return 0;
		return 64 - __builtin_clzll(nMask);
	}
#endif

#else

	template <>
	inline BIT_ENUM_t cBits::Count1Bits<UINT32>(UINT32 nVal) noexcept
	{
		//! A math trick for counting 1 bits in 32 bit numbers.
		nVal = (nVal & 0x55555555) + ((nVal & 0xAAAAAAAA) >> 1);
		nVal = (nVal & 0x33333333) + ((nVal & 0xCCCCCCCC) >> 2);
		nVal = (nVal + (nVal >> 4)) & 0x0F0F0F0F;
		nVal = (nVal + (nVal >> 16));
		nVal = (nVal + (nVal >> 8)) & 0x3F;
		return (BIT_ENUM_t)nVal;
	}

#endif

#ifdef _MSC_VER
	template <>
	inline UINT32 cBits::Rotl<UINT32>(UINT32 nVal, BIT_ENUM_t nBits) noexcept
	{
		return ::_rotl(nVal, (int)nBits);	//!< use the _WIN32 intrinsic _rotl function.
	}
	template <>
	inline UINT32 cBits::Rotr<UINT32>(UINT32 nVal, BIT_ENUM_t nBits) noexcept
	{
		return ::_rotr(nVal, (int)nBits);	//!< use the _WIN32 intrinsic _rotr function.
	}

#if !defined(_MANAGED)
	template <>
	inline BIT_ENUM_t cBits::Highest1Bit<UINT32>(UINT32 nMask) noexcept
	{
		// Use intrinsic function
		DWORD nRet;
		if (::_BitScanReverse(&nRet, nMask))
			return (BIT_ENUM_t)(nRet + 1);
		return 0;
	}
#endif

#if defined(USE_INT64) && ! defined(UNDER_CE) && defined(_MSC_VER)	// _INTEGRAL_MAX_BITS >= 64
#ifdef USE_64BIT
	template <>
	inline BIT_ENUM_t cBits::Highest1Bit<UINT64>(UINT64 nMask) noexcept
	{
		// Use intrinsic function
		DWORD nRet;
		if (::_BitScanReverse64(&nRet, nMask))
			return (BIT_ENUM_t)(nRet + 1);
		return 0;
	}
#endif
	template <>
	inline UINT64 cBits::Rotl<UINT64>(UINT64 nVal, BIT_ENUM_t nBits) noexcept
	{
		return ::_rotl64(nVal, (int)nBits);	//!< use the _WIN32 intrinsic _rotl function.
	}
	template <>
	inline UINT64 cBits::Rotr<UINT64>(UINT64 nVal, BIT_ENUM_t nBits) noexcept
	{
		return ::_rotr64(nVal, (int)nBits);	//!< use the _WIN32 intrinsic _rotr function.
	}
#endif	// USE_INT64
#endif	// _MSC_VER

	template <>
	inline BYTE cBits::Reverse<BYTE>(BYTE nVal) noexcept
	{
		//! Reverse the order of the 8 bits. using 32 or 64 bit temporary.
		//! http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith32Bits
#ifdef USE_INT64
		return (BYTE)(((nVal * CUINT64(2, 02020202)) & CUINT64(0108, 84422010)) % 1023);
#else
		return (BYTE)((((nVal * 0x0802LU & 0x22110LU) | (nVal * 0x8020LU & 0x88440LU)) * 0x10101LU) >> 16);
#endif
	}
	template <>
	inline UINT32 cBits::Reverse<UINT32>(UINT32 nVal) noexcept
	{
		//! Reverse the order of the 32 bits.
		nVal = (((nVal & 0xaaaaaaaa) >> 1) | ((nVal & 0x55555555) << 1));
		nVal = (((nVal & 0xcccccccc) >> 2) | ((nVal & 0x33333333) << 2));
		nVal = (((nVal & 0xf0f0f0f0) >> 4) | ((nVal & 0x0f0f0f0f) << 4));
		nVal = (((nVal & 0xff00ff00) >> 8) | ((nVal & 0x00ff00ff) << 8));
		return((nVal >> 16) | (nVal << 16));
	}

	template <>
	inline ULONG cBits::Reverse<ULONG>(ULONG nVal) noexcept // static
	{
		//! ULONG may be equiv to UINT32 or UINT64
#ifdef USE_LONG_AS_INT64
		return Reverse<UINT64>(nVal);
#else
		return Reverse<UINT32>(nVal);
#endif
	}

	template <typename TYPE = UINT32>
	class GRAYCORE_LINK cBitmask
	{
		//! @class Gray::cBitmask
		//! hold a mask of max TYPE size bits.
	protected:
		TYPE m_uVal;
	public:
		cBitmask(TYPE uVal = 0) noexcept : m_uVal(uVal)
		{
		}
		void SetBit(BIT_ENUM_t nBit) noexcept
		{
			m_uVal = cBits::SetBit(m_uVal, nBit);
		}
		void ClearBit(BIT_ENUM_t nBit) noexcept
		{
			m_uVal = cBits::ClearBit(m_uVal, nBit);
		}
		bool IsSet(BIT_ENUM_t nBit) const noexcept
		{
			return cBits::IsSet(m_uVal, nBit);
		}
		operator TYPE () const noexcept
		{
			return m_uVal;
		}
	};
}

#endif	// _INC_cBits_H
