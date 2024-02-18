//! @file cTextPos.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cTextPos.h"
#include "StrT.h"

namespace Gray {
const cTextPos cTextPos::k_Invalid((STREAM_POS_t)k_ITERATE_BAD, k_ITERATE_BAD, k_StrLen_UNK);
const cTextPos cTextPos::k_Zero(0, 0, 0);

StrLen_t cTextPos::GetStr2(OUT cSpanX<char>& ret) const {
    return StrT::sprintfN<char>(ret, "O=%d,Line=%d", m_lOffset, m_iLineNum);
}
}  // namespace Gray
