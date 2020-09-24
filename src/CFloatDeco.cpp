//
//! @file CFloatDeco.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CFloatDeco.h"
#include "CValT.h"
#include "StrNum.h"
#include "CDebugAssert.h"	// ASSERT

namespace Gray
{
	const double CFloatDeco::k_powersOf10[9] =	// opposite of GetCachedPower
	{
		10.0,			// is 10^2^i.  Used to convert decimal 
		100.0,			// exponents into floating-point numbers.
		1.0e4,			// 10000
		1.0e8,
		1.0e16,
		1.0e32,
		1.0e64,
		1.0e128,
		1.0e256
	};

	const UINT32 CFloatDeco::k_Exp10[10] =
	{
		// 32 bit exponent digits. [9] = 1 billion = 1.0e9 = 1000000000
		1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000
	};

	CFloatDeco CFloatDeco::GetCachedPower(int nExp2, OUT int* pnExp10) // static
	{
		//! opposite of k_powersOf10
		//! 10^-348, 10^-340, ..., 10^340, stepped by 8.

		static const CFloatDeco k_CachedPowers[87] = // 87
		{
			CFloatDeco( CUINT64(fa8fd5a0, 081c0288),-1220 ),
			CFloatDeco(  CUINT64(baaee17f, a23ebf76),-1193 ),
			CFloatDeco(  CUINT64(8b16fb20, 3055ac76),-1166 ),
			CFloatDeco(  CUINT64(cf42894a, 5dce35ea),-1140 ),
			CFloatDeco(  CUINT64(9a6bb0aa, 55653b2d),-1113 ),
			CFloatDeco(  CUINT64(e61acf03, 3d1a45df),-1087 ),
			CFloatDeco(  CUINT64(ab70fe17, c79ac6ca),-1060 ),
			CFloatDeco(  CUINT64(ff77b1fc, bebcdc4f),-1034 ),
			CFloatDeco(  CUINT64(be5691ef, 416bd60c),-1007 ),
			CFloatDeco(  CUINT64(8dd01fad, 907ffc3c),-980 ),
			CFloatDeco(  CUINT64(d3515c28, 31559a83),-954 ),
			CFloatDeco(  CUINT64(9d71ac8f, ada6c9b5),-927 ),
			CFloatDeco(  CUINT64(ea9c2277, 23ee8bcb),-901 ),
			CFloatDeco(  CUINT64(aecc4991, 4078536d),-874 ),
			CFloatDeco(  CUINT64(823c1279, 5db6ce57),-847 ),
			CFloatDeco(  CUINT64(c2109436, 4dfb5637),-821 ),
			CFloatDeco(  CUINT64(9096ea6f, 3848984f),-794 ),
			CFloatDeco(  CUINT64(d77485cb, 25823ac7),-768 ),
			CFloatDeco(  CUINT64(a086cfcd, 97bf97f4),-741 ),
			CFloatDeco(  CUINT64(ef340a98, 172aace5),-715 ),
			CFloatDeco(  CUINT64(b23867fb, 2a35b28e),-688 ),
			CFloatDeco(  CUINT64(84c8d4df, d2c63f3b),-661 ),
			CFloatDeco(  CUINT64(c5dd4427, 1ad3cdba),-635 ),
			CFloatDeco(  CUINT64(936b9fce, bb25c996),-608 ),
			CFloatDeco(  CUINT64(dbac6c24, 7d62a584),-582 ),
			CFloatDeco(  CUINT64(a3ab6658, 0d5fdaf6),-555 ),
			CFloatDeco(  CUINT64(f3e2f893, dec3f126),-529 ),
			CFloatDeco(  CUINT64(b5b5ada8, aaff80b8),-502 ),
			CFloatDeco(  CUINT64(87625f05, 6c7c4a8b),-475 ),
			CFloatDeco(  CUINT64(c9bcff60, 34c13053),-449 ),
			CFloatDeco(  CUINT64(964e858c, 91ba2655),-422 ),
			CFloatDeco(  CUINT64(dff97724, 70297ebd),-396 ),
			CFloatDeco(  CUINT64(a6dfbd9f, b8e5b88f),-369 ),
			CFloatDeco(  CUINT64(f8a95fcf, 88747d94),-343 ),
			CFloatDeco(  CUINT64(b9447093, 8fa89bcf),-316 ),
			CFloatDeco(  CUINT64(8a08f0f8, bf0f156b),-289 ),
			CFloatDeco(  CUINT64(cdb02555, 653131b6),-263 ),
			CFloatDeco(  CUINT64(993fe2c6, d07b7fac),-236 ),
			CFloatDeco(  CUINT64(e45c10c4, 2a2b3b06),-210 ),
			CFloatDeco(  CUINT64(aa242499, 697392d3),-183 ),
			CFloatDeco(  CUINT64(fd87b5f2, 8300ca0e),-157 ),
			CFloatDeco(  CUINT64(bce50864, 92111aeb),-130 ),
			CFloatDeco(  CUINT64(8cbccc09, 6f5088cc),-103 ),
			CFloatDeco(  CUINT64(d1b71758, e219652c),-77 ),
			CFloatDeco(  CUINT64(9c400000, 00000000),-50 ),
			CFloatDeco(  CUINT64(e8d4a510, 00000000),-24 ),
			CFloatDeco(  CUINT64(ad78ebc5, ac620000),3 ),
			CFloatDeco(  CUINT64(813f3978, f8940984),30 ),
			CFloatDeco(  CUINT64(c097ce7b, c90715b3),56 ),
			CFloatDeco(  CUINT64(8f7e32ce, 7bea5c70),83 ),
			CFloatDeco(  CUINT64(d5d238a4, abe98068),109 ),
			CFloatDeco(  CUINT64(9f4f2726, 179a2245),136 ),
			CFloatDeco(  CUINT64(ed63a231, d4c4fb27),162 ),
			CFloatDeco(  CUINT64(b0de6538, 8cc8ada8),189 ),
			CFloatDeco(  CUINT64(83c7088e, 1aab65db),216 ),
			CFloatDeco(  CUINT64(c45d1df9, 42711d9a),242 ),
			CFloatDeco(  CUINT64(924d692c, a61be758),269 ),
			CFloatDeco(  CUINT64(da01ee64, 1a708dea),295 ),
			CFloatDeco(  CUINT64(a26da399, 9aef774a),322 ),
			CFloatDeco(  CUINT64(f209787b, b47d6b85),348 ),
			CFloatDeco(  CUINT64(b454e4a1, 79dd1877),375 ),
			CFloatDeco(  CUINT64(865b8692, 5b9bc5c2),402 ),
			CFloatDeco(  CUINT64(c83553c5, c8965d3d),428 ),
			CFloatDeco(  CUINT64(952ab45c, fa97a0b3),455 ),
			CFloatDeco(  CUINT64(de469fbd, 99a05fe3),481 ),
			CFloatDeco(  CUINT64(a59bc234, db398c25),508 ),
			CFloatDeco(  CUINT64(f6c69a72, a3989f5c),534 ),
			CFloatDeco(  CUINT64(b7dcbf53, 54e9bece),561 ),
			CFloatDeco(  CUINT64(88fcf317, f22241e2),588 ),
			CFloatDeco(  CUINT64(cc20ce9b, d35c78a5),614 ),
			CFloatDeco(  CUINT64(98165af3, 7b2153df),641 ),
			CFloatDeco(  CUINT64(e2a0b5dc, 971f303a),667 ),
			CFloatDeco(  CUINT64(a8d9d153, 5ce3b396),694 ),
			CFloatDeco(  CUINT64(fb9b7cd9, a4a7443c),720 ),
			CFloatDeco(  CUINT64(bb764c4c, a7a44410),747 ),
			CFloatDeco(  CUINT64(8bab8eef, b6409c1a),774 ),
			CFloatDeco(  CUINT64(d01fef10, a657842c),800 ),
			CFloatDeco(  CUINT64(9b10a4e5, e9913129),827 ),
			CFloatDeco(  CUINT64(e7109bfb, a19c0c9d),853 ),
			CFloatDeco(  CUINT64(ac2820d9, 623bf429),880 ),
			CFloatDeco(  CUINT64(80444b5e, 7aa7cf85),907 ),
			CFloatDeco(  CUINT64(bf21e440, 03acdd2d),933 ),
			CFloatDeco(  CUINT64(8e679c2f, 5e44ff8f),960 ),
			CFloatDeco(  CUINT64(d433179d, 9c8cb841),986 ),
			CFloatDeco(  CUINT64(9e19db92, b4e31ba9),1013 ),
			CFloatDeco(  CUINT64(eb96bf6e, badf77d9),1039 ),
			CFloatDeco(  CUINT64(af87023b, 9bf0ee6b),1066 )
		};

		//int nExp10o = static_cast<int>(ceil((-61 - nExp2) * 0.30102999566398114)) + 374;
		double dk = (-61 - nExp2) * 0.30102999566398114 + 347;	// dk must be positive, so can do ceiling in positive
		int nExp10 = static_cast<int>(dk);
		if (nExp10 != dk)	// Round up.
			nExp10++;

		unsigned index = static_cast<unsigned>((nExp10 >> 3) + 1);
		ASSERT(IS_INDEX_GOOD_ARRAY(index, k_CachedPowers));

		*pnExp10 = -(-348 + static_cast<int>(index << 3));	// decimal exponent doesn't need lookup table
		return k_CachedPowers[index];
	}

