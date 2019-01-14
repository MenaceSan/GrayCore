//
//! @file StrCharAscii.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_StrCharAscii_H
#define _INC_StrCharAscii_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"

namespace Gray
{
	enum ASCII_TYPE
	{
		//! @enum Gray::ASCII_TYPE
		//! Make names for all the ASCII values.

		ASCII_NONE = -1,	// No char was produced.

#define StrCharASCII(a,b,c)		ASCII_##b = a,
#include "StrCharAscii.tbl"
#undef StrCharASCII
	};
}

#endif
