//! @file cUInt64.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cUInt64_H
#define _INC_cUInt64_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrArg.h"
#include "StrBuilder.h"
#include "cBits.h"
#include "cString.h"
#include "cTypes.h"

namespace Gray {
class cThreadState;

#pragma pack(push, 1)
/// <summary>
/// emulate 64 bit unsigned integer as a native type for systems that don't actually support this. QWord
/// if ! USE_INT64 then ASSUME we DONT support 64 bit int types natively. Assume all platforms support 32 bit types.
/// we don't support __int64 (_MSC_VER) or int64_t (C99/GCC standard) native.
/// like cUnion64 or LARGE_INTEGER as a native type for operators.
/// same size as UINT64
/// use typedef cUInt64 UINT64; if ! USE_INT64
/// /// </summary>
class GRAYCORE_LINK CATTR_PACKED cUInt64 {
 public:
#ifdef USE_INT64  // native support.
    typedef UINT64 UNIT_t;
    static const unsigned k_UNIT_BITS = 64;
#else
    typedef UINT32 UNIT_t;  // break into 2 parts.
    static const unsigned k_UNIT_BITS = 32;
#endif

 private:
    // don't use cUnion64 so we can use CUINT64(h,l) for init.
#ifdef USE_INT64  // native support for UINT 64 bit.
    UNIT_t _nVal = 0;
#elif defined(USE_LITTLE_ENDIAN)
    UNIT_t _nLo = 0;
    UNIT_t _nHi = 0;
#else
    UNIT_t _nHi = 0;
    UNIT_t _nLo = 0;
#endif

 public:
    cUInt64() {}
    cUInt64(UNIT_t n)
#ifdef USE_INT64
        : _nVal(n)
#else
        : _nLo(n)
#endif
    {
    }
    cUInt64(const char* pszVal, RADIX_t n = 10) {
        SetStr(StrT::ToSpanStr(pszVal), n);
    }

    // Test Operators
    bool isZero() const {
#ifdef USE_INT64
        return _nVal == 0;
#else
        return _nLo == 0 && _nHi == 0;
#endif
    }
    bool isOdd() const {
#ifdef USE_INT64
        return _nVal & 1;
#else
        return _nLo & 1;
#endif
    }
    /// <summary>
    /// Gets the state of the enumerated bit. which has value 2^nBit.
    /// Bits beyond range are considered to be 0.
    /// </summary>
    bool IsSet(BIT_ENUM_t nBit) const {
#ifdef USE_INT64
        return cBits::IsSet(_nVal, nBit);
#else
        if (nBit < k_UNIT_BITS)
            return cBits::IsSet(_nLo, nBit);
        else
            return cBits::IsSet(_nHi, nBit - k_UNIT_BITS);
#endif
    }
    bool operator==(const cUInt64& n) const {
#ifdef USE_INT64
        return _nVal == n._nVal;
#else
        return _nLo == n._nLo && _nHi == n._nHi;
#endif
    }
    bool operator!=(const cUInt64& n) const {
        return !(*this == n);
    }
    bool operator==(UNIT_t n) const {
#ifdef USE_INT64
        return _nVal == n;
#else
        return _nLo == n;
#endif
    }
    bool operator>(const cUInt64& n) const {
#ifdef USE_INT64
        return _nVal > n._nVal;
#else
        if (_nHi == n._nHi)
            return _nLo > n._nLo;
        else
            return _nHi > n._nHi;
#endif
    }

    bool operator<(const cUInt64& n) const {
#ifdef USE_INT64
        return _nVal < n._nVal;
#else
        if (_nHi == n._nHi)
            return _nLo < n._nLo;
        else
            return _nHi < n._nHi;
#endif
    }

    bool operator<=(const cUInt64& n) const {
#ifdef USE_INT64
        return _nVal <= n._nVal;
#else
        if (_nHi == n._nHi)
            return _nLo <= n._nLo;
        else
            return _nHi < n._nHi;
#endif
    }

    template <typename TYPE>
    TYPE get_Val() const {
        //! just default to assume TYPE is unsigned.
#ifdef USE_INT64
        return (TYPE)_nVal;
#else
        ASSERT(0);
        return 0;
#endif
    }

    // Math Action Operators
    void operator++() {
#ifdef USE_INT64
        _nVal++;
#else
        const UNIT_t n = _nLo;
        _nLo++;
        if (_nLo < n)  // carry bit.
            _nHi++;
#endif
    }
    void operator++(int) {
        //! Post fix increment: same as prefix
        ++*this;
    }

    void operator--() {
#ifdef USE_INT64
        _nVal--;
#else
        const UNIT_t n = _nLo;
        _nLo--;
        if (_nLo > n)  // carry bit.
            _nHi--;
#endif
    }
    void operator--(int) {
        //! Postfix decrement: same as prefix
        --*this;
    }

