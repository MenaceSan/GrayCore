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
    _TYPE_A m_a;  /// nullptr or 0 = last in array. (typically sorted by A as primary key)
    _TYPE_B m_b;  /// nullptr or 0 = last in array. (value of a keyvalue pair)

    const _TYPE_A& get_HashCode() const noexcept {
        //! Support this in case anyone wants to use it.
        return m_a;
    }
    const _TYPE_A& get_A() const noexcept {
        return this->m_a; // Key
    }
    const _TYPE_B& get_B() const noexcept {
        return this->m_b; // Value
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
class cPair : public cPairT<_TYPE_A, _TYPE_B> {
    typedef cPairT<_TYPE_A, _TYPE_B> SUPER_t;
 
    bool IsValidIndex(ITERATE_t i) const noexcept {
        //! ASSUME static array.
        //! either value is non zero?
        if (i < 0) return false;
        return this[i].m_a || this[i].m_b;
    }

    ITERATE_t FindIA(_TYPE_A a) const noexcept {
        //! ASSUME static array.
        //! brute force lookup A
        for (ITERATE_t i = 0; IsValidIndex(i); i++) {
            if (this[i].m_a == a) return i;
        }
        return k_ITERATE_BAD;
    }

    ITERATE_t FindIB(_TYPE_B b) const noexcept {
        //! ASSUME static array.
        //! brute force lookup B return index
        //! @return <0 = can't find it.
        for (ITERATE_t i = 0; IsValidIndex(i); i++) {
            if (this[i].m_b == b) return i;
        }
        return k_ITERATE_BAD;
    }

 public:
    cPair() noexcept  // Undefined values for dynamic arrays.
    {}
    cPair(_TYPE_A a, _TYPE_B b) noexcept {
        this->m_a = a;
        this->m_b = b;
    }

    /// If element is a member of a static array. TODO use cSpan ? NOT terminator.
    bool FindARetB(_TYPE_A a, _TYPE_B* pb) const noexcept {
        //! ASSUME static array.
        //! brute force lookup A to return corresponding B
        //! @return <0 = can't find it.
        ITERATE_t i = FindIA(a);
        if (i >= 0) {
            *pb = this[i].m_b;
            return true;
        }
        return false;
    }
};
}  // namespace Gray
#endif  // _INC_cPair_H
