//
//! @file CThreadArray.h
//! Thread safe arrays of stuff.
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CThreadArray_H
#define _INC_CThreadArray_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CThreadLock.h"
#include "CArraySort.h"

namespace Gray
{
	template <class TYPE>
	class CThreadLockArrayPtr
	: protected CArrayPtr < TYPE >
	{
		//! @class Gray::CThreadLockArrayPtr
		//! Thread safe array of pointers.

	public:
		typedef CArrayPtr<TYPE> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::ELEM_t ELEM_t;
	public:
		mutable CThreadLockCount m_Lock;
	public:
		CThreadLockArrayPtr()
		{
		}
		~CThreadLockArrayPtr()
		{
		}
		ITERATE_t GetSize() const
		{
			//! Used for statistical purposes. This may change of course.
			return SUPER_t::GetSize();	// just for stats purposes.
		}
		void SetSize(ITERATE_t nNewSize)
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::SetSize(nNewSize);	// just for stats purposes.
		}
		ITERATE_t Add(REF_t pObj)
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::Add(pObj); // add to tail
		}
		REF_t GetAtCheck(ITERATE_t nIndex) const
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::GetAtCheck(nIndex);
		}
		bool HasArg(TYPE* pObj) const
		{
			//! Find the index of a specified entry.
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return(SUPER_t::FindIFor(pObj) >= 0);
		}
		ELEM_t PopHead()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopHead();
		}
		ELEM_t PopTail()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopTail();
		}
		void DeleteAll()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			for (int i = 0; i < this->GetSize(); i++)
			{
				delete this->GetAt(i);
			}
			SUPER_t::RemoveAll();
		}
		bool RemoveArg(TYPE* pObj)
		{
			//! @return true = removed. false = was not here.
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::RemoveArg(pObj);
		}
	};

	template<class TYPE>
	class CThreadLockArraySmart
	: protected CArraySmart < TYPE >
	{
		//! @class Gray::CThreadLockArraySmart
		//! Thread safe array of smart pointers. NON sorted.
		typedef CArraySmart<TYPE> SUPER_t;
	public:
		mutable CThreadLockCount m_Lock;
	public:
		CThreadLockArraySmart()
		{
		}
		~CThreadLockArraySmart()
		{
		}

		ITERATE_t GetSize() const
		{
			//! Used for statistical purposes. This may change of course.
			return SUPER_t::GetSize();	// just for stats purposes.
		}

		// Locking helpers
		CSmartPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const
		{
			//! @note Its slightly dangerous to enum a thread used list.
			//!  We could read the same entry 2 times !
			//! @note NEVER NEVER lock the list and the object at the same time!
			//!  This could create a permanent deadlock!
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::GetAtCheck(nIndex);
		}
		CSmartPtr<TYPE> PopHead()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopHead();	// pull off tail
		}
		CSmartPtr<TYPE> PopTail()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopTail();	// pull off tail
		}
		bool HasArg(TYPE* pObj) const
		{
			//! Find a specified entry.
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return(SUPER_t::FindIFor(pObj) >= 0);
		}
		ITERATE_t Add(TYPE* pObj)
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::Add(pObj); // add to tail
		}
		ITERATE_t AddTail(TYPE* pObj)
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::AddTail(pObj); // add to tail
		}
		bool RemoveArg(TYPE* pObj)
		{
			//! @return true = removed. false = was not here.
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::RemoveArg(pObj);
		}
		void RemoveAll()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::RemoveAll();
		}
		void DisposeAll()
		{
			//! ASSUME TYPE supports DisposeThis(); like CXObject
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::DisposeAll();
		}
		// FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
	};

	//*************************************************

	template<class TYPE, typename _TYPECH = TCHAR >
	class CThreadLockArrayName
	: protected CArraySortName < TYPE, _TYPECH >
	{
		//! @class Gray::CThreadLockArrayName
		//! Thread Lockable, name sorted resource array.
		//! Must be locked before use of other methods!
		//! TYPE must support get_Name() and be CSmartBase
		//! does  NOT allow dupe names !

		typedef CArraySortName<TYPE, _TYPECH> SUPER_t;
	public:
		mutable CThreadLockCount m_Lock;
	public:
		CThreadLockArrayName()
		{
		}
		~CThreadLockArrayName()
		{
		}

		// Locking helpers
		CSmartPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const
		{
			//! @note Its slightly dangerous to enum a thread used list.
			//!  We could read the same entry 2 times !
			//! @note NEVER NEVER lock the list and the object at the same time!
			//!  This could create a permanent deadlock!
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::GetAtCheck(nIndex);
		}
		CSmartPtr<TYPE> FindArgForKey(const _TYPECH* pszKey) const
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return(SUPER_t::FindArgForKey(pszKey));
		}
		ITERATE_t Add(TYPE* pObj)
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return(SUPER_t::Add(pObj));
		}
		bool RemoveArgKey(TYPE* pObj)
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::RemoveArgKey(pObj);
		}
		ITERATE_t GetSize() const
		{
			//! Used for statistical purposes. This may change of course.
			return SUPER_t::GetSize();	// just for stats purposes.
		}
		void RemoveAll()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::RemoveAll();
		}
		// FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
	};

	template<class TYPE, typename _TYPE_HASH = HASHCODE_t>
	class CThreadLockArrayHash
	: protected CArraySortHash < TYPE, _TYPE_HASH >
	{
		//! @class Gray::CThreadLockArrayHash
		//! Thread safe hash.
		//! TYPE must support get_HashCode() and be CSmartBase.
		//! Does NOT allow dupe hash codes !

		// friend class CPtrTraceMgr;
		typedef CArraySortHash<TYPE, _TYPE_HASH> SUPER_t;
	public:
		mutable CThreadLockCount m_Lock;
	public:
		CThreadLockArrayHash()
		{
		}
		~CThreadLockArrayHash()
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
		CSmartPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const
		{
			//! return a reference counted pointer. NOT a bare pointer.
			//! @note Its slightly dangerous to enum a thread used list.
			//!  We could read the same entry 2 times !
			//! @note NEVER NEVER lock the list and the object at the same time!
			//!  This could create a deadlock!
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::GetAtCheck(nIndex);
		}
		ITERATE_t Add(TYPE* pObj)
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::Add(pObj);	// AddTail
		}
		CSmartPtr<TYPE> PopHead()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopHead();	// pull off head
		}
		CSmartPtr<TYPE> PopTail()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopTail();
		}
		bool RemoveArgKey(TYPE* pObj)
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::RemoveArgKey(pObj);
		}
		void RemoveAll()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::RemoveAll();
		}
		void DisposeAll()
		{
			//! ASSUME TYPE supports DisposeThis(); like CXObject
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::DisposeAll();
		}
		CSmartPtr<TYPE> FindArgForKey(_TYPE_HASH hashcode) const
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::FindArgForKey(hashcode);
		}

