//
//! @file cPtrTrace.h
//! Attempt to trace use of pointers.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cPtrTrace_H
#define _INC_cPtrTrace_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
 
#include "PtrCast.h"
#include "cDebugAssert.h"
#include "cTypeInfo.h"

namespace Gray
{
	class GRAYCORE_LINK cPtrTrace
	{
		//! @class Gray::cPtrTrace
		//! Trace each use of the a pointer in cPtrFacade/cIUnkPtr for debug purposes.
		//! If the lock count fails to go to 0 we know who the leaker was. or if the object is deleted but still has refs we can detect that as well.
		//! Add myself to the cPtrTraceMgr table if the m_p pointer is set.

	public:
		const char* m_pszType;		//!< from __typeof(TYPEINFO_t).name()
		cDebugSourceLine m_Src;		//!< where (in code) was m_p set?

		static bool sm_bActive;		//!< Turn on/off tracing via cPtrTraceMgr. be fast.

	public:
		cPtrTrace(const TYPEINFO_t& TypeInfo) noexcept
			: m_pszType(TypeInfo.name())
		{
		}
		cPtrTrace(const cPtrTrace& ref) noexcept
			: m_pszType(ref.m_pszType), m_Src(ref.m_Src)
		{
			// copy constructor.
		}

		void TraceOpen(void* p);
		void TraceClose(void* p);
	};
}

#endif
