//
//! @file CFloat.h
//! info for float/double types.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CFloat_H
#define _INC_CFloat_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CTypes.h"
#include "CBits.h"
#include "CUnitTestDecl.h"
#include "CDebugAssert.h"

UNITTEST_PREDEF(CFloat32)

namespace Gray
{
	class GRAYCORE_LINK CFloat32
	{
		//! @class Gray::CFloat32
		//! pack/unpack IEEE float32/float http://en.wikipedia.org/wiki/IEEE_754-1985
		//! Use FLT_MAX
	public:
		CUnion32 m_v; //!< holds the float32.

		static const UINT32 k_uOne = 0x3f800000;	//!< (UINT32) = float 1.0f (8 bit exponent)
		static const UINT32 k_uTwo = 0x40000000;	//!< (UINT32) = float 2.0f

		static const UINT32 k_SIGN_MASK = 0x80000000;	//!< 1 bit = value sign (Sign_bit)

		static const UINT32 k_EXP_MASK = 0x7f800000;	//!< 8 bits = signed exponent (base 2)

		static const UINT32 k_MANT_MASK = 0x007fffff;	//!< 23 bits = fractional mantissa = FLT_MANT_DIG
		static const UINT32 k_MANT_BITS = 23;			//!< 23 bits = fractional mantissa = FLT_MANT_DIG

	public:
		CFloat32()
		{
			// undefined.
		}
		CFloat32(float f)
		{
			m_v.u_f = f;
		}

		void put_Bits(UINT32 dw)
		{
			m_v.u_dw = dw;
		}

		static inline UINT32 toBits(float src)
		{
			//! reinterpret a float as an UINT32 assuming IEEE 32
			//! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
			UINT32 dst;
			STATIC_ASSERT(sizeof(src)==sizeof(dst),toBits);
			::memcpy(&dst, &src, sizeof(dst));
			return dst;
		}
		static inline float fromBits(UINT32 src)
		{
			//! reinterpret an UINT32 as a float assuming IEEE 32
			//! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
			float dst;
			STATIC_ASSERT(sizeof(src)==sizeof(dst), fromBits);
			::memcpy(&dst, &src, sizeof(dst));
			return dst;
		}

		// bool get_Negative() const
		// short get_Exponent() const
		UINT32 get_Mantissa() const
		{
			return (m_v.u_dw & k_MANT_MASK);
		}

		UNITTEST_FRIEND(CFloat32);
	};

	class GRAYCORE_LINK CFloat64
	{
		//! @class Gray::CFloat64
		//! pack/unpack IEEE float64/double http://en.wikipedia.org/wiki/IEEE_754-1985
		//! Use DBL_MAX

	public:
		CUnion64 m_v;	//!< holds the float64.

		static const UINT64 k_SIGN_MASK = CUINT64(80000000, 00000000);		//!< 1 bit = value sign (Sign_bit)

		static const UINT64 k_EXP_MASK = CUINT64(7FF00000, 00000000);		//!< 11 bits = signed exponent (base 2)

		static const UINT64 k_MANT_MASK = CUINT64(000FFFFF, FFFFFFFF);		//!< 52 bits = fractional mantissa 
		static const UINT32 k_MANT_BITS = 52;								//!< 52 bits = fractional mantissa

	public:
		CFloat64()
		{
			// undefined.
		}
		CFloat64(double d)
		{
			m_v.u_d = d;
		}

		void put_Bits(UINT64 qw)
		{
			m_v.u_qw = qw;
		}

		static inline UINT64 toBits(double src)
		{
			//! reinterpret a double as an UINT64
			//! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
			UINT64 dst;
			STATIC_ASSERT(sizeof(src)==sizeof(dst), toBits);
			::memcpy(&dst, &src, sizeof(dst));
			return dst;
		}
		static inline double fromBits(UINT64 src)
		{
			//! reinterpret an UINT64 as a double
			//! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
			double dst;
			STATIC_ASSERT(sizeof(src)==sizeof(dst), fromBits);
			::memcpy(&dst, &src, sizeof(dst));
			return dst;
		}

#if 0
		// bool get_Negative() const
		short get_Exponent() const
		{
			// TODO FIX SIGN
			return (short)((m_v.u_qw & k_EXP_MASK) >> sdf);
		}
#endif
		UINT64 get_Mantissa() const
		{
			return (m_v.u_qw & k_MANT_MASK);
		}
	};

	// class CFloat80 // long double or "double double" NOT M$?
};

#endif // _INC_CFloat_H
