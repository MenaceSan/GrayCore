//! @file cOSModDyn.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cOSModDyn_H
#define _INC_cOSModDyn_H 0x006  // Change this if we want to break the interface.
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cAppState.h"
#include "cIUnkPtr.h"
#include "cOSModImpl.h"

namespace Gray {
/// <summary>
/// Implement a DLL/SO that may be dynamically loadable. Loaded into a process to support some plugin functionality.
/// Exposes RegisterModule()
/// similar to cSingleton . BUT NOT really unique in process space. unique in its DLL space only.
/// This object is ALWAYS statically allocated inside the loaded DLL.
/// </summary>
class GRAYCORE_LINK cOSModDynImpl : public cOSModImpl {
    typedef cOSModImpl SUPER_t;
    cIUnkPtr<> _pContainer;  /// Keep a reference to my container (cXObjModule or other) to prevent unload until proper unload of the DLL.

 public:
    cOSModDynImpl(const char* pszModuleName) noexcept : cOSModImpl(pszModuleName) {}

    /// <summary>
    /// Register this dynamic loaded DLL.
    /// This DLL has loaded and is aware of *Core .
    /// Make sure EXE and DLL/SO/Module agree on structures and packing.
    /// </summary>
    /// <param name="pContainer">my container (e.g cXObjModule) if we care.</param>
    /// <returns>S_FALSE = already called this.</returns>
    virtual HRESULT RegisterModule(::IUnknown* pContainer);

    /// <summary>
    /// Unload data connected to this module that might be externally exposed. (if possible)
    /// </summary>
    /// <returns>safe to unload?</returns>
    virtual bool UnRegisterModule();

    /// <summary>
    /// The module is being unloaded if we like it or not.
    /// </summary>
    void OnProcessDetach() override;
};

#ifndef GRAY_STATICLIB
// Declare/expose/Impl *_RegisterModule() for dynamically pluggable DLL/SO. NOT in any namespace. Avoid name mangling/decoration.
#define cOSModDynImpl_DEF(N)                                                                                                                                    \
    cOSModImpl_DEF(N);                                                                                                                                          \
    extern "C" __DECL_EXPORT HRESULT GRAYCALL CATOM_CAT(Gray, _RegisterModule)(UINT32 nLibVer, OUT ::Gray::cOSModDynImpl * *ppModImpl, ::IUnknown * pContainer) { \
        if (nLibVer != _INC_cOSModDyn_H) return HRESULT_WIN32_C(ERROR_PRODUCT_VERSION);                                                                         \
        if (ppModImpl != nullptr) *ppModImpl = &N::g_Module;                                                                                                    \
        return N::g_Module.RegisterModule(pContainer);                                                                                                          \
    }
#else
#define cOSModDynImpl_DEF(N)    // do nothing. GRAY_STATICLIB
#endif

/// <summary>
/// Load a dynamic module. Call its RegisterModule(). this is the "container". 
/// Ideally we dont load it unless we already know it exposes RegisterModuleF_t.!
/// </summary>
struct GRAYCORE_LINK cOSModDyn {
    typedef HRESULT(GRAYCALL* RegisterModuleF_t)(UINT32 nLibVer, OUT cOSModDynImpl** ppModImpl, ::IUnknown* pContainer);    // like FUNCPTR_t
    cOSModule _h;                     /// the handle to the open DLL/SO/Module can be kept here. for _pImpl
    cOSModDynImpl* _pImpl = nullptr;  /// the (in process) DLL implementation. returned via RegisterModule() / RegisterModuleF_t

    HRESULT LoadAndRegisterModule(const FILECHAR_t* pszPath, ::IUnknown* pContainer, UINT32 nLibVer = _INC_cOSModDyn_H);

    /// <summary>
    /// Unloading the module is extremely dangerous. All objects it created have a vtable pointer in its space.
    /// </summary>
    bool Unload();
};
}  // namespace Gray
#endif
