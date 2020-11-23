//
//! @file cTypes.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_cTypes_H
#define _INC_cTypes_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray
{

	enum CTYPE_FLAG_TYPE_
	{
		//! @enum Gray::CTYPE_FLAG_TYPE_
		//! Bitmask to describe a native data type. used for type metadata. Fit in BYTE.
		CTYPE_FLAG_Numeric = 0x01,	//!< A numeric value of some sort. (maybe time, float or int)
		CTYPE_FLAG_NumSigned = 0x02,	//!< a signed numeric value. float or int.
		CTYPE_FLAG_Float = 0x04,	//!< Floating point. double or float.
		CTYPE_FLAG_Time = 0x08,		//!< Number represents a time. number of time units from some epoch.
		CTYPE_FLAG_Array = 0x10,	//!< An array of stuff.
		CTYPE_FLAG_Alloc = 0x20,	//!< Contains pointer to allocated memory. variable length? Blob?
		CTYPE_FLAG_StringA = 0x40,	//!< UTF8 format string.
		CTYPE_FLAG_StringW = 0x80,	//!< UNICODE format string.
		CTYPE_FLAG_UNUSED = 0xFF,	//!< This type is just a placeholder. don't use it.
	};

	// like _WIN32 MAKELPARAM(), MAKELONG() and MAKEWORD()
#define MAKEDWORD(low, high)	((UINT32)(((WORD)(low)) | (((UINT32)((WORD)(high))) << 16)))

#pragma pack(push,1)
	union CATTR_PACKED cUnion16
	{
		//! @union Gray::cUnion64
		//! What types can fit inside 16 bits? MAKEWORD(l,h)
		//! 16 bit union. size = 2 bytes
		//! This depends on USE_LITTLE_ENDIAN of course.
		BYTE  u_b[2];
		char  u_c[2];
		WORD  u_w;	//!< 16 bit words
		short u_s;
		operator WORD() const noexcept
		{
			return u_w;
		}
		void operator = (WORD w) noexcept
		{
			u_w = w;
		}
		struct
		{
#ifdef USE_LITTLE_ENDIAN
			BYTE m_Lo;	// LowPart
			BYTE m_Hi;	// HighPart
#else
			BYTE m_Hi;
			BYTE m_Lo;
#endif
		} u2;
	};

	union CATTR_PACKED cUnion32
	{
		//! @union Gray::cUnion32
		//! What types can fit inside 32 bits? MAKEDWORD(low, high)
		//! 32 bit union. size = 4 bytes
		//! This depends on USE_LITTLE_ENDIAN of course.
		//! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]

		BYTE u_b[4];
		char u_c[4];
		signed char u_sc[4];
		WORD  u_w[2];	//!< 16 bit unsigned words
		short u_s[2];	//!< 16 bit signed words
		UINT32 u_dw;	//!< 32 bit unsigned
		float u_f;		//!< 32 bit float.

		operator UINT32() const noexcept
		{
			return u_dw;
		}
		void operator = (UINT32 dw) noexcept
		{
			u_dw = dw;
		}

		struct
		{
#ifdef USE_LITTLE_ENDIAN
			cUnion16 m_Lo;	// LowPart
			cUnion16 m_Hi;	// HighPart
#else
			cUnion16 m_Hi;
			cUnion16 m_Lo;
#endif
		} u2;
	};

	union CATTR_PACKED cUnion64
	{
		//! @union Gray::cUnion64
		//! What types can fit inside 64 bits?
		//! 64 bit union. Assumes alignment if anyone cares. size = 8 bytes
		//! similar to _WIN32 LARGE_INTEGER union. or cUInt64
		//! 2 * cUnion32
		//! This depends on USE_LITTLE_ENDIAN of course.
		//! Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]

		BYTE u_b[8];		//!< Map to bytes.
		char u_c[8];
		signed char u_sc[8];
		WORD  u_w[4];		//!< 16 bit words
		short  u_s[4];		//!< 16 bit words
		UINT32 u_dw[2];		//!< HighPart=1, LowPart=0 we assume for USE_LITTLE_ENDIAN.
		float u_f[2];	//!< 2 * 32 bit floats.
		double u_d;		//!< assumed to be 64 bits.

#ifdef USE_INT64
		UINT64 u_qw;		//!< 64 bits = QuadPart = ULONGLONG.
		INT64 u_iq;			//!< 64 bits = QuadPart = LONGLONG.
		operator UINT64() const noexcept
		{
			return u_qw;
		}
		void operator = (UINT64 qw) noexcept
		{
			u_qw = qw;
		}
#endif

		struct
		{
#ifdef USE_LITTLE_ENDIAN
			cUnion32 m_Lo;	// LowPart
			cUnion32 m_Hi;	// HighPart
#else
			cUnion32 m_Hi;
			cUnion32 m_Lo;
#endif
		} u2;
	};

	// __m128

#pragma pack(pop)

	template< typename TYPE = int >
	struct cTypeLimit	// static
	{
		//! @struct Gray::cTypeLimit
		//! Numeric constants (limits) each basic type.
		//! Similar to std::numeric_limits<T>::max(), or INT_MAX
		static const TYPE k_Min;		//!< Min value TYPE can represent. negative if signed type. NOT EPSILON (near zero). e.g. INT_MIN, -FLT_MAX 
		static const TYPE k_Max;		//!< Max positive value. Can equal this value. inclusive. AKA INT_MAX, FLT_MAX, DBL_MAX
		static const BYTE k_TypeFlags;	//!< CTYPE_FLAG_TYPE_ = float, signed, etc ?

		static inline bool isNumSigned() noexcept
		{
			//! A signed number. might be float or int
			return (k_TypeFlags & CTYPE_FLAG_NumSigned) ? true : false;
		}

		// TODO Use StrNum
		// TYPE FromString(const char* pszInp);
		// StrLen_t ToString(char* pszOut, TYPE val);
	};

#ifdef _MSC_VER		// M$ this is not a redundant define.
#define CTYPE_DEF(a,_TYPE,c,d,e,f,g,h)  template<> const _TYPE cTypeLimit<_TYPE>::k_Min = e; template<> const _TYPE cTypeLimit<_TYPE>::k_Max = f; const BYTE cTypeLimit<_TYPE>::k_TypeFlags = (c);
#include "cTypes.tbl"
#undef CTYPE_DEF
#endif	// _MSC_VER

	struct GRAYCORE_LINK cTypeFloat
	{
		//! @struct Gray::cTypeFloat
		//! Basic operations for float/double type. See GrayLib::Calc class for complex operations.
		//! See GrayLib::cFloat32 or GrayLib::cFloat64

		template< typename TYPE >
		static inline bool IsFinite(TYPE a) noexcept
		{
			//! Is this a valid number? NOT Nan 'Not A Number' and NOT Inf. NOT isIndeterminate()
			//! This makes no sense for integer types.
			//! like: std::isfinite()
			return(a >= 0 || a < 0); // ! #NAN or #IND #INF similar to ! _isnan() but it works.
		}

		template< typename TYPE >
		static inline bool IsNaN(TYPE a) noexcept
		{
			//! Is this 'Not A Number'? ! IsFinite()
			//! ONLY applies to float, double. like: std::isnan() std::isinf()
			//! ((x) != (x)) would be optimized away?
			return !(a >= 0 || a < 0); // ! #NAN or #IND #INF similar to _isnan() but it works.
		}

		template< typename TYPE >
		static inline bool IsInfinite(TYPE a)
		{
			//! Does a represent infinity? Positive or negative. ! IsFinite()
			//! ONLY applies to float, double. like: std::isnan() std::isinf()

			// TODO 
			UNREFERENCED_PARAMETER(a);
			return false;
		}

		// k_NaN
		// k_Inf

	};
};

#endif
