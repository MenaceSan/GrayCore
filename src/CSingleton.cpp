//
//! @file cSingleton.cpp
//! Register singletons for proper destruction.
//! @note Yes i know that the C run time will sort of do this for me using the { static construction } technique.
//! But i want more visibility and control of the destructors and guaranteed dynamic construction and memory allocation.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cSingleton.h"
#include "cArray.h"
#include "cOSModImpl.h"
#include "cLogMgr.h"

namespace Gray
{
	class GRAYCORE_LINK cSingletonManager : public cSingletonStatic < cSingletonManager > 
	{
		//! @class Gray::cSingletonManager
		//! Register all cSingleton(s) here, so they may be destroyed in proper order at C runtime destruct.
		//! @note Yes i know the C runtime would mostly do this for me using localized statics.
		//!  but 1. i can't manually control order. 2. not thread safe. 3. can dynamic allocate (lazy load) not static allocate.

	private:
		cArrayPtr<cSingletonRegister> m_aSingletons;	//!< my list of registered singletons. In proper order.
		static bool sm_bIsDestroyed;	//!< safety catch for threads that are running past the exit code. cAppState::().isInCExit()

	public:
		cSingletonManager() noexcept
			: cSingletonStatic<cSingletonManager>(this)
		{
		}
		~cSingletonManager() noexcept
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
				cSingletonRegister* pReg = m_aSingletons.PopTail();
				delete pReg;	// should remove itself from m_aSingletons
			}
			sm_bIsDestroyed = true;
		}

		ITERATE_t ReleaseModule(HMODULE hMod)
		{
			//! IOSModuleRelease
			//! delete any singletons in hMod space.
			//! When a module is released, all its singletons from it MUST be destroyed. 
			//! @return Number of releases.
			ITERATE_t iCount = 0;
			for (ITERATE_t i = m_aSingletons.GetSize() - 1; i >= 0; i--)
			{
				cSingletonRegister* pReg = m_aSingletons[i];
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

		ITERATE_t AddReg(cSingletonRegister* pReg)
		{
			//! Add to the end. so they are destructed in reverse order of creation.
			ASSERT(isSingleCreated());
			ASSERT(pReg != nullptr);
			ASSERT(m_aSingletons.FindIFor(pReg) < 0); // not already here.
			return m_aSingletons.AddTail(pReg);
		}
		bool RemoveReg(cSingletonRegister* pReg)
		{
			//! May have already been removed if we are destructing app. but thats OK.
			DEBUG_CHECK(isSingleCreated());
			return m_aSingletons.RemoveArg(pReg);
		}
		static bool isDestroyed() noexcept
		{
			return sm_bIsDestroyed;
		}
	};

	bool cSingletonManager::sm_bIsDestroyed = false;

	cThreadLockFast cSingletonRegister::sm_LockSingle; // common lock for all cSingleton.

	cSingletonRegister::cSingletonRegister(const TYPEINFO_t& rAddrCode) noexcept
	{
#ifndef UNDER_CE
		// Track the module that created the singleton. Maybe in a DLL ?
		m_hModuleLoaded = cOSModule::GetModuleHandleForAddr(&rAddrCode);
#endif
	}

	void GRAYCALL cSingletonRegister::RegisterSingleton(cSingletonRegister& reg) noexcept // static
	{
		//! register with cSingletonManager
		//! Only register this if we know its NOT static. We called new.
		cThreadGuardFast threadguard(sm_LockSingle);	// thread sync critical section all singletons.
		if (!cSingletonManager::isSingleCreated())
		{
			// Static init will get created / destroyed in the order it was first used.
			static cSingletonManager sm_The;
			DEBUG_CHECK(cSingletonManager::isSingleCreated());
		}

		// Prevent re-registering of singletons constructed after SingletonManager shutdown (during exit)
		if (!cSingletonManager::isDestroyed())	// special case. DLL was unloaded.
		{
			cSingletonManager::I().AddReg(&reg);
		}
	}

	cSingletonRegister::~cSingletonRegister() noexcept
	{
		//! Allow Early removal of a singleton! This is sort of weird but i should allow it for DLL unload.
		cThreadGuardFast threadguard(sm_LockSingle);	// thread sync critical section all singletons.
		if (cSingletonManager::isSingleCreated())	// special case. DLL was unloaded.
		{
			cSingletonManager::I().RemoveReg(this);		// remove myself.
		}
	}

	void GRAYCALL cSingletonRegister::ReleaseModule(HMODULE hMod) // static
	{
		cSingletonManager::I().ReleaseModule(hMod);
	}
}
 