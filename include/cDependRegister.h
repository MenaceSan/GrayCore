//! @file cDependRegister.h
//! Control order or destruction. Used for singletons mostly.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cDependRegister_H
#define _INC_cDependRegister_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cHeapObject.h"
#include "cThreadLock.h"
#include "cTypeInfo.h"
#include "cString.h"

namespace Gray {
/// <summary>
/// NON template abstract base for any object that must get destroyed in proper order. (singletons). (or unload their own children)
/// Register this to allow for proper order of virtual destruction at C runtime destruct.
/// Allows for ordered destruction of singletons if modules unload. (Not in proper reverse load order)
/// @note Static singletons are not multi threaded anyhow. so don't worry about static init order for GetLockAll. assume static init is single threaded.
/// </summary>
class GRAYCORE_LINK cDependRegister {
    friend class cDependMgr;
 
 protected:
    ::HMODULE _hModuleLoaded;  /// What module implements this? (from TYPEINFO_t) So singletons can be destroyed if DLL/SO unloads. cOSModule::GetModuleHandleForAddr()

#ifdef _DEBUG
    const TYPEINFO_t& _rTypeInfo;
    cStringA _DebugTag;
#endif

 protected:
    /// <summary>
    /// Assume RegisterSingleton() will be called later!
    /// _hModuleLoaded = the top level module that created the singleton. Maybe in a DLL?
    /// </summary>
    /// <param name="rAddrCode"></param>
    cDependRegister(const TYPEINFO_t& rAddrCode) noexcept;

    /// <summary>
    /// register myself with cModuleManager. ASSUME use GetLockAll.
    /// </summary>
    void RegisterSingleton() noexcept;

    /// <summary>
    /// override this to Destroy any children I might have from some other HMODULE.
    /// </summary>
    /// <param name="hMod"></param>
    /// <returns></returns>
    virtual void ReleaseModuleChildren(::HMODULE hMod) {
        ASSERT(_hModuleLoaded != hMod);
    }

    /// <summary>
    /// Should cDependMgr call 'delete this'? MUST be IHeapObject (else isReferenced should be true)
    /// </summary>
    /// <returns></returns>
    virtual bool isReferenced() const noexcept = 0;

 public:
    /// <summary>
    /// Allow Early removal of a singleton! This is sort of weird but allow it for DLL unload.
    /// </summary>
    virtual ~cDependRegister();

    /// <summary>
    /// Destroy/Dispose all cDependRegister in an HMODULE (assume it is unloading).
    /// </summary>
    static void GRAYCALL ReleaseModule(::HMODULE hMod);
};
}  // namespace Gray
#endif
