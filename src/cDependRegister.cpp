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
#include "cAppState.h"

namespace Gray {

cLockerT<cThreadLockableFast> GRAYCALL cSingletonBase::GetLockAll() {

static cThreadLockableFast sm_LockAll;  /// lock all use of singletons to a thread.
    return sm_LockAll.Lock();
    }

/// <summary>
/// Register all cSingleton(s) here, so they may be destroyed in proper order at C runtime destruct. (or unload their own children).
/// @note Yes i know the C runtime would mostly do this for me using localized statics.
///  but 1. i can't manually control order. 2. not thread safe. 3. can dynamic allocate (lazy load) not static allocate.
/// </summary>
class GRAYCORE_LINK cDependMgr final : public cSingletonStatic<cDependMgr> {
    typedef cSingletonStatic<cDependMgr> SUPER_t;
    cArrayPtr<cDependRegister> _aSingletons;  /// my list of registered singletons. In proper dependency order. use GetLockAll().
 
 public:
    DECLARE_cSingletonStatic(cDependMgr);

    cDependMgr() noexcept : SUPER_t(this) {
        ASSERT(!cAppState::isInCExit());
    }
 
    /// <summary>
    /// clean up all singletons in a predictable order/manor.
    /// This is called by the C static runtime
    /// @note Referring to Singletons from a DLL that has already unloaded will crash.
    /// </summary>
    ITERATE_t ReleaseModule(::HMODULE hMod) {
        if (!cMem::IsValidApp(this)) return 0;  // this should never happen. but check for it in Release mode.

        const auto guard(cSingletonBase::GetLockAll());  // thread sync critical section.
        ITERATE_t iQty = _aSingletons.GetSize();
        ITERATE_t iCount = 0;
        for (ITERATE_t i = iQty; i;) {  // traverse list backwards.
            cDependRegister* pReg = _aSingletons.GetAt(--i);
            ASSERT_NN(pReg);
            if (hMod != HMODULE_NULL && pReg->_hModuleLoaded != hMod) {
                pReg->ReleaseModuleChildren(hMod);
                continue;
            }

            iCount++;
            _aSingletons.RemoveAt(i);
            if (!pReg->isReferenced()) {
                // pReg->get_HeapPtr() ?
                delete pReg;  // It also may/should try to remove itself from _aSingletons. should fail of course.
            }

            DEBUG_ASSERT(_aSingletons.GetSize() <= iQty, "_aSingletons");
            i = iQty = _aSingletons.GetSize();  // must start over.
        }
        if (iCount > 0) {
            DEBUG_MSG(("Release %d Singletons for module 0%x", iCount, hMod));
        }
        return iCount;
    }

    ~cDependMgr() noexcept {
        ReleaseModule(HMODULE_NULL);  // release all.
    }

    /// <summary>
    /// No dupes!
    /// </summary>
    bool IsRegistered(cDependRegister& reg) const noexcept {
        return _aSingletons.HasArg3(&reg);
    }

    /// <summary>
    /// Add to the end. so they are destructed in reverse order of creation. ASSUME GetLockAll()
    /// </summary>
    ITERATE_t AddRegister(cDependRegister& reg) {
        ASSERT(SUPER_t::isSingleCreated() && !cAppState::isInCExit());
        if (IsRegistered(reg)) return k_ITERATE_BAD;  // not already here.
        return _aSingletons.Add(&reg);
    }

    /// <summary>
    /// May have already been removed if we are destructing app. but thats OK. ASSUME GetLockAll()
    /// </summary>
    bool RemoveRegister(cDependRegister* pReg) {
        DEBUG_CHECK(SUPER_t::isSingleCreated());
        return _aSingletons.RemoveArg(pReg);
    }
};

cSingletonStatic_IMPL(cDependMgr);
 
cDependRegister::cDependRegister(const TYPEINFO_t& rAddrCode) noexcept
    : _hModuleLoaded(cOSModule::GetModuleHandleForAddr(&rAddrCode))
#ifdef _DEBUG
      ,
      _rTypeInfo(rAddrCode),
      _DebugTag(rAddrCode.name())
#endif
{
}

cDependRegister::~cDependRegister() {
    const auto guard(cSingletonBase::GetLockAll());        // thread sync critical section all singletons.
    if (cDependMgr::isSingleCreated()) {                   // special case. DLL was unloaded.
        bool bRet = cDependMgr::I().RemoveRegister(this);  // remove myself. might be redundant.
        UNREFERENCED_PARAMETER(bRet);
        // DEBUG_CHECK(bret);
    }
}

void cDependRegister::RegisterSingleton() noexcept {
    const auto guard(cSingletonBase::GetLockAll());  // thread sync critical section all singletons.
    // Prevent re-registering of singletons constructed after SingletonManager shutdown (during exit)
    if (cAppState::isInCExit()) return;
    auto& dm = cDependMgr::I();
    dm.AddRegister(*this);
}

void GRAYCALL cDependRegister::ReleaseModule(::HMODULE hMod) {  // static
    if (cDependMgr::isSingleCreated()) {
        cDependMgr::I().ReleaseModule(hMod);
    }
}
}  // namespace Gray
