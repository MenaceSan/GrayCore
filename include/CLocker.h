//
//! @file cLocker.h
//! Locking of objects for any reason. (Thread lock or DX buffer usage lock)
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cLocker_H
#define _INC_cLocker_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cPtrFacade.h"

namespace Gray {
/// <summary>
/// Common base for cThreadLockBase and all lock / thread lock / mutex implementations.
/// derived class can override Lock,Unlock
/// NOT thread safe. Assume caller handles thread safety. Use cInterlockedVal if we want/need thread safety here.
/// Similar to cRefBase and cInterlockedVal.
/// </summary>
class GRAYCORE_LINK cLockableBase {
 private:
    LONG m_nLockCount;  /// count Lock vs Unlock calls. Should NEVER be negative.

 protected:
    cLockableBase() noexcept : m_nLockCount(0) {}
    ~cLockableBase() noexcept {
        DEBUG_CHECK(m_nLockCount == 0);
    }
    inline LONG get_LockCount() const noexcept {
        // @note use LONG for IUnknown compatibility.
        DEBUG_CHECK(m_nLockCount >= 0);
        return m_nLockCount;
    }
    inline bool isLocked() const noexcept {
        DEBUG_CHECK(m_nLockCount >= 0);
        return (m_nLockCount != 0);
    }

    inline void IncLockCount() noexcept {
        ++m_nLockCount;
        DEBUG_CHECK(m_nLockCount >= 0);
    }
    inline LONG DecLockCount() noexcept {
        //! @return new lock count.
        --m_nLockCount;
        DEBUG_CHECK(m_nLockCount >= 0);
        return m_nLockCount;  // return new lock count.
    }
    inline void Lock() noexcept {
        // For template support. cLockerT
        IncLockCount();
    }
    inline LONG Unlock() noexcept {
        //! For template support. cLockerT
        //! @return new lock count.
        return DecLockCount();
    }
};

/// <summary>
/// Call Lock/Unlock on something for the life span of this object.
/// Stack only based guard. Used for: cThreadLockMutex, cThreadLockCrit, cThreadLockFast, ...
/// Might be used for: cDXSurfaceLock, cSemaphoreLock, cWinHeap, cDXMesh, cDXBuffer
/// TYPE must support Unlock() and probably Lock() or be based on cLockableBase*
/// m_p = the lock we are locking.
/// Similar to ATL CCritSecLock, std::unique_lock
/// like: std::scoped_lock<>
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE = cLockableBase>
class cLockerT : public cPtrFacade<TYPE> {
 public:
    explicit cLockerT(TYPE* pLock, bool bLockSuccess) noexcept : cPtrFacade<TYPE>(bLockSuccess ? pLock : nullptr) {
        // The lock may not always work ! isValidPtr()
    }
    explicit cLockerT(TYPE& rLock) noexcept : cPtrFacade<TYPE>(&rLock) {
        DEBUG_CHECK(this->isValidPtr() && this->IsEqual(&rLock));
        rLock.Lock();  // Assume lock must succeed. does it return something?
    }
    ~cLockerT() noexcept {
        if (this->isValidPtr()) {
            this->get_Ptr()->Unlock();
        }
    }
};
}  // namespace Gray

#endif
