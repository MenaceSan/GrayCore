//
//! @file cObjectFactory.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cObjectFactory_H
#define _INC_cObjectFactory_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cTypeInfo.h"
#include "cObject.h"

namespace Gray
{
	class GRAYCORE_LINK cObjectFactory
	{
		//! @class Gray::cObjectFactory
		//! abstract factory pattern for CObject based objects.
		//! Intentionally NOT cSingleton. derived class may be. This is probably a static.
		//! @note Allocation of the created object is unknown! probably dynamic and must delete ? NEVER cSingleton (it has its own mechanism for that).
		//! Similar to IClassFactory 
		
	public:
		const ATOMCHAR_t* const m_pszName;		//! The main type name we can create by. Might have multiple alternate aliases for interfaces. e.g. "IObjectName"
		const cTypeInfo& m_TypeInfo;		//! the typeid(TYPE) of some object we would create.

	public:
		static void GRAYCALL RegisterFactory(const cObjectFactory& factory) noexcept;

	public:
		cObjectFactory(const ATOMCHAR_t* pszName, const TYPEINFO_t& rTypeInfo) noexcept
			: m_pszName(pszName)
			, m_TypeInfo((const cTypeInfo&)rTypeInfo)
		{
			// register this m_TypeInfo with cObjectService
			RegisterFactory(*this);
		}

		//! Create CObject of some derived cTypeInfo. AKA CreateInstance()
		virtual CObject* CreateObject() const = 0;
	};

	template <class TYPE = CObject>
	class cObjectFactoryT : public cObjectFactory
	{
		//! @class Gray::cObjectFactoryT
		//! factory pattern for CObject based objects. 
	public:
		cObjectFactoryT(const ATOMCHAR_t* pszName = nullptr, const TYPEINFO_t& rTypeInfo = typeid(TYPE)) noexcept
			: cObjectFactory(pszName == nullptr ? rTypeInfo.name() : pszName, rTypeInfo)
		{
		}
		virtual TYPE* CreateObjectT() const = 0;
		CObject* CreateObject() const override
		{
			return CreateObjectT();
		}
	};
}

#endif
