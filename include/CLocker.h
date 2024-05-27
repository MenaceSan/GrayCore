//! @file cLocker.h
//! Locking of objects for any reason. (Thread lock or DX buffer usage lock)
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cLocker_H
#define _INC_cLocker_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cPtrFacade.h"
#include "cTimeSys.h"

namespace Gray {
/// <summary>
/// Common base for cThreadLockable and all lock / thread lock / mutex implementations.
/// derived class can override Lock,Unlock
/// NOT itself thread safe. Assume caller handles thread safety. Use cInterlockedVal if we want/need thread safety here.
/// Similar to cRefBase and cInterlockedVal.
/// </summary>
class GRAYCORE_LINK cLockableBase {
    template <class T>
    friend struct cLockerT;

    /// <summary>
    /// count Lock vs Unlock calls.
    /// Should NEVER be negative. like REFCOUNT_t.
    /// ONLY changed from within a lock so no extra thread protect needed.
    /// </summary>
    LONG m_nLockCount;

 protected:
    cLockableBase() noexcept : m_nLockCount(0) {}
    ~cLockableBase() noexcept {
        DEBUG_CHECK(!isLockCount());
    }

    inline void IncLockCount() noexcept {
        ++m_nLockCount;
        DEBUG_CHECK(isLockCount());
    }
    inline LONG DecLockCount() noexcept {
        --m_nLockCount;
        DEBUG_CHECK(m_nLockCount >= 0);
        return m_nLockCount;
    }

    /// Wait for unique lock. m_nLockCount == 0
    bool WaitUnique(TIMESYSD_t nWaitMS);

 public:
    inline LONG get_LockCount() const noexcept {
        // @note use LONG for IUnknown compatibility.
        DEBUG_CHECK(m_nLockCount >= 0);
        return m_nLockCount;
    }
    inline bool isLockCount() const noexcept {
        DEBUG_CHECK(m_nLockCount >= 0);
        return m_nLockCount != 0;
    }
    /// Stub for Unlock for non _MT, cThreadLockableStub
    void Unlock() {
        DecLockCount();
    }
};

/// <summary>
/// Call Lock/Unlock on something for the life span of this object. Guard.
/// Stack only based guard. Used for: cThreadLockableMutex, cThreadLockableCrit, cThreadLockableX, ...
/// Might be used for: cDXSurfaceLock, cSemaphoreLock, cWinHeap, cDXMesh, cDXBuffer
/// TYPE must support Unlock() and probably Lock() or be based on cLockableBase*
/// m_p = the lock we are locking.
/// Similar to ATL CCritSecLock, std::unique_lock
/// like: std::scoped_lock<>
/// </summary>
/// <typeparam name="TYPE">cLockableBase</typeparam>
template <class TYPE>
struct cLockerT : public cPtrFacade<TYPE> {
    typedef cLockerT<TYPE> THIS_t;

    /// <summary>
    /// The lock may not always work ! isValidPtr() = nullptr allows failure to lock. or early unlock.
    /// </summary>
    explicit cLockerT(TYPE* pLockable, bool bLockSuccess) noexcept : cPtrFacade<TYPE>(pLockable) {
        UNREFERENCED_PARAMETER(bLockSuccess);
    }

    // cLockerT(TYPE& rLockable) noexcept : THIS_t(rLockable.Lock()) {}

    // Copy construct is the normal usage.
    ~cLockerT() noexcept {
        if (this->isValidPtr()) {  // check that lock wasn't cleared earlier.
            this->get_Ptr()->Unlock();
        }
    }
};
}  // namespace Gray
#endif
