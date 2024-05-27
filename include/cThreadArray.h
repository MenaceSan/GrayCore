//! @file cThreadArray.h
//! Thread safe arrays of stuff.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cThreadArray_H
#define _INC_cThreadArray_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArrayRef.h"
#include "cArraySortRef.h"
#include "cThreadLock.h"

namespace Gray {
/// <summary>
/// Thread safe array of pointers.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cThreadLockArrayPtr : protected cArrayPtr<TYPE> {
 public:
    typedef cArrayPtr<TYPE> SUPER_t;

 public:
    mutable cThreadLockableX m_Lock;

 public:
    cThreadLockArrayPtr() {}
    ~cThreadLockArrayPtr() {}
    ITERATE_t GetSize() const {
        //! Used for statistical purposes. This may change of course. expose protected.
        return SUPER_t::GetSize();  // just for stats purposes.
    }
    void SetSize(ITERATE_t nNewSize) {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        SUPER_t::SetSize(nNewSize);        // just for stats purposes.
    }
    ITERATE_t Add(TYPE* pObj) {
        // add to tail
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::Add(pObj);         // add to tail
    }
    TYPE* GetAtCheck(ITERATE_t nIndex) const {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::GetAtCheck(nIndex);
    }
    bool HasArg(TYPE* pObj) const {
        //! Find the index of a specified entry.
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::FindIForN(pObj) >= 0;
    }
    TYPE* PopHead() {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::PopHead();
    }
    TYPE* PopTail() {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::PopTail();
    }
    void DeleteAll() {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        SUPER_t::DeleteAll();
    }
    bool RemoveArg(TYPE* pObj) {
        //! @return true = removed. false = was not here.
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::RemoveArg(pObj);
    }
};

/// <summary>
/// Thread safe array of smart pointers. NON sorted.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cThreadLockArrayRef : protected cArrayRef<TYPE> {
    typedef cArrayRef<TYPE> SUPER_t;

 public:
    mutable cThreadLockableX m_Lock;

 public:
    ITERATE_t GetSize() const {
        //! Used for statistical purposes. This may change of course.
        return SUPER_t::GetSize();  // just for stats purposes.
    }

    // Locking helpers

    /// <summary>
    /// thread safe get.
    /// @note Its slightly dangerous to enum/iterate a thread used list. We could read the same entry 2 times unless we lock for the life of the iteration!
    /// @note NEVER NEVER lock the list and the object at the same time! This could create a permanent deadlock!
    /// </summary>
    /// <param name="nIndex"></param>
    /// <returns></returns>
    cRefPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::GetAtCheck(nIndex);
    }
    cRefPtr<TYPE> PopHead() {
        // a queue.
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::PopHead();         // pop off head
    }
    cRefPtr<TYPE> PopTail() {
        // stack form = tail = latest.
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::PopTail();         // pop off tail
    }
    bool HasArg(TYPE* pObj) const {
        //! Find a specified entry.
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::FindIFor3(pObj) >= 0;
    }
    ITERATE_t Add(TYPE* pObj) {
        // add to tail
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::Add(pObj);         // add to tail
    }

    bool RemoveArg(TYPE* pObj) {
        //! @return true = removed. false = was not here.
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::RemoveArg(pObj);
    }
    void RemoveAll() {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        SUPER_t::RemoveAll();
    }
    void DisposeAll() {
        //! ASSUME TYPE supports DisposeThis(); like cXObject
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        SUPER_t::DisposeAll();
    }
    // FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
};

//*************************************************

/// <summary>
/// Thread Lockable, name sorted resource array.
/// Must be locked before use of other methods!
/// TYPE must support get_Name() and be cRefBase
/// does  NOT allow dupe names !
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="_TYPECH"></typeparam>
template <class TYPE, typename _TYPECH = TCHAR>
class cThreadLockArrayName : protected cArraySortName<TYPE, _TYPECH> {
    typedef cArraySortName<TYPE, _TYPECH> SUPER_t;

 public:
    mutable cThreadLockableX m_Lock;

 public:
    cThreadLockArrayName() {}
    ~cThreadLockArrayName() {}

    // Locking helpers
    cRefPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const {
        //! @note Its slightly dangerous to enum a thread used list.
        //!  We could read the same entry 2 times !
        //! @note NEVER NEVER lock the list and the object at the same time!
        //!  This could create a permanent deadlock!
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::GetAtCheck(nIndex);
    }
    cRefPtr<TYPE> FindArgForKey(const _TYPECH* pszKey) const {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::FindArgForKey(pszKey);
    }
    ITERATE_t AddSort(TYPE* pObj) {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::AddSort(pObj);
    }
    bool RemoveArgKey(TYPE* pObj) {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::RemoveArgKey(pObj);
    }
    ITERATE_t GetSize() const {
        //! Used for statistical purposes. This may change of course.
        return SUPER_t::GetSize();  // just for stats purposes.
    }
    void RemoveAll() {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        SUPER_t::RemoveAll();
    }
    // FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
};

