//
//! @file CThreadLockRW.h
//! Read/Write declarative thread locking. similar to __linux__ pthread_rwlock_t
//! more flexible/efficient than CThreadLock as most lockers are just readers.
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//! @todo CThreadLockRW

#ifndef _INC_CThreadLockRW_H
#define _INC_CThreadLockRW_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CThreadLock.h"
#include "CInterlockedVal.h"
#include "CNonCopyable.h"

UNITTEST_PREDEF(CThreadLockRW)

namespace Gray
{
	class GRAYCORE_LINK CThreadLockRWS : protected CNonCopyable
	{
		//! @class Gray::CThreadLockRWS
		//! Simple NON recursive, NON upgradeable, read / write locking.
		//! from: http://www.viksoe.dk/code/rwmonitor.htm
		//! @todo CThreadLockRW NOT complete
		//! similar to https://msdn.microsoft.com/en-us/library/windows/desktop/aa904937(v=vs.85).aspx e.g. InitializeSRWLock()

	private:
		CInterlockedInt m_nReaders;
		CInterlockedInt m_nWriters;

	public:
		void IncReadLockCount()
		{
			for (;;)
			{
				m_nReaders.IncV();
				if (m_nWriters == 0)
					break;
				m_nReaders.DecV();
				CThreadId::SleepCurrent(0);
			}
		}
		void DecReadLockCount()
		{
			m_nReaders.DecV();
		}

		void Lock()
		{
			// Write lock
			for (;;)
			{
				if (m_nWriters.Exchange(1) == 1)
				{
					CThreadId::SleepCurrent(0);
				}
				else
				{
					while (m_nReaders != 0)
						CThreadId::SleepCurrent(0);
					break;
				}
			}
		}

		void Unlock()
		{
			// Write unlock
			m_nWriters.DecV();
		}
	};

	class GRAYCORE_LINK CThreadLockRW : public CThreadLockFast
	{
		//! @class Gray::CThreadLockRW
		//! @todo CThreadLockRW
		//! cheap RW declaring thread locking mechanism.
		//! Recursive and upgradeable (i.e. i can start with read and upgrade to write permissions on the same thread)
		//! Only one thread may write lock something.
		//! multiple read locks may be released out of  order, i.e first read locker releases with other readers = unknown read thread.
		//! similar to : Linux thread locking pthread_rwlock_wrlock()
		//!
		//! RULES:
		//!  FR = first thread reader.
		//!  FW = first thread writer. (just like a normal thread lock)
		//!  OR = other thread reader
		//!  OW = other thread writer.
		//!  if the first locker is write, FR=go, FW=go, OR=wait, OW=wait
		//!  if the first locker is reader, FR=go, FW=go, OR=go, OW=wait.

		typedef CThreadLockFast SUPER_t;

	public:
		// CThreadLock::m_nLockThreadID;		// First reader or writers .
		// CThreadLock::m_nLockCount;			// How many write locks (for orig m_nLockThreadID)
		CInterlockedInt m_nReadLockCount;		//!< How many readers (for orig m_nLockThreadID)
		CInterlockedInt m_nOtherReadLockCount;	//!< How many outside (not on orig thread) readers
		bool m_bLostOrder;	//!< can't figure who is thread.

	public:
		CThreadLockRW()
		: m_bLostOrder(false)
		{
		}
		~CThreadLockRW()
		{
			ASSERT(m_nReadLockCount == 0);
			ASSERT(m_nOtherReadLockCount == 0);
		}

		inline void IncReadLockCount()
		{
			// this is a slightly softer READ lock.
			if (SUPER_t::LockTry())
			{
				// I'm first locker or at least the same thread as the first.
				m_nReadLockCount.IncV();
				return;
			}
			// some other thread has the write/read lock before me. can i just read it ?
			m_nOtherReadLockCount.IncV();
			if (m_nReadLockCount == get_LockCount())	// another reader is OK.
			{
			}
			else
			{
			}
		}
		inline void DecReadLockCount()
		{
		}
		UNITTEST_FRIEND(CThreadLockRW);
	};

	class CThreadGuardRead : public CLockerT < CThreadLockRW >
	{
		//! @class Gray::CThreadGuardRead
		//! I only want to read from this.

	public:
		CThreadGuardRead(CThreadLockRW& rLock)
		: CLockerT<CThreadLockRW>(rLock)
		{
		}
		// TODO: call IncReadLockCount
	};

	typedef CLockerT<CThreadLockRW> CThreadGuardWrite;	// I only want to write to this.

	//******************************************************

	class GRAYCORE_LINK CThreadLockableRW
	: public CSmartBase
	, public CThreadLockRW
	{
		//! @class Gray::CThreadLockableRW
		//! An smart pointer referenced object that can be read/write locked.
		//! similar to CThreadLockableObj
	};

	template<class TYPE>
	class CSmartReadPtr : public CSmartPtr<TYPE>, public CThreadGuardRead
	{
		//! @class Gray::CSmartReadPtr
		//! I promise to only read from the CThreadLockableRW based object.
		//! If another thread is open writing then we must wait.
		//! If any thread has other read opens then it's OK.
		//! No need to lock an object if 2 threads are just reading it!
		//! we MUST record the read action in case a writer (on another thread) comes along.
		//! @note this only returns 'const' pointers of course.

		CSmartReadPtr(TYPE* pObj)
		: CSmartPtr<TYPE>(pObj)
		, CThreadGuardRead(*pObj)
		{
		}

		const TYPE* get_Ptr() const
		{
			return(this->m_pObj);
		}
	};
	template<class TYPE>
	class CSmartWritePtr : public CSmartPtr<TYPE>, public CThreadGuardWrite
	{
		//! @class Gray::CSmartWritePtr
		//! we would like to write to the CThreadLockableRW based object.
		//! If another thread has it open (read or write) then we must wait.
		CSmartWritePtr(TYPE* pObj)
		: CSmartPtr<TYPE>(pObj)
		, CThreadGuardWrite(*pObj)
		{
		}
	};
};
#endif // _INC_CThreadLockRW_H
