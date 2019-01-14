//
//! @file CSingleton.cpp
//! Register singletons for proper destruction.
//! @note Yes i know that the C run time will sort of do this for me using the { static construction } technique.
//! But i want more visibility and control of the destructors and guaranteed dynamic construction and memory allocation.
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#include "StdAfx.h"
#include "CSingleton.h"
#include "CArray.h"
#include "COSModImpl.h"
#include "CLogMgr.h"

namespace Gray
{
	class GRAYCORE_LINK CSingletonManager : public CSingletonStatic < CSingletonManager >, public IOSModuleRelease
	{
		//! @class Gray::CSingletonManager
		//! Register all CSingleton(s) here, so they may be destroyed in proper order at C runtime destruct.
		//! @note Yes i know the C runtime would mostly do this for me using localized statics.
		//!  but 1. i can't manually control order. 2. not thread safe. 3. can dynamic allocate (lazy load) not static allocate.

	private:
		CArrayPtr<CSingletonRegister> m_aSingletons;	//!< my list of registered singletons. In proper order.
		static bool sm_bIsDestroyed;	//!< safety catch for threads that are running past the exit code. CAppState::().isInCExit()

	public:
		CSingletonManager()
			: CSingletonStatic<CSingletonManager>(this)
		{
		}
		~CSingletonManager()
		{
			//! clean up all singletons in a predictable order/manor.
			//! This is called by the C static runtime
			//! @note Destroying Singletons from a DLL that has already unloaded will crash.

			for (ITERATE_t iCount = 0; !m_aSingletons.IsEmpty(); iCount++)
			{
				if (iCount >= SHRT_MAX)
				{
					// Some sort of deadlock of singletons creating/using each other in destructors.
					ASSERT(iCount < SHRT_MAX);
					break;
				}
				CSingletonRegister* pReg = m_aSingletons.PopTail();
				delete pReg;
			}
			sm_bIsDestroyed = true;
		}

		virtual ITERATE_t ReleaseModule(HMODULE hMod) override
		{
			//! IOSModuleRelease
			//! When a module is released, all its singletons from it MUST be destroyed. 
			//! Any singletons in hMod space must go.
			//! @return Number of releases.
			ITERATE_t iCount = 0;
			for (ITERATE_t i = m_aSingletons.GetSize() - 1; i >= 0; i--)
			{
				CSingletonRegister* pReg = m_aSingletons[i];
#ifndef UNDER_CE
				if (hMod != HMODULE_NULL && pReg->m_hModuleLoaded != hMod)
					continue;
#endif
				iCount++;
				m_aSingletons.RemoveAt(i);
				delete pReg;
				i = m_aSingletons.GetSize();	// must start over.
			}
			if (iCount > 0)
			{
				DEBUG_MSG(("Release %d Singletons for module 0%x", iCount, hMod));
			}
			return iCount;
		}

		ITERATE_t AddReg(CSingletonRegister* pReg)
		{
			//! Add to the end. so they are destructed in reverse order of creation.
			ASSERT(isSingleCreated());
			ASSERT(pReg != nullptr);
			ASSERT(m_aSingletons.FindIFor(pReg) < 0); // not already here.
			return m_aSingletons.AddTail(pReg);
		}
		bool RemoveReg(CSingletonRegister* pReg)
		{
			//! May have already been removed if we are destructing app. but thats OK.
			ASSERT(isSingleCreated());
			return m_aSingletons.RemoveArg(pReg);
		}
		static bool isDestroyed()
		{
			return sm_bIsDestroyed;
		}
	};

	bool CSingletonManager::sm_bIsDestroyed = false;

	CThreadLockFast CSingletonRegister::sm_LockSingle; // common lock for all CSingleton.

	CSingletonRegister::CSingletonRegister(const TYPEINFO_t& rAddrCode)
	{
#ifndef UNDER_CE
		// Track the module that created the singleton. Maybe in a DLL ?
		m_hModuleLoaded = COSModule::GetModuleHandleForAddr(&rAddrCode);
#endif
	}

	void CSingletonRegister::RegisterSingleton()
	{
		//! register with CSingletonManager
		//! Only register this if we know its NOT static. We called new.
		CThreadGuardFast threadguard(sm_LockSingle);	// thread sync critical section all singletons.
		if (!CSingletonManager::isSingleCreated())
		{
			// Static init will get created / destroyed in the order it was first used.
			static CSingletonManager sm_The;
			ASSERT(CSingletonManager::isSingleCreated());
		}

		// Prevent re-registering of singletons constructed after SingletonManager shutdown (during exit)
		if (!CSingletonManager::isDestroyed())
			CSingletonManager::I().AddReg(this);
	}

	CSingletonRegister::~CSingletonRegister()
	{
		//! Allow Early removal of a singleton! This is sort of weird but i should allow it.
		CThreadGuardFast threadguard(sm_LockSingle);	// thread sync critical section all singletons.
		CSingletonManager::I().RemoveReg(this);
	}

	void GRAYCALL CSingletonRegister::ReleaseModule(HMODULE hMod) // static
	{
		CSingletonManager::I().ReleaseModule(hMod);
	}
}

//*************************************************************************
#ifdef USE_UNITTESTS
#include "CUnitTest.h"
#include "CLogMgr.h"

namespace Gray
{
	class CUnitTestSing : public CSingleton < CUnitTestSing >
	{
	public:
		CUnitTestSing() : CSingleton<CUnitTestSing>(this, typeid(CUnitTestSing))
		{
		}
		CHEAPOBJECT_IMPL;
	};
};
UNITTEST_CLASS(CSingletonRegister)
{
	UNITTEST_METHOD(CSingletonRegister)
	{
		UNITTEST_TRUE(!CUnitTestSing::isSingleCreated());
		{
			CUnitTestSing inst;
			UNITTEST_TRUE(CUnitTestSing::isSingleCreated());
			// Will be destructed at app close.
		}
		UNITTEST_TRUE(!CUnitTestSing::isSingleCreated());
	}
};
UNITTEST_REGISTER(CSingletonRegister, UNITTEST_LEVEL_Core);
#endif
