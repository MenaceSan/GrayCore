//
//! @file cObjectCreator.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cObjectCreator.h"
#include "cArchive.h"

namespace Gray
{
	// CObjectCreationMgr : public cSingleton<>
	// Use something like the Singleton register.

#ifndef _MFC_VER
	void CObject::Serialize(cArchive& a) // virtual
	{
		// Emulate MFC method.
		UNREFERENCED_REFERENCE(a);
	}
#endif


}
