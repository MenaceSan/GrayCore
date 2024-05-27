//! @file cRefLockable.h
//! Locking of objects for access by multiple threads
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cRefLockable_H
#define _INC_cRefLockable_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cRefPtr.h"
#include "cThreadLockRW.h"

namespace Gray {
/// <summary>
/// Base class for a dynamic data structure that may be locked for multi threaded access (cThreadLockableX)
/// AND/or locked for delete/usage (cRefBase).
/// These are fairly cheap and fast.
/// </summary>
struct GRAYCORE_LINK cRefLockable : public cRefBase, public cThreadLockableX {
    cRefLockable(REFCOUNT_t iStaticRefCount = 0) noexcept : cRefBase(iStaticRefCount) {}
 
    virtual void onThreadLockFail(TIMESYSD_t dwWaitMS) {
        //! a DEBUG trap for locks failing.
        UNREFERENCED_PARAMETER(dwWaitMS);
    }
};

/// <summary>
/// Base class for a smart pointer referenced object that can be locked in read or write mode. cThreadLockRW
/// similar to cRefLockable
/// </summary>
class GRAYCORE_LINK cRefLockableRW : public cRefBase, public cThreadLockRW {
    // RefRead  // lock for reading only.
};

/// <summary>
/// Both reference and thread (write) lock this cRefLockable*. cThreadLockableX or cThreadLockRW object.
/// If another thread has it open (read or write) then we must wait.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cRefGuardPtr : public cLockerT<TYPE>, public cRefPtr<TYPE> {
    cRefGuardPtr(TYPE* pObj) : cLockerT<TYPE>(*pObj), cRefPtr<TYPE>(pObj) {}
};

/// <summary>
/// Both reference and read (only) lock this cRefLockableRW object.
/// @note this only returns 'const' pointers. ReleasePtr MUST be called last!
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cRefReadPtr : public cThreadGuardRead, public cRefPtr<TYPE> {
    cRefReadPtr(TYPE* pObj) : cThreadGuardRead(*pObj), cRefPtr<TYPE>(pObj) {}
};

}  // namespace Gray
#endif  // _INC_cRefLockable_H
