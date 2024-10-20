//! @file PtrCast.h
//! A pointer to some struct or class. Not used for pointers to basic/intrinsic types.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_PtrCast_H
#define _INC_PtrCast_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"

#ifndef UNDER_CE
#include <typeinfo>  // type_info& typeid(class type) std::
#endif

namespace Gray {

    // TODO PtrNN<T> // like not_null<T> for M$

/// <summary>
/// Cast const void pointer to a const type*. Ignore C++ warnings.
/// put C26493 - "don't use c-style casts". warning in one place.
/// @note: DANGER. use static_cast or dynamic_cast on complex casting.
/// </summary>
template <typename T>
constexpr const T* PtrCast(const void* p) {  // VOLATILE?
    // return reinterpret_cast<const T*>(p);
    return (const T*)p;
}

/// <summary>
/// Cast void pointer to a type. Ignore C++ warnings.
/// put C26493 - "don't use c-style casts". warning in one place.
/// @note: DANGER. use static_cast or dynamic_cast on complex casting.
/// </summary>
template <typename T>
constexpr T* PtrCast(void* p) {  // VOLATILE?
    // return reinterpret_cast<T*>(p);
    return (T*)p;
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
    if (p == nullptr) return true;  // nullptr is always castable to nullptr.
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

/// <summary>
/// Cast a pointer to a number big enough to hold the value.
/// like PtrToUlong(), PtrToInt or HandleToULong(). UINT_PTR, or size_t ? NOT ptrdiff_t, INT_PTR (signed)
/// </summary>
/// <param name="p"></param>
/// <returns></returns>
constexpr UINT_PTR CastPtrToNum(const void* p) {  // VOLATILE?
    return (UINT_PTR)p;
}
constexpr UINT_PTR CastPtrToNumV(const VOLATILE void* p) {  // VOLATILE?
    return (UINT_PTR)p;
}

/// <summary>
/// like ULongToPtr(). NOT ULongToHandle()
/// NOTE: Gcc does not allow constexpr !!!! https://stackoverflow.com/questions/54174694/reinterpret-castvolatile-uint8-t37-is-not-a-constant-expression
/// </summary>
inline void* CastNumToPtr(UINT_PTR n) {
    return (void*)n;
}
template <typename T = void>
inline T* CastNumToPtrT(UINT_PTR n) {
    return PtrCast<T>(CastNumToPtr(n));
}

}  // namespace Gray
#endif  // _INC_PtrCast_H
