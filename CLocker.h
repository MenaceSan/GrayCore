//
//! @file CLocker.h
//! Locking of objects for any reason. (Thread lock or DX buffer usage lock)
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CLocker_H
#define _INC_CLocker_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CPtrFacade.h"

namespace Gray
{
	class GRAYCORE_LINK CLockableBase
	{
		//! @class Gray::CLockableBase
		//! Common base for CThreadLockBase and all lock / thread lock / mutex implementations.
		//! derived class can override Lock,Unlock
		//! NOT thread safe. Assume caller handles thread safety. Use CInterlockedVal if we want/need thread safety here.
		//! Similar to CSmartBase and CInterlockedVal.
	private:
		LONG m_nLockCount;	//!< count Lock vs Unlock calls. Should NEVER be negative.

	protected:
		CLockableBase()
		: m_nLockCount(0)
		{
		}
		~CLockableBase()
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
			// For template support. CLockerT
			IncLockCount();
		}
		inline LONG Unlock()
		{
			// For template support. CLockerT
			return DecLockCount();
		}
	};

	template<class TYPE = CLockableBase>
	class CLockerT : public CPtrFacade < TYPE >
	{
		//! @class Gray::CLockerT
		//! Lock/Unlock something for the life span of this object.
		//! Stack only based guard. Used for: CThreadLockMutex, CThreadLockCrit, CThreadLockFast, ...
		//! Might be used for: CDXSurfaceLock, CSemaphoreLock, CWinHeap, CDXMesh, CDXBuffer
		//! TYPE must support Unlock() and probably Lock() or be based on CLockableBase*
		//! m_p = the lock we are locking.
		//! Similar to ATL CCritSecLock
		//
	public:
		explicit CLockerT(TYPE* pLock, bool bLockSuccess)
		: CPtrFacade<TYPE>(bLockSuccess ? pLock : nullptr)
		{
			// The lock may not always work !
		}
		explicit CLockerT(TYPE& rLock)
		: CPtrFacade<TYPE>(&rLock)
		{
			ASSERT(this->m_p != nullptr);
			this->m_p->Lock();
		}
		~CLockerT()
		{
			if (this->m_p != nullptr)
			{
				this->m_p->Unlock();
			}
		}
	};
};

#endif
