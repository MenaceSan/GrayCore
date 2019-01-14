//
//! @file StrConst.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#include "StdAfx.h"
#include "StrConst.h"

namespace Gray
{
	const CStrConst CStrConst::k_Empty = CStrConst(&CStrConst::k_EmptyA, &CStrConst::k_EmptyW);	// same as CSTRCONST("")
}
