//
//! @file CUInt64.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//
#include "StdAfx.h"
#include "CUInt64.h"
#include "CRandomDef.h"
#include "CLogMgr.h"

namespace Gray
{
	StrLen_t CUInt64::GetStr(char* pszOut, StrLen_t iOutMax, RADIX_t uRadixBase) const
	{
#ifdef USE_INT64
		return StrT::ULtoA(m_u, pszOut, iOutMax, uRadixBase);
#else
		ASSERT(0);	// TODO
		return 0;
#endif
	}

	cString CUInt64::GetStr(RADIX_t uRadixBase) const
	{
		//! @note We can estimate the string size via get_Highest1Bit()
		char szTmp[StrT::k_LEN_MAX];
		GetStr(szTmp, _countof(szTmp), uRadixBase);
		return szTmp;
	}

	bool CUInt64::SetStr(const char* pszVal, RADIX_t uRadixBase, const char** ppszEnd)
	{
#ifdef USE_INT64
		m_u = StrT::toUL(pszVal, ppszEnd, uRadixBase);
#else
		ASSERT(0); // TODO
#endif
		return true;
	}

	BIT_ENUM_t CUInt64::get_Highest1Bit() const
	{
#ifdef USE_INT64
		return CBits::Highest1Bit(m_u);
#else
		if (m_uHi)
			return CBits::Highest1Bit(m_uHi) + k_UNIT_BITS;
		return CBits::Highest1Bit(m_uLo);
#endif
	}

	HRESULT CUInt64::SetRandomBits(BIT_ENUM_t nBits)
	{
		ASSERT(nBits <= 64);
#ifdef USE_INT64
		g_Rand.GetNoise(&m_u, sizeof(m_u));
		m_u &= (((UNIT_t)1) << nBits) - 1;
#else
		m_uLo = g_Rand.get_RandUns();
		if (nBits < k_UNIT_BITS)
		{
			m_uLo &= (((UNIT_t)1) << nBits) - 1;
		}
		else
		{
			m_uHi = g_Rand.get_RandUns();
			m_uHi &= (((UNIT_t)1) << (nBits - k_UNIT_BITS)) - 1;
		}
#endif
		return S_OK;
	}

	void CUInt64::SetPowerMod(const CUInt64& base, const CUInt64& exponent, const CUInt64& modulus)
	{
		//! Set *this to 'base' to the power of 'exponent' then modulus.
		//! *this = ((base^exponent)%modulus)

#ifdef USE_INT64
		m_u = 1;
		bool bOne = true;
		UNIT_t nBitMask = ((UNIT_t)1) << (k_UNIT_BITS - 1);
		do
		{
			if (!bOne)
			{
				CUInt64 n(*this);
				*this *= n;
				*this %= modulus;
			}
			if (exponent.m_u & nBitMask)
			{
				*this *= base;
				*this %= modulus;
				bOne = false;
			}
			nBitMask >>= 1;
		} while (nBitMask != 0);
#else
		ASSERT(0);	// TODO
#endif
	}

	bool CUInt64::isPrime() const
	{
		//! This function uses Fermat's (little) Theorem 100 times to test the primeness of a
		//! (large) positive integer.
		//! if p is prime then for any integer x, ( x^p - x ) will be evenly divisible by p.
		//! Small Primes: 2 3 5 7 11 13 17 19 23 29 31 37 41 43 47 53 59 61 67 71
		//! http://en.wikipedia.org/wiki/Primality_test
		//! @note this can be VERY slow for big numbers.

		if (!isOdd())		// do the easy test first. Even numbers are never prime.
		{
			return(*this == 2);
		}
		ASSERT(!isZero());	// zero is not a valid test.

		CUInt64 pminus1(*this);
		pminus1 -= 1;
		if (pminus1.isZero())	// 1 is not prime.
			return false;

		BIT_ENUM_t nBits = get_Highest1Bit() - 1;	// test number is less than the prime.
		ITERATE_t nTries = nBits;	// this ought to be enough tries to make sure it is prime.
		if (nTries < 10)
			nTries = 10;
		if (nTries > 100)
			nTries = 100;

		while (--nTries != 0)
		{
			CUInt64 x;
			x.SetRandomBits(nBits);	// random test number less than the prime. since it is not div 2, >= div 3
			ASSERT(x < *this);
			if (x.isZero())	// not a useful test.
				continue;
			CUInt64 r;
			r.SetPowerMod(x, pminus1, *this);
			if (r != 1)
				return false;	// Not prime.
		}

		return true;	// Seems to be prime.
	}

