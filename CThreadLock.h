//
//! @file CThreadLock.h
//! Locking of objects for access by multiple threads
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CThreadLock_H
#define _INC_CThreadLock_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CNonCopyable.h"
#include "CTimeSys.h"
#include "CSmartPtr.h"
#include "COSHandle.h"
#include "CLocker.h"
#include "FileName.h"

#if defined(__linux__)
#include <pthread.h>
#endif
UNITTEST_PREDEF(CThreadId)

namespace Gray
{
#ifdef _WIN32
	typedef DWORD		THREADID_t;		//!< CreateThread uses LPDWORD in 64 bit code.
#define _SIZEOF_THREADID 4	// sizeof(THREADID_t)
#else
	typedef pthread_t	THREADID_t;		//!< @note old __linux__ gettid() is not compatible with pthreads
#define _SIZEOF_THREADID _SIZEOF_PTR
#endif

	class GRAYCORE_LINK CThreadId
	{
		//! @class Gray::CThreadId
		//! base static namespace for common thread functions.
		//! ASSUME all code wants defined(_MT) ?

	protected:
		THREADID_t m_dwThreadId;		//!< unique thread id. i.e. stack base pointer. (Use the MFC name)

	public:
		static const THREADID_t k_NULL = 0;	//!< Not a valid thread Id.

	public:
		CThreadId(THREADID_t nThreadId = k_NULL)
			: m_dwThreadId(nThreadId)
		{
		}
		THREADID_t GetThreadId() const
		{
			//! Similar to the MFC CWorkerThread call.
			//! __linux__ don't use the old fashioned gettid(). Also (m_hThread == THREADID_t)
			return m_dwThreadId;
		}
		THREADID_t get_HashCode() const
		{
			//! Get a unique hash code for the thread.
			return m_dwThreadId;
		}
		bool isCurrentThread() const
		{
			//!< Is this the current running thread?
			return(IsEqualId(m_dwThreadId, GetCurrentId()));
		}
		bool isValidId() const
		{
			return IsValidId(m_dwThreadId);
		}
		void InitCurrentId()
		{
			//! set equal to the current thread id.
			m_dwThreadId = GetCurrentId();
		}

		static inline THREADID_t GetCurrentId()
		{
			//! @note We ASSUME this is VERY fast.
			//! ASSUME IsValidThreadId();
#ifdef _WIN32
			return ::GetCurrentThreadId();
#else	// __linux__
			return ::pthread_self();
#endif
		}
		static inline bool IsValidId(THREADID_t id)
		{
			//! @todo actually check for the system thread.
			return (id != CThreadId::k_NULL);
		}
		static inline bool IsEqualId(THREADID_t a, THREADID_t b)
		{
			//! Are these id's the same thread? In Linux this might be similar to _WIN32 HANDLE.
#ifdef _WIN32
			return (a == b);
#else
			return ::pthread_equal(a, b);
#endif
		}
		static inline void SleepCurrent(TIMESYS_t uMs = CTimeSys::k_FREQ)
		{
			//! Sleep current thread for n Milliseconds. CTimeSys::k_FREQ
			//! Let the OS schedule something else during this time.
#ifdef _WIN32
			::Sleep(uMs);
#else
			::usleep((uMs) * 1000);	// Sleep current thread.
#endif
		};

		UNITTEST_FRIEND(CThreadId);
	};

	class GRAYCORE_LINK CThreadState
	{
		//! @class Gray::CThreadState
		//! Query the status/state of a thread/job and possibly attempt to cancel it.
		//! Similar to ICancellable and useful with IStreamProgressCallback

	protected:
		bool m_bThreadRunning;		//!< called CreateThread() onThreadCreate(), and inside Run(), until onThreadExit()
		VOLATILE bool m_bThreadStopping;		//!< trying to stop the thread nicely. Do this before TerminateThread()

	public:
		CThreadState()
			: m_bThreadRunning(false)
			, m_bThreadStopping(false)
		{
		}

		bool isThreadRunning() const
		{
			//! Running. though may be stopping/sleeping/suspended/etc.
			return m_bThreadRunning;
		}
		bool isThreadStopping() const
		{
			//! Thread MUST periodically check this !
			return m_bThreadStopping;
		}

