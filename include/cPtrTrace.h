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
 
#include "cPtrFacade.h"
#include "PtrCast.h"
#include "cDebugAssert.h"
#include "cTypeInfo.h"

namespace Gray
{
	class GRAYCORE_LINK cPtrTrace
	{
		//! @class Gray::cPtrTrace
		//! Trace each use of the a pointer in cPtrFacade/cIUnkPtr/cRefPtr for _DEBUG purposes.
		//! If the lock count fails to go to 0 we know who the leaker was. or if the object is deleted but still has refs we can detect that as well.
		//! Add myself to the cPtrTraceMgr table if the m_p pointer is set.

	public:
		const TYPEINFO_t& m_TypeInfo;		//!< for __typeof(TYPEINFO_t).name()
		IUnknown* m_pIUnk;		//!< Different implementations have different ways to resolve this. so store it. Pointer to my shared object
		cDebugSourceLine m_Src;		//!< where (in code) was m_p set?

		static bool sm_bActive;		//!< Turn on/off global tracing via cPtrTraceMgr. be fast.

	public:
		cPtrTrace(const TYPEINFO_t& typeInfo) noexcept
			: m_TypeInfo(typeInfo)
			, m_pIUnk(nullptr)
		{
			// ASSUME m_Src will be populated.
		}
		cPtrTrace(const TYPEINFO_t& typeInfo, IUnknown* pIUnk, const cDebugSourceLine& src) noexcept
			: m_TypeInfo(typeInfo)
			, m_pIUnk(pIUnk)
			, m_Src(src)
		{
		}
		cPtrTrace(const cPtrTrace& ref) noexcept
			: m_TypeInfo(ref.m_TypeInfo), m_pIUnk(ref.m_pIUnk), m_Src(ref.m_Src)
		{
			// copy constructor.
		}

		void Attach(IUnknown* pIUnk, const cDebugSourceLine& src)
		{
			ASSERT(pIUnk != nullptr);	// get_Ptr()
			m_Src = src; //  DEBUGSOURCELINE; 
			TraceAttach(pIUnk);	// attach trace.
		}

		void TraceAttach(IUnknown* pIUnk);
		void TraceRelease(IUnknown* pIUnk);
	};
}

#endif
