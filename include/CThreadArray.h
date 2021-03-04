//
//! @file cThreadArray.h
//! Thread safe arrays of stuff.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cThreadArray_H
#define _INC_cThreadArray_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cThreadLock.h"
#include "cArraySortRef.h"
#include "cArrayRef.h"

namespace Gray
{
	template <class TYPE>
	class cThreadLockArrayPtr
	: protected cArrayPtr < TYPE >
	{
		//! @class Gray::cThreadLockArrayPtr
		//! Thread safe array of pointers.

	public:
		typedef cArrayPtr<TYPE> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::ELEM_t ELEM_t;
	public:
		mutable cThreadLockCount m_Lock;
	public:
		cThreadLockArrayPtr()
		{
		}
		~cThreadLockArrayPtr()
		{
		}
		ITERATE_t GetSize() const
		{
			//! Used for statistical purposes. This may change of course.
			return SUPER_t::GetSize();	// just for stats purposes.
		}
		void SetSize(ITERATE_t nNewSize)
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::SetSize(nNewSize);	// just for stats purposes.
		}
		ITERATE_t Add(REF_t pObj)
		{
			// add to tail
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::Add(pObj); // add to tail
		}
		REF_t GetAtCheck(ITERATE_t nIndex) const
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::GetAtCheck(nIndex);
		}
		bool HasArg(TYPE* pObj) const
		{
			//! Find the index of a specified entry.
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return(SUPER_t::FindIFor(pObj) >= 0);
		}
		ELEM_t PopHead()
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopHead();
		}
		ELEM_t PopTail()
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopTail();
		}
		void DeleteAll()
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			for (int i = 0; i < this->GetSize(); i++)
			{
				delete this->GetAt(i);
			}
			SUPER_t::RemoveAll();
		}
		bool RemoveArg(TYPE* pObj)
		{
			//! @return true = removed. false = was not here.
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::RemoveArg(pObj);
		}
	};

	template<class TYPE>
	class cThreadLockArrayRef
	: protected cArrayRef < TYPE >
	{
		//! @class Gray::cThreadLockArrayRef
		//! Thread safe array of smart pointers. NON sorted.
		typedef cArrayRef<TYPE> SUPER_t;
	public:
		mutable cThreadLockCount m_Lock;
	public:
		cThreadLockArrayRef()
		{
		}
		~cThreadLockArrayRef()
		{
		}

		ITERATE_t GetSize() const
		{
			//! Used for statistical purposes. This may change of course.
			return SUPER_t::GetSize();	// just for stats purposes.
		}

		// Locking helpers
		cRefPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const
		{
			//! @note Its slightly dangerous to enum a thread used list.
			//!  We could read the same entry 2 times !
			//! @note NEVER NEVER lock the list and the object at the same time!
			//!  This could create a permanent deadlock!
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::GetAtCheck(nIndex);
		}
		cRefPtr<TYPE> PopHead()
		{
			// a queue.
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopHead();	// pop off head
		}
		cRefPtr<TYPE> PopTail()
		{
			// stack form = tail = latest.
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopTail();	// pop off tail
		}
		bool HasArg(TYPE* pObj) const
		{
			//! Find a specified entry.
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::FindIFor(pObj) >= 0 ;
		}
		ITERATE_t Add(TYPE* pObj)
		{
			// add to tail
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::Add(pObj); // add to tail
		}
		ITERATE_t AddTail(TYPE* pObj)
		{
			// add to tail = latest. aka Push to stack.
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::AddTail(pObj); // add to tail
		}
		bool RemoveArg(TYPE* pObj)
		{
			//! @return true = removed. false = was not here.
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::RemoveArg(pObj);
		}
		void RemoveAll()
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::RemoveAll();
		}
		void DisposeAll()
		{
			//! ASSUME TYPE supports DisposeThis(); like cXObject
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::DisposeAll();
		}
		// FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
	};

	//*************************************************

	template<class TYPE, typename _TYPECH = TCHAR >
	class cThreadLockArrayName
	: protected cArraySortName < TYPE, _TYPECH >
	{
		//! @class Gray::cThreadLockArrayName
		//! Thread Lockable, name sorted resource array.
		//! Must be locked before use of other methods!
		//! TYPE must support get_Name() and be cRefBase
		//! does  NOT allow dupe names !

		typedef cArraySortName<TYPE, _TYPECH> SUPER_t;
	public:
		mutable cThreadLockCount m_Lock;
	public:
		cThreadLockArrayName()
		{
		}
		~cThreadLockArrayName()
		{
		}

		// Locking helpers
		cRefPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const
		{
			//! @note Its slightly dangerous to enum a thread used list.
			//!  We could read the same entry 2 times !
			//! @note NEVER NEVER lock the list and the object at the same time!
			//!  This could create a permanent deadlock!
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::GetAtCheck(nIndex);
		}
		cRefPtr<TYPE> FindArgForKey(const _TYPECH* pszKey) const
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return(SUPER_t::FindArgForKey(pszKey));
		}
		ITERATE_t Add(TYPE* pObj)
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return(SUPER_t::Add(pObj));
		}
		bool RemoveArgKey(TYPE* pObj)
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::RemoveArgKey(pObj);
		}
		ITERATE_t GetSize() const
		{
			//! Used for statistical purposes. This may change of course.
			return SUPER_t::GetSize();	// just for stats purposes.
		}
		void RemoveAll()
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::RemoveAll();
		}
		// FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
	};

	template<class TYPE, typename _TYPE_HASH = HASHCODE_t>
	class cThreadLockArrayHash
	: protected cArraySortHash < TYPE, _TYPE_HASH >
	{
		//! @class Gray::cThreadLockArrayHash
		//! Thread safe hash.
		//! TYPE must support get_HashCode() and be cRefBase.
		//! Does NOT allow dupe hash codes !

		// friend class cPtrTraceMgr;
		typedef cArraySortHash<TYPE, _TYPE_HASH> SUPER_t;
	public:
		mutable cThreadLockCount m_Lock;
	public:
		cThreadLockArrayHash()
		{
		}
		~cThreadLockArrayHash()
		{
		}

		bool IsEmpty() const
		{
			//! Used for statistical purposes. This may change of course.
			return SUPER_t::IsEmpty();
		}
		ITERATE_t GetSize() const
		{
			//! Used for statistical purposes. This may change of course.
			return SUPER_t::GetSize();
		}
		cRefPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const
		{
			//! return a reference counted pointer. NOT a bare pointer.
			//! @note Its slightly dangerous to enum a thread used list.
			//!  We could read the same entry 2 times !
			//! @note NEVER NEVER lock the list and the object at the same time!
			//!  This could create a deadlock!
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::GetAtCheck(nIndex);
		}
		ITERATE_t Add(TYPE* pObj)
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::Add(pObj);	// AddTail
		}
		cRefPtr<TYPE> PopHead()
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopHead();	// pull off head
		}
		cRefPtr<TYPE> PopTail()
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopTail();
		}
		bool RemoveArgKey(TYPE* pObj)
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::RemoveArgKey(pObj);
		}
		void RemoveAll()
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::RemoveAll();
		}
		void DisposeAll()
		{
			//! ASSUME TYPE supports DisposeThis(); like cXObject
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::DisposeAll();
		}
		cRefPtr<TYPE> FindArgForKey(_TYPE_HASH hashcode) const
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::FindArgForKey(hashcode);
		}

