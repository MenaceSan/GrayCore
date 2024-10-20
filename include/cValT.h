//! @file cValT.h
//! templates for comparing, swapping values of any type.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cValT_H
#define _INC_cValT_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cMem.h"
#include <type_traits>  // std::is_pointer, std::is_reference, std::addressof

#if defined(_MSC_VER)
namespace experimental {
template <class AlwaysVoid, template <class...> class Op, class... Args>
struct detector {
    using value_t = std::false_type;
};
template <template <class...> class Op, class... Args>
struct detector<std::void_t<Op<Args...>>, Op, Args...> {
    using value_t = std::true_type;
};
template <template <class...> class Op, class... Args>
using is_detected = typename detector<void, Op, Args...>::value_t;
}  // namespace experimental
#else
#include <experimental/type_traits>
#endif

namespace Gray {

// detect operator overload. https://stackoverflow.com/questions/49429546/check-whether-an-operator-is-overloaded-in-c and https://www.cppstories.com/2019/07/detect-overload-from-chars/
template <class T>
using equal_to_t = decltype(std::declval<T>() == std::declval<T>());
template <class T>
using less_than_t = decltype(std::declval<T>() < std::declval<T>());

// Does TYPE have == operator?
template <class TYPE>
static constexpr bool has_equal_to() {
    return ::experimental::is_detected<equal_to_t, TYPE>::value;
}
template <class TYPE>
static constexpr bool has_less_than() {
    return ::experimental::is_detected<less_than_t, TYPE>::value;
}

// @todo check for method support for bool IsEqual() const; or COMPARE_t Compare() ??

/// Is exactly the same object?
/// Similar to JavaScript ===. https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Strict_equality
template <class TYPE>
static inline bool IsEqual3(TYPE r1, TYPE r2) noexcept {
    if constexpr (std::is_reference<TYPE>())
        return std::addressof(r1) == std::addressof(r2);
    else
        return r1 == r2;  // scalar values or std::is_pointer
}

/// <summary>
/// Helper functions for an arbitrary value/object type in memory. We may compare these.
/// Deal with it as an array of bytes.
/// Similar to System.IComparable in .NET
/// </summary>
struct GRAYCORE_LINK cValT {  // static. Value/Object of some type in memory.
    /// <summary>
    /// Implement cheapest possible compare for equality only. default to cMem::Compare() if no native or overloaded operator for == ?
    /// Like: std::equal_to()
    /// </summary>
    template <class TYPE>
    static constexpr bool IsEqual(const TYPE& a, const TYPE& b) {
        if constexpr (has_equal_to<TYPE>())
            return a == b;  // type has == operator.
        else
            return cMem::Compare(&a, &b, sizeof(TYPE)) == COMPARE_Equal; 
    }
    template <class TYPE>
    static inline bool IsEqual(const TYPE* a, const TYPE* b) noexcept {
        if (a == nullptr) return b == nullptr;
        if (b == nullptr) return false;
        return IsEqual(*a, *b);
    }

    /// <summary>
    /// use the native or overloaded operator for -lt- else default to cMem. like: std::less()
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    template <class TYPE>
    static constexpr bool IsLess(const TYPE& a, const TYPE& b) {
        if constexpr (has_less_than<TYPE>())
            return a < b;
        else
            return cMem::Compare(&a, &b, sizeof(TYPE)) < 0;
    }

    /// <summary>
    /// Generic/default compare 2 TYPE values.
    /// Similar to the C++ (Three-way comparison, spaceship operator) https://en.cppreference.com/w/cpp/language/operator_comparison
    /// Overload this template for any specific TYPE Compare. ASSUME support operators -gt- ==.
    /// Similar to .NET IComparable but for any types.
    /// </summary>
    /// <returns>COMPARE_t 0=COMPARE_Equal.</returns>
    template <class TYPE>
    static inline COMPARE_t Compare(const TYPE& a, const TYPE& b) noexcept {
        if constexpr (!has_equal_to<TYPE>() || !has_less_than<TYPE>()) {
            return cMem::Compare(&a, &b, sizeof(TYPE));
        } else {
            if (IsEqual(a, b)) return COMPARE_Equal;  // is equal. 0. assume TYPE has -eq- operator.
            if (IsLess(a, b)) return COMPARE_Less;    // is less than. assume TYPE has -lt- operator.
            return COMPARE_Greater;                   // else must be greater than.
        }
    }