#if 0
		ITERATE_t FindIForAK(const TYPE* pBase) const
		{
			// NOTE: Index must be considered invalid immediately!!
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::FindIForAK(pBase);
		}
#endif

		// FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
	};

	template<class TYPE, class _TYPE_KEY = ITERATE_t>
	class CThreadLockArrayValue
	: protected CArraySortValue < TYPE, _TYPE_KEY >
	{
		//! @class Gray::CThreadLockArrayValue
		//! Thread safe array
		//! TYPE must support get_SortValue() and be CSmartBase
		//! does allow dupe get_SortValue but not dupe objects
		typedef CArraySortValue<TYPE, _TYPE_KEY> SUPER_t;
	public:
		mutable CThreadLockCount m_Lock;
	public:
		CThreadLockArrayValue()
		{
		}
		~CThreadLockArrayValue()
		{
		}

		ITERATE_t GetSize() const
		{
			//! Used for statistical purposes. This may change of course.
			return SUPER_t::GetSize();
		}
		CSmartPtr<TYPE> GetAtCheck(ITERATE_t nIndex) const
		{
			//! @note Its slightly dangerous to enum a thread used list.
			//!  We could read the same entry 2 times !
			//! @note NEVER NEVER lock the list and the object at the same time!
			//!  This could create a permanent deadlock!
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::GetAtCheck(nIndex);
		}
		ITERATE_t Add(TYPE* pObj)
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return(SUPER_t::Add(pObj));	// AddTail
		}
		ITERATE_t AddAfter(TYPE* pObj)
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::AddAfter(pObj);	// AddAfter
		}
		CSmartPtr<TYPE> PopHead()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopHead();	// pull off tail
		}
		CSmartPtr<TYPE> PopTail()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::PopTail();
		}
		bool RemoveArg(TYPE* pObj)
		{
			//! Since this can have dupes we should not use RemoveArgKey()
			//! @return true = removed. false = was not here.
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::RemoveArg(pObj);
		}
		void RemoveAll()
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::RemoveAll();
		}
		void DisposeAll()
		{
			//! ASSUME TYPE supports DisposeThis(); like CXObject
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			SUPER_t::DisposeAll();
		}
#if 0
		ITERATE_t FindIForAK(const TYPE* pBase) const
		{
			// NOTE: Index must be considered invalid immediately!!
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return(SUPER_t::FindIForAK(pBase));
		}
#endif
		CSmartPtr<TYPE> FindArgForKey(_TYPE_KEY index) const
		{
			CThreadGuard threadguard(m_Lock);	// thread sync critical section.
			return SUPER_t::FindArgForKey(index);
		}

		// FindIForKey, RemoveAt must use a lock outside as well ! (for index to be meaningful)
	};

#if 0
	template<class TYPE>
	class CThreadLockArrayWait
	: protected CArraySmart < TYPE >
	{
		//! @class Gray::CThreadLockArrayWait
		//! Thread safe array
		//! @todo Create an array where i can just wait for new stuff to be added to it.
		//! Used as a job queue.
	};
#endif
};
#endif