    cUInt64& operator+=(const cUInt64& n) {
#ifdef USE_INT64
        _nVal += n._nVal;
#else
        _nLo += n._nLo;
        if (_nLo < n._nLo)  // carry bit.
            _nHi++;
        _nHi += n._nHi;
#endif
        return *this;
    }
    cUInt64& operator-=(const cUInt64& n) {
#ifdef USE_INT64
        _nVal -= n._nVal;
#else
        ASSERT(0);
#endif
        return *this;
    }
    cUInt64& operator*=(const cUInt64& x) {
        cUInt64 ans;
#ifdef USE_INT64
        _nVal *= x._nVal;
#else
        ASSERT(0);
#endif
        return *this;
    }
    cUInt64 operator*(const cUInt64& x) const {
        cUInt64 ans;
#ifdef USE_INT64
        ans._nVal = _nVal * x._nVal;
#else
        ASSERT(0);
#endif
        return ans;
    }
    void operator%=(const cUInt64& x) {
        //! Modulus *this by x.
#ifdef USE_INT64
        _nVal %= x._nVal;
#else
        ASSERT(0);
#endif
    }

    // Bit Action Operators
    void SetBit(BIT_ENUM_t uiBit) {
#ifdef USE_INT64
        _nVal |= ((UNIT_t)1) << uiBit;
#else
        if (uiBit < k_UNIT_BITS) {
            _nLo |= ((UNIT_t)1) << uiBit;
        } else {
            _nHi |= ((UNIT_t)1) << (uiBit - k_UNIT_BITS);
        }
#endif
    }

    cUInt64& operator|=(const cUInt64& n) {
#ifdef USE_INT64
        _nVal |= n._nVal;
#else
        _nLo |= n._nLo;
        _nHi |= n._nHi;
#endif
        return *this;
    }

    cUInt64& operator&=(const cUInt64& n) {
#ifdef USE_INT64
        _nVal &= n._nVal;
#else
        _nLo &= n._nLo;
        _nHi &= n._nHi;
#endif
        return *this;
    }

    cUInt64& operator^=(const cUInt64& n) {
#ifdef USE_INT64
        _nVal ^= n._nVal;
#else
        _nLo ^= n._nLo;
        _nHi ^= n._nHi;
#endif
        return *this;
    }

    cUInt64& operator<<=(BIT_ENUM_t uiBits) {
#ifdef USE_INT64
        _nVal <<= uiBits;
#else
        if (uiBits < k_UNIT_BITS) {
            (_nHi <<= uiBits) |= (_nLo >> (k_UNIT_BITS - uiBits));
            _nLo <<= uiBits;
        } else {
            _nHi = _nLo << (uiBits - k_UNIT_BITS);
            _nLo = 0;
        }
#endif
        return *this;
    }

    cUInt64& operator>>=(BIT_ENUM_t uiBits) {
#ifdef USE_INT64
        _nVal >>= uiBits;
#else
        if (uiBits < k_UNIT_BITS) {
            (_nLo >>= uiBits) |= (_nHi << (k_UNIT_BITS - uiBits));
            _nHi >>= uiBits;
        } else {
            _nLo = _nHi >> (uiBits - k_UNIT_BITS);
            _nHi = 0;
        }
#endif
        return *this;
    }

    void BuildStr(StrBuilder<char>& sb, RADIX_t nRadixBase = 10) const;
    cString GetStr(RADIX_t nRadixBase = 10) const;
    HRESULT SetStr(const cSpan<char>& src, RADIX_t nRadixBase = 10);

    BIT_ENUM_t get_Highest1Bit() const;
    HRESULT SetRandomBits(BIT_ENUM_t nBits);
    void SetPowerMod(const cUInt64& base, const cUInt64& exponent, const cUInt64& modulus);

    bool isPrime() const;
    int SetRandomPrime(BIT_ENUM_t nBits, cThreadState* pCancel = nullptr);
    void OpBitShiftLeft1(UNIT_t nBitMask);

    static void GRAYCALL Divide(const cUInt64& dividend, const cUInt64& divisor, OUT cUInt64& quotient, OUT cUInt64& remainder);
    static void GRAYCALL EuclideanAlgorithm(const cUInt64& x, const cUInt64& y, OUT cUInt64& a, OUT cUInt64& b, OUT cUInt64& g);
};

#pragma pack(pop)

inline cUInt64 operator+(const cUInt64& roUI64_1, const cUInt64& roUI64_2) {
    cUInt64 temp = roUI64_1;
    temp += roUI64_2;
    return temp;
}

inline cUInt64 operator|(const cUInt64& roUI64_1, const cUInt64& roUI64_2) {
    cUInt64 temp = roUI64_1;
    temp |= roUI64_2;
    return temp;
}

inline cUInt64 operator&(const cUInt64& roUI64_1, const cUInt64& roUI64_2) {
    cUInt64 temp = roUI64_1;
    temp &= roUI64_2;
    return temp;
}

inline cUInt64 operator^(const cUInt64& roUI64_1, const cUInt64& roUI64_2) {
    cUInt64 temp = roUI64_1;
    temp ^= roUI64_2;
    return temp;
}

inline cUInt64 operator<<(const cUInt64& n, BIT_ENUM_t uiBits) {
    cUInt64 temp = n;
    temp <<= uiBits;
    return temp;
}

inline cUInt64 operator>>(const cUInt64& n, BIT_ENUM_t uiBits) {
    cUInt64 temp = n;
    temp >>= uiBits;
    return temp;
}
}  // namespace Gray
#endif
