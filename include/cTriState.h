//! @file cTriState.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

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
    BITOP_t _eVal;  /// BITOP_t. 0=false, 1=true, -1=unknown 3rd state.
 public:
    cTriState(BITOP_t eVal = BITOP_t::_Toggle) noexcept : _eVal(eVal) {
        //! default = BITOP_t::_Toggle
        DEBUG_ASSERT(isInternalValidState(), "cTriState");
    }
    cTriState(bool bVal) noexcept : _eVal(bVal ? BITOP_t::_Set : BITOP_t::_Clear) {}
    bool isInternalValidState() const noexcept {
        //! Is it one of the 3 valid values ?
        return _eVal == BITOP_t::_Toggle || _eVal == BITOP_t::_Clear || _eVal == BITOP_t::_Set;
    }
    bool isTriState() const noexcept {
        return _eVal == BITOP_t::_Toggle;
    }
    bool get_Bool() const {
        //! ASSUME Must not be tristate.
        ASSERT(_eVal != BITOP_t::_Toggle);
        return _eVal != BITOP_t::_Clear;
    }
    bool GetBoolDef(bool bDefault = false) const noexcept {
        //! Get the bool with supplied default if tristate.
        if (_eVal == BITOP_t::_Toggle) return bDefault;
        return _eVal != BITOP_t::_Clear;
    }
    BITOP_t get_Tri() const noexcept {
        return _eVal;
    }
    void put_Tri(BITOP_t eVal) {
        _eVal = eVal;
        ASSERT(isInternalValidState());
    }
    operator BITOP_t() const noexcept {
        return get_Tri();
    }
};
}  // namespace Gray
#endif