	int CUInt64::SetRandomPrime(BIT_ENUM_t nBits, CThreadState* pCancel)
	{
		//! This function generates/finds/guesses a random prime.
		//! @return Number of tries to get a prime.

		ASSERT(nBits < 64);
		ASSERT(nBits > 1);
		SetRandomBits(nBits);
		SetBit(0);			// make odd. even numbers are not prime of course.
		SetBit(nBits - 1);	// MUST be large.
		ASSERT(isOdd());

		int iTries = 0;
		if (pCancel != nullptr)
		{
			// break this into n threads to utilize more CPU power. this is just one thread.
			for (;; iTries++)
			{
				if (pCancel->isThreadStopping())
					return -1;
				if (isPrime())
					break;
				*this += 2;		// Try next odd number.
			}
		}
		else
		{
			for (; !isPrime(); iTries++)
				*this += 2;		// Try next odd number.
		}

		ASSERT(isPrime());	// double check
		ASSERT(this->get_Highest1Bit() <= nBits);	// must not grow.
		return iTries;
	}

	void CUInt64::OpBitShiftLeft1(UNIT_t nBitMask)
	{
#ifdef USE_INT64
		UNIT_t nTmp = m_u;
		UNIT_t nCarryBit = nTmp >> (k_UNIT_BITS - 1);
		ASSERT(nCarryBit == 0 || nCarryBit == 1);
		m_u = ((nTmp << 1) | nBitMask);
#else
		// _addcarry_u64
		ASSERT(0);	// TODO
#endif
	}

	void GRAYCALL CUInt64::Divide(const CUInt64& dividend, const CUInt64& divisor, OUT CUInt64& quotient, OUT CUInt64& remainder)
	{
		//! Division with remainder
		//! get quotient,remainder for dividend/divisor.

		if (&dividend == &quotient ||
			&dividend == &remainder ||
			&divisor == &quotient ||
			&divisor == &remainder)
		{
			// ThrowUserException( "CUInt64 Divide Cannot write quotient and remainder into the same variable." );
			return;
		}
		if (divisor.isZero())
		{
			// ThrowUserException( "CUInt64 Divide by zero." );
			return;
		}
		if (dividend.isZero())
		{
			quotient = (UNIT_t)0;
			remainder = (UNIT_t)0;
			return;
		}

		// If dividend.m_nBlksUse < divisor.m_nBlksUse, then dividend < divisor, and we can be sure that divisor doesn't go into
		// dividend at all.
		if (dividend < divisor)
		{
			// The quotient is 0 and dividend is the remainder
			quotient = (UNIT_t)0;
			remainder = dividend;
			return;
		}

		// At this point we know dividend >= divisor > 0.  (Whew!)
		quotient = (UNIT_t)0;
		remainder = (UNIT_t)0;

#ifdef USE_INT64
		BIT_ENUM_t nBits = k_UNIT_BITS;
		UNIT_t nBlkTmp = dividend.m_u;
		while (nBits-- != 0)
		{
			remainder.OpBitShiftLeft1((nBlkTmp >> nBits) & 1);
			if (divisor <= remainder)
			{
				quotient.OpBitShiftLeft1(1);
				remainder -= divisor;
			}
			else
			{
				quotient.OpBitShiftLeft1(0);
			}
		}
#else
		ASSERT(0);
#endif
		ASSERT((quotient * divisor + remainder) == dividend);	// test reverse operation.
	}

	void GRAYCALL CUInt64::EuclideanAlgorithm(const CUInt64& x, const CUInt64& y, OUT CUInt64& a, OUT CUInt64& b, OUT CUInt64& g) // static
	{
		//! This function uses the Euclidean algorithm to find the greatest common divisor
		//! g of the positive integers x and y and also two integers a and b such that
		//! ax - by = g, 1 <= a <= y and 0 <= b < x.
		//!
		//! This function will fail in undefined ways if either x or y is zero.
#ifdef USE_INT64
		if (x <= y)
		{
			CUInt64 q;
			CUInt64 r;
			Divide(y, x, q, r);
			if (r == 0)
			{
				a = 1;
				b = (UNIT_t)0;
				g = x;
			}
			else
			{
				CUInt64 ap;
				EuclideanAlgorithm(x, r, ap, b, g);
				// a = ap + b * q;
				a = b;
				a *= q;
				a += ap;
			}
		}
		else
		{
			CUInt64 ap;
			CUInt64 bp;
			EuclideanAlgorithm(y, x, bp, ap, g);
			// a = y - ap;
			a = y;
			a -= ap;
			// b = x - bp;
			b = x;
			b -= bp;
		}
#else
		ASSERT(0);	// TODO
#endif
	}
}

