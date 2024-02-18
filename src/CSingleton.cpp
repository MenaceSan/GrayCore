//! @file cSingleton.cpp
//! Register singletons for proper destruction.
//! @note Yes i know that the C run time will sort of do this for me using the { static construction } technique.
//! But i want more visibility and control of the destructors and guaranteed dynamic construction and memory allocation.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cArray.h"
#include "cLogMgr.h"
#include "cOSModImpl.h"
#include "cSingleton.h"

namespace Gray {
/// <summary>
/// Register all cSingleton(s) here, so they may be destroyed in proper order at C runtime destruct.
/// @note Yes i know the C runtime would mostly do this for me using localized statics.
///  but 1. i can't manually control order. 2. not thread safe. 3. can dynamic allocate (lazy load) not static allocate.
/// </summary>
class GRAYCORE_LINK cSingletonManager final : public cSingletonStatic<cSingletonManager> {
    cArrayPtr<cSingletonRegister> m_aSingletons;  /// my list of registered singletons. In proper order. castable to cSingletonStatic?
    static bool sm_bIsDestroyed;                  /// App is closing. safety catch for threads that are running past the exit code. cAppState::().isInCExit()

 public:
    cSingletonManager() noexcept : cSingletonStatic<cSingletonManager>(this) {}

    /// <summary>
    /// App is closing? isInCExit
    /// </summary>
    /// <returns></returns>
    static bool IsDestroyed() noexcept {
        return sm_bIsDestroyed;
    }

    /// <summary>
    /// clean up all singletons in a predictable order/manor.
    /// This is called by the C static runtime
    /// @note Referring to Singletons from a DLL that has already unloaded will crash.
    /// </summary>
    ITERATE_t ReleaseModule(::HMODULE hMod) {
        if (this == nullptr)  // this should never happen. but check for it in Release mode.
            return 0;

        ITERATE_t iQty = m_aSingletons.GetSize();
        ITERATE_t iCount = 0;
        for (ITERATE_t i = iQty; i;) {  // traverse list backwards.
            cSingletonRegister* pReg = m_aSingletons.GetAt(--i);
            ASSERT_NN(pReg);
            if (hMod != HMODULE_NULL && pReg->m_hModuleLoaded != hMod) {
                pReg->ReleaseModuleChildren(hMod);
                continue;
            }

            iCount++;
            m_aSingletons.RemoveAt(i);
            delete pReg;  // may/should try to remove itself from m_aSingletons

            DEBUG_ASSERT(m_aSingletons.GetSize() <= iQty, "m_aSingletons");
            i = iQty = m_aSingletons.GetSize();  // must start over.
        }
        if (iCount > 0) {
            DEBUG_MSG(("Release %d Singletons for module 0%x", iCount, hMod));
        }
        return iCount;
    }

    ~cSingletonManager() noexcept {
        ReleaseModule(HMODULE_NULL);  // release all.
        sm_bIsDestroyed = true;
    }

    ITERATE_t AddRegister(cSingletonRegister& reg) {
        //! Add to the end. so they are destructed in reverse order of creation.
        ASSERT(isSingleCreated() && !IsDestroyed());
        ASSERT(m_aSingletons.FindIFor(&reg) < 0);  // not already here.
        return m_aSingletons.Add(&reg);
    }
    bool RemoveRegister(cSingletonRegister* pReg) {
        //! May have already been removed if we are destructing app. but thats OK.
        DEBUG_CHECK(isSingleCreated());
        return m_aSingletons.RemoveArg(pReg);
    }
};

bool cSingletonManager::sm_bIsDestroyed = false;

cThreadLockCount cSingletonRegister::sm_LockSingle;  // common lock for all cSingleton.

cSingletonRegister::cSingletonRegister(const TYPEINFO_t& rAddrCode) noexcept {
#ifndef UNDER_CE
    //! Track the module that created the singleton. Maybe in a DLL ? @note __vfptr is based on allocation (new) NOT the same as the code that implements it !!!
    m_hModuleLoaded = cOSModule::GetModuleHandleForAddr(&rAddrCode);
#endif
}

void cSingletonRegister::RegisterSingleton() noexcept {
    //! register with cSingletonManager
    //! Only register this if we know its NOT static. We called new.
    const auto guard(sm_LockSingle.Lock());  // thread sync critical section all singletons.
    // Prevent re-registering of singletons constructed after SingletonManager shutdown (during exit)
    if (!cSingletonManager::IsDestroyed()) {  // special case. Core DLL was unloaded.
        if (!cSingletonManager::isSingleCreated()) {
            // Static init will get created / destroyed in the order it was first used.
            static cSingletonManager sm_The;
            DEBUG_CHECK(cSingletonManager::isSingleCreated());
        }
        cSingletonManager::I().AddRegister(*this);
    }
}

cSingletonRegister::~cSingletonRegister() noexcept {
    //! Allow Early removal of a singleton! This is sort of weird but i should allow it for DLL unload.
    const auto guard(sm_LockSingle.Lock());           // thread sync critical section all singletons.
    if (cSingletonManager::isSingleCreated()) {       // special case. DLL was unloaded.
        cSingletonManager::I().RemoveRegister(this);  // remove myself.
    }
}

void GRAYCALL cSingletonRegister::ReleaseModule(::HMODULE hMod) {  // static
    cSingletonManager::I().ReleaseModule(hMod);
}
}  // namespace Gray
