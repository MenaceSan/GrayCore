//! @file cUInt64.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cLogMgr.h"
#include "cRandom.h"
#include "cUInt64.h"

namespace Gray {
template class GRAYCORE_LINK cBitmask<UINT64>;  // force implementation/instantiate for DLL/SO.

void cUInt64::BuildStr(StrBuilder<char>& sb, RADIX_t uRadixBase) const {
#ifdef USE_INT64
    sb.AdvanceWrite(StrT::ULtoA(_nVal, sb.get_SpanWrite(), uRadixBase));
#else
    ASSERT(0);  // TODO
    return 0;
#endif
}

cString cUInt64::GetStr(RADIX_t uRadixBase) const {
    //! encode value as string.
    //! @note We can estimate the max string size via get_Highest1Bit()
    char szTmp[StrNum::k_LEN_MAX_DIGITS_INT];
    StrBuilder<char> sb(TOSPAN(szTmp));
    BuildStr(sb, uRadixBase);
    return szTmp;
}

HRESULT cUInt64::SetStr(const cSpan<char>& src, RADIX_t nRadixBase) {
#ifdef USE_INT64
    const char* ppszEnd = nullptr;
    _nVal = StrT::toUL(src.get_PtrConst(), &ppszEnd, nRadixBase);
    return CastN(HRESULT, ppszEnd - src.get_PtrConst());
#else
    ASSERT(0);  // TODO
    return E_NOTIMPL;
#endif
}

BIT_ENUM_t cUInt64::get_Highest1Bit() const {
#ifdef USE_INT64
    return cBits::Highest1Bit(_nVal);
#else
    if (_nHi) return cBits::Highest1Bit(_nHi) + k_UNIT_BITS;
    return cBits::Highest1Bit(_nLo);
#endif
}

HRESULT cUInt64::SetRandomBits(BIT_ENUM_t nBits) {
    ASSERT(nBits <= 64);
#ifdef USE_INT64
    g_Rand.GetNoise(TOSPANT(_nVal));
    _nVal &= (((UNIT_t)1) << nBits) - 1;
#else
    _nLo = g_Rand.get_RandUns();
    if (nBits < k_UNIT_BITS) {
        _nLo &= (((UNIT_t)1) << nBits) - 1;
    } else {
        _nHi = g_Rand.get_RandUns();
        _nHi &= (((UNIT_t)1) << (nBits - k_UNIT_BITS)) - 1;
    }
#endif
    return S_OK;
}

void cUInt64::SetPowerMod(const cUInt64& base, const cUInt64& exponent, const cUInt64& modulus) {
    //! Set *this to 'base' to the power of 'exponent' then modulus.
    //! *this = ((base^exponent)%modulus)

#ifdef USE_INT64
    _nVal = 1;
    bool bOne = true;
    UNIT_t nBitMask = ((UNIT_t)1) << (k_UNIT_BITS - 1);
    do {
        if (!bOne) {
            cUInt64 n(*this);
            *this *= n;
            *this %= modulus;
        }
        if (exponent._nVal & nBitMask) {
            *this *= base;
            *this %= modulus;
            bOne = false;
        }
        nBitMask >>= 1;
    } while (nBitMask != 0);
#else
    ASSERT(0);  // TODO
#endif
}

bool cUInt64::isPrime() const {
    //! This function uses Fermat's (little) Theorem 100 times to test the primeness of a
    //! (large) positive integer.
    //! if p is prime then for any integer x, ( x^p - x ) will be evenly divisible by p.
    //! Small Primes: 2 3 5 7 11 13 17 19 23 29 31 37 41 43 47 53 59 61 67 71
    //! http://en.wikipedia.org/wiki/Primality_test
    //! @note this can be VERY slow for big numbers.

    if (!isOdd()) return (*this == 2);  // do the easy test first. Even numbers are never prime.

    ASSERT(!isZero());  // zero is not a valid test.

    cUInt64 pminus1(*this);
    pminus1 -= 1;
    if (pminus1.isZero()) return false;  // 1 is not prime.

    const BIT_ENUM_t nBits = get_Highest1Bit() - 1;  // test number is less than the prime.
    ITERATE_t nTries = nBits;                        // this ought to be enough tries to make sure it is prime.
    if (nTries < 10) nTries = 10;
    if (nTries > 100) nTries = 100;

    while (--nTries != 0) {
        cUInt64 x;
        x.SetRandomBits(nBits);  // random test number less than the prime. since it is not div 2, >= div 3
        ASSERT(x < *this);
        if (x.isZero()) continue;  // not a useful test.
        cUInt64 r;
        r.SetPowerMod(x, pminus1, *this);
        if (r != 1) return false;  // Not prime.
    }

    return true;  // Seems to be prime.
}

int cUInt64::SetRandomPrime(BIT_ENUM_t nBits, cThreadState* pCancel) {
    //! This function generates/finds/guesses a random prime.
    //! @return Number of tries to get a prime.

    ASSERT(nBits < 64);
    ASSERT(nBits > 1);
    SetRandomBits(nBits);
    SetBit(0);          // make odd. even numbers are not prime of course.
    SetBit(nBits - 1);  // MUST be large.
    ASSERT(isOdd());

    int iTries = 0;
    if (pCancel != nullptr) {
        // break this into n threads to utilize more CPU power. this is just one thread.
        for (;; iTries++) {
            if (pCancel->isThreadStopping()) return -1;
            if (isPrime()) break;
            *this += 2;  // Try next odd number.
        }
    } else {
        for (; !isPrime(); iTries++) *this += 2;  // Try next odd number.
    }

    ASSERT(isPrime());                         // double check
    ASSERT(this->get_Highest1Bit() <= nBits);  // must not grow.
    return iTries;
}

void cUInt64::OpBitShiftLeft1(UNIT_t nBitMask) {
#ifdef USE_INT64
    const UNIT_t nTmp = _nVal;
    const UNIT_t nCarryBit = nTmp >> (k_UNIT_BITS - 1);
    ASSERT(nCarryBit == 0 || nCarryBit == 1);
    UNREFERENCED_PARAMETER(nCarryBit);
    _nVal = ((nTmp << 1) | nBitMask);
#else
    // _addcarry_u64
    ASSERT(0);  // TODO
#endif
}

void GRAYCALL cUInt64::Divide(const cUInt64& dividend, const cUInt64& divisor, OUT cUInt64& quotient, OUT cUInt64& remainder) {
    //! Division with remainder
    //! get quotient,remainder for dividend/divisor.

    if (&dividend == &quotient || &dividend == &remainder || &divisor == &quotient || &divisor == &remainder) {
        // ThrowUserException( "cUInt64 Divide Cannot write quotient and remainder into the same variable." );
        return;
    }
    if (divisor.isZero()) {
        // ThrowUserException( "cUInt64 Divide by zero." );
        return;
    }
    if (dividend.isZero()) {
        quotient = (UNIT_t)0;
        remainder = (UNIT_t)0;
        return;
    }

    // dividend at all.
    if (dividend < divisor) {
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
    UNIT_t nBlkTmp = dividend._nVal;
    while (nBits-- != 0) {
        remainder.OpBitShiftLeft1((nBlkTmp >> nBits) & 1);
        if (divisor <= remainder) {
            quotient.OpBitShiftLeft1(1);
            remainder -= divisor;
        } else {
            quotient.OpBitShiftLeft1(0);
        }
    }
#else
    ASSERT(0);
#endif
    ASSERT((quotient * divisor + remainder) == dividend);  // test reverse operation.
}

void GRAYCALL cUInt64::EuclideanAlgorithm(const cUInt64& x, const cUInt64& y, OUT cUInt64& a, OUT cUInt64& b, OUT cUInt64& g) {  // static
    //! This function uses the Euclidean algorithm to find the greatest common divisor
    //! g of the positive integers x and y and also two integers a and b such that
    //! ax - by = g, 1 <= a <= y and 0 <= b < x.
    //!
    //! This function will fail in undefined ways if either x or y is zero.
#ifdef USE_INT64
    if (x <= y) {
        cUInt64 q;
        cUInt64 r;
        Divide(y, x, q, r);
        if (r == 0) {
            a = 1;
            b = (UNIT_t)0;
            g = x;
        } else {
            cUInt64 ap;
            EuclideanAlgorithm(x, r, ap, b, g);
            // a = ap + b * q;
            a = b;
            a *= q;
            a += ap;
        }
    } else {
        cUInt64 ap;
        cUInt64 bp;
        EuclideanAlgorithm(y, x, bp, ap, g);
        // a = y - ap;
        a = y;
        a -= ap;
        // b = x - bp;
        b = x;
        b -= bp;
    }
#else
    ASSERT(0);  // TODO
#endif
}
}  // namespace Gray
