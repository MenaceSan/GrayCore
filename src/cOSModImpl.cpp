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
cOSModImpl::cOSModImpl(const char* pszModuleName) noexcept : m_pszModuleName(pszModuleName) {
    // Always static allocated. NEVER heap or Stack allocated. So Zero init is NEVER needed!
    DEBUG_CHECK(!StrT::IsWhitespace(m_pszModuleName));
    OnProcessAttachTry();
}

bool cOSModImpl::OnProcessAttach() {  // virtual
    // DLL_PROCESS_ATTACH
    ASSERT(!m_bProcessAttached);
    m_bProcessAttached = true;

    DEBUG_MSG(("%s:OnProcessAttach 0%x", LOGSTR(m_pszModuleName), (UINT)CastPtrToNum(m_hModule)));

#ifdef _MFC_VER
    // Extension DLL one-time initialization
    if (!::AfxInitExtensionModule(m_AFXExt, (HINSTANCE)m_hModule)) return false;
        // perhaps new CDynLinkLibrary ?
#endif
    return true;
}

bool cOSModImpl::OnProcessAttachTry() {  // private
    // NOTE: In the LoadModule (dynamic) case this MAY get called BEFORE the constructor for cOSModDynImpl.
    if (!IsLoaded()) return true;  // Wait for the race to be over. m_hModule MUST be set.
#if defined(_DEBUG) && !defined(UNDER_CE)
    HINSTANCE hInstDllTest = cOSModule::GetModuleHandleForAddr(m_pszModuleName);
    ASSERT(m_hModule == hInstDllTest);
#endif
    return this->OnProcessAttach();
}

void cOSModImpl::OnProcessDetach() {  // virtual
    // DLL_PROCESS_DETACH
    DEBUG_MSG(("%s:OnProcessDetach 0%x", LOGSTR(m_pszModuleName), (UINT)CastPtrToNum(m_hModule)));
#ifdef _MFC_VER
    ::AfxTermExtensionModule(m_AFXExt);
#endif
    // Try to release my singletons in proper order.
    cDependRegister::ReleaseModule(m_hModule);
}

#ifdef _WIN32
bool cOSModImpl::DllMain(HINSTANCE hMod, DWORD dwReason) {  // virtual
    switch (dwReason) {
        case DLL_PROCESS_DETACH:
            if (!cMem::IsValidApp(this)) return false;  // i've seen this happen in release mode. VS2019. just do nothing.
            ASSERT(hMod == m_hModule);
            this->OnProcessDetach();
            break;
        case DLL_PROCESS_ATTACH:
            // NOTE: In the LoadModuel (dynamic) case this will get called BEFORE the constructor for cOSModDynImpl.
            ASSERT(m_hModule == HMODULE_NULL);
            ASSERT(hMod != HMODULE_NULL);
            m_hModule = hMod;
            return this->OnProcessAttachTry();
        case DLL_THREAD_ATTACH:  // a new thread has been created.
            break;
        case DLL_THREAD_DETACH:
            // cThreadLocalSys ??
            break;
        default:
            DEBUG_ERR(("%s:DllMain event=%d", LOGSTR(m_pszModuleName), dwReason));
            break;
    }
    return true;
}

#elif defined(__linux__)
void cOSModImpl::SOConstructor() {
    m_hModule = cOSModule::GetModuleHandleForAddr(m_pszModuleName);
    this->OnProcessAttachTry();
}
#endif
}  // namespace Gray
