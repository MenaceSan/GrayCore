//
//! @file CObjectCreator.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CObjectCreator_H
#define _INC_CObjectCreator_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "CTypeInfo.h"
#include "CObject.h"

namespace Gray
{
	class GRAYCORE_LINK CObjectCreator
	{
		//! @class Gray::CObjectCreator
		//! Service Locater / Creator pattern for CObject based objects
		//! https://en.wikipedia.org/wiki/Service_locator_pattern
		//! like MFC CRuntimeClass. used to create CObject based objects by string name.
		//! Allow runtime binding. Create a new object by name. It may be overriden.
		//! Typically one would ask for an Interface and a concrete object would be created.
		//! similar to Object Injection. ALA Ninject.
		//! Used to allow creation of overloaded singletons.
		//! TODO Allocation of the object is unknown! Is it static? must delete ? CSmartPtr? ???

	public:
		const char* const m_pszName;		//! The main name we can create by. Might have multiple alternate aliases for interfaces. e.g. "IObjectName"
		const CTypeInfo& m_TypeInfo;		//! the typeid() of some object we would create.

	public:
		CObjectCreator(const char* pszName, const TYPEINFO_t& rTypeInfo)
			: m_pszName(pszName)
			, m_TypeInfo((const CTypeInfo&)rTypeInfo)
		{
		}

		//! Create CObject of some derived CTypeInfo.
		virtual CObject* CreateObject() const = 0;

		static CObject* GRAYCALL CreateObject(const char* pszName);
		static CObject* GRAYCALL CreateObject(const TYPEINFO_t& type);
	};

	// CObjectFactory
}

#ifndef _MFC_VER
// Dynamic object is one that can be created knowing only its name and perhaps some interface that it supports.
#define DECLARE_DYNAMIC(c)			//__noop
#define IMPLEMENT_DYNAMIC(c, cb)	//__noop
#endif // _MFC_VER

#endif
