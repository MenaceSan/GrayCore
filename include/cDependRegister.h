//! @file cDependRegister.h
//! Control order or destruction. Used for singletons mostly.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cModuleRegister_H
#define _INC_cModuleRegister_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cHeapObject.h"
#include "cThreadLock.h"
#include "cTypeInfo.h"

namespace Gray {
/// <summary>
/// NON template abstract base for any object that must get destroyed in proper order. (singletons). (or unload their own children)
/// Register this to allow for proper order of virtual destruction at C runtime destruct.
/// Allows for ordered destruction of singletons if modules unload. (Not in proper reverse load order)
/// @note Static singletons are not multi threaded anyhow. so don't worry about static init order for sm_LockSingle. assume static init is single threaded.
/// </summary>
class GRAYCORE_LINK cDependRegister : public IHeapObject {
    friend class cDependMgr;
    // CHEAPOBJECT_IMPL;  /// Get the top level "new" pointer in the case of multiple inheritance.

 protected:
#ifndef UNDER_CE
    ::HMODULE m_hModuleLoaded;  /// What module loaded this ? So singletons can be destroyed if DLL/SO unloads. cOSModule::GetModuleHandleForAddr()
#endif
 public:
    static cThreadLockableX sm_LockSingle;  /// common lock for all cSingleton.

 protected:
    cDependRegister(const TYPEINFO_t& rAddrCode) noexcept;
    virtual ~cDependRegister();

    /// <summary>
    /// register myself with cModuleManager
    /// </summary>
    void RegisterSingleton() noexcept;

    /// <summary>
    /// override this to Destroy any children I might have from some other HMODULE.
    /// </summary>
    /// <param name="hMod"></param>
    /// <returns></returns>
    virtual void ReleaseModuleChildren(::HMODULE hMod) {
        ASSERT(m_hModuleLoaded != hMod);
    }

    /// <summary>
    /// Should SingletonManager call delete on this? MUST be IHeapObject (else isReferenced should be true)
    /// </summary>
    /// <returns></returns>
    virtual bool isReferenced() const noexcept = 0;

 public:
    /// <summary>
    /// Destroy all cDependRegister in an HMODULE (assume it is unloading).
    /// </summary>
    static void GRAYCALL ReleaseModule(::HMODULE hMod);
};
}  // namespace Gray
#endif
