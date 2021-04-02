//
//! @file cFloat.h
//! info for float/double types.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cFloat_H
#define _INC_cFloat_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cTypes.h"
#include "cBits.h"
#include "cMem.h"
#include "cDebugAssert.h"

namespace Gray
{
	class GRAYCORE_LINK cFloat32
	{
		//! @class Gray::cFloat32
		//! pack/unpack IEEE float32/float http://en.wikipedia.org/wiki/IEEE_754-1985
		//! Use FLT_MAX
	public:
		cUnion32 m_v; //!< holds the float32.

		static const UINT32 k_uOne = 0x3f800000;	//!< (UINT32) = float 1.0f (8 bit exponent)
		static const UINT32 k_uTwo = 0x40000000;	//!< (UINT32) = float 2.0f

		static const UINT32 k_SIGN_MASK = 0x80000000;	//!< 1 bit = value sign (Sign_bit)

		static const UINT32 k_EXP_MASK = 0x7f800000;	//!< 8 bits = signed exponent (base 2)

		static const UINT32 k_MANT_MASK = 0x007fffff;	//!< 23 bits = fractional mantissa = FLT_MANT_DIG
		static const UINT32 k_MANT_BITS = 23;			//!< 23 bits = fractional mantissa = FLT_MANT_DIG

	public:
		cFloat32() noexcept
		{
			// undefined.
		}
		cFloat32(float f) noexcept
		{
			m_v.u_f = f;
		}

		void put_Bits(UINT32 dw) noexcept
		{
			m_v.u_dw = dw;
		}

		static inline UINT32 toBits(float src) noexcept
		{
			//! reinterpret a float as an UINT32 assuming IEEE 32
			//! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
			UINT32 dst = 0;
			STATIC_ASSERT(sizeof(src) == sizeof(dst), toBits);
			cMem::Copy(&dst, &src, sizeof(dst));
			return dst;
		}
		static inline float fromBits(UINT32 src) noexcept
		{
			//! reinterpret an UINT32 as a float assuming IEEE 32
			//! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
			float dst = 0;
			STATIC_ASSERT(sizeof(src) == sizeof(dst), fromBits);
			cMem::Copy(&dst, &src, sizeof(dst));
			return dst;
		}

		// bool get_Negative() const
		// short get_Exponent() const
		UINT32 get_Mantissa() const noexcept
		{
			return (m_v.u_dw & k_MANT_MASK);
		}
	};

	class GRAYCORE_LINK cFloat64
	{
		//! @class Gray::cFloat64
		//! pack/unpack IEEE float64/double http://en.wikipedia.org/wiki/IEEE_754-1985
		//! Use DBL_MAX

	public:
		cUnion64 m_v;	//!< holds the float64.

		static const UINT64 k_SIGN_MASK = CUINT64(80000000, 00000000);		//!< 1 bit = value sign (Sign_bit)

		static const UINT64 k_EXP_MASK = CUINT64(7FF00000, 00000000);		//!< 11 bits = signed exponent (base 2)

		static const UINT64 k_MANT_MASK = CUINT64(000FFFFF, FFFFFFFF);		//!< 52 bits = fractional mantissa 
		static const UINT32 k_MANT_BITS = 52;								//!< 52 bits = fractional mantissa

	public:
		cFloat64() noexcept
		{
			// undefined value.
		}
		cFloat64(double d) noexcept
		{
			m_v.u_d = d;
		}

		void put_Bits(UINT64 qw) noexcept
		{
			m_v.u_qw = qw;
		}

		static inline UINT64 toBits(double src) noexcept
		{
			//! reinterpret a double as an UINT64
			//! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
			UINT64 dst;
			STATIC_ASSERT(sizeof(src) == sizeof(dst), toBits);
			cMem::Copy(&dst, &src, sizeof(dst));
			return dst;
		}
		static inline double fromBits(UINT64 src) noexcept
		{
			//! reinterpret an UINT64 as a double
			//! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
			double dst;
			STATIC_ASSERT(sizeof(src) == sizeof(dst), fromBits);
			cMem::Copy(&dst, &src, sizeof(dst));
			return dst;
		}

#if 0
		// bool get_Negative() const
		short get_Exponent() const noexcept
		{
			// TODO FIX SIGN
			return (short)((m_v.u_qw & k_EXP_MASK) >> sdf);
		}
#endif
		UINT64 get_Mantissa() const noexcept
		{
			return (m_v.u_qw & k_MANT_MASK);
		}
	};

	// class cFloat80 // long double or "double double" NOT in M$?
}

#endif // _INC_cFloat_H
