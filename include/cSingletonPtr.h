//! @file cSingletonPtr.h
//!	A reference counted singleton
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cSingletonPtr_H
#define _INC_cSingletonPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cRefPtr.h"
#include "cSingleton.h"

namespace Gray {

/// <summary>
/// A lazy loaded / released Singleton. ASSUME TYPE is based on cSingleton AND cRefBase
/// </summary>
/// <typeparam name="TYPE">cSingleton AND cRefBase </typeparam>
template <class TYPE>
class cSingletonPtr : protected cRefPtr<TYPE> {  // cRefPtr protects from release.
    typedef cRefPtr<TYPE> SUPER_t;

 public:
    /// <summary>
    /// create instance now or later?
    /// Allocate a reference automatically by default.
    /// Attach to cSingleton
    /// createNow = false = defer Allocate until later
    /// </summary>
    cSingletonPtr(bool createNow = true) : SUPER_t(createNow ? TYPE::get_Single() : nullptr) {}

    /// <summary>
    /// Late Attach/Create the Object that is base on cSingleton (or equiv).
    /// Call this as often as we want. Maybe cSingletonPtr changed/upgraded ?
    /// If i created an empty cSingletonPtr(false) (as part of some class) this is how I populate it on that classes constructor later/lazy.
    /// </summary>
    void InitPtr() {
        this->put_Ptr(TYPE::get_Single());
    }

    // cRefPtr is protected so expose the parts i allow. cPtrFacade
    using SUPER_t::get_Ptr;
    using SUPER_t::get_PtrT;
    using SUPER_t::isValidPtr;
    using SUPER_t::ReleasePtr;

    using SUPER_t::operator TYPE*;
    using SUPER_t::operator->;

    inline operator TYPE&() const noexcept {
        DEBUG_CHECK(SUPER_t::isValidPtr());
        return *this->get_Ptr();
    }
};

#define DECLARE_cSingletonPtr DECLARE_cSingleton
#define cSingletonPtr_IMPL cSingleton_IMPL

}  // namespace Gray
#endif
