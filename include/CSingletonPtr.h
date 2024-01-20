//
//! @file cSingletonPtr.h
//!	A reference counted singleton
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cSingletonPtr_H
#define _INC_cSingletonPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cRefPtr.h"
#include "cSingleton.h"

namespace Gray {
/// <summary>
/// Base class for a cSingleton that is reference counted and lazy created/loaded.
/// This will be destroyed when the last reference is released. recreated again on demand.
/// e.g. a public service (shared by all) that is loaded on demand and released when no one needs it.
/// @note These objects are normally cHeapObject, but NOT ALWAYS ! (allow static versions using StaticConstruct() and k_REFCOUNT_STATIC)
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cSingletonRefBase : public cSingleton<TYPE>, public cRefBase {
 protected:
    cSingletonRefBase(TYPE* pObject, const TYPEINFO_t& rAddrCode, REFCOUNT_t iRefCountStart = 0) : cSingleton<TYPE>(pObject, rAddrCode), cRefBase(iRefCountStart) {
        //! typically this == pObject
    }
    CHEAPOBJECT_IMPL;
};

/// <summary>
/// A reference to a cSingletonRefBase<> based TYPE or a type that has both cSingleton and cRefBase.
/// A Lazy loaded singleton.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cSingletonPtr : protected cRefPtr<TYPE>  // cRefPtr protected for read only.
{
    typedef cRefPtr<TYPE> SUPER_t;

 public:
    /// <summary>
    /// create instance now or later?
    /// Allocate a reference automatically by default.
    /// Attach to cSingletonRefBase
    /// false = defer Allocate until later
    /// </summary>
    cSingletonPtr(bool bInitNow = true) : SUPER_t(bInitNow ? TYPE::get_Single() : nullptr) {}

    /// <summary>
    /// If i created an empty cSingletonPtr(false) (as part of some class) this is how I populate it on that classes constructor later.
    /// Attach to cSingletonRefBase
    /// </summary>
    void InitPtr() {
        this->put_Ptr(TYPE::get_Single());
    }

    // cRefPtr is protected so expose the parts i allow. cPtrFacade
    void ReleasePtr() {
        SUPER_t::ReleasePtr();
    }
    inline bool isValidPtr() const noexcept {
        return SUPER_t::isValidPtr();
    }
    inline TYPE* get_Ptr() const noexcept {
        return SUPER_t::get_Ptr();
    }

    /// <summary>
    /// Cast pointer to another type.
    /// This is probably a compile time up-cast but check it anyhow.
    /// This shouldn't return nullptr if not starting as nullptr.
    /// </summary>
    /// <typeparam name="_DST_TYPE"></typeparam>
    /// <returns></returns>
    template <class _DST_TYPE>
    _DST_TYPE* get_PtrT() const {
        if (this == nullptr) return nullptr;
        return PtrCastCheck<_DST_TYPE>(this->get_Ptr());  // dynamic for DEBUG only. Should NEVER return nullptr here !
    }

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
}  // namespace Gray
#endif