		virtual bool RequestStopThread(bool bWillWait = false)
		{
			UNREFERENCED_PARAMETER(bWillWait);
			m_bThreadStopping = true;
			return isThreadRunning();
		}
	};

	class GRAYCORE_LINK CThreadLockBase : public CLockableBase, protected CNonCopyable
	{
		//! @class Gray::CThreadLockBase
		//! ASSUME sizeof(THREADID_t) <= sizeof(INT_PTR) _SIZEOF_PTR for __DECL_ALIGN.

	protected:
		THREADID_t __DECL_ALIGN(_SIZEOF_THREADID) m_nLockThreadID;	//!< The thread that has the lock. CThreadId:k_NULL is not locked.
	public:
		CThreadLockBase()
			: m_nLockThreadID(CThreadId::k_NULL)
		{
		}
		inline bool isLocked() const
		{
			//! Only thread safe way to test this is to look at m_nLockThreadID, NOT m_nLockCount
			return m_nLockThreadID != CThreadId::k_NULL;
		}
		inline THREADID_t get_ThreadLockOwner() const
		{
			//! @return CThreadId::k_NULL = not locked.
			THREADID_t nTid1 = m_nLockThreadID;
			ASSERT(nTid1 == CThreadId::k_NULL || CThreadId::IsValidId(nTid1));
			return(nTid1);
		}
		inline bool isThreadLockedByCurrent() const
		{
			THREADID_t nTid1 = m_nLockThreadID;
			THREADID_t nTid2 = CThreadId::GetCurrentId();
			return(CThreadId::IsEqualId(nTid1, nTid2));
		}
	};

	class GRAYCORE_LINK CThreadLockFast : public CThreadLockBase
	{
		//! @class Gray::CThreadLockFast
		//! used with any data structure that may be locked for multi threaded access.
		//! These are fairly cheap and fast. Slow on actual collision. (but collide assumed to be infrequent)
		//! @note reentrant, multi locks on a single thread are allowed and counted.
		typedef CThreadLockBase SUPER_t;
	public:
		CThreadLockFast()
		{
		}
		CThreadLockFast(const CThreadLockFast& a)
		{
			//! Copy constructor should never actually be called!
			//! but if it is, just make a new copy that is not locked!
			//! pretend we copied stuff?
			UNREFERENCED_REFERENCE(a);
		}
		~CThreadLockFast()
		{
		}

		bool ClearThreadLockOwner(THREADID_t nTid)
		{
			//! Special case if a thread is hard terminated.
			//! Only clear if nTid is the current owner.
			THREADID_t nTidowner = InterlockedN::CompareExchange(&m_nLockThreadID, CThreadId::k_NULL, nTid);
			return CThreadId::IsEqualId(nTidowner, nTid);
		}

		void Lock();
		bool LockTry(TIMESYSD_t dwDelayMS = 0);

		inline void Unlock()
		{
			//! inline version of this made bad code?
			//! ASSUME I own the lock. so this call doesn't really need to be thread safe.
			ASSERT(isThreadLockedByCurrent());
			if (SUPER_t::DecLockCount() == 0)
			{
				InterlockedN::Exchange(&m_nLockThreadID, CThreadId::k_NULL);
			}
		}
	};

	typedef CLockerT<CThreadLockFast> CThreadGuardFast;

	class GRAYCORE_LINK CThreadLockMutex : public CThreadLockBase
	{
		//! @class Gray::CThreadLockMutex
		//! Base class for data structure that may be locked for multi threaded/process access.
		//! These are expensive size wise but fast.
		//! lock something and wait for it to be freed.
		//! @note reentrant, multi locks on a single thread are allowed and counted.
		//! fast collision resolution.
		//! Mutex = 1 bathroom door 1 lock. Semaphore = a set of X keys to X bathrooms.
		//! m_nLockThreadID = API wont tell me if it is locked. i have to track this myself.
		//! @note I've seen use of this in Windows Service on W10 cause app exit !? 2015

		typedef CThreadLockBase SUPER_t;
	public:
#ifdef _WIN32
		COSHandle m_Mutex;
#else // __linux__
		pthread_mutex_t m_Mutex;	// PTHREAD_MUTEX_INITIALIZER
		static const pthread_mutex_t k_MutexInit;	// PTHREAD_MUTEX_INITIALIZER
#endif
	protected:
		bool m_bInitialOwner;		//!< I also lock this myself.

