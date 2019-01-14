//
//! @file CCodeProfiler.h
//! Declare entry/exit from a function such that it will build a profile.
//! Write out a profile PCP file.
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CCodeProfiler_H
#define _INC_CCodeProfiler_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CTimeSys.h"
#include "CUnitTestDecl.h"
#include "CDebugAssert.h"

UNITTEST_PREDEF(CCodeProfileFunc)

namespace Gray
{
	class GRAYCORE_LINK CCodeProfileFunc 
	{
		//! @class Gray::CCodeProfileFunc
		//! profile the entry/exit for a function.
		//! This is ALWAYS stack based so its thread safe.

		friend class CCodeProfilerControl;

	private:
		CDebugSourceLine m_src;		//!< Record source location of this function.
		CTimePerf m_nTimeStart;		//!< Function enter Start time in system clock ticks

		static bool sm_bActive;		//!< are we actively measuring? Thread Safe read.

	private:
		void StopTime();

	public:
		CCodeProfileFunc(CDebugSourceLine src)
			: m_src(src)	// Current source file + Current line in that file
			, m_nTimeStart(sm_bActive)	// Cheat a little and burn off 4 instructions inside counted function time.
		{
			//! record Start/Record cycle count on object construct.			
		}

		~CCodeProfileFunc()
		{
			if (sm_bActive)	// inline check for maximum speed.
				StopTime();
		}

		UNITTEST_FRIEND(CCodeProfileFunc);
	};

	// CCodeProfileFunc usage requires only single declaration at beginning of function
#if 0 // defined(_DEBUG)
#define CODEPROFILEFUNC()	CCodeProfileFunc _tagPROFILE_CLASS(DEBUGSOURCELINE)
#else
#define CODEPROFILEFUNC()	__noop	// compile out profiling. Do nothing.
#endif
};
#endif
