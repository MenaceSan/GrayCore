//
//! @file cFloatDeco.h
//! convert numbers to/from string.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_cFloatDeco_H
#define _INC_cFloatDeco_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cFloat.h"
#include "StrConst.h"
#include "cDebugAssert.h"	// ASSERT

namespace Gray
{
	// _umul128 TODO MulDiv ?

	class GRAYCORE_LINK cFloatDeco
	{
		//! @class Gray::cFloatDeco
		//! holds a decomposed double/float value. ignore sign.
		//! Support class for conversion of double/float to string. Used with cFloat64

	public:
		static const double k_powersOf10[9];	//!< Table giving binary powers of 10

		static const UINT32 k_Exp10[10];		//!< Table of decimal digits to fit in 32 bit space.

		static const UINT64 k_MANT_MASK_X = CUINT64(00100000, 00000000);	// Extra hidden bit. k_MANT_MASK+1

		UINT64 m_uMant;		//!< Hold Mantissa.
		int m_iExp2;		//!< Hold base 2 Biased Exponent

	public:
		cFloatDeco() noexcept
			: m_uMant(0), m_iExp2(0)
		{}

		cFloatDeco(UINT64 uMan, int iExp2) noexcept
			: m_uMant(uMan), m_iExp2(iExp2)
		{}

		cFloatDeco(double d) noexcept
		{
			//! Decompose d.
			static const int k_DpExponentBias = 0x3FF + cFloat64::k_MANT_BITS;

			cFloat64 u(d);

			const int iExpBiased = (u.m_v.u_qw & cFloat64::k_EXP_MASK) >> cFloat64::k_MANT_BITS;
			const UINT64 nMantSig = u.get_Mantissa();
			if (iExpBiased != 0)
			{
				m_uMant = nMantSig + k_MANT_MASK_X;
				m_iExp2 = iExpBiased - k_DpExponentBias;
			}
			else
			{
				m_uMant = nMantSig;
				m_iExp2 = 1 - k_DpExponentBias;
			}
		}

		cFloatDeco operator-(const cFloatDeco& rhs) const
		{
			//! Do math on decomposed number.
			//! ASSUME same m_iExp2
			ASSERT(m_iExp2 == rhs.m_iExp2);
			ASSERT(m_uMant >= rhs.m_uMant);
			return cFloatDeco(m_uMant - rhs.m_uMant, m_iExp2);
		}

		cFloatDeco operator*(const cFloatDeco& rhs) const
		{
			//! Do math on decomposed numbers.

			UINT64 h;
#if defined(_MSC_VER) && defined(_M_AMD64)
			UINT64 l = _umul128(m_uMant, rhs.m_uMant, &h);
			if (l & (UINT64(1) << 63)) // rounding
				h++;
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && defined(__x86_64__)
			unsigned __int128 p = static_cast<unsigned __int128>(m_uMant) * static_cast<unsigned __int128>(rhs.m_uMant);
			h = p >> 64;
			UINT64 l = static_cast<UINT64>(p);
			if (l & (UINT64(1) << 63)) // rounding
				h++;
#else
			const UINT64 M32 = 0xFFFFFFFF;
			const UINT64 a = m_uMant >> 32;
			const UINT64 b = m_uMant & M32;
			const UINT64 c = rhs.m_uMant >> 32;
			const UINT64 d = rhs.m_uMant & M32;
			const UINT64 ac = a * c;
			const UINT64 bc = b * c;
			const UINT64 ad = a * d;
			const UINT64 bd = b * d;
			UINT64 tmp = (bd >> 32) + (ad & M32) + (bc & M32);
			tmp += 1U << 31;  /// mult_round
			h = ac + (ad >> 32) + (bc >> 32) + (tmp >> 32);
#endif
			return cFloatDeco(h, m_iExp2 + rhs.m_iExp2 + 64);
		}

		cFloatDeco Normalize() const
		{
			//! Fix m_iExp2 by making m_uMant as large as possible.
			//! cBits::Highest1Bit<>
			ASSERT(m_uMant != 0);
			BIT_ENUM_t nBit = 64 - cBits::Highest1Bit(m_uMant);
			return cFloatDeco(m_uMant << nBit, m_iExp2 - nBit);
		}

		void NormalizedBoundaries(cFloatDeco* minus, cFloatDeco* plus) const
		{
			cFloatDeco pl = cFloatDeco((m_uMant << 1) + 1, m_iExp2 - 1).Normalize();
			cFloatDeco mi = (m_uMant == k_MANT_MASK_X) ? cFloatDeco((m_uMant << 2) - 1, m_iExp2 - 2) : cFloatDeco((m_uMant << 1) - 1, m_iExp2 - 1);
			mi.m_uMant <<= mi.m_iExp2 - pl.m_iExp2;
			mi.m_iExp2 = pl.m_iExp2;
			*plus = pl;
			*minus = mi;
		}

#if 0
		double get_Double() const
		{
			//! re-compose double. Convert back to a double. for testing.
			cFloat64 f(sdfsdf);
			return f.m_v.u_d;
		}
#endif

		static inline unsigned GetCountDecimalDigit32(UINT32 n) noexcept
		{
			//! How many decimal digits?
			//! k_Exp10
			//! Simple C++ implementation is faster than Highest1Bit/__builtin_clz version in this case.
			if (n < 10) return 1;
			if (n < 100) return 2;
			if (n < 1000) return 3;
			if (n < 10000) return 4;
			if (n < 100000) return 5;
			if (n < 1000000) return 6;
			if (n < 10000000) return 7;
			if (n < 100000000) return 8;
			if (n < 1000000000) return 9;
			return 10;
		}

		static cFloatDeco GRAYCALL GetCachedPower(int nExp2, OUT int* pnExp10);
		static double GRAYCALL toDouble(UINT32 frac1, UINT32 frac2, int nExp10);

		static void GRAYCALL GrisuRound(char* pszOut, StrLen_t len, UINT64 delta, UINT64 rest, UINT64 ten_kappa, UINT64 wp_w);
		static StrLen_t GRAYCALL Grisu2(double dVal, char* pszOut, OUT int* pnExp10);

		static StrLen_t GRAYCALL MantRound(char* pszOut, StrLen_t nMantLength);
		static StrLen_t GRAYCALL MantAdjust(char* pszOut, StrLen_t nMantLength, StrLen_t nMantLengthNew);

		static StrLen_t GRAYCALL FormatE(char* pszOut, StrLen_t nMantLength, int nExp10, char chE);
		static StrLen_t GRAYCALL FormatF(char* pszOut, StrLen_t nMantLength, int nExp10, int iDecPlacesWanted);
	};
}

#endif
