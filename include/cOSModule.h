//! @file cOSModule.h
//! Manages links to a *.dll module file.
//! __linux__ link with "dl" library
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cOSModule_H
#define _INC_cOSModule_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "FuncPtr.h"
#include "cBits.h"
#include "cFilePath.h"
#include "cHandlePtr.h"
#include "cMime.h"

#ifdef _WIN32
#define MODULE_EXT MIME_EXT_dll

#elif defined(__linux__)
#include <dlfcn.h>
#define MODULE_EXT MIME_EXT_so

#else
#error NOOS
#endif

namespace Gray {
#define HMODULE_NULL ((::HMODULE) nullptr)  // This sometimes means the current process module.

/// <summary>
/// manage access to a dynamically loaded *.DLL file. (or .SO in __linux__)
/// in _WIN32 HMODULE is just a load address. Not the same as cOSHandle.
/// ASSUME Default = loaded into my app space ! Use cOSModuleX for other processes modules.
/// Inside a DLL there may be procedures and resources.
/// DLL's are "shared objects" or "shared libraries" in __linux__
/// __linux__ link with "dl" library
/// other times use 1. static binding or 2. delayed binding to DLL
/// @todo Get module footprint info. how much memory does it use?
/// @not VERY IMPORTANT!! for M$ C++ 2019, Objects allocated (new) in a DLL have a vtable assigned by the module. when unloaded this vtable is unloaded. This means all these objects are corrupt. Even though the memory they occupy is still valid.
/// </summary>
class GRAYCORE_LINK cOSModule {
    ::HMODULE m_hModule;  /// sometimes the same as HINSTANCE ? = loading address of the code. cHandlePtr NOT cOSHandle.
    UINT32 m_uFlags;      /// k_Load_RefCount= I am responsible to unload this since i loaded it. k_Load_Resource = This is not callable code. init was not called and refs not loaded.
#if defined(__linux__)
    cStringF m_sModuleName;  /// Must store this if __linux__ can't query it directly.
#endif

 protected:
    /// <summary>
    /// decrement my ref count for this module. Assume someone else will clear m_hModule
    /// @note if this is the last ref to the DLL then it will be unloaded!
    /// @note DANGER! If we free a DLL that has vtable stuff in it, any objects based on these vtables are now broken!
    /// </summary>
    void FreeModuleLast();

 public:
#ifdef _WIN32

#ifndef LOAD_LIBRARY_AS_IMAGE_RESOURCE
#define LOAD_LIBRARY_AS_IMAGE_RESOURCE 0x00000020  // __GNUC__
#endif
    static const UINT k_Load_Normal = 0;                                 /// Flags.
    static const UINT k_Load_Preload = DONT_RESOLVE_DLL_REFERENCES;      /// 0x01. Just load into memory but don't call any init code. DANGER: No recovery from this. MUST unload. Diff from __linux RTLD_LAZY.
    static const UINT k_Load_Resource = LOAD_LIBRARY_AS_IMAGE_RESOURCE;  /// 0x20. This is just a resource load. RTLD_LAZY, LOAD_LIBRARY_AS_IMAGE_RESOURCE
#elif defined(__linux__)
    static const UINT k_Load_Normal = RTLD_NOW;     /// Flags.
    static const UINT k_Load_Preload = RTLD_LAZY;   /// Lazy load. Don't call init till we know we need them. Diff from __linux RTLD_LAZY, RTLD_NOLOAD
    static const UINT k_Load_Resource = RTLD_LAZY;  /// This is just a resource load. RTLD_LAZY, LOAD_LIBRARY_AS_IMAGE_RESOURCE
#endif
    static const UINT k_Load_OSMask = 0x0FFFFFFF;
    static const UINT k_Load_ByName = 0x40000000;      /// try to find it (by just its file name, NOT Path) already loaded first. NOT OS flag.
    static const UINT k_Load_NoRefCount = 0x80000000;  /// I DO NOT own the ref count. Don't free. NOT OS Flag.

    cOSModule(::HMODULE hModule = HMODULE_NULL, UINT32 uFlags = k_Load_Normal);
    cOSModule(const FILECHAR_t* pszModuleName, UINT32 uFlags);
    cOSModule(cOSModule&& m);
    ~cOSModule();

    static MIME_t GRAYCALL CheckModuleTypeFile(const FILECHAR_t* pszPathName);

#ifndef UNDER_CE
    /// <summary>
    /// Get the handle for the module this pAddr is in. Do NOT increment ref count for ::HMODULE.
    /// code may be part of a shared object or DLL. track its handle in case it is unloaded dynamically
    /// </summary>
    /// <param name="pAddr">a function pointer?</param>
    /// <returns>cAppState::get_HModule() = just part of the current EXE. or HMODULE_NULL = error;</returns>
    static ::HMODULE GRAYCALL GetModuleHandleForAddr(const void* pAddr);
#endif