	public:
		CThreadLockMutex(const CThreadLockMutex& a)
			: m_bInitialOwner(false)
		{
			//! Copy constructor should never actually be used.
			//! but if it is  just make a new copy that is not locked!
			//! DUMMY = pretend we copied stuff.
			UNREFERENCED_REFERENCE(a);
			InitLockMutex(nullptr, false);
		}
		CThreadLockMutex(const FILECHAR_t* pszMutexName = nullptr, bool bInitialOwner = false)
			: m_bInitialOwner(bInitialOwner)
		{
			InitLockMutex(pszMutexName, bInitialOwner);
		}
		inline ~CThreadLockMutex()
		{
			// DEBUG_CHECK( ! IsLocked());
			if (m_bInitialOwner && isLocked())	 // m_nLockCount == 1
			{
				this->Unlock();
			}
#if defined(__linux__)
			::pthread_mutex_destroy(&m_Mutex);
#endif
		}

	protected:
		void InitLockMutex(const FILECHAR_t* pszMutexName, bool bInitialOwner)
		{
			//! Create a system mutex.
			//! @arg bInitialOwner = owned by the thread that created this.
#ifdef _WIN32
			m_Mutex.AttachHandle(_FNF(::CreateMutex)(nullptr, bInitialOwner, pszMutexName));
			if (bInitialOwner)
			{
				LockInternal();
			}
#else // __linux__
			// Allow this mutex to be opened multiple times in the same thread.
			m_Mutex = k_MutexInit;		// same as ::pthread_mutex_init() ??
			if (bInitialOwner)
			{
				Lock();
			}
#endif
		}

		inline void LockInternal()
		{
			//! i was able to acquire this Mutex. some other way like WaitForMultipleObjects() ?
			m_nLockThreadID = CThreadId::GetCurrentId();
			SUPER_t::IncLockCount();
			ASSERT(isThreadLockedByCurrent()); // may have several locks on the same thread.
		}

	public:
		inline bool Lock()
		{
			//! code to lock a thread.
			//! This will wait forever for the resource to be free !
			//! @note It should NOT wait if it is in the same thread.
#ifdef _WIN32
			HRESULT hRes = m_Mutex.WaitForSingleObject(CTimeSys::k_INF);
			if (hRes != S_OK)
			{
				return false;
			}
#else
			// If the mutex is already locked (by diff thread), the calling thread blocks until the mutex becomes available
			int iRet = ::pthread_mutex_lock(&m_Mutex);
			if (iRet)
			{
				// error on lock. EINVAL
				return false;
			}
#endif
			LockInternal(); // may have several locks on the same thread.
			return true;
		}
		inline bool Unlock()
		{
			// Assume i own the lock. so thread safe isn't a worry on entry.
			ASSERT(isThreadLockedByCurrent()); // may have several locks on the same thread.
			if (SUPER_t::DecLockCount() <= 0)
			{
				m_nLockThreadID = CThreadId::k_NULL;
			}
#ifdef _WIN32
			return ::ReleaseMutex(m_Mutex) ? true : false;
#else
			::pthread_mutex_unlock(&m_Mutex);
			return true;
#endif
		}

		bool LockTry(TIMESYSD_t dwDelayMS = 0)
		{
			//! Try to lock the mutex. give up after a certain amount of time if it is locked by another thread.
			//! dwDelayMS = amount of time to wait. 0 = don't wait
#ifdef _WIN32
			HRESULT hRes = m_Mutex.WaitForSingleObject(dwDelayMS);
			if (hRes != S_OK)
			{
				return false;
			}
#elif defined(__USE_XOPEN2K)	// __linux__
			CTimeSpec tSpec(dwDelayMS);
			int iRet = ::pthread_mutex_timedlock(&m_Mutex, &tSpec);
			if (iRet != 0)
			{
				return false;
			}
#else
			// if pthread_mutex_timedlock() is not available.
			TIMESYSD_t dwWaitTimeMS = 0;
			for (;;)
			{
				if (::pthread_mutex_trylock(&m_Mutex) == 0)
					break;
				if (dwDelayMS <= 0)	// EBUSY
				{
					return false; // FAILED to lock
				}
				CThreadId::SleepCurrent(dwWaitTimeMS); // wait for a tick.
				if (dwWaitTimeMS == 0)
				{
					dwWaitTimeMS = 1;
				}
				else
				{
					dwDelayMS--;
				}
			}
#endif
			LockInternal();
			return true;
		}
	};

