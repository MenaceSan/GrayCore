//! @file cObjectService.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cAppState.h"
#include "cArchive.h"
#include "cOSModule.h"
#include "cObjectService.h"
#include "cSingleton.h"

namespace Gray {

HRESULT cObject::Serialize(cArchive& a) {  // virtual
    // Emulate MFC method. cArchive = CArchive
    UNREFERENCED_REFERENCE(a);
    return S_OK;
}

const void* const* cObject::GetVTable() const {
    return cTypeInfo::GetVTable(this);
}

cObjectFactory::cObjectFactory(const TYPEINFO_t& rTypeInfo, const ATOMCHAR_t* pszTypeName) noexcept
    : m_pszTypeName(pszTypeName ? pszTypeName : cTypeInfo::GetSymName(rTypeInfo.name())), _HashCode(StrT::GetHashCode32(m_pszTypeName)), m_TypeInfo((const cTypeInfo&)rTypeInfo) {
    // register this m_TypeInfo typeid(TYPE) with cObjectService
    cObjectService& service = cObjectService::I();
    service.RegisterFactory(*this);
}
cObjectFactory::~cObjectFactory() {
    // De-register the cObjectService (if not Process close)
    if (cAppState::isInCExit()) return;
    cObjectService& service = cObjectService::I();
    service.RemoveFactory(*this);
}
::HMODULE cObjectFactory::get_HModule() const {
    return cOSModule::GetModuleHandleForAddr(&m_TypeInfo);
}

//**************************************

bool cObjectService::RegisterFactory(cObjectFactory& factory) noexcept {
    // add this to a cObjectService registration list.
    auto i = _Factories.FindIForKey(factory.get_Name());
    if (i != k_ITERATE_BAD) return false;  // already here.
    _Factories.Add(&factory);
    return true;
}
bool cObjectService::RemoveFactory(cObjectFactory& factory) noexcept {
    return _Factories.RemoveArg(&factory);
}

void cObjectService::ReleaseModuleChildren(::HMODULE hMod) {  // override;
    for (ITERATE_t i = _Factories.GetSize(); i;) {
        const cObjectFactory* factory = _Factories.GetAt(--i);
        ASSERT_NN(factory);
        if (factory->get_HModule() != hMod) continue;
        _Factories.RemoveAt(i);
    }
}

cObject* GRAYCALL cObjectService::CreateObject(const ATOMCHAR_t* pszTypeName) {  // static
    // Unknown allocation / free of this object !
    UNREFERENCED_PARAMETER(pszTypeName);
    cObjectService& service = cObjectService::I();
    UNREFERENCED_PARAMETER(service);
    ASSERT(0);
    // m_aSingletons cSingletonManager& ism =
    // TODO
    return nullptr;
}
cObject* GRAYCALL cObjectService::CreateObject(const TYPEINFO_t& type) {  // static
    // Unknown allocation / free of this object !
    UNREFERENCED_REFERENCE(type);
    cObjectService& service = cObjectService::I();
    UNREFERENCED_PARAMETER(service);
    ASSERT(0);
    // TODO
    return nullptr;
}
}  // namespace Gray