/// <summary>
/// Thread safe hash.
/// TYPE must support get_HashCode() and be cRefBase.
/// Does NOT allow dupe hash codes !
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="_TYPE_HASH"></typeparam>
template <class TYPE, typename _TYPE_HASH = HASHCODE_t>
class cThreadLockArrayHash : protected cArraySortHash<TYPE, _TYPE_HASH> {
    // friend class cPtrTraceMgr;
    typedef cArraySortHash<TYPE, _TYPE_HASH> SUPER_t;

 public:
    mutable cThreadLockableX m_Lock;

 public:
    cThreadLockArrayHash() {}
    ~cThreadLockArrayHash() {}

    bool IsEmpty() const {
        //! Used for statistical purposes. This may change of course.
        return SUPER_t::isEmpty();
    }
    ITERATE_t GetSize() const noexcept {
        //! Used for statistical purposes. This may change of course.
        return SUPER_t::GetSize();
    }
    cRefPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const {
        //! return a reference counted pointer. NOT a bare pointer.
        //! @note Its slightly dangerous to enum a thread used list.
        //!  We could read the same entry 2 times !
        //! @note NEVER NEVER lock the list and the object at the same time!
        //!  This could create a deadlock!
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::GetAtCheck(nIndex);
    }
    ITERATE_t AddSort(TYPE* pObj) {
        // AKA Push = add to Tail.
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::AddSort(pObj);    // AddTail
    }
    cRefPtr<TYPE> PopHead() {
        // Act as a Queue, not a stack.
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::PopHead();         // pull off head
    }
    cRefPtr<TYPE> PopTail() {
        // Act as a stack, not a Queue
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::PopTail();
    }
    bool RemoveArgKey(TYPE* pObj) {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::RemoveArgKey(pObj);
    }
    void RemoveAll() {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        SUPER_t::RemoveAll();
    }
    void DisposeAll() {
        //! ASSUME TYPE supports DisposeThis(); like cXObject
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        SUPER_t::DisposeAll();
    }
    cRefPtr<TYPE> FindArgForKey(_TYPE_HASH hashcode) const {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::FindArgForKey(hashcode);
    }

    // FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
};

/// <summary>
/// Thread safe array
/// TYPE must support get_SortValue() and be cRefBase
/// does allow dupe get_SortValue but not dupe objects
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="_TYPE_KEY"></typeparam>
template <class TYPE, class _TYPE_KEY = ITERATE_t>
class cThreadLockArrayValue : protected cArraySortValue<TYPE, _TYPE_KEY> {
    typedef cArraySortValue<TYPE, _TYPE_KEY> SUPER_t;

 public:
    mutable cThreadLockableX m_Lock;

 public:
  
    ITERATE_t GetSize() const {
        //! Used for statistical purposes. This may change of course.
        return SUPER_t::GetSize();
    }
    cRefPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const {
        //! @note Its slightly dangerous to enum a thread used list.
        //!  We could read the same entry 2 times !
        //! @note NEVER NEVER lock the list and the object at the same time!
        //!  This could create a permanent deadlock!
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::GetAtCheck(nIndex);
    }
    ITERATE_t AddSort(TYPE* pObj) {
        // add to tail.
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::AddSort(pObj);
    }
    ITERATE_t AddAfter(TYPE* pObj) {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::AddAfter(pObj);    // AddAfter
    }
    cRefPtr<TYPE> PopHead() {
        // as Queue
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::PopHead();         // pull off tail
    }
    cRefPtr<TYPE> PopTail() {
        // as Stack
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::PopTail();
    }
    bool RemoveArg(TYPE* pObj) {
        //! Since this can have dupes we should not use RemoveArgKey()
        //! @return true = removed. false = was not here.
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::RemoveArg(pObj);
    }
    void RemoveAll() {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        SUPER_t::RemoveAll();
    }
    void DisposeAll() {
        //! ASSUME TYPE supports DisposeThis(); like cXObject
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        SUPER_t::DisposeAll();
    }
    cRefPtr<TYPE> FindArgForKey(_TYPE_KEY index) const {
        const auto guard(m_Lock.Lock());  // thread sync critical section.
        return SUPER_t::FindArgForKey(index);
    }

    // FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
};

#if 0
	/// <summary>
	/// Thread safe array. Used as a job queue.
	/// @todo Create an array where i can just wait for new stuff to be added to it.
	/// </summary>
	/// <typeparam name="TYPE"></typeparam>
	template<class TYPE>
	class cThreadLockArrayWait
	: protected cArrayRef < TYPE >
	{
	};
#endif
}  // namespace Gray
#endif
