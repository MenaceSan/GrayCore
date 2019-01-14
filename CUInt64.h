//
//! @file CUInt64.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CUInt64_H
#define _INC_CUInt64_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrArg.h"
#include "CBits.h"
#include "CTypes.h"
#include "CString.h"

UNITTEST_PREDEF(CUInt64)

namespace Gray
{
	class CThreadState;

#pragma pack(push,1)
	class GRAYCORE_LINK CATTR_PACKED CUInt64
	{
		//! @class Gray::CUInt64
		//! emulate 64 bit unsigned integer as a native type for systems that don't actually support this. QWord
		//! if ! USE_INT64 then ASSUME we DONT support 64 bit int types natively. Assume all platforms support 32 bit types.
		//! we don't support __int64 (_MSC_VER) or int64_t (C99/GCC standard) native.
		//! like CUnion64 or LARGE_INTEGER as a native type for operators.
		//! same size as UINT64
		//! use typedef CUInt64 UINT64; if ! USE_INT64

	public:
#ifdef USE_INT64	// native support.
		typedef UINT64 UNIT_t;
		static const unsigned k_UNIT_BITS = 64;
#else
		typedef UINT32 UNIT_t;	// break into 2 parts.
		static const unsigned k_UNIT_BITS = 32;
#endif

	private:
		// don't use CUnion64 so we can use CUINT64(h,l) for init.
#ifdef USE_INT64		// native support for UINT 64 bit.
		UNIT_t m_u;
#elif defined(USE_LITTLE_ENDIAN)
		UNIT_t m_uLo;	
		UNIT_t m_uHi;
#else
		UNIT_t m_uHi;
		UNIT_t m_uLo;
#endif

	public:
		CUInt64()
#ifdef USE_INT64
		: m_u(0)
#else
		: m_uLo(0)
		, m_uHi(0)
#endif
		{
		}
		CUInt64(UNIT_t n)
#ifdef USE_INT64
		: m_u(n)
#else
		: m_uLo(n)
		, m_uHi(0)
#endif
		{
		}
		CUInt64(const char* pszVal, RADIX_t n = 10)
		{
			SetStr(pszVal, n);
		}

		// Test Operators
		bool isZero() const
		{
#ifdef USE_INT64
			return m_u == 0;
#else
			return m_uLo == 0 && m_uHi == 0;
#endif
		}
		bool isOdd() const
		{
#ifdef USE_INT64
			return m_u & 1;
#else
			return m_uLo & 1;
#endif
		}
		bool IsSet(BIT_ENUM_t nBit) const
		{
			//! Gets the state of the enumerated bit.
			//! which has value 2^nBit.
			//! Bits beyond m_nBlksUse are considered to be 0.
#ifdef USE_INT64
			return CBits::IsSet(m_u, nBit);
#else
			if ( nBit < k_UNIT_BITS )
				return CBits::IsSet( m_uLo, nBit );
			else
				return CBits::IsSet( m_uHi, nBit-k_UNIT_BITS );
#endif
		}
		bool operator == (const CUInt64& n) const
		{
#ifdef USE_INT64
			return m_u == n.m_u;
#else
			return m_uLo == n.m_uLo && m_uHi == n.m_uHi;
#endif
		}
		bool operator != (const CUInt64& n) const
		{
			return !(*this == n);
		}
		bool operator == (UNIT_t n) const
		{
#ifdef USE_INT64
			return m_u == n;
#else
			return m_uLo == n;
#endif
		}
		bool operator>(const CUInt64& n) const
		{
#ifdef USE_INT64
			return m_u > n.m_u;
#else
			if(m_uHi == n.m_uHi)
				return m_uLo > n.m_uLo;
			else
				return m_uHi > n.m_uHi;
#endif
		}

		bool operator<(const CUInt64& n) const
		{
#ifdef USE_INT64
			return m_u < n.m_u;
#else
			if(m_uHi == n.m_uHi)
				return m_uLo < n.m_uLo;
			else
				return m_uHi < n.m_uHi;
#endif
		}

		bool operator <=(const CUInt64& n) const
		{
#ifdef USE_INT64
			return m_u <= n.m_u;
#else
			if(m_uHi == n.m_uHi)
				return m_uLo <= n.m_uLo;
			else
				return m_uHi < n.m_uHi;
#endif
		}

		template <typename TYPE>
		TYPE get_Val() const
		{
			//! just default to assume TYPE is unsigned.
#ifdef USE_INT64
			return (TYPE)m_u;
#else
			ASSERT(0);
			return 0;
#endif
		}

		// Math Action Operators
		void operator++()
		{
#ifdef USE_INT64
			m_u++;
#else
			UNIT_t n = m_uLo;
			m_uLo++;
			if(m_uLo < n)	// carry bit.
				m_uHi++;
#endif
		}
		void operator ++(int)
		{
			//! Post fix increment: same as prefix
			++*this;
		}

		void operator--()
		{
#ifdef USE_INT64
			m_u--;
#else
			UNIT_t n = m_uLo;
			m_uLo--;
			if(m_uLo > n) // carry bit.
				m_uHi--;
#endif
		}
		void operator --(int)
		{
			//! Postfix decrement: same as prefix
			--*this;
		}