	double GRAYCALL CFloatDeco::toDouble(UINT32 fracHi, UINT32 fracLo, int nExp10) // static
	{
		// Make a double from a base 10 exponent.

		double fraction = (1.0e9 * fracHi) + fracLo;	// Mantissa = Combine the ints.

		if (nExp10 != 0)
		{
			// Generate a floating-point number that represents the exponent.
			// Do this by processing the exponent one bit at a time to combine
			// many powers of 2 of 10. Then combine the exponent with the fraction.

			bool bExpNegative = false;
			if (nExp10 < 0)
			{
				bExpNegative = true;
				nExp10 = -nExp10;	// unsigned nExp
			}
			else
			{
				bExpNegative = false;
			}

			double dblExp = 1.0;
			for (int i = 0; nExp10 != 0; nExp10 >>= 1, i++)
			{
				if (nExp10 & 01)
				{
					if (IS_INDEX_BAD_ARRAY(i, CFloatDeco::k_powersOf10))
					{
						// VALUE IS WRONG!! OVERFLOW.
#ifdef _INC_CLogMgr_H
						// errno = ERANGE; ASSERT();
						DEBUG_ERR(("Exponent overflow."));
#endif
						break;
					}
					dblExp *= CFloatDeco::k_powersOf10[i];
				}
			}
			if (bExpNegative)
			{
				fraction /= dblExp;
			}
			else
			{
				fraction *= dblExp;
			}
		}

		return fraction;
	}

