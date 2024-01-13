//
//! @file cTriState.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cTriState_H
#define _INC_cTriState_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cBits.h"
#include "cDebugAssert.h"

namespace Gray {
/// <summary>
/// a value with 3 states, like boost::tribool.
/// Similar to .NET VB TriState or bool? or Nullable<bool>.
/// </summary>
class cTriState {
    BITOP_t m_iVal;  /// BITOP_t. 0=false, 1=true, -1=unknown 3rd state.
 public:
    cTriState(BITOP_t eVal = BITOP_t::_Toggle) noexcept : m_iVal(eVal) {
        //! default = BITOP_t::_Toggle
        DEBUG_ASSERT(isInternalValidState(), "cTriState");
    }
    cTriState(bool bVal) noexcept : m_iVal(bVal ? BITOP_t::_Set : BITOP_t::_Clear) {}
    bool isInternalValidState() const noexcept {
        //! Is it one of the 3 valid values ?
        return m_iVal == BITOP_t::_Toggle || m_iVal == BITOP_t::_Clear || m_iVal == BITOP_t::_Set;
    }
    bool isTriState() const noexcept {
        return m_iVal == BITOP_t::_Toggle;
    }
    bool get_Bool() const {
        //! ASSUME Must not be tristate.
        ASSERT(m_iVal != BITOP_t::_Toggle);
        return m_iVal != BITOP_t::_Clear;
    }
    bool GetBoolDef(bool bDefault = false) const noexcept {
        //! Get the bool with supplied default if tristate.
        if (m_iVal == BITOP_t::_Toggle) return bDefault;
        return m_iVal != BITOP_t::_Clear;
    }
    BITOP_t get_Tri() const noexcept {
        return m_iVal;
    }
    void put_Tri(BITOP_t eVal) {
        m_iVal = eVal;
        ASSERT(isInternalValidState());
    }
    operator BITOP_t() const noexcept {
        return get_Tri();
    }
};
}  // namespace Gray
#endif
