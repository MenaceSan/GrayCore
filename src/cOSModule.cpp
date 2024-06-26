//! @file cOSModule.cpp
//! @note
//!  HINSTANCE and HMODULE are usually/sometimes interchangeable.
//! __linux__ link with 'dl' library
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cAppState.h"
#include "cFilePath.h"
#include "cLogMgr.h"
#include "cMime.h"
#include "cOSModule.h"
#include "cSystemHelper.h"

namespace Gray {
template <>
GRAYCORE_LINK void CloseHandleType(::HMODULE h) noexcept {
#ifdef _WIN32
    ::FreeLibrary(h);
#elif defined(__linux__)
    ::dlclose(h);
#endif
}

cOSModule::cOSModule(::HMODULE hModule, UINT32 uFlags) : m_hModule(hModule), m_uFlags(uFlags) {
    //! @arg bHaveRefCount = Must call FreeModuleLast()
}
cOSModule::cOSModule(cOSModule&& m) : m_hModule(m.m_hModule), m_uFlags(m.m_uFlags) {
    m.ClearModule();
}

cOSModule::cOSModule(const FILECHAR_t* pszModuleName, UINT32 uFlags) : m_hModule(HMODULE_NULL), m_uFlags(k_Load_Normal) {
    //! Use isValidModule()
    AttachModuleName(pszModuleName, uFlags);
}
cOSModule::~cOSModule() {
    FreeModuleLast();
}

StrLen_t cOSModule::GetModulePath(cSpanX<FILECHAR_t> ret) const {
    ASSERT(isValidModule());
#ifdef _WIN32
    const bool isApp = m_hModule == cAppState::get_HModule();
    const DWORD uRet = _FNF(::GetModuleFileName)(isApp ? HMODULE_NULL : m_hModule, ret.GetTPtrW<FILECHAR_t>(), (DWORD)ret.get_MaxLen());
    if (uRet <= 0) {
        ret.get_PtrWork()[0] = '\0';
        const HRESULT hRes = HResult::GetLastDef(E_FAIL);
        if (hRes == HRESULT_WIN32_C(ERROR_MOD_NOT_FOUND) && cAppState::isInCExit()) return 0;  // this can happen when unloading.
        DEBUG_WARN(("cOSModule::GetModulePath ERR='%s'", LOGERR(hRes)));
    }
    return uRet;
#else
    cString sModuleName = this->get_Name();
    return StrT::Copy(ret, sModuleName.get_CPtr());
#endif
}

cStringF cOSModule::get_Name() const {
#ifdef _WIN32
    FILECHAR_t szModuleName[cFilePath::k_MaxLen];
    const StrLen_t dwLen = GetModulePath(TOSPAN(szModuleName));
    if (dwLen <= 0) return "";
    return szModuleName;
#else
    if (!m_sModuleName.IsEmpty()) return m_sModuleName;
    if (m_hModule == cAppState::get_HModule()) return cAppState::get_AppFilePath();  // Get the apps module name.

    ::Dl_info info;
    int iRet = ::dladdr(m_hModule, &info);  // TODO: TEST THIS __linux__. MIGHT NOT WORK !
    if (iRet == 0) return "";               // I didn't load this and i don't know its name.

    return info.dli_fname;  // the file name.
#endif
}

void cOSModule::FreeModuleLast() {
    if (!isValidModule()) return;
    if (m_uFlags & cOSModule::k_Load_NoRefCount) return;  // Don't free. I didnt load this.
    if (!(m_uFlags & cOSModule::k_Load_Preload) && !(m_uFlags & cOSModule::k_Load_ByName)) {
        DEBUG_MSG(("FreeModule('%s')", LOGSTR(get_Name())));    // Log this.
    }
    CloseHandleType<::HMODULE>(m_hModule);
}

void cOSModule::FreeThisModule() {
    //! @note don't name this 'FreeModule' since that can be overloaded by _WIN32 "#define".
    if (!isValidModule()) return;
    FreeModuleLast();
    DetachModule();
}

MIME_t GRAYCALL cOSModule::CheckModuleTypeFile(const FILECHAR_t* pszPathName) {  // static
    //! Does this file appear to be a module/PE type ?
    //! @return 0 = MIME_t::_UNKNOWN, 3=MIME_t::_EXE, 2=MIME_t::_DLL.
    //! @todo __linux__ must check the MIME type on the actual file.

    static const FILECHAR_t* k_Exts[] = {
        // alternate names.
        _FN(MIME_EXT_dll),  // MIME_t::_DLL
        _FN(MIME_EXT_exe),  // MIME_t::_EXE
        _FN(MIME_EXT_ocx),
        _FN(MIME_EXT_so),  // Linux
    };
    const FILECHAR_t* pszExt = cFilePath::GetFileNameExt(StrT::ToSpanStr(pszPathName));
    if (pszExt == nullptr) return MIME_t::_UNKNOWN;  // not true for __linux__. use MIME type.

    ITERATE_t i = StrT::SpanFind(pszExt, TOSPAN(k_Exts));
    if (i < 0) return MIME_t::_UNKNOWN;  // no = MIME_t::_UNKNOWN

    return (i == 1) ? MIME_t::_EXE : MIME_t::_Dll;
}

#ifndef UNDER_CE
::HMODULE GRAYCALL cOSModule::GetModuleHandleForAddr(const void* pAddr) {  // static
    if (pAddr == nullptr) return HMODULE_NULL;

#ifdef _WIN32
    ::HMODULE hModule = HMODULE_NULL;
    if (!_FNF(::GetModuleHandleEx)(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (const FILECHAR_t*)pAddr, &hModule)) return HMODULE_NULL;
    return hModule;
#else
    Dl_info info;
    int iRet = ::dladdr(pAddr, &info);
    if (iRet == 0) return HMODULE_NULL;
    return info.dli_fbase;
#endif
}
#endif

FUNCPTR_t cOSModule::GetSymbolAddress(const char* pszSymbolName) const {
    ASSERT(isValidModule());
#ifdef UNDER_CE
    return ::GetProcAddressA(m_hModule, pszSymbolName);
#elif defined(_WIN32)
    return ::GetProcAddress(m_hModule, pszSymbolName);
#elif defined(__linux__)
    // add an _ to the front of the name ?
    return (FUNCPTR_t)::dlsym(m_hModule, pszSymbolName);
#endif
}

bool cOSModule::AttachModuleName(const FILECHAR_t* pszModuleName, UINT32 uFlags) {
    //! is the DLL/SO already loaded? Find it by name.
    //! Full Path is NOT necessary
    //! @arg uFlags = cOSModule::k_Load_NoRefCount

    FreeThisModule();

#ifdef _WIN32
#if !defined(UNDER_CE)
    if (!_FNF(::GetModuleHandleEx)((uFlags & k_Load_NoRefCount) ? GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT : 0, pszModuleName, &m_hModule)) {
        ClearModule();
        return false;
    }
    if (!isValidModule()) return false;

#else
    m_hModule = _FNF(::GetModuleHandle)(pszModuleName);  // no ref count.
    if (!isValidModule()) return false;

    if (!(uFlags & k_Load_NoRefCount)) {
        // get a new ref by loading it by its name.
#ifndef UNDER_CE
        ::SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
        ::HMODULE const hMod2 = _FNF(::LoadLibrary)(get_Name());
        if (hMod2 == HMODULE_NULL) {
            uFlags |= k_Load_NoRefCount;  // I didn't get a ref for some reason. though it is loaded.
        } else {
            m_hModule = hMod2;
        }
    }
#endif
#elif defined(__linux__)
    m_hModule = ::dlopen(pszModuleName, (uFlags & k_Load_OSMask) | RTLD_NOLOAD);  //  (since glibc 2.2)
    if (!isValidModule()) return false;
    if (!(uFlags & k_Load_NoRefCount)) {
        ::HMODULE const hMod2 = ::dlopen(get_Name(), uFlags & k_Load_OSMask);
        if (hMod2 == HMODULE_NULL) {
            uFlags |= k_Load_NoRefCount;  // I didn't get a ref for some reason. though it is loaded.
        } else {
            m_hModule = hMod2;
        }
    }
#endif
    m_uFlags = uFlags;  // found the handle. Do i need to unload it?
    return true;
}

HRESULT cOSModule::LoadModule(const FILECHAR_t* pszModuleName, UINT32 uFlags) {
    //! @arg uFlags =
    //!  k_Load_ByName = find if its already loaded first. full path isn't important.
    //!  LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_AS_IMAGE_RESOURCE
    //! @note
    //!  We should use cAppStateModuleLoad with this.

    FreeThisModule();

    if (cBits::HasAny(uFlags, cOSModule::k_Load_ByName)) {
        // Just look for it already loaded first. ignore full path. just use whatever is same name.
        // Solves problem of having several versions of the same DLL (w/diff paths). get rid of path info!
        bool bRet = AttachModuleName(cFilePath::GetFileName(StrT::ToSpanStr(pszModuleName)), uFlags);
        if (bRet) return S_FALSE;  // already loaded and made a new ref to it.

        // Not already loaded. Try to load it.
        uFlags &= ~cOSModule::k_Load_ByName;
    }

#ifdef _WIN32
    // _WIN32 will add .dll to the name automatically.
#ifndef UNDER_CE
    ::SetErrorMode(SEM_FAILCRITICALERRORS);  // no error dialog.
#endif
    m_hModule = _FNF(::LoadLibraryEx)(pszModuleName, nullptr, uFlags & k_Load_OSMask);  // file name of module
#elif defined(__linux__)
    // Linux requires the file extension.
    m_sModuleName = pszModuleName;
    m_hModule = ::dlopen(pszModuleName, uFlags & k_Load_OSMask);  // RTLD_NOW or RTLD_LAZY
#endif
    if (!isValidModule()) return GetLastErrorDef();

    m_uFlags = uFlags;  // I'm responsible to unload this when I'm done.
    return S_OK;
}

HRESULT cOSModule::LoadModuleWithSymbol(const FILECHAR_t* pszModuleName, const char* pszSymbolName) {
#if 0
	HRESULT hRes = LoadModule(pszModuleName, k_Load_Preload | k_Load_ByName);
	if (FAILED(hRes)) return hRes;
	FUNCPTR_t pAddr = GetSymbolAddress(pszSymbolName);
	if (pAddr == nullptr) {
		FreeThisModule();
		return hRes == S_FALSE ? HRESULT_WIN32_C(ERROR_INVALID_FUNCTION) : HRESULT_WIN32_C(ERROR_CALL_NOT_IMPLEMENTED);
	}
	if (hRes != S_FALSE) {
		// It was k_Load_Preload Loaded ! unload and reload correctly.
		hRes = LoadModule(get_Name(), k_Load_Normal);
	}
#else
    HRESULT hRes = LoadModule(pszModuleName, k_Load_ByName);
    if (FAILED(hRes)) return hRes;
    FUNCPTR_t pAddr = GetSymbolAddress(pszSymbolName);
    if (pAddr == nullptr) {
        FreeThisModule();  // dec ref count.
        return hRes == S_FALSE ? HRESULT_WIN32_C(ERROR_INVALID_FUNCTION) : HRESULT_WIN32_C(ERROR_CALL_NOT_IMPLEMENTED);
    }
#endif
    return hRes;
}
}  // namespace Gray