	void CFloatDeco::GrisuRound(char* pszOut, StrLen_t len, UINT64 delta, UINT64 rest, UINT64 ten_kappa, UINT64 wp_w) // static
	{
		//! Round last digit down?
		while (rest < wp_w && delta - rest >= ten_kappa &&
			(rest + ten_kappa < wp_w ||  // closer
				wp_w - rest > rest + ten_kappa - wp_w))
		{
			pszOut[len - 1]--;
			rest += ten_kappa;
		}
	}

	StrLen_t CFloatDeco::Grisu2(double dVal, char* pszOut, OUT int* pnExp10) // static
	{
		if (dVal == 0)	// special case
		{
			*pszOut = '0';
			*pnExp10 = 0;
			return 1;
		}

		CFloatDeco w_m;
		CFloatDeco w_p;
		const CFloatDeco v(dVal);
		v.NormalizedBoundaries(&w_m, &w_p);

		const CFloatDeco c_mk = GetCachedPower(w_p.m_iExp2, pnExp10);
		const CFloatDeco W = v.Normalize() * c_mk;
		CFloatDeco Wp = w_p * c_mk;
		CFloatDeco Wm = w_m * c_mk;
		Wm.m_uMant++;
		Wp.m_uMant--;

		UINT64 delta = Wp.m_uMant - Wm.m_uMant;

		const CFloatDeco one(UINT64(1) << -Wp.m_iExp2, Wp.m_iExp2);
		const CFloatDeco wp_w = Wp - W;
		UINT32 p1 = static_cast<UINT32>(Wp.m_uMant >> -one.m_iExp2);
		UINT64 p2 = Wp.m_uMant & (one.m_uMant - 1);

		int kappa = static_cast<int>(GetCountDecimalDigit32(p1));
		ASSERT(kappa <= 10);

		StrLen_t nLength = 0;
		while (kappa > 0)
		{
			UINT32 d = p1;
			if (kappa > 1)
			{
				const UINT32 kd = k_Exp10[kappa - 1];
				d /= kd;
				p1 %= kd;
			}
			else
			{
				p1 = 0;
			}
			if (d != 0 || nLength > 0)
				pszOut[nLength++] = '0' + static_cast<char>(d);
			kappa--;
			UINT64 tmp = (static_cast<UINT64>(p1) << -one.m_iExp2) + p2;
			if (tmp <= delta)
			{
				*pnExp10 += kappa;
				GrisuRound(pszOut, nLength, delta, tmp, static_cast<UINT64>(k_Exp10[kappa]) << -one.m_iExp2, wp_w.m_uMant);
				return nLength;
			}
		}

		// kappa = 0
		for (;;)
		{
			p2 *= 10;
			delta *= 10;
			char d = static_cast<char>(p2 >> -one.m_iExp2);
			if (d || nLength > 0)
				pszOut[nLength++] = '0' + d;
			p2 &= one.m_uMant - 1;
			kappa--;
			if (p2 < delta)
			{
				*pnExp10 += kappa;
				GrisuRound(pszOut, nLength, delta, p2, one.m_uMant, wp_w.m_uMant * k_Exp10[-kappa]);
				return nLength;
			}
		}
	}

