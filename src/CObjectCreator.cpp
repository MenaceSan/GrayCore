//
//! @file CObjectCreator.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CObjectCreator.h"
#include "CArchive.h"

namespace Gray
{
	// CObjectCreationMgr : public CSingleton<>
	// Use something like the Singleton register.

#ifndef _MFC_VER
	void CObject::Serialize(cArchive& a) // virtual
	{
		// Emulate MFC method.
		UNREFERENCED_REFERENCE(a);
	}
#endif


}