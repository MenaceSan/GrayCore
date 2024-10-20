//! @file cOSModImpl.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cOSModImpl_H
#define _INC_cOSModImpl_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArraySort.h"
#include "cAtom.h"
#include "cOSModule.h"
#include "cSingleton.h"

#ifdef _MFC_VER
#include "SysRes.h"
#include <afxwin.h>  // MFC extensions AFX_EXTENSION_MODULE. includes <afxdll_.h>
#endif

namespace Gray {
/// <summary>
/// My implementation of a DLL/SO. _WINDLL
/// Must be only one of these in a single link space for each DLL/SO. So it is not a true cSingleton .
/// Always static allocated. NEVER heap or Stack allocated.
/// Assume g_Module gets defined for the DLL/SO. On some derived class based on cOSModImpl named g_Module. In some new name space.
/// e.g. cOSModImpl g_Module("ModuleName");
/// @todo Support _WIN32 DLL_THREAD_ATTACH and DLL_THREAD_DETACH ?
/// Similar to MFC AFX_EXTENSION_MODULE DLLModule or CAtlDllModuleT
/// @note See #include "GrayLib/include/System/cOSModDyn.h" for cOSModDynImpl_DEF()
/// This might have a corresponding cXObjModulePtr. cIUnkPtr can be used alternatively.
/// </summary>
class GRAYCORE_LINK cOSModImpl {
    /// <summary>
    /// My internal name. Just derive this from the file name ?
    /// </summary>
    const char* const _pszModuleName;
    /// <summary>
    /// My id assigned to me when loaded DllMain(). DLL_PROCESS_ATTACH
    /// address NOT cOSHandle (Windows) or cOSHandle (Linux)?
    /// should be same as GetModuleHandleForAddr(g_Module)
    /// </summary>
    ::HMODULE _hModule = HMODULE_NULL;

    /// <summary>
    /// DLL_PROCESS_ATTACH happen BEFORE constructor! OnProcessAttach only called once!
    /// </summary>
    bool _isProcessAttached = false;

#ifdef _MFC_VER
    ::AFX_EXTENSION_MODULE _AFXExt;  // I might be a MFC extension module. hResource, IsInit
#endif

 private:
    bool OnProcessAttachTry();

 public:
    /// <summary>
    /// Always static allocated. NEVER heap or Stack allocated. So Zero init is NEVER needed!
    /// </summary>
    cOSModImpl(const char* pszModuleName) noexcept;

    virtual ~cOSModImpl() {}

    const char* get_Name() const {
        return _pszModuleName;
    }

    /// both constructor and OnProcessAttach called?
    bool IsLoaded() const noexcept {
        return _pszModuleName != nullptr && _hModule != HMODULE_NULL;
    }

    ::HMODULE get_HMod() const noexcept {
        return _hModule;
    }

    /// <summary>
    /// This module has been loaded in a process space.
    /// </summary>
    /// <returns></returns>
    virtual bool OnProcessAttach();
    virtual void OnProcessDetach();  // DLL_PROCESS_DETACH

#ifdef _WIN32
    virtual bool DllMain(HINSTANCE hInstDll, DWORD dwReason);
#elif defined(__linux__)
    void SOConstructor();
#endif
};

#ifndef GRAY_STATICLIB
// ASSUME cOSModImpl_DEF is defined for DLL/SO. (and is outside namespace).
// @note See #include <GrayCore/include/cOSModDyn.h> for cOSModDynImpl_DEF()
// Declare/expose DllMain()
#ifdef _WIN32  // _WINDLL
#define cOSModImpl_DEF(N) \
    __DECL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hInstDll, DWORD dwReason, LPVOID) { return N::g_Module.DllMain(hInstDll, dwReason); }
#elif defined(__linux__)
#define cOSModImpl_DEF(N)                                                          \
    CATTR_CONSTRUCTOR void _cdecl SOConstructor() { N::g_Module.SOConstructor(); } \
    CATTR_DESTRUCTOR void _cdecl SODestructor() { g_Module.OnProcessDetach(); }
#endif
#endif

}  // namespace Gray

#endif