	StrLen_t CFloatDeco::MantRound(char* pszOut, StrLen_t nMantLength) // static 
	{
		// Truncate Mantissa.
		// Round the last digit.
		if (nMantLength > 0 && pszOut[nMantLength] > '5')
		{
			for (int i = nMantLength - 1; ; i--)	// round up then cascade this up if necessary.
			{
				char ch = pszOut[i];
				if (ch != '.')
				{
					ch++;
					if (ch <= '9')
					{
						pszOut[i] = ch;
						break;
					}
					pszOut[i] = '0';	// Roll to 0
				}
				if (i <= 0)
				{
					// Need to carry shift. e.g. 99.99 rounds up to 100
					::memmove(pszOut + 1, pszOut, nMantLength + 1);
					pszOut[0] = '1';
					nMantLength++;
					break;
				}
			}
		}
		return nMantLength;	// new length.
	}

	StrLen_t CFloatDeco::MantAdjust(char* pszOut, StrLen_t nMantLength, StrLen_t nMantLengthNew)  // static
	{
		//! Change mantissa size up or down.
		//! @return size change.

		StrLen_t iDelta = nMantLengthNew - nMantLength;
		if (iDelta == 0)
			return 0;
		if (iDelta < 0)
		{
			// Chop off decimal places. 
			MantRound(pszOut, nMantLengthNew);
		}
		else
		{
			// Post Pad out with 0.
			if (nMantLengthNew > StrNum::k_LEN_MAX_DIGITS)
			{
				nMantLengthNew = StrNum::k_LEN_MAX_DIGITS;
			}
			CValArray::FillQty<char>(pszOut + nMantLength, iDelta, '0');
		}
		return iDelta;
	}

