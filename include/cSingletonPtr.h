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
/// Base class for a cSingletonStatic that is reference counted and lazy created/loaded. Use with cSingletonPtr.
/// This will be destroyed when the last reference is released. recreated again on demand.
/// e.g. a public service (shared by all) that is loaded on demand and released when no one needs it.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cSingletonRefBase : public cSingletonStatic<TYPE>, public cRefBase, public cHeapObject, public cObject {
    typedef cSingletonStatic<TYPE> SUPER_t;
    // CHEAPOBJECT_IMPL;

 protected:
    cSingletonRefBase(TYPE* pObject, const TYPEINFO_t& rAddrCode, REFCOUNT_t iRefCountStart = 0) : SUPER_t(pObject), cRefBase(iRefCountStart) {
        //! typically this == pObject
    }
};

/// <summary>
/// A lazy loaded / released cSingletonStatic. ASSUME TYPE is cSingletonRefBase base or at least cSingletonStatic, cRefBase, cHeapObject, cObject based. 
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cSingletonPtr : protected cRefPtr<TYPE> {  // cRefPtr protects from release.
    typedef cRefPtr<TYPE> SUPER_t;

    /// <summary>
    /// The singleton is ONLY created on reference.
    /// </summary>
    /// <returns></returns>
    static TYPE* GRAYCALL get_SingleCreate() {
        if (!TYPE::isSingleCreated()) {
            const auto guard(cDependRegister::sm_LockSingle.Lock());  // thread sync critical section.
            if (!TYPE::isSingleCreated()) {
                auto p = new TYPE();
                DEBUG_CHECK(TYPE::isSingleCreated());
            }
        }
        return PtrCastCheck<TYPE>(TYPE::get_SingleU());
    }

 public:
    /// <summary>
    /// create instance now or later?
    /// Allocate a reference automatically by default.
    /// Attach to cSingletonRefBase
    /// createNow = false = defer Allocate until later
    /// </summary>
    cSingletonPtr(bool createNow = true) : SUPER_t(createNow ? get_SingleCreate() : nullptr) {}

    /// <summary>
    /// Late Attach/Create the Object that is base on cSingletonRefBase (or equiv).
    /// Call this as often as we want. Maybe cSingletonPtr changed/upgraded ?
    /// If i created an empty cSingletonPtr(false) (as part of some class) this is how I populate it on that classes constructor later/lazy.
    /// </summary>
    void InitPtr() {
        this->put_Ptr(get_SingleCreate());
    }

    // cRefPtr is protected so expose the parts i allow. cPtrFacade
    using SUPER_t::get_Ptr;
    using SUPER_t::get_PtrT;
    using SUPER_t::isValidPtr;
    using SUPER_t::ReleasePtr;

    inline operator TYPE*() const noexcept {
        return this->get_Ptr();
    }
    inline TYPE* operator->() const noexcept {
        return this->get_Ptr();
    }
    inline operator TYPE&() const noexcept {
        DEBUG_CHECK(SUPER_t::isValidPtr());
        return *this->get_Ptr();
    }
};

#define SINGLETONPTR_IMPL(T) \
    friend cSingletonPtr<T>; \
    CHEAPOBJECT_IMPL

}  // namespace Gray
#endif
