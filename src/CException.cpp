//
//! @file CException.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CException.h"
#include "CString.h"
#include "CTimeSys.h"
#include "CCodeProfiler.h"
#include "CLogMgr.h"
#include "StrU.h"
#include "CObjectCreator.h"

namespace Gray
{
	cException* cExceptionHolder::get_Ex() const
	{
		//! Get the cException version of the exception if it is based on it. Use dynamic_cast.
		//! @return nullptr if a native MFC "::CException" or STL "stl::exception".
		return DYNPTR_CAST(cException, m_p);
	}

	BOOL cExceptionHolder::GetErrorMessage(LOGCHAR_t* lpszError, StrLen_t nLenMaxError) const
	{
		if (nLenMaxError <= 1)
			return false;
		if (m_p == nullptr)
		{
			lpszError[0] = '?';
			lpszError[1] = '\0';
			return false;
		}

#ifdef _MFC_VER	// using _MFC_VER.
#ifdef USE_UNICODE
		GChar_t szTmp[cExceptionHolder::k_MSG_MAX_SIZE];
		m_p->GetErrorMessage(szTmp, (UINT)cExceptionHolder::k_MSG_MAX_SIZE, nullptr);
		StrU::UNICODEtoUTF8(lpszError, nLenMaxError, szTmp, cExceptionHolder::k_MSG_MAX_SIZE);
		return true;
#else
		return m_p->GetErrorMessage(lpszError, (UINT)nLenMaxError, nullptr);
#endif
#else
		cException* pEx = get_Ex();
		if (pEx == nullptr)	// NOT cException based. STL exception.
		{
			StrT::CopyLen(lpszError, m_p->what(), nLenMaxError);
			return true;
		}
		return pEx->GetErrorMessage(lpszError, nLenMaxError, nullptr);
#endif
	}
	CStringL cExceptionHolder::get_ErrorStr() const
	{
		if (m_p == nullptr)
		{
			return "?";
		}

#ifdef _MFC_VER	// using _MFC_VER.
		LOGCHAR_t szTmp[cExceptionHolder::k_MSG_MAX_SIZE];
		GetErrorMessage(szTmp, STRMAX(szTmp));
		return szTmp;
#else
		cException* pEx = get_Ex();
		if (pEx == nullptr)	// NOT cException based. raw STL/MFC exception.
		{
			return m_p->what();
		}
		return pEx->get_ErrorStr();
#endif
	}
	LOGLEV_TYPE cExceptionHolder::get_Severity() const
	{
		cException* pEx = get_Ex();
		if (pEx == nullptr)	// NOT cException based. raw STL/MFC  exception.
		{
			return LOGLEV_CRIT;
		}
		return pEx->get_Severity();
	}

	//*********************************************************************
	const LOGCHAR_t* cException::k_szDescriptionDefault = "Exception";

	IMPLEMENT_DYNAMIC(cException, cExceptionBase);

	BOOL cException::GetErrorMessage(GChar_t* lpszError, UINT nLenMaxError, UINT* pnHelpContext) // virtual
	{
		//! Compatible with MFC CException and CFileException. Assume this is typically overridden at a higher level.
		//! @note stupid BOOL return is for MFC compatibility.
		//! allow UNICODE version of this.
		CODEPROFILEFUNC();
		UNREFERENCED_PARAMETER(pnHelpContext);
		StrLen_t iLen = StrT::sprintfN(lpszError, nLenMaxError - 1,
			_GT("%s'%s'"),
			StrArg<GChar_t>(CLogLevel::GetPrefixStr(m_eSeverity)), StrArg<GChar_t>(m_pszDescription));
		UNREFERENCED_PARAMETER(iLen);
		return true;
	}

	GRAYCORE_LINK CStringL cException::get_ErrorStr() // similar to STL what()
	{
		// Get error in UTF8.
		GChar_t szTmp[cExceptionHolder::k_MSG_MAX_SIZE];
		GetErrorMessage(szTmp, STRMAX(szTmp), nullptr);
		return szTmp;
	}

	//***************************************************************************

	BOOL cExceptionHResult::GetErrorMessage(GChar_t* lpszError, UINT nLenMaxError, UINT* pnHelpContext) // virtual
	{
		//! Compatible with MFC CException and CFileException
		//! @note pointless BOOL return is for MFC compatibility.
		CODEPROFILEFUNC();
		UNREFERENCED_PARAMETER(pnHelpContext);

		if (m_hResultCode != S_OK)
		{
			// return the message defined by the system for the error code
			GChar_t szCode[cExceptionHolder::k_MSG_MAX_SIZE];
			StrLen_t nChars = HResult::GetTextV(m_hResultCode, szCode, STRMAX(szCode), k_va_list_empty); // pnHelpContext
			if (nChars > 0)
			{
				StrT::sprintfN(lpszError, nLenMaxError - 1,
					_GT("Error Pri=%d, Code=0%x(%s), Desc='%s'"),
					m_eSeverity, (UINT)m_hResultCode, StrArg<GChar_t>(szCode), StrArg<GChar_t>(m_pszDescription));
				return true;
			}
		}

		return SUPER_t::GetErrorMessage(lpszError, nLenMaxError, pnHelpContext);
	}
}

//***************************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"

UNITTEST_CLASS(cException)
{
	class cExceptionTest : public cException
	{
	public:
		cExceptionTest(const char* pszMsg, LOGLEV_TYPE eLogLevel)
			: cException(pszMsg, eLogLevel)
		{
		}
	};

	UNITTEST_METHOD(cException)
	{
#ifdef _CPPUNWIND
		static const char* k_pszMsg = "Test Exception";
		GRAY_TRY
		{
			GRAY_THROW cExceptionTest(k_pszMsg, LOGLEV_TRACE);
		}
		GRAY_TRY_CATCH(cExceptionBase, ex)
		{
			// We should get here.
			cExceptionHolder exh(ex);	// kill it.
			UNITTEST_TRUE(exh.get_Severity() == LOGLEV_TRACE);
			CStringL sMsg = exh.get_ErrorStr();	// may have prefix.
			UNITTEST_TRUE(sMsg.Contains(k_pszMsg));
			return;	// done.
		}
#ifndef _MFC_VER
		GRAY_TRY_CATCHALL
		{
			UNITTEST_TRUE(false);	// should not get here !
		}
#endif
		GRAY_TRY_END
		UNREACHABLE_CODE(__noop);	// should never get here.
#else
		// No exceptions allowed
#endif
	}
};
UNITTEST_REGISTER(cException, UNITTEST_LEVEL_Core);
#endif
