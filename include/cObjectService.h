//! @file cObjectService.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cObjectService_H
#define _INC_cObjectService_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#pragma once

#include "cArraySort.h"
#include "cObjectFactory.h"
#include "cSingleton.h"

namespace Gray {
/// <summary>
/// Service Locater / Creator pattern for cObject based objects
/// collection of cObjectFactory(s) and cSingleton by type name.
/// Allow runtime binding. Create a new object by name. It may be overridden.
/// Typically one would ask for an Interface and a concrete object would be created.
/// similar to Object Injection. ALA Ninject.
/// https://en.wikipedia.org/wiki/Service_locator_pattern
/// like MFC CRuntimeClass. used to create cObject based objects by string name.
/// </summary>
class GRAYCORE_LINK cObjectService final : public cSingleton<cObjectService> {
    SINGLETON_IMPL(cObjectService);
    cArraySortPtrName<cObjectFactory, ATOMCHAR_t> _Factories;  // all factories.

 protected:
    cObjectService() noexcept : cSingleton<cObjectService>(this, typeid(cObjectService)) {}

 public:
    bool RegisterFactory(cObjectFactory& factory) noexcept;
    bool RemoveFactory(cObjectFactory& factory) noexcept;
    void ReleaseModuleChildren(::HMODULE hMod) override;

    static cObject* GRAYCALL CreateObject(const ATOMCHAR_t* pszTypeName);
    static cObject* GRAYCALL CreateObject(const TYPEINFO_t& type);
};
}  // namespace Gray

#endif
