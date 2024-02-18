//! @file StrConst.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrConst.h"

namespace Gray {
const cStrConst cStrConst::k_Empty(&cStrConst::k_EmptyA, &cStrConst::k_EmptyW, 0);  // static same as CSTRCONST("")
const cStrConst cStrConst::k_CrLf(CSTRCONST(STR_CRLF));
}  // namespace Gray
