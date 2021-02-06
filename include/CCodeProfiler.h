//
//! @file cCodeProfiler.h
//! Declare entry/exit from a function such that it will build a profile.
//! Write out a profile PCP file.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cCodeProfiler_H
#define _INC_cCodeProfiler_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cTimeSys.h"
#include "cUnitTestDecl.h"
#include "cDebugAssert.h"

namespace Gray
{
	class GRAYCORE_LINK cCodeProfileFunc
	{
		//! @class Gray::cCodeProfileFunc
		//! profile the entry/exit for a function.
		//! This is ALWAYS stack based so its thread safe.

		friend class cCodeProfilerControl;

	private:
		cDebugSourceLine m_src;		//!< Record source location of this function.
		cTimePerf m_nTimeStart;		//!< Function enter Start time in system clock ticks

		static bool sm_bActive;		//!< are we actively measuring? Thread Safe read.

	private:
		void StopTime();

	public:
		cCodeProfileFunc(cDebugSourceLine src)
			: m_src(src)	// Current source file + Current line in that file
			, m_nTimeStart(sm_bActive)	// Cheat a little and burn off 4 instructions inside counted function time.
		{
			//! record Start/Record cycle count on object construct.			
		}

		~cCodeProfileFunc()
		{
			if (sm_bActive)	// inline check for maximum speed.
				StopTime();
		}

		UNITTEST_FRIEND(cCodeProfiler);
	};

	// cCodeProfileFunc usage requires only single declaration at beginning of function
#if 0 // defined(_DEBUG)
#define CODEPROFILEFUNC()	cCodeProfileFunc _tagPROFILE_CLASS(DEBUGSOURCELINE)
#else
#define CODEPROFILEFUNC()	__noop	// compile out profiling. Do nothing.
#endif
};
#endif