#if 0
		ITERATE_t FindIForAK(const TYPE* pBase) const
		{
			// NOTE: Index must be considered invalid immediately!!
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::FindIForAK(pBase);
		}
#endif

		// FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
	};

	template<class TYPE, class _TYPE_KEY = ITERATE_t>
	class cThreadLockArrayValue
	: protected cArraySortValue < TYPE, _TYPE_KEY >
	{
		//! @class Gray::cThreadLockArrayValue
		//! Thread safe array
		//! TYPE must support get_SortValue() and be cRefBase
		//! does allow dupe get_SortValue but not dupe objects
		typedef cArraySortValue<TYPE, _TYPE_KEY> SUPER_t;
	public:
		mutable cThreadLockCount m_Lock;
	public:
		cThreadLockArrayValue()
		{
		}
		~cThreadLockArrayValue()
		{
		}

		ITERATE_t GetSize() const
		{
			//! Used for statistical purposes. This may change of course.
			return SUPER_t::GetSize();
		}
		cRefPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const
		{
			//! @note Its slightly dangerous to enum a thread used list.
			//!  We could read the same entry 2 times !
			//! @note NEVER NEVER lock the list and the object at the same time!
			//!  This could create a permanent deadlock!
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::GetAtCheck(nIndex);
		}
		ITERATE_t Add(TYPE* pObj)
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::Add(pObj) ;	// AddTail
		}
		ITERATE_t AddAfter(TYPE* pObj)
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::AddAfter(pObj);	// AddAfter
		}
		cRefPtr<TYPE> PopHead()
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopHead();	// pull off tail
		}
		cRefPtr<TYPE> PopTail()
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopTail();
		}
		bool RemoveArg(TYPE* pObj)
		{
			//! Since this can have dupes we should not use RemoveArgKey()
			//! @return true = removed. false = was not here.
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::RemoveArg(pObj);
		}
		void RemoveAll()
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::RemoveAll();
		}
		void DisposeAll()
		{
			//! ASSUME TYPE supports DisposeThis(); like cXObject
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::DisposeAll();
		}
#if 0
		ITERATE_t FindIForAK(const TYPE* pBase) const
		{
			// NOTE: Index must be considered invalid immediately!!
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return(SUPER_t::FindIForAK(pBase));
		}
#endif
		cRefPtr<TYPE> FindArgForKey(_TYPE_KEY index) const
		{
			cThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::FindArgForKey(index);
		}

		// FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
	};

#if 0
	template<class TYPE>
	class cThreadLockArrayWait
	: protected cArrayRef < TYPE >
	{
		//! @class Gray::cThreadLockArrayWait
		//! Thread safe array
		//! @todo Create an array where i can just wait for new stuff to be added to it.
		//! Used as a job queue.
	};
#endif
};
#endif
