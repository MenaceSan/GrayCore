//! @file cOSModImpl.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "FuncPtr.h"
#include "cAppState.h"
#include "cDebugAssert.h"
#include "cLogMgr.h"
#include "cMem.h"
#include "cOSModImpl.h"

namespace Gray {
cOSModImpl::cOSModImpl(const char* pszModuleName) noexcept : _pszModuleName(pszModuleName) {
    DEBUG_CHECK(!StrT::IsWhitespace(_pszModuleName));
    OnProcessAttachTry();
}

bool cOSModImpl::OnProcessAttach() {  // virtual
    // DLL_PROCESS_ATTACH
    ASSERT(!_isProcessAttached);
    _isProcessAttached = true;

    DEBUG_MSG(("%s:OnProcessAttach 0%x", LOGSTR(_pszModuleName), (UINT)CastPtrToNum(_hModule))); // truncate 64 bit.

#ifdef _MFC_VER
    // Extension DLL one-time initialization
    if (!::AfxInitExtensionModule(_AFXExt, (HINSTANCE)_hModule)) return false;
        // perhaps new CDynLinkLibrary ?
#endif
    return true;
}

bool cOSModImpl::OnProcessAttachTry() {  // private
    // NOTE: In the LoadModule (dynamic) case this MAY get called BEFORE the constructor for cOSModDynImpl.
    if (!IsLoaded()) return true;  // Wait for the race to be over. _hModule MUST be set.
#if defined(_DEBUG) && !defined(UNDER_CE)
    HINSTANCE hInstDllTest = cOSModule::GetModuleHandleForAddr(_pszModuleName);
    ASSERT(_hModule == hInstDllTest);
#endif
    return this->OnProcessAttach();
}

void cOSModImpl::OnProcessDetach() {  // virtual
    // DLL_PROCESS_DETACH
    DEBUG_MSG(("%s:OnProcessDetach 0%x", LOGSTR(_pszModuleName), (UINT)CastPtrToNum(_hModule)));
#ifdef _MFC_VER
    ::AfxTermExtensionModule(_AFXExt);
#endif
    // Try to release my singletons in proper order. this happens BEFORE static destructs.
    cDependRegister::ReleaseModule(_hModule);
}

#ifdef _WIN32
bool cOSModImpl::DllMain(HINSTANCE hMod, DWORD dwReason) {  // virtual
    switch (dwReason) {
        case DLL_PROCESS_DETACH:
            if (!cMem::IsValidApp(this)) return false;  // i've seen this happen in release mode. VS2019. just do nothing.
            ASSERT(hMod == _hModule);
            this->OnProcessDetach();
            break;
        case DLL_PROCESS_ATTACH:
            // NOTE: In the LoadModuel (dynamic) case this will get called BEFORE the constructor for cOSModDynImpl.
            ASSERT(_hModule == HMODULE_NULL);
            ASSERT(hMod != HMODULE_NULL);
            _hModule = hMod;
            return this->OnProcessAttachTry();
        case DLL_THREAD_ATTACH:  // a new thread has been created.
            break;
        case DLL_THREAD_DETACH:
            // cThreadLocalSys ??
            break;
        default:
            DEBUG_ERR(("%s:DllMain event=%d", LOGSTR(_pszModuleName), dwReason));
            break;
    }
    return true;
}

#elif defined(__linux__)
void cOSModImpl::SOConstructor() {
    _hModule = cOSModule::GetModuleHandleForAddr(_pszModuleName);
    this->OnProcessAttachTry();
}
#endif
}  // namespace Gray