		CUInt64& operator+=(const CUInt64& n)
		{
#ifdef USE_INT64
			m_u += n.m_u;
#else
			m_uLo += n.m_uLo;
			if(m_uLo < n.m_uLo) // carry bit.
				m_uHi++;
			m_uHi += n.m_uHi;
#endif
			return *this;
		}
		CUInt64& operator-=(const CUInt64& n)
		{
#ifdef USE_INT64
			m_u -= n.m_u;
#else
			ASSERT(0);
#endif
			return *this;
		}
		CUInt64& operator *=(const CUInt64& x)
		{
			CUInt64 ans;
#ifdef USE_INT64
			m_u *= x.m_u;
#else
			ASSERT(0);
#endif
			return *this;
		}
		CUInt64 operator *(const CUInt64& x) const
		{
			CUInt64 ans;
#ifdef USE_INT64
			ans.m_u = m_u * x.m_u;
#else
			ASSERT(0);
#endif
			return ans;
		}
		void operator %=(const CUInt64& x)
		{
			//! Modulus *this by x.
#ifdef USE_INT64
			m_u %= x.m_u;
#else
			ASSERT(0);
#endif
		}

		// Bit Action Operators
		void SetBit(BIT_ENUM_t uiBit)
		{
#ifdef USE_INT64
			m_u |= ((UNIT_t)1) << uiBit;
#else
			if ( uiBit < k_UNIT_BITS )
			{
				m_uLo |= ((UNIT_t)1) << uiBit;
			}
			else
			{
				m_uHi |= ((UNIT_t)1) << ( uiBit - k_UNIT_BITS );
			}
#endif
		}

		CUInt64& operator|=(const CUInt64& n)
		{
#ifdef USE_INT64
			m_u |= n.m_u;
#else
			m_uLo |= n.m_uLo;
			m_uHi |= n.m_uHi;
#endif
			return *this;
		}

		CUInt64& operator&=(const CUInt64& n)
		{
#ifdef USE_INT64
			m_u &= n.m_u;
#else
			m_uLo &= n.m_uLo;
			m_uHi &= n.m_uHi;
#endif
			return *this;
		}

		CUInt64& operator^=(const CUInt64& n)
		{
#ifdef USE_INT64
			m_u ^= n.m_u;
#else
			m_uLo ^= n.m_uLo;
			m_uHi ^= n.m_uHi;
#endif
			return *this;
		}

		CUInt64& operator<<=(BIT_ENUM_t uiBits)
		{
#ifdef USE_INT64
			m_u <<= uiBits;
#else
			if(uiBits < k_UNIT_BITS)
			{
				(m_uHi <<= uiBits) |= (m_uLo >> (k_UNIT_BITS-uiBits));
				m_uLo <<= uiBits;
			}
			else
			{
				m_uHi = m_uLo << (uiBits-k_UNIT_BITS);
				m_uLo = 0;
			}
#endif
			return *this;
		}

		CUInt64& operator>>=(BIT_ENUM_t uiBits)
		{
#ifdef USE_INT64
			m_u >>= uiBits;
#else
			if(uiBits < k_UNIT_BITS)
			{
				(m_uLo >>= uiBits) |= (m_uHi << (k_UNIT_BITS-uiBits));
				m_uHi >>= uiBits;
			}
			else
			{
				m_uLo = m_uHi >> (uiBits-k_UNIT_BITS);
				m_uHi = 0;
			}
#endif
			return *this;
		}

		StrLen_t GetStr(char* pszOut, StrLen_t iOutMax, RADIX_t nBaseRadix = 10) const;
		cString GetStr(RADIX_t nBaseRadix = 10) const;
		bool SetStr(const char* pszVal, RADIX_t nBaseRadix = 10, const char** ppszEnd = (const char**) nullptr);

		BIT_ENUM_t get_Highest1Bit() const;
		HRESULT SetRandomBits(BIT_ENUM_t nBits);
		void SetPowerMod(const CUInt64& base, const CUInt64& exponent, const CUInt64& modulus);

		bool isPrime() const;
		int SetRandomPrime(BIT_ENUM_t nBits, CThreadState* pCancel = nullptr);
		void OpBitShiftLeft1(UNIT_t nBitMask);

		static void GRAYCALL Divide(const CUInt64& dividend, const CUInt64& divisor, OUT CUInt64& quotient, OUT CUInt64& remainder);
		static void GRAYCALL EuclideanAlgorithm(const CUInt64& x, const CUInt64& y, OUT CUInt64& a, OUT CUInt64& b, OUT CUInt64& g);

		UNITTEST_FRIEND(CUInt64);
	};

#pragma pack(pop)

	inline CUInt64 operator+(const CUInt64& roUI64_1, const CUInt64& roUI64_2)
	{
		CUInt64 temp = roUI64_1;
		temp += roUI64_2;
		return temp;
	}

	inline CUInt64 operator|(const CUInt64& roUI64_1, const CUInt64& roUI64_2)
	{
		CUInt64 temp = roUI64_1;
		temp |= roUI64_2;
		return temp;
	}

	inline CUInt64 operator&(const CUInt64& roUI64_1, const CUInt64& roUI64_2)
	{
		CUInt64 temp = roUI64_1;
		temp &= roUI64_2;
		return temp;
	}

	inline CUInt64 operator^(const CUInt64& roUI64_1, const CUInt64& roUI64_2)
	{
		CUInt64 temp = roUI64_1;
		temp ^= roUI64_2;
		return temp;
	}

	inline CUInt64 operator<<(const CUInt64& n, BIT_ENUM_t uiBits)
	{
		CUInt64 temp = n;
		temp <<= uiBits;
		return temp;
	}

	inline CUInt64 operator>>(const CUInt64& n, BIT_ENUM_t uiBits)
	{
		CUInt64 temp = n;
		temp >>= uiBits;
		return temp;
	}
};

#endif