    /// <summary>
    /// Get a Generic function call address in the module. assume nothing about the functions args.
    /// @note No such thing as a UNICODE proc/function/symbol name! object formats existed before UNICODE. Except for UNDER_CE which does support UNICODE proc names!?
    /// @note this does not work if loaded using LOAD_LIBRARY_AS_IMAGE_RESOURCE or LOAD_LIBRARY_AS_DATAFILE
    /// </summary>
    /// <param name="pszSymbolName"></param>
    /// <returns>address</returns>
    FUNCPTR_t GetSymbolAddress(const char* pszSymbolName) const;

    bool isValidModule() const noexcept {
        return m_hModule != HMODULE_NULL;
    }
    operator ::HMODULE() const noexcept {
        return m_hModule;
    }
    ::HMODULE get_HModuleX() const noexcept {
        return m_hModule;
    }
    /// <summary>
    /// Get the modules handle as an int.
    /// </summary>
    /// <returns></returns>
    UINT_PTR get_ModuleInt() const noexcept {
        return CastPtrToNum(m_hModule);
    }
    /// <summary>
    /// We cant call this. its not loaded as code.
    /// </summary>
    bool isResourceModule() const noexcept {
        return cBits::HasAny(m_uFlags, k_Load_Preload | k_Load_Resource);
    }

    /// <summary>
    /// Get the file path to the loaded module.
    /// @note there is not absolute rule that it must have one.
    /// </summary>
    /// <param name="name"></param>
    /// <returns>length, 0 = fail.</returns>
    StrLen_t GetModulePath(cSpanX<FILECHAR_t> ret) const;

    /// <summary>
    /// get Full path to the module. for name sorting.
    /// </summary>
    cStringF get_Name() const;

    HRESULT GetLastErrorDef(HRESULT hResDef = E_FAIL) const noexcept {
#ifdef _WIN32
        return HResult::GetLastDef(hResDef);
#elif defined(__linux__)
        // Just a string?
        const char* pszError = ::dlerror();
        UNREFERENCED_PARAMETER(pszError);
        return E_NOTIMPL;
#endif
    }

    void AttachModule(::HMODULE hModule = HMODULE_NULL, UINT32 uFlags = k_Load_Normal) {
        FreeModuleLast();
        m_hModule = hModule;
        m_uFlags = uFlags;
    }
    void ClearModule() noexcept {
        // Don't decrement load count (FreeModuleLast) even if i should.
        m_hModule = HMODULE_NULL;
        m_uFlags = k_Load_Normal;
#ifdef __linux__
        m_sModuleName.Empty();
#endif
    }
    ::HMODULE DetachModule() noexcept {
        const ::HMODULE hModule = m_hModule;
        ClearModule();
        return hModule;
    }

    /// <summary>
    /// decrement my usage count. It might be freed/unloaded.
    /// </summary>
    void FreeThisModule();

    bool AttachModuleName(const FILECHAR_t* pszModuleName, UINT32 uFlags = k_Load_NoRefCount);
    HRESULT LoadModule(const FILECHAR_t* pszModuleName, UINT32 uFlags = k_Load_Normal);

    /// <summary>
    /// Load this module ONLY if it exposes this symbol.
    /// </summary>
    /// <param name="pszModuleName"></param>
    /// <param name="pszSymbolName"></param>
    /// <returns>HRESULT_WIN32_C(ERROR_CALL_NOT_IMPLEMENTED) = I don't have this symbol. 'The specified module could not be found. (08007007e)' = HRESULT_WIN32_C(ERROR_MOD_NOT_FOUND) = Not what it seems. Might be one of my dependencies could not be
    /// found!</returns>
    HRESULT LoadModuleWithSymbol(const FILECHAR_t* pszModuleName, const char* pszSymbolName);

#ifdef _WIN32
    // Load or enum resources ?? cWinResource, cAppRes
#endif
};

/// <summary>
/// track a single TYPE based entry point/procedure/function in the DLL/SO/Module file.
/// @note It is VERY important to know the proper number and size of args before calling m_pFunc !!! (if not _cdecl)
/// </summary>
/// <typeparam name="TYPE">FUNCPTR_t</typeparam>
template <class TYPE>
class cOSModuleFunc {
 public:
    TYPE m_pFunc;  /// something like:  typedef int (FAR WINAPI *FUNCTYPENAME)();
 public:
    cOSModuleFunc(TYPE pFunc = nullptr) : m_pFunc(pFunc) {}
    ~cOSModuleFunc() {}
    void ClearFuncAddress() {
        m_pFunc = nullptr;
    }
    bool put_FuncAddress(TYPE pFunc) {
        m_pFunc = pFunc;
        if (m_pFunc == nullptr) return false;
        return true;
    }
    bool put_FuncGeneric(FUNCPTR_t pFunc) {
        m_pFunc = CastFPtrTo<TYPE>(pFunc);
        if (m_pFunc == nullptr) return false;
        return true;
    }
    bool isValidFunc() const {
        return m_pFunc != nullptr;  // IsValidFunction()
    }
    operator TYPE() const {
        return m_pFunc;
    }
};

#ifndef GRAY_STATICLIB  // force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cOSModuleFunc<FUNCPTR_t>;
#endif
}  // namespace Gray
#endif  // _INC_cOSModule_H