	typedef CLockerT<CThreadLockMutex> CThreadGuardMutex;

#ifdef _WIN32
	class GRAYCORE_LINK CThreadLockCrit : public CThreadLockBase
	{
		//! @class Gray::CThreadLockCrit
		//! Base class for data structure that may be locked for multi threaded access.
		//! These are fairly expensive size wise but fast.
		//! Use a cheaper CThreadLockFast for normal data that wont collide often.
		//! Same as MFC CComCriticalSection
		//! @note This is essentially the same as a mutex. CPU burn during collision.
		//! @note reentrant, multi locks on a single thread are allowed and counted.
		//! m_nLockThreadID = i have to glean this info myself. not sure why the API wont give it to me.

	protected:
		CRITICAL_SECTION m_CritSection;	// RTL_CRITICAL_SECTION, more efficient than a MUTEX. but takes more memory to store.

	private:
		void LockInternal()
		{
			m_nLockThreadID = CThreadId::GetCurrentId();
			IncLockCount();
			ASSERT(isThreadLockedByCurrent()); // may have several locks on the same thread.
		}
		void InitLockCrit()
		{
			//! RTL_CRITICAL_SECTION
			::InitializeCriticalSection(&m_CritSection);
		}

	public:
		CThreadLockCrit(const CThreadLockCrit& a)
		{
			//! Copy constructor should never actually be used.
			//! but if it is  just make a new copy that is not locked!
			//! DUMMY = pretend we copied stuff.
			UNREFERENCED_REFERENCE(a);
			InitLockCrit();
		}
		CThreadLockCrit()
		{
			InitLockCrit();
		}
		inline ~CThreadLockCrit()
		{
			//! DEBUG_CHECK( ! IsLocked());
			::DeleteCriticalSection(&m_CritSection);
		}

	public:
		inline void Lock()
		{
			//! code to lock a thread.
			//! This will wait forever for the resource to be free !
			//! @note It will NOT wait if it is in the same thread.
			::EnterCriticalSection(&m_CritSection);
			LockInternal();
		}
		inline void Unlock()
		{
			ASSERT(isThreadLockedByCurrent()); // may have several locks on the same thread.
			if (DecLockCount() <= 0)
			{
				m_nLockThreadID = CThreadId::k_NULL;
			}
			::LeaveCriticalSection(&m_CritSection);
		}

#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0400)
		bool LockTry()
		{
			//! don't wait.
			if (::TryEnterCriticalSection(&m_CritSection) == 0)
				return false;
			LockInternal();
			return true;
		}
#endif
	};
#else
	typedef CThreadLockMutex CThreadLockCrit;	// just substitute it if not _WIN32.
#endif // _WIN32

	class CThreadLockStub : public CLockableBase
	{
		//! @class Gray::CThreadLockStub
		//! Stub that does nothing. For stub out in single thread environments or debug usage.
	public:
		inline THREADID_t get_ThreadLockOwner() const
		{
			//! @return CThreadId::k_NULL = not locked.
			return (THREADID_t)isLocked() ? 1 : 0;
		}
	};

	//****************************************************************************

#if defined(_MT) || defined(__linux__)
	typedef CThreadLockFast CThreadLockCount;
#else
	typedef CThreadLockStub CThreadLockCount;
#endif
	typedef CLockerT<CThreadLockCount> CThreadGuard;	// instantiated locker.

	//*********************************************

	class GRAYCORE_LINK CThreadLockableObj
		: public CSmartBase
		, public CThreadLockCount
	{
		//! @class Gray::CThreadLockableObj
		//! Base class for a dynamic data structure that may be locked for multi threaded access (CThreadLockCount)
		//!  and locked for delete/usage (CSmartBase).
		//! These are fairly cheap and fast.
	public:
		CThreadLockableObj(int iStaticRefCount = 0)
			: CSmartBase(iStaticRefCount)
		{
		}
		virtual ~CThreadLockableObj()
		{
		}
		virtual void onThreadLockFail(TIMESYSD_t dwWaitMS)
		{
			//! a DEBUG trap for locks failing.
			UNREFERENCED_PARAMETER(dwWaitMS);
		}
	};