    /// <summary>
    /// Pointer compare treated special.
    /// @note cMem::Compare() is a backwards numeric compare for USE_LITTLE_ENDIAN (Intel) machines.
    /// @note we need this because INT_MAX-INT_MIN is not positive !!! (and 0-0xFFFFFFFF is not negative)
    /// </summary>
    template <class TYPE>
    static inline COMPARE_t Compare(const TYPE* a, const TYPE* b) noexcept {
        if (a == nullptr) {
            if (b == nullptr) return COMPARE_Equal;
            return COMPARE_Less;
        }
        if (b == nullptr) return COMPARE_Greater;
        return Compare(*a, *b);
    }

    /// <summary>
    /// Replace MAX() macro
    /// like #include 'xutility' or "#include 'minmax.h' ?
    /// </summary>
    /// <returns>larger of 2 values</returns>
    template <class TYPE>
    constexpr static TYPE Max(const TYPE& a, const TYPE& b) noexcept {
        return IsLess(a, b) ? b : a;
    }
    /// <summary>
    /// Replace MIN() macro.
    /// </summary>
    /// <returns>smaller of 2 values</returns>
    template <class TYPE>
    constexpr static TYPE Min(const TYPE& a, const TYPE& b) noexcept {
        return IsLess(a, b) ? a : b;
    }

    /// <summary>
    /// Replace ABS(n) macro.
    /// no conflict with 'math.h' abs()
    /// Does nothing for unsigned types of course.
    /// </summary>
    /// <returns>absolute value</returns>
    template <class TYPE>
    constexpr static TYPE Abs(const TYPE& n) noexcept {
        return n < 0 ? (-n) : n;
    }

    /// <summary>
    /// Reverse the byte order in an intrinsic type. default impl.
    /// Like __GNUC__ __builtin_bswap16(), __builtin_bswap32, etc
    /// Similar to: ntohl(), htonl(), ntohs(), htons().
    /// </summary>
    template <class TYPE>
    static constexpr TYPE ReverseBytes(TYPE nVal) noexcept {
        cMem::CopyReverse(&nVal, &nVal, sizeof(nVal));
        return nVal;
    }

    /// <summary>
    /// Host byte order to network order (big endian). like htonl() htons()
    /// </summary>
    template <typename TYPE>
    static constexpr TYPE HtoN(TYPE nVal) noexcept {
#ifdef USE_LITTLE_ENDIAN
        //! Assume bytes are correct bit order but larger types are not ordered correctly.
        return ReverseBytes(nVal);
#else
        return nVal;  // no change needed.
#endif
    }

    /// <summary>
    /// Network byte order (big endian) to host order. like ntohl() ntohs().
    /// Network order = BigEndian = High order comes first = Not Intel.
    /// </summary>
    template <typename TYPE>
    static inline TYPE NtoH(TYPE nVal) noexcept {
#ifdef USE_LITTLE_ENDIAN
        //! Assume bytes are correct bit order but larger types are not ordered correctly.
        return ReverseBytes(nVal);
#else
        return nVal;  // no change needed.
#endif
    }

    /// <summary>
    /// Host byte order to little endian. (Intel)
    /// </summary>
    template <typename TYPE>
    static constexpr TYPE HtoLE(TYPE nVal) noexcept {
#ifdef USE_LITTLE_ENDIAN
        return nVal;  // no change needed.
#else
        //! Assume bytes are correct bit order but larger types are not ordered correctly.
        return ReverseBytes(nVal);
#endif
    }