//********************************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"
UNITTEST_CLASS(CUInt64)
{
	void UnitTestStr(const CUInt64& d1, RADIX_t r)
	{
		char szTmp1[1024];
		d1.GetStr(szTmp1, STRMAX(szTmp1), r);

		CUInt64 d2(szTmp1, r);
		UNITTEST_TRUE(d1 == d2);
		char szTmp2[1024];
		d2.GetStr(szTmp2, STRMAX(szTmp2), r);
		UNITTEST_TRUE(!StrT::Cmp(szTmp2, szTmp1));

		CUInt64 d3;
		d3.SetStr(szTmp2, r);
		UNITTEST_TRUE(d1 == d3);
		UNITTEST_TRUE(d1 == d2);
	}
	void UnitTestStr(RADIX_t r)
	{
		CUInt64 d1;
		d1.SetRandomBits(64);
		UnitTestStr(d1, r);
	}

	void UnitTestPrimes()
	{
		for (CUInt64::UNIT_t i = 2; i < 200; i++)
		{
			CUInt64 n(i);
			if (n.isPrime())
			{
				CUnitTests::sm_pLog->addDebugInfoF("Prime=%d", i);
			}
		}
	}

	UNITTEST_METHOD(CUInt64)
	{
		//! Hold numbers. simple assignment
		const CUInt64 nu1(1);
		const CUInt64 nu2(2);
		const CUInt64 nu19(19);
		const CUInt64 nu25(25);

		UNITTEST_TRUE(nu19 == 19);

		CUInt64 nux1(1234567890);
		UNITTEST_TRUE(nux1.get_Val<UINT32>() == 1234567890);
		CUInt64 nux2(nux1);
		UNITTEST_TRUE(nux1 == nux2);
		UNITTEST_TRUE(nux2.get_Val<UINT32>() == 1234567890);
		nux2 = nu25;
		UNITTEST_TRUE(nux2 == nu25);
		UNITTEST_TRUE(nux2 == 25);
		CUInt64 nux3;

		UnitTestPrimes();

		// strings to numbers
		static const char k_TmpX[] = "1234567890123456789";	// as big as 64 bits will hold.
		static const char k_Tmp16[] = "10000000000000000";	// 16 power.

		bool bRet = nux1.SetStr(k_TmpX);
		UNITTEST_TRUE(bRet);

		char szTmp[1024];
		StrLen_t iLen = nux1.GetStr(szTmp, STRMAX(szTmp));
		UNITTEST_TRUE(iLen);
		UNITTEST_TRUE(!memcmp(szTmp, k_TmpX, sizeof(k_TmpX) - 1));

		UnitTestStr(2);
		UnitTestStr(10);
		UnitTestStr(16);
		UnitTestStr(26);

		// Simple Tests
		UNITTEST_TRUE(nu25.isOdd());
		nux1.SetStr(k_Tmp16, 0x10);	// hex.

		UNITTEST_TRUE(CUInt64((CUInt64::UNIT_t)0).get_Highest1Bit() == 0);
		UNITTEST_TRUE(CUInt64(1).get_Highest1Bit() == 1);
		UNITTEST_TRUE(CUInt64(4095).get_Highest1Bit() == 12);
		UNITTEST_TRUE(CUInt64(4096).get_Highest1Bit() == 13);

		// Primitives.
		UINT32 u32 = nu19.get_Val<UINT32>();
		UNITTEST_TRUE(u32 == 19);
		UINT64 u64 = nu25.get_Val<UINT64>();
		UNITTEST_TRUE(u64 == 25);

#if 0	// TODO someday Make all these tests work for CUint64
		static const char k_Tmp9s[] = "999999999999999999";	// +1 to get k_Tmp18
		static const char k_Tmp18[] = "1000000000000000000";	// 18 power
		static const char k_Tmp9[] = "1000000000";	// 9 power

		// Bit ops.
		// 25 is binary 11001.
		nux1 = 25;

		iBit = 33;
		nux1 <<= iBit;
		UNITTEST_TRUE(nux1.IsSet(iBit + 4)); // 1
		UNITTEST_TRUE(nux1.IsSet(iBit + 3)); // 1
		UNITTEST_TRUE(!nux1.IsSet(iBit + 2)); // 0
		UNITTEST_TRUE(!nux1.IsSet(iBit + 1)); // 0
		UNITTEST_TRUE(nux1.IsSet(iBit + 0)); // 1
		UNITTEST_TRUE(nux1.get_Highest1Bit() == iBit + 5); // 5

		// Effectively add 2^32.
		nux1.ModBit(iBit + 32, true);
		UNITTEST_TRUE(nux1.IsSet(iBit + 32)); // 1
		nux1 >>= iBit;

		UNITTEST_TRUE(nux1 == CUInt64("4294967321")); // 4294967321
		nux1.ModBit(31, true);
		nux1.ModBit(32, false);
		UNITTEST_TRUE(nux1.get_Val<UINT>() == 2147483673U); // 2147483673

		// NOTE: Do a series of random opposite operations that cancel each other out.
		// bits set/clear, shift up/dn, add/sub, mul/div

		// Bit shifts.
		nux1 = k_TmpX;
		nux2 = nux1;
		nux2 <<= 31;
		// iLen = nux2.GetStr( szTmp, STRMAX(szTmp));

		nux3 = nux1;
		nux3 *= CUInt64(1UL << 31);
		// iLen = nux3.GetStr( szTmp, STRMAX(szTmp));

		UNITTEST_TRUE(nux2 == nux3);
		nux2 >>= 31;
		UNITTEST_TRUE(nux1 == nux2);

		nux2 <<= 31;
		nux2 <<= 41;
		nux2 <<= 51;
		nux2 <<= 61;
		nux2 >>= 31;
		nux2 >>= 41;
		nux2 >>= 51;
		nux2 >>= 61;
		UNITTEST_TRUE(nux1 == nux2);

		// Add
		nux1 = nu1 + nu2;
		UNITTEST_TRUE(nux1 == 3);
		nux1.SetStr(k_Tmp9s);
		nux1.OpAdd1(1);
		UNITTEST_TRUE(!nux1.isOdd());
		UNITTEST_TRUE(nux1 == CUInt64(k_Tmp32));

		nux2 = k_Tmp16;
		nux3 = nux1 + nux2;
		nux3 += nux1;
		iLen = nux3.GetStr(szTmp, STRMAX(szTmp));
		UNITTEST_TRUE(nux3 == CUInt64("200000000000000010000000000000000"));

		// Subtract
		nux1.SetStr(k_Tmp32);
		nux1.OpSubtract1(1);
		UNITTEST_TRUE(nux1.isOdd());
		UNITTEST_TRUE(nux1 == CUInt64(k_Tmp9s));

		nux2 = k_Tmp16;
		nux1++;
		nux3 = nux1 - nux2;
		nux3 -= nux2;
		UNITTEST_TRUE(nux3 == CUInt64("99999999999999980000000000000000"));

		// Multiply()
		nux1.SetStr(k_Tmp32);
		nux1.OpMultiply(2);
		// iLen = nux1.GetStr( szTmp, STRMAX(szTmp));
		UNITTEST_TRUE(nux1 == CUInt64("200000000000000000000000000000000"));

		nux2 = k_Tmp16;
		nux3 = nux2 * nux2;
		UNITTEST_TRUE(nux3 == CUInt64(k_Tmp32));
		nux3 *= nux1;
		// iLen = nux3.GetStr( szTmp, STRMAX(szTmp));
		UNITTEST_TRUE(nux3 == CUInt64("20000000000000000000000000000000000000000000000000000000000000000"));

		// Divide.
		nux1.SetStr(k_Tmp32);
		nux1.OpDivide(2);
		UNITTEST_TRUE(nux1 == CUInt64("50000000000000000000000000000000"));

		nux1.SetStr(k_Tmp32);
		nux2.SetStr(k_Tmp16);
		nux3 = nux1 / nux2;
		iLen = nux3.GetStr(szTmp, STRMAX(szTmp));
		UNITTEST_TRUE(nux3 == nux2);

		// Modulus.
		nux1.SetStr(k_Tmp32);
		nux2 = nux1;
		nux1++;
		nux3 = nux1 % nux2;
		UNITTEST_TRUE(nux3 == 1);

		// Div/Mod combined.
		nux1.SetStr(k_TmpX);
		nux2.SetStr(k_Tmp16);
		CUInt64 quotient, remainder;
		CUInt64::Divide(nux1, nux2, quotient, remainder);
		nux3 = (quotient * nux2) + remainder;	// test reverse Divide operation.
		UNITTEST_TRUE(nux3 == nux1);

		// power math
		nux3.SetPower(CUInt64(k_Tmp16), nu2);
		UNITTEST_TRUE(nux3 == CUInt64(k_Tmp32));

		UNITTEST_TRUE(nu19.isPrime());
		UNITTEST_TRUE(!nu25.isPrime());
		UNITTEST_TRUE(CUInt64("689572171629632424814677540353").isPrime());

		if (CUnitTestCur::IsTestInteractive())
		{
			// This can take a long time.
			// 2^1024 - 2^960 - 1 + 2^64 * { [2^894 pi] + 129093 }

			log.addDebugInfoF("Calculating big prime");

			CUInt64 p;
			p.SetRandomPrime(128);
			UnitTestStr(p, 10);

			static const char* k_Prime = "7";
			CUInt64 p1024(k_Prime);
			UNITTEST_TRUE(p1024.isPrime());
		}
#endif
	}
};
UNITTEST_REGISTER(CUInt64, UNITTEST_LEVEL_Core);
#endif