	//****************************************************************************

	template<class TYPE = CThreadLockableObj >
	class CThreadLockPtr : public CSmartPtr < TYPE >
	{
		//! @class Gray::CThreadLockPtr
		//! a CSmartPtr (inc ref count for delete protection) that also thread locks the object. (like CLockerT)
		//! Similar to the MFC CMultiLock or CSingleLock and CSmartPtr
		//! @note TYPE we are referring to MUST be based on CThreadLockableObj
		//! Thread Lock an object for as long as 'this' object persists.
		//! May wait if some other thread has the object locked.
		//! @note there must also be a non thread locking smart pointer ref to the object.
		//!	always thread locking an object for its whole life makes no sense.

		typedef CThreadLockPtr<TYPE> THIS_t;

#if defined(_MT) || defined(__linux__)
		//! NON _MT Stub does nothing really.

	private:
		void SetFirstLockObj(TYPE* p2)
		{
			//! @note Lock can throw !
			if (p2 != nullptr)
			{
#ifdef _DEBUG
				ASSERT(DYNPTR_CAST(CThreadLockableObj, p2) != nullptr);
				ASSERT(DYNPTR_CAST(TYPE, p2) != nullptr);
#endif
				p2->IncRefCount();
				p2->Lock();
			}
			this->m_p = p2;
		}
		bool SetFirstLockObjTry(TYPE* p2, TIMESYSD_t dwWaitMS)
		{
			//! @note Lock can throw !
			//! dwWaitMS = 0 = don't wait.
			if (p2 != nullptr)
			{
#ifdef _DEBUG
				ASSERT(DYNPTR_CAST(CThreadLockableObj, p2) != nullptr);
				ASSERT(DYNPTR_CAST(TYPE, p2) != nullptr);
#endif
				p2->IncRefCount();
				if (!p2->LockTry(dwWaitMS))
				{
					if (dwWaitMS)
					{
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
		CThreadLockPtr()
		{
		}
		CThreadLockPtr(TYPE* p2)
		{
			//! @note = assignment will auto destroy previous and use this constructor.
			SetFirstLockObj(p2);
		}
		CThreadLockPtr(TYPE* p2, TIMESYSD_t dwWaitMS)
		{
			//! @note = assignment will auto destroy previous and use this constructor.
			//! dwWaitMS = 0 = don't wait.
			SetFirstLockObjTry(p2, dwWaitMS);
		}
		CThreadLockPtr(const THIS_t& ref)
		{
			//! using the assignment auto constructor is not working so use this.
			SetFirstLockObj(ref.get_Ptr());
		}
		~CThreadLockPtr()
		{
			ReleasePtr();
		}

		void ReleasePtr()
		{
			TYPE* p2 = this->m_p;	// make local copy
			if (p2 != nullptr)
			{
				this->m_p = nullptr;
#ifdef _DEBUG
				ASSERT(DYNPTR_CAST(TYPE, p2) != nullptr);
#endif
				p2->Unlock();
				p2->DecRefCount();
			}
		}

		operator TYPE*() const
		{
			return(this->m_p);
		}
		TYPE& operator * () const
		{
			ASSERT(this->m_p != nullptr); return *(this->m_p);
		}
		TYPE* operator -> () const
		{
			ASSERT(this->m_p != nullptr); return(this->m_p);
		}

		void put_Ptr(TYPE* p2)
		{
			//! override CSmartPtr put_Ptr
			if (p2 == this->m_p)
				return;
			ReleasePtr();
			SetFirstLockObj(p2);
		}
		bool SetLockObjTry(TYPE* p2, TIMESYSD_t dwWaitMS)
		{
			if (p2 == this->m_p)
				return true;
			ReleasePtr();
			return SetFirstLockObjTry(p2, dwWaitMS);
		}

		// Assignment
		const THIS_t& operator = (TYPE* p2)
		{
			put_Ptr(p2);
			return *this;
		}
		const THIS_t& operator = (const THIS_t& ref)
		{
			put_Ptr(ref.m_p);
			return *this;
		}
#endif // defined(_MT) || __linux__
	};

	typedef CThreadLockPtr<CThreadLockableObj> CThreadLockPtrX;
};

#endif // _INC_CThreadLock_H
