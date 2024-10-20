//! @file cPair.h
//! Associate 2 arbitrary typed values.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cPair_H
#define _INC_cPair_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"
#include "cValSpan.h"  // ITERATE_t

namespace Gray {
/// <summary>
/// The aggregate/simple type for simple static const init. AKA Tuple.
/// similar to "std::pair" or "std::tuple<>" or "System.Collections.Generic.KeyValuePair<>"
/// not the same as cValueRange (same type)
/// @note if i give this a constructor then the compiler won't allow it to be static initialized.
/// </summary>
/// <typeparam name="_TYPE_A"></typeparam>
/// <typeparam name="_TYPE_B"></typeparam>
template <class _TYPE_A, class _TYPE_B>
struct cPairT {
    _TYPE_A _a;  /// nullptr or 0 = last in static array. (typically sorted by A as primary key)
    _TYPE_B _b;  /// nullptr or 0 = last in static array. (value of a keyvalue pair)

    /// <summary>
    /// Support this in case anyone wants to use it.
    /// </summary>
    const _TYPE_A& get_HashCode() const noexcept {
        return _a;
    }
    const _TYPE_A& get_A() const noexcept {
        return this->_a;  // Key
    }
    const _TYPE_B& get_B() const noexcept {
        return this->_b;  // Value
    }
};

/// <summary>
/// Associated pair of simple things. Like cArrayVal is to cArray
/// typically LAST ENTRY in static table = { 0  or nullptr }, in either place.
/// typically sorted by _TYPE_A. but not assumed/enforced.
/// typically in a static table!
/// </summary>
/// <typeparam name="_TYPE_A"></typeparam>
/// <typeparam name="_TYPE_B"></typeparam>
template <class _TYPE_A, class _TYPE_B>
struct cPair : public cPairT<_TYPE_A, _TYPE_B> {
    typedef cPairT<_TYPE_A, _TYPE_B> SUPER_t;
    cPair() noexcept {}  // Undefined values for dynamic arrays.
    cPair(_TYPE_A a, _TYPE_B b) noexcept {
        this->_a = a;
        this->_b = b;
    }
};
}  // namespace Gray
#endif  // _INC_cPair_H
