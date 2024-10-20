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
/// @not VERY IMPORTANT!! for M$ C++ 2019, Objects allocated (new) in a DLL have a vtable assigned by the module!! when unloaded this vtable is unloaded. This means all these objects are corrupt. Even though the memory they occupy is still valid and the actual implementation code for the class might still be loaded.
/// </summary>
class GRAYCORE_LINK cOSModule {
    ::HMODULE _hModule = HMODULE_NULL;  /// sometimes the same as HINSTANCE ? = loading address of the code. cHandlePtr NOT cOSHandle.
    UINT32 _uFlags = 0;                 /// k_Load_Normal=0= I am responsible to unload this since i loaded it. k_Load_Resource = This is not callable code. init was not called and refs not loaded.
    mutable cStringF _modulePath;       /// Must store this. __linux__ can't query it directly.

 protected:
    void DebugTestPoint() const;

    /// <summary>
    /// decrement my ref count for this module. Assume someone else will clear _hModule
    /// @note if this is the last ref to the DLL then it will be unloaded!
    /// @note DANGER! If we free a DLL that has vtable stuff in it, any objects based on these vtables are now broken!
    /// </summary>
    void FreeModuleLast();

 public:
#ifdef _WIN32

#ifndef LOAD_LIBRARY_AS_IMAGE_RESOURCE
#define LOAD_LIBRARY_AS_IMAGE_RESOURCE 0x00000020  // __GNUC__
#endif
    static const UINT k_Load_Normal = 0;                                 /// Flags. decrement ref count of destruct.
    static const UINT k_Load_Preload = DONT_RESOLVE_DLL_REFERENCES;      /// 0x01. Just load into memory but don't call any init code. DANGER: No recovery from this. MUST unload. Diff from __linux RTLD_LAZY.
    static const UINT k_Load_Resource = LOAD_LIBRARY_AS_IMAGE_RESOURCE;  /// 0x20. This is just a resource load. RTLD_LAZY, LOAD_LIBRARY_AS_IMAGE_RESOURCE
#elif defined(__linux__)
    static const UINT k_Load_Normal = RTLD_NOW;     /// Flags.
    static const UINT k_Load_Preload = RTLD_LAZY;   /// Lazy load. Don't call init till we know we need them. Diff from __linux RTLD_LAZY, RTLD_NOLOAD
    static const UINT k_Load_Resource = RTLD_LAZY;  /// This is just a resource load. RTLD_LAZY, LOAD_LIBRARY_AS_IMAGE_RESOURCE
#endif
    static const UINT k_Load_OSMask = 0x0FFFFFFF;
    static const UINT k_Load_ByName = 0x40000000;      /// try to find it (by just its file name, NOT Path) already loaded first. NOT OS flag.
    static const UINT k_Load_NoRefCount = 0x80000000;  /// I DO NOT own the ref count. Don't free/ decrement ref count on destruct. NOT OS Flag.

    cOSModule();
    cOSModule(::HMODULE hModule, UINT32 uFlags);
    cOSModule(const FILECHAR_t* pszModuleName, UINT32 uFlags);
    cOSModule(cOSModule&& m);
    ~cOSModule();

    /// <summary>
    /// Evaluate file type from name. Does this file appear to be a module/PE type ?
    /// @todo __linux__ must check the MIME type on the actual file.
    /// </summary>
    /// <returns>0 = MIME_t::_UNKNOWN, 3=MIME_t::_EXE, 2=MIME_t::_DLL.</returns>
    static MIME_t GRAYCALL CheckModuleTypeFile(const cSpan<FILECHAR_t>& path);

    /// <summary>
    /// Get the handle for the module this pAddr is in. Do NOT increment ref count for ::HMODULE.
    /// code may be part of a shared object or DLL. track its handle in case it is unloaded dynamically. use vtable or typeid() pointer for the object ?
    /// </summary>
    /// <param name="pAddr">a function pointer?</param>
    /// <returns>cAppState::get_HModule() = just part of the current EXE. or HMODULE_NULL = error;</returns>
    static ::HMODULE GRAYCALL GetModuleHandleForAddr(const void* pAddr);

    /// <summary>
    /// Get a Generic function call address in the module. assume nothing about the functions args/signature/ABI.
    /// @note No such thing as a UNICODE proc/function/symbol name! object formats existed before UNICODE. Except for UNDER_CE which does support UNICODE proc names!?
    /// @note this does not work if loaded using LOAD_LIBRARY_AS_IMAGE_RESOURCE or LOAD_LIBRARY_AS_DATAFILE
    /// </summary>
    /// <param name="pszSymbolName"></param>
    /// <returns>address</returns>
    FUNCPTR_t GetSymbolAddress(const char* pszSymbolName) const;

