//
//! @file PtrCast.h
//! A pointer to some struct or class. Not used for pointers to basic/intrinsic types.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_PtrCast_H
#define _INC_PtrCast_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"

namespace Gray {
/// <summary>
/// like ULongToPtr(). NOT ULongToHandle()
/// </summary>
template <typename T = void>
static constexpr T* CastNumToPtr(UINT_PTR n) noexcept {
    return (T*)(void*)n;
}

/// <summary>
/// Cast a pointer to a number big enough to hold the value.
/// like PtrToUlong() or HandleToULong(). UINT_PTR, or size_t ? NOT ptrdiff_t, INT_PTR (signed)
/// </summary>
/// <param name="p"></param>
/// <returns></returns>
constexpr UINT_PTR PtrCastToNum(const VOLATILE void* p) noexcept {
    return (UINT_PTR)p;
}

/// <summary>
/// Cast void pointer to a type. Ignore C++ warnings.
/// put C26493 - "don't use c-style casts". warning in one place.
/// </summary>
template <typename T>
static constexpr const T* PtrCast(const void* p) noexcept {  // VOLATILE?
    return static_cast<const T*>(p);
    // return (const T*)p;
}
/// <summary>
/// Cast void pointer to a type. Ignore C++ warnings.
/// put C26493 - "don't use c-style casts". warning in one place.
/// </summary>
template <typename T>
static constexpr T* PtrCast(void* p) noexcept {  // VOLATILE?
    return static_cast<T*>(p);
    // return (T*)p;
}

#ifdef _CPPRTTI
#define DYNPTR_CAST(t, p) (dynamic_cast<t*>(p))  // safer down casting.

/// <summary>
/// Is it this top Type? like MFC IsKindOf(), std::is_pointer()
/// </summary>
/// <typeparam name="_TYPE">Is it this top Type?</typeparam>
/// <typeparam name="_TYPE_FROM">Source Type</typeparam>
/// <param name="p">pointer to test</param>
/// <returns></returns>
template <class _TYPE, class _TYPE_FROM>
bool IsTopType(_TYPE_FROM* p) noexcept {
    if (p == nullptr) return false;
    return typeid(*p) == typeid(_TYPE);
}

/// <summary>
/// will this dynamic_cast work ? istypeof() ? like std::is_base_of()
/// </summary>
template <class _TYPE, class _TYPE_FROM>
inline bool IsValidCast(_TYPE_FROM* p) noexcept {
    if (p == nullptr)  // nullptr is always castable to nullptr.
        return true;
    _TYPE* p2 = dynamic_cast<_TYPE*>(p);
    return p2 != nullptr;
}
#else
#define DYNPTR_CAST(t, p) (static_cast<t*>(p))  // dynamic_cast isn't available ?
//! @todo no _CPPRTTI so use cObject for this like MFC IsKindOf() does ?
template <class _TYPE, class _TYPE_FROM>
bool IsTopType(_TYPE_FROM* p) noexcept {
    return true;
}
#endif

// https://stackoverflow.com/questions/1785426/c-sharp-null-coalescing-operator-equivalent-for-c, Similar to C# ?? operator.
// beware a might be evaluated twice ? // dont evaluate b if a is nullptr. So leave this as macro. null coalesce.
#define SAFE_PROPN(a, b) (((a) != nullptr) ? ((a)->b) : nullptr)
#define SAFE_PROP(a, b, c) (((a) != nullptr) ? ((a)->b) : (c))  // AKA NULL_PROP() ?

/// <summary>
/// like static_cast() but with some extra checking. // dynamic_cast for DEBUG only.
/// up-casting can be dangerous. We assume it is safe in this case but check it anyhow.
/// a static_cast that gets verified in _DEBUG mode. fast normal static_cast in release mode.
/// nullptr is valid.
/// </summary>
template <class _TYPE, class _TYPE_FROM>
inline _TYPE* PtrCastCheck(_TYPE_FROM* p) {
#ifdef _DEBUG
    ASSERT(IsValidCast<_TYPE>(p));
#endif
    return static_cast<_TYPE*>(p);
}
}  // namespace Gray
#endif  // _INC_PtrCast_H
