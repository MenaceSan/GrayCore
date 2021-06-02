//
//! @file cPtrTraceMgr.h
//! Attempt to trace use of pointers.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cPtrTraceMgr_H
#define _INC_cPtrTraceMgr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cPtrTrace.h"
#include "cSingleton.h"
#include "cArraySort.h"
#include "cThreadLock.h"

namespace Gray
{
	class cLogProcessor;

	class GRAYCORE_LINK cPtrTraceMgr
		: public cSingleton < cPtrTraceMgr >
	{
		//! @class Gray::cPtrTraceMgr
		//! USE_PTRTRACE_IUNK = We are tracing all calls to cIUnkPtr<> so we can figure out who is not releasing their ref.
		friend class cSingleton < cPtrTraceMgr >;
		friend class cPtrTrace;

	public:
		mutable cThreadLockCount m_Lock;
		cArraySortVal<cPtrTrace*> m_aTraces;	//!< may be up-cast cPtrTrace to cIUnkBasePtr or cRefPtr

	protected:
		cPtrTraceMgr() noexcept
			: cSingleton<cPtrTraceMgr>(this, typeid(cPtrTraceMgr))
		{
		}
		~cPtrTraceMgr() noexcept
		{
		}

	public:
		virtual int TraceDump(cLogProcessor* pLog, ITERATE_t iCountExpected);
		CHEAPOBJECT_IMPL;
	};
}

#endif
