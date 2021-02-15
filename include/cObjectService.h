//
//! @file cObjectService.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cObjectService_H
#define _INC_cObjectService_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#pragma once

#include "cObjectFactory.h"
#include "cSingleton.h"
#include "cArray.h"

namespace Gray
{
	template <class TYPE = CObject>
	class cObjectFactoryT2 : public cObjectFactoryT<TYPE>, cSingleton<cObjectFactoryT2<TYPE>>
	{
		//! @class Gray::cObjectFactoryT2
		//! factory pattern for CObject based objects. sealed type.
	public:
		cObjectFactoryT2() noexcept
			: cObjectFactoryT(typeid(TYPE).name(), typeid(TYPE))
			, cSingleton(this, typeid(cObjectFactoryT<TYPE>))
		{
			// Allow overriden type name ??
		}
		TYPE* CreateObject() const override
		{
			return new TYPE;
		}
	};

	class GRAYCORE_LINK cObjectService : public cSingleton<cObjectService>
	{
		//! @class Gray::cObjectService
		//! Service Locater / Creator pattern for CObject based objects
		//! collection of cObjectFactory(s) and cSingleton by type name.
		//! Allow runtime binding. Create a new object by name. It may be overridden.
		//! Typically one would ask for an Interface and a concrete object would be created.
		//! similar to Object Injection. ALA Ninject.
		//! https://en.wikipedia.org/wiki/Service_locator_pattern
		//! like MFC CRuntimeClass. used to create CObject based objects by string name. 

	public:

		cObjectService() noexcept
			: cSingleton<cObjectService>(this, typeid(cObjectService))
		{
		}

		static CObject* GRAYCALL CreateObject(const ATOMCHAR_t* pszName);
		static CObject* GRAYCALL CreateObject(const TYPEINFO_t& type);
		// 		static void GRAYCALL ReleaseModule(HMODULE hMod);

	};

}

#endif
