//! @file cOSModDyn.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cAppState.h"
#include "cExceptionBase.h"
#include "cOSModDyn.h"
#include "cLogMgr.h"

namespace Gray {
HRESULT cOSModDynImpl::RegisterModule(IUnknown* pContainer) {
    // ASSUME *Core DLL is the correct version and struct packing is correct.
    if (m_pContainer != nullptr) return S_FALSE;  // Already called this! Don't call it again!
    const HRESULT hRes = cAppState::CheckValidSignatureX();
    if (FAILED(hRes)) return hRes;
    m_pContainer = pContainer;
    ASSERT(IsLoaded());
    return S_OK;  // We are good
}

bool cOSModDynImpl::UnRegisterModule() {
    m_pContainer.ReleasePtr();
    return true; // safe i guess.
}

void cOSModDynImpl::OnProcessDetach() {
    bool isSafe = UnRegisterModule();  // always make sure we call this before unload. call again here just to be safe.
    ASSERT(isSafe || cAppState::I().isInCExit());   // this is bad!
    SUPER_t::OnProcessDetach();
}

HRESULT cOSModDyn::LoadAndRegisterModule(const FILECHAR_t* pszPath, IUnknown* pContainer, UINT32 nLibVer) {
    const ITERATE_t nAllocCountPrev = cHeap::g_Stats._Allocs;  // must be the same if we skip/unload the DLL !
    UNREFERENCED_PARAMETER(nAllocCountPrev);

#ifdef USE_64BIT
#define _RegisterModuleS GRAY_NAMES "_RegisterModule"  // Gray_RegisterModule name as a string. 64 bit
#else
#define _RegisterModuleS "_" GRAY_NAMES "_RegisterModule@12"  // Gray_RegisterModule name as a string. 32 bit name has decoration.
#endif

    // test load it first to make sure it has proper COSMod_RegisterModuleS symbol exported.
    cAppStateModuleLoad dllLoad(_h);  // declare load context for this thread.
    HRESULT hRes = _h.LoadModuleWithSymbol(pszPath, _RegisterModuleS);
    if (FAILED(hRes)) {
        // HRESULT_WIN32_C(ERROR_CALL_NOT_IMPLEMENTED) // This is not a DLL we know how to use. skip it.
        return hRes;
    }

    hRes = HRESULT_WIN32_C(ERROR_UNSUPPORTED_TYPE);  // should not happen!
    RegisterModuleF_t* pRegisterModule = reinterpret_cast<RegisterModuleF_t*>(_h.GetSymbolAddress(_RegisterModuleS));
    if (pRegisterModule != nullptr) {
        GRAY_TRY {
            // Have the module register its services. or create cXObjEventSink
            hRes = (pRegisterModule)(nLibVer, &_pImpl, pContainer);
        }
        GRAY_TRY_CATCHALL {
            hRes = E_FAIL;  // no idea. throw ?
        }
        GRAY_TRY_END;
    }
    if (FAILED(hRes)) {
        _h.FreeThisModule();
        // DEBUG_CHECK( nAllocCountPrev == cBlob::sm_nAllocs );
        return hRes;
    }
    return S_OK;
}

void cOSModDyn::Unload() {
    if (_pImpl != nullptr) {
        bool isSafe = _pImpl->UnRegisterModule();
        if (!isSafe && !cAppState::I().isInCExit()) {
            // This is bad! Don't unload it or we will crash!
            cLogMgr::I().addEventF(LOG_ATTR_0, LOGLVL_t::_CRIT, "Unload of module '%s' is NOT safe!", LOGSTR(_pImpl->get_Name()));
            return;
        }
        cSingletonRegister::ReleaseModule(_h);
        _pImpl = nullptr;
    }
    _h.FreeThisModule();  // decrement my count. It might be freed.
}
}  // namespace Gray
