//
//! @file StrConst.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "StrConst.h"

namespace Gray
{
	const cStrConst cStrConst::k_Empty = cStrConst(&cStrConst::k_EmptyA, &cStrConst::k_EmptyW);	// static same as CSTRCONST("")
	const cStrConst cStrConst::k_CrLf = CSTRCONST(STR_CRLF);
}
