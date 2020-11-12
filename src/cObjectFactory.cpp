//
//! @file cObjectFactory.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cObjectFactory.h"
#include "cArchive.h"

namespace Gray
{
	// cObjectFactory<T> : public cSingleton<cObjectFactory<T>>
	// Use something like the Singleton register for creation of a given type.

#ifndef _MFC_VER
	void CObject::Serialize(cArchive& a) // virtual
	{
		// Emulate MFC method.
		UNREFERENCED_REFERENCE(a);
	}
#endif


}
