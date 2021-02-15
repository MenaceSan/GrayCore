//
//! @file cExceptionAssert.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cExceptionAssert_H
#define _INC_cExceptionAssert_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cException.h"

namespace Gray
{
	class GRAYCORE_LINK cExceptionAssert : public cException
	{
		//! @class Gray::cExceptionAssert
		//! Asserts can be seen as exceptions. details from coded ASSERT stuff.
	protected:
		const LOGCHAR_t* const m_pExp;
		const cDebugSourceLine m_Src;	// DEBUGSOURCELINE __FILE__ __LINE__
	public:
		cExceptionAssert(const LOGCHAR_t* pExp, LOGLEV_TYPE eSeverity, const cDebugSourceLine& src);
		virtual ~cExceptionAssert() THROW_DEF;
		virtual BOOL GetErrorMessage(GChar_t* lpszError, UINT nMaxError, UINT* pnHelpContext) override;
		static void GRAYCALL Throw(const LOGCHAR_t* pExp, const cDebugSourceLine& src);
	};
}

#endif
