//! @file cFloatDeco.h
//! convert numbers to/from string.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cFloatDeco_H
#define _INC_cFloatDeco_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrConst.h"
#include "cDebugAssert.h"  // ASSERT
#include "cFloat.h"

#if defined(__GNUC__) && (__GNUC__ > 4)
__extension__ typedef __int128 int128;
__extension__ typedef unsigned __int128 uint128;
#endif

namespace Gray {
// _umul128 TODO MulDiv ?

/// <summary>
/// Holds a decomposed double/float value. ignore sign.
/// Support class for conversion of double/float to string. Used with cFloat64
/// </summary>
class GRAYCORE_LINK cFloatDeco {
 public:
    static const double k_PowersOf10[9];                              /// Table giving binary powers of 10
    static const UINT32 k_Exp10[10];                                  /// Table of decimal digits to fit in 32 bit space.
    static const UINT64 k_MANT_MASK_X = CUINT64(00100000, 00000000);  /// Extra hidden bit. k_MANT_MASK+1

    UINT64 _uMant = 0;  /// Hold Mantissa.
    int _iExp2 = 0;     /// Hold base 2 Biased Exponent

 public:
    cFloatDeco() noexcept {}
    cFloatDeco(UINT64 uMan, int iExp2) noexcept : _uMant(uMan), _iExp2(iExp2) {}

    cFloatDeco(double d) noexcept {
        //! Decompose d.
        static const int k_DpExponentBias = 0x3FF + cFloat64::k_MANT_BITS;
        const cFloat64 u(d);
        const int iExpBiased = (u._v.u_qw & cFloat64::k_EXP_MASK) >> cFloat64::k_MANT_BITS;
        const UINT64 nMantSig = u.get_Mantissa();
        if (iExpBiased != 0) {
            _uMant = nMantSig + k_MANT_MASK_X;
            _iExp2 = iExpBiased - k_DpExponentBias;
        } else {
            _uMant = nMantSig;
            _iExp2 = 1 - k_DpExponentBias;
        }
    }

    cFloatDeco operator-(const cFloatDeco& rhs) const {
        //! Do math on decomposed number.
        //! ASSUME same _iExp2
        ASSERT(_iExp2 == rhs._iExp2);
        ASSERT(_uMant >= rhs._uMant);
        return cFloatDeco(_uMant - rhs._uMant, _iExp2);
    }

    cFloatDeco operator*(const cFloatDeco& rhs) const {
        //! Do math on decomposed numbers.
        UINT64 h;
#if defined(_MSC_VER) && defined(_M_AMD64)
        const UINT64 l = _umul128(_uMant, rhs._uMant, &h);
        if (l & (UINT64{1} << 63))  // rounding
            h++;
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && defined(__x86_64__)
        uint128 p = static_cast<uint128>(_uMant) * static_cast<uint128>(rhs._uMant);
        h = p >> 64;
        UINT64 l = static_cast<UINT64>(p);
        if (l & (UINT64(1) << 63))  // rounding
            h++;
#else
        const UINT64 M32 = 0xFFFFFFFF;
        const UINT64 a = _uMant >> 32;
        const UINT64 b = _uMant & M32;
        const UINT64 c = rhs._uMant >> 32;
        const UINT64 d = rhs._uMant & M32;
        const UINT64 ac = a * c;
        const UINT64 bc = b * c;
        const UINT64 ad = a * d;
        const UINT64 bd = b * d;
        UINT64 tmp = (bd >> 32) + (ad & M32) + (bc & M32);
        tmp += 1U << 31;  /// mult_round
        h = ac + (ad >> 32) + (bc >> 32) + (tmp >> 32);
#endif
        return cFloatDeco(h, _iExp2 + rhs._iExp2 + 64);
    }

    cFloatDeco Normalize() const {
        //! Fix _iExp2 by making _uMant as large as possible.
        //! cBits::Highest1Bit<>
        ASSERT(_uMant != 0);
        BIT_ENUM_t nBit = 64 - cBits::Highest1Bit(_uMant);
        return cFloatDeco(_uMant << nBit, _iExp2 - nBit);
    }

    void NormalizedBoundaries(cFloatDeco* minus, cFloatDeco* plus) const {
        cFloatDeco pl = cFloatDeco((_uMant << 1) + 1, _iExp2 - 1).Normalize();
        cFloatDeco mi = (_uMant == k_MANT_MASK_X) ? cFloatDeco((_uMant << 2) - 1, _iExp2 - 2) : cFloatDeco((_uMant << 1) - 1, _iExp2 - 1);
        mi._uMant <<= mi._iExp2 - pl._iExp2;
        mi._iExp2 = pl._iExp2;
        *plus = pl;
        *minus = mi;
    }

#if 0
	double get_Double() const {
		//! re-compose double. Convert back to a double. for testing.
		cFloat64 f(sdfsdf);
		return f._v.u_d;
	}
#endif

    static inline unsigned GetCountDecimalDigit32(UINT32 n) noexcept {
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

    static cFloatDeco GRAYCALL GetCachedPower(int nExp2, OUT int& rnExp10);
    static double GRAYCALL toDouble(UINT32 frac1, UINT32 frac2, int nExp10);

    static void GRAYCALL GrisuRound(char* pszOut, StrLen_t len, UINT64 delta, UINT64 rest, UINT64 ten_kappa, UINT64 wp_w) noexcept;
    static StrLen_t GRAYCALL Grisu2(double dVal, char* pszOut, OUT int& rnExp10);

    static StrLen_t GRAYCALL MantRound(char* pszOut, StrLen_t nMantLength);
    /// <summary>
    /// Change mantissa size up or down.
    /// </summary>
    /// <param name="pszOut"></param>
    /// <param name="nMantLength"></param>
    /// <param name="nMantLengthNew"></param>
    /// <returns>size change.</returns>
    static StrLen_t GRAYCALL MantAdjust(char* pszOut, StrLen_t nMantLength, StrLen_t nMantLengthNew);

    /// <summary>
    /// like ecvt() using e exponent.
    /// </summary>
    /// <param name="pszOut">string to contains digits.</param>
    /// <param name="nMantLength"></param>
    /// <param name="nExp10"></param>
    /// <param name="chE">'e' or 'E'</param>
    /// <returns></returns>
    static StrLen_t GRAYCALL FormatE(char* pszOut, StrLen_t nMantLength, int nExp10, char chE);
    static StrLen_t GRAYCALL FormatF(char* pszOut, StrLen_t nMantLength, int nExp10, int iDecPlacesWanted);
};
}  // namespace Gray
#endif
