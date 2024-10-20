//! @file cTypes.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cTypes_H
#define _INC_cTypes_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"
#include <math.h>  // isnan()

namespace Gray {
/// <summary>
/// Bitmask to describe a native data type. used for type metadata. Fit in BYTE.
/// </summary>
enum CTYPE_FLAG_TYPE_ : BYTE {
    CTYPE_FLAG_Unsigned = 0x01,
    CTYPE_FLAG_Numeric = 0x01,    /// A numeric value of some sort. (maybe time, float or int)
    CTYPE_FLAG_NumSigned = 0x02,  /// a signed numeric value. float or int. CTYPE_FLAG_Numeric
    CTYPE_FLAG_Float = 0x04,      /// Floating point. double or float. also CTYPE_FLAG_NumSigned|CTYPE_FLAG_Numeric
    CTYPE_FLAG_Time = 0x08,       /// Number represents a time. number of time units from some epoch. CTYPE_FLAG_Numeric
    CTYPE_FLAG_Array = 0x10,      /// An array of stuff.
    CTYPE_FLAG_Alloc = 0x20,      /// Contains pointer to allocated memory. variable length? Blob? else const or static.
    CTYPE_FLAG_StringA = 0x40,    /// UTF8 format string.
    CTYPE_FLAG_StringW = 0x80,    /// UNICODE format string.
    CTYPE_FLAG_UNUSED = 0xFF,     /// This type is just a placeholder. don't use it.
};

// like _WIN32 MAKELPARAM(), MAKELONG() and MAKEWORD()
#define MAKEDWORD(low, high) ((UINT32)(((WORD)(low)) | (((UINT32)((WORD)(high))) << 16)))

#pragma pack(push, 1)
/// <summary>
/// What types can fit inside 16 bits? MAKEWORD(l,h)
/// 16 bit union. size = 2 bytes
/// This depends on USE_LITTLE_ENDIAN of course.
/// </summary>
union CATTR_PACKED cUnion16 {
    BYTE u_b[2];
    char u_c[2];
    WORD u_w;  /// 16 bit words
    short u_s;
    operator WORD() const noexcept {
        return u_w;
    }
    void operator=(WORD w) noexcept {
        u_w = w;
    }
    struct {
#ifdef USE_LITTLE_ENDIAN
        BYTE _Lo;  // LowPart
        BYTE _Hi;  // HighPart
#else
        BYTE _Hi;
        BYTE _Lo;
#endif
    } u2;
};

/// <summary>
/// What types can fit inside 32 bits? MAKEDWORD(low, high)
/// 32 bit union. size = 4 bytes
/// This depends on USE_LITTLE_ENDIAN of course.
/// Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
/// </summary>
union CATTR_PACKED cUnion32 {
    BYTE u_b[4];
    char u_c[4];
    signed char u_sc[4];
    WORD u_w[2];   /// 16 bit unsigned words
    short u_s[2];  /// 16 bit signed words
    UINT32 u_dw;   /// 32 bit unsigned
    float u_f;     /// 32 bit float.

    operator UINT32() const noexcept {
        return u_dw;
    }
    void operator=(UINT32 dw) noexcept {
        u_dw = dw;
    }

    struct {
#ifdef USE_LITTLE_ENDIAN
        cUnion16 _Lo;  // LowPart
        cUnion16 _Hi;  // HighPart
#else
        cUnion16 _Hi;
        cUnion16 _Lo;
#endif
    } u2;
};

/// <summary>
/// What types can fit inside 64 bits?
/// 64 bit union. Assumes alignment if anyone cares. size = 8 bytes
/// similar to _WIN32 LARGE_INTEGER union. or cUInt64
/// 2 * cUnion32
/// This depends on USE_LITTLE_ENDIAN of course.
/// Warning in __GNUC__ reinterpret_ warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
/// </summary>
union CATTR_PACKED cUnion64 {
    BYTE u_b[8];  /// Map to bytes.
    char u_c[8];
    signed char u_sc[8];
    WORD u_w[4];     /// 16 bit words
    short u_s[4];    /// 16 bit words
    UINT32 u_dw[2];  /// HighPart=1, LowPart=0 we assume for USE_LITTLE_ENDIAN.
    float u_f[2];    /// 2 * 32 bit floats.
    double u_d;      /// assumed to be 64 bits.

#ifdef USE_INT64
    UINT64 u_qw;  /// 64 bits = QuadPart = ULONGLONG.
    INT64 u_iq;   /// 64 bits = QuadPart = LONGLONG.
    operator UINT64() const noexcept {
        return u_qw;
    }
    void operator=(UINT64 qw) noexcept {
        u_qw = qw;
    }
#endif

    struct {
#ifdef USE_LITTLE_ENDIAN
        cUnion32 _Lo;  // LowPart
        cUnion32 _Hi;  // HighPart
#else
        cUnion32 _Hi;
        cUnion32 _Lo;
#endif
    } u2;
};

// __m128

#pragma pack(pop)

/// <summary>
/// Meta data for Numeric types (limits) for each basic type. store const values
///  Similar to std::numeric_limits<T>::max(), or INT_MAX
/// Use StrNum::toValue<TYPE>(const char* pszInp); and StrNum::ValueToA<T>(_TYPE val, ... );
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <typename TYPE = int>
struct cTypeLimit {                 // static
    [[nodiscard]] static constexpr TYPE Min() noexcept; /// Min value TYPE can represent. negative if signed type. NOT EPSILON (near zero). e.g. INT_MIN, -FLT_MAX
    [[nodiscard]] static constexpr TYPE Max() noexcept; /// Max positive value. Can equal this value. inclusive. AKA INT_MAX, UINT_MAX, FLT_MAX, DBL_MAX, DWORD_MAX, SHRT_MAX, _UI16_MAX, _UI32_MAX
    [[nodiscard]] static constexpr BYTE TypeFlags() noexcept;   /// CTYPE_FLAG_TYPE_ bits for float, signed, etc ?
};

#define CTYPE_DEF(a, _TYPE, c, d, e, f, g, h)  template <> struct cTypeLimit<_TYPE> { \
    [[nodiscard]] static constexpr _TYPE Min() noexcept { return e; } \
    [[nodiscard]] static constexpr _TYPE Max() noexcept { return f; } \
    [[nodiscard]] static constexpr BYTE TypeFlags() noexcept { return c; } \
}; 
#include "cTypes.tbl"
#undef CTYPE_DEF

}  // namespace Gray
#endif
