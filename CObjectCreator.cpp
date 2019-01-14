//
//! @file CObjectCreator.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//
#include "StdAfx.h"
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