    bool isValidModule() const noexcept {
        return _hModule != HMODULE_NULL;
    }
    operator ::HMODULE() const noexcept {
        return _hModule;
    }
    ::HMODULE get_HModuleX() const noexcept {
        return _hModule;
    }
    /// <summary>
    /// Get the modules handle as an int.
    /// </summary>
    UINT_PTR get_ModuleInt() const noexcept {
        return CastPtrToNum(_hModule);
    }
    /// <summary>
    /// We cant call this. its not loaded as code.
    /// </summary>
    bool isResourceModule() const noexcept {
        return cBits::HasAny(_uFlags, k_Load_Preload | k_Load_Resource);
    }

    /// <summary>
    /// Get the file path to the loaded module.
    /// @note there is not absolute rule that it must have one.
    /// </summary>
    /// <param name="name"></param>
    /// <returns>length, 0 = fail.</returns>
    StrLen_t GetModulePath(cSpanX<FILECHAR_t> ret) const;
    cStringF get_ModulePath() const;

    /// <summary>
    /// get Full path to the module. for name sorting.
    /// </summary>
    cStringF get_Name() const {
        return get_ModulePath();
    }

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
        _hModule = hModule;  // assumed valid.
        _uFlags = uFlags;
    }
    void ClearModule() noexcept {
        // Don't decrement load count (FreeModuleLast) even if i should.
        _hModule = HMODULE_NULL;
        _uFlags = k_Load_Normal;
        _modulePath.Empty();
    }
    ::HMODULE DetachModule() noexcept {
        const ::HMODULE hModule = _hModule;
        ClearModule();
        return hModule;
    }

    /// <summary>
    /// decrement my usage count. It might be freed/unloaded if NOT k_Load_NoRefCount.
    /// </summary>
    void FreeThisModule();

    /// <summary>
    /// is the DLL/SO already loaded? Find it by name. Full Path is NOT necessary
    /// </summary>
    /// <param name="pszModuleName"></param>
    /// <param name="uFlags">cOSModule::k_Load_NoRefCount</param>
    /// <returns></returns>
    bool AttachModuleName(const FILECHAR_t* pszModuleName, UINT32 uFlags);

    /// <summary>
    /// Load HMODULE by file path.
    /// @note We should use cAppStateModuleLoad with this.
    /// </summary>
    /// <param name="pszModuleName"></param>
    /// <param name="uFlags">k_Load_ByName = find if its already loaded first. full path isn't important. equiv to: LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_AS_IMAGE_RESOURCE</param>
    /// <returns></returns>
    HRESULT LoadModule(const FILECHAR_t* pszModuleName, UINT32 uFlags = k_Load_Normal);

    /// <summary>
    /// Load this module ONLY if it exposes this symbol.
    /// @NOTE: This may load a bunch of child modules as well.
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
/// @note It is VERY important to know the proper number and size of args before calling _pFunc !!! (if not _cdecl)
/// </summary>
/// <typeparam name="TYPE">FUNCPTR_t</typeparam>
template <class TYPE>
struct cOSModuleFunc {
    TYPE _pFunc = nullptr;  /// something like:  typedef int (FAR WINAPI *FUNCTYPENAME)();

    cOSModuleFunc(TYPE pFunc = nullptr) : _pFunc(pFunc) {}
    ~cOSModuleFunc() {}

    void ClearFuncAddress() {
        _pFunc = nullptr;
    }
    bool put_FuncAddress(TYPE pFunc) {
        _pFunc = pFunc;
        if (_pFunc == nullptr) return false;
        return true;
    }
    bool put_FuncGeneric(FUNCPTR_t pFunc) {
        _pFunc = CastFPtrTo<TYPE>(pFunc);
        if (_pFunc == nullptr) return false;
        return true;
    }
    bool isValidFunc() const {
        return _pFunc != nullptr;  // IsValidFunction()
    }
    operator TYPE() const {
        return _pFunc;
    }
};

#ifndef GRAY_STATICLIB  // force implementation/instantiate for DLL/SO.
template struct GRAYCORE_LINK cOSModuleFunc<FUNCPTR_t>;
#endif
}  // namespace Gray
#endif  // _INC_cOSModule_H
