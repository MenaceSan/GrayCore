//
//! @file cTextPos.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cTextPos.h"

namespace Gray {
const cTextPos cTextPos::k_Invalid((STREAM_POS_t)k_ITERATE_BAD, k_ITERATE_BAD, k_StrLen_UNK);
const cTextPos cTextPos::k_Zero(0, 0, 0);

StrLen_t cTextPos::GetStr2(OUT char* pszOut, StrLen_t nLenOut) const {
    return StrT::sprintfN<char>(pszOut, nLenOut, "O=%d,Line=%d", m_lOffset, m_iLineNum);
}
}  // namespace Gray
