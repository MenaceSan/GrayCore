//
//! @file CTriState.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CTriState_H
#define _INC_CTriState_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CBits.h"
#include "CDebugAssert.h"

namespace Gray
{
	class CTriState
	{
		//! @class Gray::CTriState
		//! a value with 3 states, like boost::tribool 
		//! Similar to .NET VB TriState or bool? or Nullable<bool>.
	private:
		signed char m_iVal;	//!< BITOP_TYPE. 0=false, 1=true, -1=unknown 3rd state.

	public:
		CTriState(BITOP_TYPE eVal = BITOP_TOGGLE)
			: m_iVal((signed char)eVal)
		{
			//! default = BITOP_TOGGLE
			ASSERT(isInternalValidState());
		}
		CTriState(bool bVal)
			: m_iVal((signed char)(bVal ? BITOP_SET : BITOP_CLR))
		{
		}
		bool isInternalValidState() const
		{
			//! Is it one of the 3 valid values ?
			return m_iVal == BITOP_TOGGLE || m_iVal == BITOP_CLR || m_iVal == BITOP_SET;
		}
		bool isTriState() const
		{
			return m_iVal == BITOP_TOGGLE;
		}
		bool get_Bool() const
		{
			//! ASSUME Must not be tristate.
			ASSERT(m_iVal != BITOP_TOGGLE);
			return (bool)m_iVal;
		}
		bool GetBoolDef(bool bDefault = false) const
		{
			//! Get the bool with supplied default if tristate.
			if (m_iVal == BITOP_TOGGLE)
				return bDefault;
			return (bool)m_iVal;
		}
		BITOP_TYPE get_Tri() const
		{
			return (BITOP_TYPE)m_iVal;
		}
		void put_Tri(BITOP_TYPE eVal)
		{
			m_iVal = (signed char)eVal;
			ASSERT(isInternalValidState());
		}
		operator BITOP_TYPE() const
		{
			return get_Tri();
		}
	};
}
#endif