    /// <summary>
    /// Little Endian (Intel) to host byte order.
    /// </summary>
    template <typename TYPE>
    static inline TYPE LEtoH(TYPE nVal) noexcept {
#ifdef USE_LITTLE_ENDIAN
        return nVal;  // no change needed.
#else
        //! Assume bytes are correct bit order but larger types are not ordered correctly.
        return ReverseBytes(nVal);
#endif
    }

    /// <summary>
    /// Get a data TYPE value from an unaligned TYPE pointer.
    /// Like the _WIN32 UNALIGNED macro.
    /// @note some architectures will crash if you try to access unaligned data. (PowerPC)
    /// In this case we need to memcpy() to a temporary buffer first.
    /// </summary>
    template <typename TYPE>
    static inline TYPE GetUnaligned(const void* pData) noexcept {
#ifdef _MSC_VER
        return *((const UNALIGNED TYPE*)pData);
#else
        return *((const TYPE*)pData);
#endif
    }

    /// <summary>
    /// Set data value from an unknown unaligned TYPE pointer.
    /// Like the _WIN32 UNALIGNED macro.
    /// @note some architectures will crash if you try to access unaligned data. (PowerPC)
    /// In this case we need to memcpy() to a temporary buffer first.
    /// </summary>
    template <typename TYPE>
    static inline void SetUnaligned(void* pData, TYPE nVal) noexcept {
#ifdef _MSC_VER
        *((UNALIGNED TYPE*)pData) = nVal;
#else
        *((TYPE*)pData) = nVal;
#endif
    }

    /// <summary>
    /// Get bytes packed as LE (Intel). ( Not "Network order" which is big endian.)
    /// </summary>
    template <typename TYPE>
    static inline TYPE GetLEtoH(const void* pData) noexcept {
        return LEtoH(GetUnaligned<TYPE>(pData));
    }
    /// <summary>
    /// Set bytes packed as LE (Intel). ( Not "Network order" which is big endian.)
    /// </summary>
    template <typename TYPE>
    static inline void SetHtoLE(void* pData, TYPE nVal) noexcept {
        return SetUnaligned(pData, HtoLE(nVal));
    }
    /// <summary>
    /// Get bytes packed as BE (Network order, Not Intel). similar to CopyNtoH()
    /// </summary>
    template <typename TYPE>
    static inline TYPE GetNtoH(const void* pData) noexcept {
        return NtoH(GetUnaligned<TYPE>(pData));
    }
    /// <summary>
    /// Set bytes packed as BE (Network order, Not Intel). similar to CopyHtoN()
    /// </summary>
    template <typename TYPE>
    static inline void SetHtoN(void* pData, TYPE nVal) noexcept {
        return SetUnaligned(pData, HtoN(nVal));
    }

    /// <summary>
    /// Get 3 (Network order, Big Endian) BYTEs as a host value.
    /// opposite of SetHtoN3()
    /// </summary>
    static inline UINT32 GetNtoH3(const BYTE* p) noexcept {
        return CastN(UINT32, p[0]) << 16 | CastN(UINT32, p[1]) << 8 | p[2];
    }

