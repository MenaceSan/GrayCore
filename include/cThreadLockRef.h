//! @file cThreadLockRef.h
//! Locking of objects for access by multiple threads
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cThreadLockRef_H
#define _INC_cThreadLockRef_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cRefPtr.h"
#include "cThreadLockRW.h"

namespace Gray {
/// <summary>
/// Base class for a dynamic data structure that may be locked for multi threaded access (cThreadLockCount)
/// AND/or locked for delete/usage (cRefBase).
/// These are fairly cheap and fast.
/// </summary>
struct GRAYCORE_LINK cThreadLockableRef : public cRefBase, public cThreadLockCount {
    cThreadLockableRef(REFCOUNT_t iStaticRefCount = 0) noexcept : cRefBase(iStaticRefCount) {}
    ~cThreadLockableRef() override {}

    virtual void onThreadLockFail(TIMESYSD_t dwWaitMS) {
        //! a DEBUG trap for locks failing.
        UNREFERENCED_PARAMETER(dwWaitMS);
    }

    // RefLock  // lock ref and thread.
    // RefRead  // lock for reading only.
};

//******************************************************

/// <summary>
/// Abstract base for a smart pointer referenced object that can be locked in read or write mode.
/// similar to cThreadLockableRef
/// </summary>
class GRAYCORE_LINK cRefBaseRW : public cRefBase, public cThreadLockRW {};

/// <summary>
/// Both reference and read lock this cRefBaseRW object.
/// @note this only returns 'const' pointers. ReleasePtr MUST be called last!
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cRefReadPtr : public cThreadGuardRead, public cRefPtr<TYPE> {
    cRefReadPtr(TYPE* pObj) : cThreadGuardRead(*pObj), cRefPtr<TYPE>(pObj) {}
};

/// <summary>
/// Both reference and write lock this cRefBaseRW object.
/// If another thread has it open (read or write) then we must wait.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cRefGuardPtr : public cLockerT<TYPE>, public cRefPtr<TYPE> {
    cRefGuardPtr(TYPE* pObj) : cLockerT<TYPE>(*pObj), cRefPtr<TYPE>(pObj) {}
};

//****************************************************************************

/// <summary>
/// a cRefPtr (inc ref count for delete protection) that also thread locks the object. (like cLockerT)
/// Similar to the MFC CMultiLock or CSingleLock and cRefPtr
/// Thread Lock an object for as long as 'this' object persists.
/// May wait if some other thread has the object locked.
/// @note there is also a non thread locking smart pointer ref to the object. cRefPtr
/// always thread locking an object for its whole life makes no sense.
/// </summary>
/// <typeparam name="TYPE">MUST be based on cThreadLockableRef</typeparam>
template <class TYPE = cThreadLockableRef>
class cThreadGuardRef : public cRefPtr<TYPE> {
    typedef cThreadGuardRef<TYPE> THIS_t;

#if defined(_MT) || defined(__linux__)
    //! NON _MT Stub does nothing really.

 private:
    void SetFirstLockObj(TYPE* p2) {
        //! @note Lock can throw !
        if (p2 != nullptr) {
#ifdef _DEBUG
            ASSERT(DYNPTR_CAST(cThreadLockableRef, p2) != nullptr);
            ASSERT(DYNPTR_CAST(TYPE, p2) != nullptr);
#endif
            p2->IncRefCount();
            p2->Lock();
        }
        this->m_p = p2;
    }
    bool SetFirstLockObjTry(TYPE* p2, TIMESYSD_t dwWaitMS) {
        //! @note Lock can throw !
        //! dwWaitMS = 0 = don't wait.
        if (p2 != nullptr) {
#ifdef _DEBUG
            ASSERT(DYNPTR_CAST(cThreadLockableRef, p2) != nullptr);
            ASSERT(DYNPTR_CAST(TYPE, p2) != nullptr);
#endif
            p2->IncRefCount();
            if (!p2->LockTry(dwWaitMS)) {
                if (dwWaitMS) {
                    p2->onThreadLockFail(dwWaitMS);
                }
                p2->DecRefCount();
                this->m_p = nullptr;
                return false;
            }
        }
        this->m_p = p2;
        return true;
    }

 public:
    // Construct and destruct
    cThreadGuardRef() {}
    cThreadGuardRef(TYPE* p2) {
        //! @note = assignment will auto destroy previous and use this constructor.
        SetFirstLockObj(p2);
    }
    cThreadGuardRef(TYPE* p2, TIMESYSD_t dwWaitMS) {
        //! @note = assignment will auto destroy previous and use this constructor.
        //! dwWaitMS = 0 = don't wait.
        SetFirstLockObjTry(p2, dwWaitMS);
    }
    cThreadGuardRef(const THIS_t& ref) {
        //! using the assignment auto constructor is not working so use this.
        SetFirstLockObj(ref.get_Ptr());
    }
    ~cThreadGuardRef() {
        ReleasePtr();
    }

    void ReleasePtr() {
        TYPE* p2 = this->m_p;  // make local copy
        if (p2 != nullptr) {
            this->m_p = nullptr;
#ifdef _DEBUG
            ASSERT(DYNPTR_CAST(TYPE, p2) != nullptr);
#endif
            p2->Unlock();
            p2->DecRefCount();
        }
    }

    operator TYPE*() const {
        return this->m_p;
    }
    TYPE& operator*() const {
        ASSERT_NN(this->m_p);
        return *(this->m_p);
    }
    TYPE* operator->() const {
        ASSERT_NN(this->m_p);
        return this->m_p;
    }

    void put_Ptr(TYPE* p2) {
        //! override cRefPtr put_Ptr
        if (p2 == this->m_p) return;
        ReleasePtr();
        SetFirstLockObj(p2);
    }
    bool SetLockObjTry(TYPE* p2, TIMESYSD_t dwWaitMS) {
        if (p2 == this->m_p) return true;
        ReleasePtr();
        return SetFirstLockObjTry(p2, dwWaitMS);
    }

    // Assignment
    const THIS_t& operator=(TYPE* p2) {
        put_Ptr(p2);
        return *this;
    }
    const THIS_t& operator=(const THIS_t& ref) {
        put_Ptr(ref.m_p);
        return *this;
    }
#endif  // defined(_MT) || __linux__
};

typedef cThreadGuardRef<cThreadLockableRef> cThreadGuardRefX;
}  // namespace Gray
#endif  // _INC_cThreadLockRef_H
