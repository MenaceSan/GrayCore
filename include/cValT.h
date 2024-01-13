//
//! @file cValT.h
//! templates for comparing, swapping values of any type.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cValT_H
#define _INC_cValT_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"
#include <utility>  // std::move

namespace Gray {
/// <summary>
/// General return type from a compare. Similar to _WIN#2 VARCMP_GT
/// </summary>
typedef int COMPARE_t;  /// result of compare. 0=same, 1=a>b, -1=a<b
enum COMPARE_TYPE {
    COMPARE_Less = -1,    /// VARCMP_LT
    COMPARE_Equal = 0,    /// VARCMP_EQ
    COMPARE_Greater = 1,  /// VARCMP_GT
};

/// <summary>
/// Helper functions for an arbitrary value/object type in memory. We may compare these.
/// Similar to System.IComparable in .NET
/// </summary>
struct GRAYCORE_LINK cValT {  // static. Value/Object of some type in memory.
    /// <summary>
    /// swap 2 values. similar to cMem::Swap() but uses the intrinsic = operator.
    /// dangerous for complex struct that has pointers and such. may not do a 'deep' copy.
    /// assume TYPE has a safe overloaded = operator. like std::swap()
    /// Overload this template for any specific TYPE Swaps.
    /// </summary>
    template <class TYPE>
    static inline void Swap(TYPE& a, TYPE& b) noexcept {
        TYPE tmp = std::move(a);    // use the move operator.
        a = std::move(b);
        b = std::move(tmp);
    }

    /// <summary>
    /// Generic/default compare 2 TYPE values.
    /// Overload this template for any specific TYPE Compare. ASSUME support operators > ==.
    /// Similar to .NET IComparable but for any types.
    /// @note cMem::Compare() is a backwards numeric compare for USE_LITTLE_ENDIAN (Intel) machines.
    /// @note we need this because INT_MAX-INT_MIN is not positive !!! (and 0-0xFFFFFFFF is not negative)
    /// </summary>
    /// <returns>COMPARE_t 0=COMPARE_Equal.</returns>
    template <class TYPE>
    static inline COMPARE_t Compare(const TYPE& a, const TYPE& b) noexcept {
        if (a > b) return COMPARE_Greater;  // is greater than. assume TYPE has operator.
        if (a == b) return COMPARE_Equal;   // is equal. 0. assume TYPE has operator.
        return COMPARE_Less;                // must be less than.
    }

    /// <summary>
    /// Replace MAX() macro
    /// like #include 'xutility' or "#include 'minmax.h' ?
    /// </summary>
    /// <returns>larger of 2 values</returns>
    template <class TYPE>
    constexpr static TYPE Max(const TYPE a, const TYPE b) noexcept {
        return (a > b) ? a : b;
    }
    /// <summary>
    /// replace MIN() macro.
    /// </summary>
    /// <returns>smaller of 2 values</returns>
    template <class TYPE>
    constexpr static TYPE Min(const TYPE a, const TYPE b) noexcept {
        return (a > b) ? b : a;
    }

    /// <summary>
    /// replace ABS(n) macro.
    /// no conflict with 'math.h' abs()
    /// Does nothing for unsigned types of course.
    /// </summary>
    /// <returns>absolute value</returns>
    template <class TYPE>
    constexpr static TYPE Abs(const TYPE n) noexcept {
        return (n < 0) ? (-n) : n;
    }
}; 
}  // namespace Gray
#endif