    /// <summary>
    /// Set 3 (Network order, Big Endian) BYTEs from host value.
    /// opposite of GetNtoH3()
    /// </summary>
    static inline void SetHtoN3(BYTE* p, UINT nVal) noexcept {
        p[0] = CastN(BYTE, (nVal >> 16) & 0xFF);
        p[1] = CastN(BYTE, (nVal >> 8) & 0xFF);  // HIBYTE
        p[2] = CastN(BYTE, (nVal)&0xFF);         // LOBYTE
    }
};

/// <summary>
/// Reverse the bytes in an intrinsic 16 bit type WORD. e.g. 0x1234 = 0x3412.
/// like ntohs(),htons(), MAKEWORD()
/// </summary>
/// <param name="nVal"></param>
/// <returns></returns>
template <>
constexpr WORD cValT::ReverseBytes<WORD>(WORD nVal) noexcept {  // static
#if 0                                                           // defined(_MSC_VER)
    return _byteswap_ushort(nVal);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(x);
#else
    return CastN(WORD, (nVal >> 8) | (nVal << 8));
#endif
}

/// <summary>
/// Reverse the bytes in an intrinsic 32 bit type UINT32. like ntohl(),htonl()
/// </summary>
template <>
constexpr UINT32 cValT::ReverseBytes<UINT32>(UINT32 nVal) noexcept {  // static
#if 0                                                                 // defined(_MSC_VER)
    return _byteswap_ulong(nVal);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(x);
#else
    nVal = (nVal >> 16) | (nVal << 16);
    return ((nVal & 0xff00ff00UL) >> 8) | ((nVal & 0x00ff00ffUL) << 8);
#endif
}

#ifdef USE_INT64
/// <summary>
/// Reverse the bytes in an intrinsic 64 bit type UINT64.
/// </summary>
template <>
constexpr UINT64 cValT::ReverseBytes<UINT64>(UINT64 nVal) noexcept {  // static
#if 0                                                                 // defined(_MSC_VER)
return _byteswap_uint64(nVal);
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(x);
#else
    nVal = (nVal >> 32) | (nVal << 32);
    nVal = ((nVal & 0xff00ff00ff00ff00ULL) >> 8) | ((nVal & 0x00ff00ff00ff00ffULL) << 8);
    return ((nVal & 0xffff0000ffff0000ULL) >> 16) | ((nVal & 0x0000ffff0000ffffULL) << 16);
#endif
}
#endif
#if 0 // ndef USE_LONG_AS_INT64
template <>
constexpr ULONG cValT::ReverseBytes<ULONG>(ULONG nVal) noexcept {  // static
    //! ULONG may be equiv to UINT32 or UINT64
    // return ReverseBytes<UINT64>(nVal);
    return ReverseBytes<UINT32>(nVal);
}
#endif

#if 0  // USE_LITTLE_ENDIAN
	template <>
	inline UINT32 cValT::GetNtoH<UINT32>(const void* pData)	{
		const BYTE* p = (const BYTE*)pData;
		return ((UINT32)p[0] << 24)
			| ((UINT32)p[1] << 16)
			| ((UINT32)p[2] << 8)
			| ((UINT32)p[3]);
	}
	template <>
	inline void cValT::SetHtoN<UINT32>(void* pData, UINT32 nVal) {
		BYTE* p = (BYTE*)pData;
		p[0] = (BYTE)(nVal >> 24);
		p[1] = (BYTE)(nVal >> 16);
		p[2] = (BYTE)(nVal >> 8);
		p[3] = (BYTE)(nVal);
	}

	template <>
	inline UINT64 cValT::GetNtoH<UINT64>(const void* pData)	{
		const BYTE* p = (const BYTE*)pData;
		return ((UINT64)p[0] << 56)
			| ((UINT64)p[1] << 48)
			| ((UINT64)p[2] << 40)
			| ((UINT64)p[3] << 32)
			| ((UINT64)p[4] << 24)
			| ((UINT64)p[5] << 16)
			| ((UINT64)p[6] << 8)
			| ((UINT64)p[7]);
	}
	template <>
	inline void cValT::SetHtoN<UINT64>(void* pData, UINT64 nVal) {
		BYTE* p = (BYTE*)pData;
		p[0] = (BYTE)(nVal >> 56);
		p[1] = (BYTE)(nVal >> 48);
		p[2] = (BYTE)(nVal >> 40);
		p[3] = (BYTE)(nVal >> 32);
		p[4] = (BYTE)(nVal >> 24);
		p[5] = (BYTE)(nVal >> 16);
		p[6] = (BYTE)(nVal >> 8);
		p[7] = (BYTE)(nVal);
	}
#endif

/// Create a compare operator for some structure as compare of bytes
#define DECLARE_cValT(TYPE)                                 \
    inline bool operator==(const TYPE& m2) const noexcept { \
        return cMem::IsEqual(this, &m2, sizeof(TYPE));      \
    };

}  // namespace Gray
#endif
