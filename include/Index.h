//! @file Index.h
//! Difference of 2 pointers in memory.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_Index_H
#define _INC_Index_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"

namespace Gray {
#define CastN(T, N) ((T)(N))  // static_cast<T>(N)	// Cast numeric to different numeric. no clear rule on type cast for arithmetic values. T{ 1 } or T(1) or static_cast<T>(1)

typedef UINT_PTR HASHCODE_t;                /// could hold a pointer converted to a number? maybe 64 or 32 bit ? same as size_t.
typedef UINT32 HASHCODE32_t;                /// always 32 bits. consistent value. k_HASHCODE_CLEAR
constexpr HASHCODE_t k_HASHCODE_CLEAR = 0;  /// not a valid index.

#ifdef USE_64BIT
#define _SIZEOF_PTR 8  /// bytes = sizeof(void*) for __DECL_ALIGN macro. Can't do sizeof(x). uintptr_t
#define USE_FILE_POS64
#else
#define _SIZEOF_PTR 4  /// bytes = sizeof(void*) for __DECL_ALIGN macro. Can't do sizeof(x). uintptr_t
#endif

#if (_MFC_VER > 0x0600)
typedef INT_PTR ITERATE_t;  /// MFC 6 uses INT_PTR for array indexes.
#else
typedef int ITERATE_t;  /// like size_t but signed
#endif
constexpr ITERATE_t k_ITERATE_BAD = -1;
constexpr ITERATE_t k_ARG_ARRAY_MAX = 256;  // arbitrary Max number of arguments in user entered array.

typedef size_t COUNT_t;  /// like size_t but a count of things that might NOT be bytes. ASSUME unsigned. _countof(x)

#define IS_INDEX_BAD(i, q) (CastN(COUNT_t, i) >= CastN(COUNT_t, q))  /// cast the (likely) int to unsigned to check for negatives.
#define IS_INDEX_GOOD(i, q) (CastN(COUNT_t, i) < CastN(COUNT_t, q))  /// cast the (likely) int to unsigned to check for negatives.

#define IS_INDEX_BAD_ARRAY(i, a) IS_INDEX_BAD(i, _countof(a))
#define IS_INDEX_GOOD_ARRAY(i, a) IS_INDEX_GOOD(i, _countof(a))

/// <summary>
/// get diff of 2 pointers of the same type to get index diff. (NOT bytes)
/// INT_PTR is same type as intptr_t, ITERATE_t or ptrdiff_t ?
/// Assume b is an element in array a.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <param name="a"></param>
/// <param name="b"></param>
/// <returns></returns>
template <typename TYPE>
static inline ptrdiff_t GET_INDEX_IN(TYPE* a, TYPE* b) noexcept {
    return b - a;
}

#define _sizeofm(s, m) sizeof(((s*)0)->m)  /// size_t of a structure member/field (like offsetof()) nullptr

}  // namespace Gray
#endif
