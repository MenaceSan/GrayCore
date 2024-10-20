//! @file cTextPos.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrT.h"
#include "cTextPos.h"

namespace Gray {
const cTextPos cTextPos::k_Invalid(k_STREAM_POS_ERR, k_ITERATE_BAD, k_StrLen_UNK);
const cTextPos cTextPos::k_Zero(0, 0, 0);

StrLen_t cTextPos::GetStr2(cSpanX<char> ret) const {
    return StrT::sprintfN<char>(ret, "O=%d,Line=%d", _nOffset, _nLineNum);
}
}  // namespace Gray
