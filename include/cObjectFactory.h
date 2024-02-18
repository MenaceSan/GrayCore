//! @file cObjectFactory.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cObjectFactory_H
#define _INC_cObjectFactory_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "StrT.h"
#include "cObject.h"
#include "cSingleton.h"
#include "cTypeInfo.h"

namespace Gray {
/// <summary>
/// similar to _WIN32 IClassFactory.
/// </summary>
DECLARE_INTERFACE(IObjectFactory) {  // MIDL_INTERFACE("0C3E2E71-B93C-11d2-AAD0-006007654325")
    IGNORE_WARN_INTERFACE(IObjectFactory);
    /// Create cObject of some derived cTypeInfo. AKA CreateInstance(). Caller must know how to dispose this! cRefPtr or delete.
    virtual [[nodiscard]] cObject* CreateObject() const = 0;
};

/// <summary>
/// abstract factory pattern for cObject based objects.
/// Intentionally NOT cSingleton. derived class may be. This is probably a static.
/// @note Allocation of the created object is unknown! probably dynamic and must delete ? NEVER cSingleton (it has its own mechanism for that).
/// Similar to IClassFactory
/// </summary>
struct GRAYCORE_LINK cObjectFactory : public IObjectFactory {
    /// <summary>
    /// The main type name static allocated. Can use this for dynamic object creation.
    /// Might have multiple alternate aliases for interfaces. e.g. "IObjectName"
    /// MUST be first for use with StrT::SpanFindHead
    /// </summary>
    const ATOMCHAR_t* const m_pszTypeName;
    const HASHCODE32_t _HashCode;  /// Hash/ATOMCODE_t of m_pszTypeName

    /// <summary>
    /// the typeid(TYPE) of the object we would create with CreateObject().
    /// </summary>
    const cTypeInfo& m_TypeInfo;

    cObjectFactory(const TYPEINFO_t& rTypeInfo, const ATOMCHAR_t* pszTypeName = nullptr) noexcept;
    virtual ~cObjectFactory();
    ::HMODULE get_HModule() const;

    const ATOMCHAR_t* get_Name() const noexcept {
        return m_pszTypeName;
    }

    /// <summary>
    /// Get unique code
    /// </summary>
    HASHCODE32_t get_HashCode() const noexcept {
        return _HashCode;
    }
};

template <typename _TYPE>
class cObjectFactoryT : public cSingleton<cObjectFactoryT<_TYPE> >, public cObjectFactory {
    typedef cObjectFactoryT<_TYPE> THIS_t;
    SINGLETON_IMPL(THIS_t);
    cObjectFactoryT() : cObjectFactory(typeid(_TYPE)), cSingleton<THIS_t>(this, typeid(THIS_t)) {}

 public:
    virtual [[nodiscard]] cObject* CreateObject() const {
        return new _TYPE();
    }
};
}  // namespace Gray
#endif
