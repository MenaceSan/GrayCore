//! @file cDependRegister.cpp
//! Register singletons (and other objects) for proper destruction.
//! @note Yes i know that the C run time will sort of do this for me using the { static construction } technique.
//! But i want more visibility and control of the destructors and guaranteed dynamic construction and memory allocation.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "FuncPtr.h"
#include "cArray.h"
#include "cDependRegister.h"
#include "cLogMgr.h"
#include "cOSModImpl.h"
#include "cSingleton.h"

namespace Gray {
/// <summary>
/// Register all cSingleton(s) here, so they may be destroyed in proper order at C runtime destruct. (or unload their own children).
/// @note Yes i know the C runtime would mostly do this for me using localized statics.
///  but 1. i can't manually control order. 2. not thread safe. 3. can dynamic allocate (lazy load) not static allocate.
/// </summary>
class GRAYCORE_LINK cDependMgr final : public cSingletonStatic<cDependMgr> {
    typedef cSingletonStatic<cDependMgr> SUPER_t;
    cArrayPtr<cDependRegister> m_aSingletons;  /// my list of registered singletons. In proper order. 
    static bool sm_bIsDestroyed;                 /// App is closing. safety catch for threads that are running past the exit code. cAppState::().isInCExit()

 public:
    cDependMgr() noexcept : cSingletonStatic<cDependMgr>(this) {
        ASSERT(!sm_bIsDestroyed);
    }

    /// <summary>
    /// App is closing? isInCExit()
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
        if (!cMem::IsValidApp(this)) return 0;  // this should never happen. but check for it in Release mode.

        ITERATE_t iQty = m_aSingletons.GetSize();
        ITERATE_t iCount = 0;
        for (ITERATE_t i = iQty; i;) {  // traverse list backwards.
            cDependRegister* pReg = m_aSingletons.GetAt(--i);
            ASSERT_NN(pReg);
            if (hMod != HMODULE_NULL && pReg->m_hModuleLoaded != hMod) {
                pReg->ReleaseModuleChildren(hMod);
                continue;
            }

            iCount++;
            m_aSingletons.RemoveAt(i);
            if (!pReg->isReferenced()) {
                // pReg->get_HeapPtr() ?
                delete pReg;  // It also may/should try to remove itself from m_aSingletons. should fail of course.
            }

            DEBUG_ASSERT(m_aSingletons.GetSize() <= iQty, "m_aSingletons");
            i = iQty = m_aSingletons.GetSize();  // must start over.
        }
        if (iCount > 0) {
            DEBUG_MSG(("Release %d Singletons for module 0%x", iCount, hMod));
        }
        return iCount;
    }

    ~cDependMgr() noexcept {
        sm_bIsDestroyed = true;       // ASSUME isInCExit()
        ReleaseModule(HMODULE_NULL);  // release all.
    }

    ITERATE_t AddRegister(cDependRegister& reg) {
        //! Add to the end. so they are destructed in reverse order of creation.
        ASSERT(SUPER_t::isSingleCreated() && !IsDestroyed());
        ASSERT(!m_aSingletons.HasArg3(&reg));  // not already here.
        return m_aSingletons.Add(&reg);
    }
    bool RemoveRegister(cDependRegister* pReg) {
        //! May have already been removed if we are destructing app. but thats OK.
        DEBUG_CHECK(SUPER_t::isSingleCreated());
        return m_aSingletons.RemoveArg(pReg);
    }
};

bool cDependMgr::sm_bIsDestroyed = false;

cThreadLockableX cDependRegister::sm_LockSingle;  // common lock for all cSingleton.

cDependRegister::cDependRegister(const TYPEINFO_t& rAddrCode) noexcept {
#ifndef UNDER_CE
    //! Track the module that created the singleton. Maybe in a DLL ? @note __vfptr is based on allocation (new) NOT the same as the code that implements it !!!
    m_hModuleLoaded = cOSModule::GetModuleHandleForAddr(&rAddrCode);
#endif
}

cDependRegister::~cDependRegister() {
    //! Allow Early removal of a singleton! This is sort of weird but i should allow it for DLL unload.
    const auto guard(sm_LockSingle.Lock());          // thread sync critical section all singletons.
    if (cDependMgr::isSingleCreated()) {       // special case. DLL was unloaded.
        cDependMgr::I().RemoveRegister(this);  // remove myself.
    }
}

void cDependRegister::RegisterSingleton() noexcept {
    const auto guard(sm_LockSingle.Lock());  // thread sync critical section all singletons.
    // Prevent re-registering of singletons constructed after SingletonManager shutdown (during exit)
    if (!cDependMgr::IsDestroyed()) {  // special case. Core DLL was unloaded.
        if (!cDependMgr::isSingleCreated()) {
            // Static init will get created / destroyed in the order it was first used.
            static cDependMgr sm_The;
            DEBUG_CHECK(cDependMgr::isSingleCreated());
        }
        cDependMgr::I().AddRegister(*this);
    }
}

void GRAYCALL cDependRegister::ReleaseModule(::HMODULE hMod) {  // static
    cDependMgr::I().ReleaseModule(hMod);
}
}  // namespace Gray
