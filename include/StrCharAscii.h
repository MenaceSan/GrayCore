//
//! @file StrCharAscii.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_StrCharAscii_H
#define _INC_StrCharAscii_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"

namespace Gray {
/// <summary>
/// enumeration of names for all the ASCII values. control chars.
/// </summary>
enum class ASCII_t {
    _NONE = -1,  // No char was produced.
#define StrCharASCII(a, b, c) _##b = a,
#include "StrCharAscii.tbl"
#undef StrCharASCII
};
}  // namespace Gray

#endif
