//
//! @file CExceptionAssert.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CExceptionAssert_H
#define _INC_CExceptionAssert_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CException.h"

namespace Gray
{
	class GRAYCORE_LINK cExceptionAssert : public cException
	{
		//! @class Gray::cExceptionAssert
		//! Asserts can be seen as exceptions. details from coded ASSERT stuff.
	protected:
		const LOGCHAR_t* const m_pExp;
		const CDebugSourceLine m_Src;	// DEBUGSOURCELINE __FILE__ __LINE__
	public:
		cExceptionAssert(const LOGCHAR_t* pExp, LOGLEV_TYPE eSeverity, const CDebugSourceLine& src);
		virtual ~cExceptionAssert() THROW_DEF;
		virtual BOOL GetErrorMessage(GChar_t* lpszError, UINT nMaxError, UINT* pnHelpContext) override;
		static void GRAYCALL Throw(const LOGCHAR_t* pExp, const CDebugSourceLine& src);
	};
};

#endif
