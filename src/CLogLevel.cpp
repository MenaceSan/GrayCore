//! @file cLogLevel.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrA.h"
#include "StrU.h"
#include "cLogLevel.h"

namespace Gray {
const LOGCHAR_t* const cLogLevel::k_pszPrefixes[static_cast<int>(LOGLVL_t::_QTY) + 1] = {
// LOGLVL_t::_WARN, nullptr term
#define LOGLEVELDEF(a, b, c, d) c,
#include "cLogLevel.tbl"
#undef LOGLEVELDEF
    nullptr,  // LOGLVL_t::_QTY
};

const LOGCHAR_t* GRAYCALL cLogLevel::GetPrefixStr(LOGLVL_t eLogLevel) {  // static
    //! Describe the LOGLVL_t
    if (eLogLevel < LOGLVL_t::_WARN) return "";
    if (eLogLevel >= LOGLVL_t::_QTY) return "";
    return k_pszPrefixes[static_cast<int>(eLogLevel)];
}
}  // namespace Gray
