//
//! @file cCodeProfiler.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cCodeProfiler.h"
#include "cFile.h"
#include "cThreadLock.h"
#include "cString.h"
#include "cAppState.h"

namespace Gray
{
	bool cCodeProfileFunc::sm_bActive = false;	// static

#pragma pack(push,2)
	struct CATTR_PACKED cCodeProfilerItem
	{
		//! @struct Gray::cCodeProfilerItem
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

	class GRAYCORE_LINK cCodeProfilerControl : public cSingleton< cCodeProfilerControl >
	{
		//! @class Gray::cCodeProfilerControl
		//! Thread locked singleton stream to write out to. Can be shared by multiple threads.

		friend class cCodeProfileFunc;
		friend class cSingleton< cCodeProfilerControl >;

	protected:
		cFile m_File;
		mutable cThreadLockCount m_Lock;
		PROCESSID_t m_ProcessId;

	public:
		cCodeProfilerControl()
			: cSingleton<cCodeProfilerControl>(this, typeid(cCodeProfilerControl))
			, m_ProcessId(cAppState::get_CurrentProcessId())
		{
		}
		~cCodeProfilerControl()
		{
			StopTime();
		}

		bool get_Active() const
		{
			return cCodeProfileFunc::sm_bActive;
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
			cThreadGuard lock(m_Lock);
			cCodeProfileFunc::sm_bActive = true;
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
			cThreadGuard lock(m_Lock);
			cCodeProfileFunc::sm_bActive = false;
			m_File.Close();
		}

		void WriteTime(const cTimePerf& nTimeEnd, const class cCodeProfileFunc& Func)
		{
			// cThreadLockableRef
			ASSERT(get_Active());
			if (!m_File.isFileOpen())
			{
				return;
			}
			cThreadGuard lock(m_Lock);
			if (!m_File.isFileOpen())
			{
				return;
			}
			StrLen_t iLenFile = StrT::Len(Func.m_src.m_pszFile);
			StrLen_t iLenFunc = StrT::Len(Func.m_src.m_pszFunction);
			StrLen_t iLenTotal = sizeof(cCodeProfilerItem) + iLenFile + iLenFunc + 2;

			BYTE Tmp[1024];
			ASSERT(iLenTotal < (StrLen_t) sizeof(Tmp));

			cCodeProfilerItem* pLogItem = reinterpret_cast<cCodeProfilerItem*>(Tmp);
			pLogItem->m_wSize = (WORD)iLenTotal;
			pLogItem->m_uLine = Func.m_src.m_uLine;
			pLogItem->m_ProcessID = m_ProcessId;
			pLogItem->m_ThreadID = cThreadId::GetCurrentId();
			pLogItem->m_Cycles = nTimeEnd.m_nTime - Func.m_nTimeStart.m_nTime;
			// pLogItem->m_Time = nTime;

			cMem::Copy(Tmp + sizeof(cCodeProfilerItem), Func.m_src.m_pszFile, iLenFile + 1);
			cMem::Copy(Tmp + sizeof(cCodeProfilerItem) + iLenFile + 1, Func.m_src.m_pszFunction, iLenFunc + 1);

			m_File.WriteX(Tmp, iLenTotal);
		}
	};

	void cCodeProfileFunc::StopTime()
	{
		//! Record time when this object is destroyed.
		cTimePerf nTimeEnd(true);	// End cycle count
		ASSERT(sm_bActive);
		ASSERT(m_nTimeStart.isTimeValid());
		// Write to log file
		cCodeProfilerControl* pController = cCodeProfilerControl::get_Single();
		pController->WriteTime(nTimeEnd, *this);
	}
}
 