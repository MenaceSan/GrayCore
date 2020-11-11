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

namespace Gray
{
	class GRAYCORE_LINK cLockableBase
	{
		//! @class Gray::cLockableBase
		//! Common base for cThreadLockBase and all lock / thread lock / mutex implementations.
		//! derived class can override Lock,Unlock
		//! NOT thread safe. Assume caller handles thread safety. Use cInterlockedVal if we want/need thread safety here.
		//! Similar to cRefBase and cInterlockedVal.
	private:
		LONG m_nLockCount;	//!< count Lock vs Unlock calls. Should NEVER be negative.

	protected:
		cLockableBase() noexcept
		: m_nLockCount(0)
		{
		}
		~cLockableBase()
		{
			ASSERT(m_nLockCount == 0);
		}
		inline LONG get_LockCount() const
		{
			ASSERT(m_nLockCount >= 0);
			return m_nLockCount;
		}
		inline bool isLocked() const
		{
			ASSERT(m_nLockCount >= 0);
			return(m_nLockCount != 0);
		}

		inline void IncLockCount()
		{
			++m_nLockCount;
			ASSERT(m_nLockCount >= 0);
		}
		inline LONG DecLockCount()
		{
			--m_nLockCount;
			ASSERT(m_nLockCount >= 0);
			return m_nLockCount;
		}
		inline void Lock()
		{
			// For template support. cLockerT
			IncLockCount();
		}
		inline LONG Unlock()
		{
			// For template support. cLockerT
			return DecLockCount();
		}
	};

	template<class TYPE = cLockableBase>
	class cLockerT : public cPtrFacade < TYPE >
	{
		//! @class Gray::cLockerT
		//! Lock/Unlock something for the life span of this object.
		//! Stack only based guard. Used for: cThreadLockMutex, cThreadLockCrit, cThreadLockFast, ...
		//! Might be used for: CDXSurfaceLock, CSemaphoreLock, cWinHeap, CDXMesh, CDXBuffer
		//! TYPE must support Unlock() and probably Lock() or be based on cLockableBase*
		//! m_p = the lock we are locking.
		//! Similar to ATL CCritSecLock
		//
	public:
		explicit cLockerT(TYPE* pLock, bool bLockSuccess)
			: cPtrFacade<TYPE>(bLockSuccess ? pLock : nullptr)
		{
			// The lock may not always work ! this->m_p == nullptr
		}
		explicit cLockerT(TYPE& rLock)
			: cPtrFacade<TYPE>(&rLock)
		{
			ASSERT(this->m_p == &rLock && this->m_p != nullptr);
			rLock.Lock();
		}
		~cLockerT()
		{
			if (this->m_p != nullptr)
			{
				this->m_p->Unlock();
			}
		}
	};
};

#endif
