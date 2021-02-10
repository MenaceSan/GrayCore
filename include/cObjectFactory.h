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
	template <class TYPE = CObject>
	class cObjectFactory
	{
		//! @class Gray::cObjectFactory
		//! abstract factory  pattern for CObject based objects.
		//! Inentionally NOT singleton. derived class may be.
		//! @note Allocation of the object is unknown! Is it static? must delete ? NEVER cSingleton (it has its own mechanism for that).
		//! Similar to IClassFactory
		
	public:
		const ATOMCHAR_t* const m_pszName;		//! The main type name we can create by. Might have multiple alternate aliases for interfaces. e.g. "IObjectName"
		const cTypeInfo& m_TypeInfo;		//! the typeid(TYPE) of some object we would create.

	public:
		cObjectFactory(const ATOMCHAR_t* pszName, const TYPEINFO_t& rTypeInfo = typeid(TYPE)) noexcept
			: m_pszName(pszName)
			, m_TypeInfo((const cTypeInfo&)rTypeInfo)
		{
			// TODO register this m_TypeInfo with cObjectService
		}

		//! Create CObject of some derived cTypeInfo.
		virtual TYPE* CreateObject() const = 0;
	};
}

#endif