	StrLen_t CFloatDeco::FormatE(char* pszOut, StrLen_t nMantLength, int nExp10, char chE) // static
	{
		//! like ecvt() using e exponent.
		//! @arg pszOut = string to contains digits.
		//! @arg chE = 'e' or 'E'

		StrLen_t nExponent1 = (nMantLength + nExp10) - 1;	// 10^(nExponent1-1) <= v < 10^nExponent1
		StrLen_t i;

		if (nMantLength == 1)
		{
			// no decimal point. e.g. 1e30
			i = 1;
		}
		else
		{
			// 1 digit place. e.g. 1234e30 -> 1.234e33
			::memmove(&pszOut[2], &pszOut[1], nMantLength - 1);	// insert decimal point.
			pszOut[1] = '.';
			i = nMantLength + 1;
		}

		pszOut[i++] = chE;	// or Capital 'e' 

							// Write out the exponent part
		if (nExponent1 < 0)
		{
			pszOut[i++] = '-';
			nExponent1 = -nExponent1;
		}
		else
		{
			pszOut[i++] = '+';
		}

		static const char k_cDigitsLut[200] =
		{
			'0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
			'1', '0', '1', '1', '1', '2', '1', '3', '1', '4', '1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
			'2', '0', '2', '1', '2', '2', '2', '3', '2', '4', '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
			'3', '0', '3', '1', '3', '2', '3', '3', '3', '4', '3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
			'4', '0', '4', '1', '4', '2', '4', '3', '4', '4', '4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
			'5', '0', '5', '1', '5', '2', '5', '3', '5', '4', '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
			'6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
			'7', '0', '7', '1', '7', '2', '7', '3', '7', '4', '7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
			'8', '0', '8', '1', '8', '2', '8', '3', '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
			'9', '0', '9', '1', '9', '2', '9', '3', '9', '4', '9', '5', '9', '6', '9', '7', '9', '8', '9', '9'
		};

		if (nExponent1 >= 100)
		{
			pszOut[i++] = '0' + static_cast<char>(nExponent1 / 100);
			nExponent1 %= 100;
		}

		const char* d = k_cDigitsLut + nExponent1 * 2;
		pszOut[i++] = d[0];
		pszOut[i++] = d[1];

		pszOut[i] = '\0';
		return i;
	}

	StrLen_t CFloatDeco::FormatF(char* pszOut, StrLen_t nMantLength, int nExp10, int iDecPlacesWanted) // static
	{
		//! like fcvt(). Might be k_LEN_MAX_DIGITS digits long.
		//! @arg pszOut contains digits.

		ASSERT(nMantLength >= 0);
		StrLen_t nDecPlaceO = nMantLength + nExp10;	// where does the decimal place go?

		if (nExp10 >= 0)
		{
			// Whole numbers only. No decimal places. 
			// 1234e7 -> 12340000000.0
			// Beware k_LEN_MAX_DIGITS
			ASSERT(nDecPlaceO + iDecPlacesWanted + 1 < StrNum::k_LEN_MAX_DIGITS);

			CValArray::FillQty<char>(pszOut + nMantLength, nExp10, '0'); // post pad end.
			nMantLength += nExp10;
		}
		else if (nDecPlaceO > 0)
		{
			// Some decimal places and some whole numbers.
			// 1234e-2 -> 12.34
			ASSERT(nDecPlaceO < StrNum::k_LEN_MAX_DIGITS);
			::memmove(&pszOut[nDecPlaceO + 1], &pszOut[nDecPlaceO], nMantLength - nDecPlaceO);	// make space.
		}
		else
		{
			// nExp10 < 0 = No whole numbers. just decimal.
			// 1234e-6 -> 0.001234
			nDecPlaceO = -nDecPlaceO;
			ASSERT(nMantLength + nDecPlaceO + 2 < StrNum::k_LEN_MAX_DIGITS);

			::memmove(&pszOut[nDecPlaceO + 2], &pszOut[0], nMantLength);
			pszOut[0] = '0';
			CValArray::FillQty<char>(pszOut + 2, nDecPlaceO, '0'); // pre-pad with 0.

			nMantLength += nDecPlaceO + 1;
			nDecPlaceO = 1;
		}

		if (iDecPlacesWanted == 0)	// odd but allowed.
		{
			pszOut[nDecPlaceO] = '\0';
			return nDecPlaceO;
		}

		pszOut[nDecPlaceO] = '.';
		nMantLength++;	// include '.'

		if (iDecPlacesWanted > 0)	// Adjust number of decimal places. up or down. else just use them all.
		{
			int iDecDiff = 1 + iDecPlacesWanted - (nMantLength - nDecPlaceO);
			if (iDecDiff > 0)
			{
				// Beware k_LEN_MAX_DIGITS
				ASSERT(nMantLength + iDecDiff < StrNum::k_LEN_MAX_DIGITS);
				CValArray::FillQty<char>(pszOut + nMantLength, iDecDiff, '0'); // post pad with 0.
				nMantLength += iDecDiff;
			}
			else if (iDecDiff < 0)
			{
				nMantLength = MantRound(pszOut, nMantLength + iDecDiff);
			}
			ASSERT(nMantLength >= 0);
		}

		pszOut[nMantLength] = '\0';
		return nMantLength;
	}
}
