//
//! @file cObjectService.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cObjectService.h"
#include "cArchive.h"

namespace Gray
{

#ifndef _MFC_VER
	void CObject::Serialize(cArchive& a) // virtual
	{
		// Emulate MFC method.
		UNREFERENCED_REFERENCE(a);
	}
#endif


}
