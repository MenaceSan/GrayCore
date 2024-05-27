//! @file cValT.h
//! templates for comparing, swapping values of any type.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cValT_H
#define _INC_cValT_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"
#include <type_traits>  // std::is_pointer, std::is_reference, std::addressof
#include <utility>      // std::move

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

// detect overload. https://stackoverflow.com/questions/49429546/check-whether-an-operator-is-overloaded-in-c and https://www.cppstories.com/2019/07/detect-overload-from-chars/
template <class T>
using equal_to_t = decltype(std::declval<T>() == std::declval<T>());
template <class T>
using less_than_t = decltype(std::declval<T>() < std::declval<T>());

template <class TYPE>
static constexpr bool has_equal_to() {
    return ::experimental::is_detected<equal_to_t, TYPE>::value;
}
template <class TYPE>
static constexpr bool has_less_than() {
    return ::experimental::is_detected<less_than_t, TYPE>::value;
}

/// <summary>
/// General return type from a compare.
/// Similar to _WIN#2 VARCMP_GT.
/// Similar to the C++ (Three-way comparison, spaceship operator) https://en.cppreference.com/w/cpp/language/operator_comparison
/// </summary>
typedef int COMPARE_t;  /// result of "Three-way" compare. 0=same, 1=a>b, -1=a<b, COMPRET_t
enum COMPRET_t {
    COMPARE_Less = -1,    /// like VARCMP_LT
    COMPARE_Equal = 0,    /// like VARCMP_EQ
    COMPARE_Greater = 1,  /// like VARCMP_GT
};

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
            return a == b;
        else
            return ::memcmp(&a, &b, sizeof(TYPE)) == 0;
    }
    template <class TYPE>
    static inline bool IsEqual(const TYPE* a, const TYPE* b) noexcept {
        if (a == nullptr) return b == nullptr;
        if (b == nullptr) return false;
        return IsEqual(*a, *b);
    }

    /// <summary>
    /// use the native or overloaded operator for -lt- else default to cMem? like: std::less()
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    template <class TYPE>
    static constexpr bool IsLess(const TYPE& a, const TYPE& b) {
        if constexpr (has_less_than<TYPE>())
            return a < b;
        else
            return ::memcmp(&a, &b, sizeof(TYPE)) < 0;
    }

    /// <summary>
    /// Generic/default compare 2 TYPE values. Similar to the C++ (Three-way comparison, spaceship operator) https://en.cppreference.com/w/cpp/language/operator_comparison
    /// Overload this template for any specific TYPE Compare. ASSUME support operators -gt- ==.
    /// Similar to .NET IComparable but for any types.
    /// @note cMem::Compare() is a backwards numeric compare for USE_LITTLE_ENDIAN (Intel) machines.
    /// @note we need this because INT_MAX-INT_MIN is not positive !!! (and 0-0xFFFFFFFF is not negative)
    /// </summary>
    /// <returns>COMPARE_t 0=COMPARE_Equal.</returns>
    template <class TYPE>
    static inline COMPARE_t Compare(const TYPE& a, const TYPE& b) noexcept {
        if (IsEqual(a, b)) return COMPARE_Equal;  // is equal. 0. assume TYPE has -eq- operator.
        if (IsLess(a, b)) return COMPARE_Less;    // is less than. assume TYPE has -lt- operator.
        return COMPARE_Greater;                   // else must be greater than.
    }

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
    /// swap 2 values. similar to cMem::Swap() but uses the intrinsic = operator.
    /// dangerous for complex struct that has pointers and such. may not do a 'deep' copy.
    /// assume TYPE has a safe overloaded = operator. like std::swap()
    /// Overload this template for any specific TYPE Swaps.
    /// </summary>
    template <class TYPE>
    static inline void Swap(TYPE& a, TYPE& b) noexcept {
        TYPE tmp = std::move(a);  // use the std::move operator.
        a = std::move(b);
        b = std::move(tmp);
    }
};
}  // namespace Gray
#endif
