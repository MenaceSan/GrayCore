//
//! @file cObjectService.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cObjectService.h"
#include "cArchive.h"
#include "cSingleton.h"

namespace Gray
{

#ifndef _MFC_VER
	void CObject::Serialize(cArchive& a) // virtual
	{
		// Emulate MFC method.
		UNREFERENCED_REFERENCE(a);
	}
#endif

	void GRAYCALL cObjectFactory::RegisterFactory(const cObjectFactory& factory) noexcept // static 
	{
		// TODO add this to a cObjectService registration list.

		// No dupes.
		UNREFERENCED_REFERENCE(factory);
	}

	CObject* GRAYCALL cObjectService::CreateObject(const ATOMCHAR_t* pszName) // static 
	{
		// Unknown allocation / free of this object !
		UNREFERENCED_PARAMETER(pszName);

		// m_aSingletons cSingletonManager& ism = 
		// TODO

		return nullptr;
	}
	CObject* GRAYCALL cObjectService::CreateObject(const TYPEINFO_t& type) // static 
	{
		// Unknown allocation / free of this object !
		UNREFERENCED_REFERENCE(type);
		// TODO
		return nullptr;
	}

}
