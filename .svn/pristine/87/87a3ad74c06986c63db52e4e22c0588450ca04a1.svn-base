//
//! @file CCodeProfiler.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CCodeProfiler.h"
#include "CFile.h"
#include "CThreadLock.h"
#include "CString.h"
#include "CAppState.h"

namespace Gray
{
	bool CCodeProfileFunc::sm_bActive = false;	// static

#pragma pack(push,2)
	struct CATTR_PACKED CCodeProfilerItem
	{
		//! @struct Gray::CCodeProfilerItem
		//! profile log file item struct
		WORD m_wSize;			//!< Number of bytes in this record
		WORD m_uLine;			//!< Line number in source file
		UINT_PTR m_ProcessID;	//!< Current process ID PROCESSID_t
		UINT_PTR m_ThreadID;	//!< Current thread ID THREADID_t
		UINT64 m_Cycles;		//!< how many cycles inside this? TIMEPERF_t
		// ?? INT64 m_Time;		//!< What did this start absolutely ? (so sub stuff can be removed.)

		// File name string. 0 term
		// Func/Function name string. 0 term
	};
#pragma pack(pop)

	class GRAYCORE_LINK CCodeProfilerControl : public CSingleton< CCodeProfilerControl >
	{
		//! @class Gray::CCodeProfilerControl
		//! Thread locked singleton stream to write out to. Can be shared by multiple threads.

		friend class CCodeProfileFunc;
		friend class CSingleton< CCodeProfilerControl >;

	protected:
		cFile m_File;
		mutable CThreadLockCount m_Lock;
		PROCESSID_t m_ProcessId;

	public:
		CCodeProfilerControl()
			: CSingleton<CCodeProfilerControl>(this, typeid(CCodeProfilerControl))
			, m_ProcessId(CAppState::get_CurrentProcessId())
		{
		}
		~CCodeProfilerControl()
		{
			StopTime();
		}

		bool get_Active() const
		{
			return CCodeProfileFunc::sm_bActive;
		}

		bool put_Active(bool bActive)
		{
			if (bActive == get_Active())
				return true;
			if (bActive)
				return StartTime();
			StopTime();
			return true;
		}

	protected:
		bool StartTime()
		{
			CThreadGuard lock(m_Lock);
			CCodeProfileFunc::sm_bActive = true;
			if (m_File.isFileOpen())
			{
				return true;
			}
			// open it. PCP file.
			HRESULT hRes = m_File.OpenX(_FN("profile.pcp"), OF_CREATE | OF_SHARE_DENY_NONE | OF_WRITE | OF_BINARY);	// append
			if (FAILED(hRes))
			{
				return false;
			}
			m_File.SeekToEnd();		// Start at end of file
			return true;
		}

		void StopTime()
		{
			CThreadGuard lock(m_Lock);
			CCodeProfileFunc::sm_bActive = false;
			m_File.Close();
		}

		void WriteTime(const CTimePerf& nTimeEnd, const class CCodeProfileFunc& Func)
		{
			// CThreadLockableObj
			ASSERT(get_Active());
			if (!m_File.isFileOpen())
			{
				return;
			}
			CThreadGuard lock(m_Lock);
			if (!m_File.isFileOpen())
			{
				return;
			}
			StrLen_t iLenFile = StrT::Len(Func.m_src.m_pszFile);
			StrLen_t iLenFunc = StrT::Len(Func.m_src.m_pszFunction);
			StrLen_t iLenTotal = sizeof(CCodeProfilerItem) + iLenFile + iLenFunc + 2;

			BYTE Tmp[1024];
			ASSERT(iLenTotal < (StrLen_t) sizeof(Tmp));

			CCodeProfilerItem* pLogItem = reinterpret_cast<CCodeProfilerItem*>(Tmp);
			pLogItem->m_wSize = (WORD)iLenTotal;
			pLogItem->m_uLine = Func.m_src.m_uLine;
			pLogItem->m_ProcessID = m_ProcessId;
			pLogItem->m_ThreadID = CThreadId::GetCurrentId();
			pLogItem->m_Cycles = nTimeEnd.m_nTime - Func.m_nTimeStart.m_nTime;
			// pLogItem->m_Time = nTime;

			CMem::Copy(Tmp + sizeof(CCodeProfilerItem), Func.m_src.m_pszFile, iLenFile + 1);
			CMem::Copy(Tmp + sizeof(CCodeProfilerItem) + iLenFile + 1, Func.m_src.m_pszFunction, iLenFunc + 1);

			m_File.WriteX(Tmp, iLenTotal);
		}
	};

	void CCodeProfileFunc::StopTime()
	{
		//! Record time when this object is destroyed.
		CTimePerf nTimeEnd(true);	// End cycle count
		ASSERT(sm_bActive);
		ASSERT(m_nTimeStart.isTimeValid());
		// Write to log file
		CCodeProfilerControl* pController = CCodeProfilerControl::get_Single();
		pController->WriteTime(nTimeEnd, *this);
	}
}

//***************************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"

UNITTEST_CLASS(CCodeProfileFunc)
{
	void TestFunction()
	{
		CCodeProfileFunc tester_CCodeProfileFunc(DEBUGSOURCELINE);
		CThreadId::SleepCurrent(2);		// Waste some time.
	}
	UNITTEST_METHOD(CCodeProfileFunc)
	{
		TestFunction();
		// Now look to see if the function recorded something.
	}
};
UNITTEST_REGISTER(CCodeProfileFunc, UNITTEST_LEVEL_Core);
#endif
