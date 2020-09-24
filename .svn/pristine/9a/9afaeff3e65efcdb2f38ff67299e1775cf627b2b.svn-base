//
//! @file CPtrTrace.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CPtrFacade.h"
#include "CIUnkPtr.h"

#ifdef __linux__
// _uuidof(IUnknown) = "00000000-0000-0000-C000-000000000046"
GRAYCORE_LINK GUID IID_IUnknown = { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };	// access the GUID value via ref to this.
#endif

#include "CLogMgr.h"
#include "CAppState.h"
#include "CThreadArray.h"
#include "CSingleton.h"

namespace Gray
{
	class GRAYCORE_LINK CPtrTraceMgr
		: public CSingleton < CPtrTraceMgr >
	{
		//! @class Gray::CPtrTraceMgr
		//! USE_IUNK_TRACE = We are tracing all calls to CIUnkPtr<> so we can figure out who is not releasing their ref.
		friend class CSingleton < CPtrTraceMgr >;
		friend class CPtrTrace;

	public:
		static bool sm_bActive;		//!< Turn on/off CPtrTraceMgr
		mutable CThreadLockCount m_Lock;
		CArraySortVal<CPtrTrace*> m_aTraces;

	protected:
		CPtrTraceMgr();
		~CPtrTraceMgr();

	public:
		void GRAYCALL TraceDump(CLogProcessor& log, ITERATE_t iCountExpected);
		static void GRAYCALL TracePtr(CPtrTrace* p2, void* p, bool bAdd);

		CHEAPOBJECT_IMPL;
	};

	void GRAYCALL CPtrTrace::TraceDump(CLogProcessor& log, ITERATE_t iCountExpected) // static
	{
		CPtrTraceMgr::I().TraceDump(log, iCountExpected);
	}

	void CPtrTrace::TraceOpen(void* p)
	{
		CPtrTraceMgr::TracePtr(this, p, true);
	}

	void CPtrTrace::TraceClose(void* p)
	{
		CPtrTraceMgr::TracePtr(this, p, false);
	}

	//****************************************************

	bool CPtrTraceMgr::sm_bActive = false;

	CPtrTraceMgr::CPtrTraceMgr()
		: CSingleton<CPtrTraceMgr>(this, typeid(CPtrTraceMgr))
	{
	}
	CPtrTraceMgr::~CPtrTraceMgr()
	{
	}

	void GRAYCALL CPtrTraceMgr::TracePtr(CPtrTrace* pIUnkTrace, void* p, bool bAdd) // static
	{
		//! find the tracking record for this lock.

		if (!sm_bActive)	// not tracking this now.
			return;
		if (CAppState::isInCExit())	// can't track this here.
			return;

		UNREFERENCED_PARAMETER(p);
		CPtrTraceMgr& mgr = CPtrTraceMgr::I();

		CThreadGuard threadguard(mgr.m_Lock);	// thread sync critical section.
		if (bAdd)
		{
			mgr.m_aTraces.Add(pIUnkTrace);
		}
		else
		{
			mgr.m_aTraces.RemoveArgKey(pIUnkTrace);
		}
	}

	void CPtrTraceMgr::TraceDump(CLogProcessor& log, ITERATE_t iCountExpected)
	{
		//! Dump all the IUnks that are left not released !!!

		CThreadGuard threadguard(m_Lock);	// thread sync critical section.
		ITERATE_t iCount = m_aTraces.GetSize();
		int iLockCountTotal = 0;

		for (ITERATE_t i = 0; i < iCount; i++)
		{
			CPtrTrace* pIUnkTrace = m_aTraces.GetAt(i);
			if (pIUnkTrace == nullptr)
				break;
			CIUnkBasePtr* p2 = (CIUnkBasePtr*)pIUnkTrace;
			int iLockCount2 = p2->get_RefCount();
			log.addInfoF("IUnknown=0%x, Type=%s, Locks=%d, File='%s',%d",
				(UINT_PTR)p2->get_Ptr(), LOGSTR(pIUnkTrace->m_pszType), iLockCount2,
				LOGSTR(pIUnkTrace->m_Src.m_pszFile), pIUnkTrace->m_Src.m_uLine);
			iLockCountTotal += iLockCount2;
		}

		log.addEventF(LOG_ATTR_DEBUG, (iCount == iCountExpected) ? LOGLEV_INFO : LOGLEV_ERROR,
			"IUnk Dump of %d objects %d locks (of %d expected).",
			iCount, iLockCountTotal, iCountExpected);
	}
};

//******************************************************************

#ifdef USE_UNITTESTS 
#include "CUnitTest.h"
#include "CLogEvent.h"

UNITTEST_CLASS(CPtrTraceMgr)
{
	UNITTEST_METHOD(CPtrTraceMgr)
	{
		bool bPrevActive = CPtrTraceMgr::sm_bActive;
		ITERATE_t nPrevCount = CPtrTraceMgr::I().m_aTraces.GetSize();
		CPtrTraceMgr::sm_bActive = true;

#ifdef USE_IUNK_TRACE
		{
			int iRefCount = 0;
			CIUnkPtr<CLogEvent> p1(new CLogEvent(LOG_ATTR_0, LOGLEV_ANY, "UnitTest", ""));
			IUNK_ATTACH(p1);
			iRefCount = p1.get_RefCount();
			UNITTEST_TRUE(iRefCount == 1);
			CIUnkPtr<CLogEvent> p2(p1);
			IUNK_ATTACH(p2);
			iRefCount = p1.get_RefCount();
			UNITTEST_TRUE(iRefCount == 2);
			CPtrTraceMgr::I().TraceDump(*sm_pLog, 2 + nPrevCount);
			p2 = nullptr;	// ReleasePtr
			iRefCount = p1.get_RefCount();
			UNITTEST_TRUE(iRefCount == 1);

			CSmartBasePtr pBase(new CSmartBase());
			UNITTEST_TRUE(pBase != nullptr);

			IUnknown* pObject = nullptr;
			HRESULT hRes = pBase->QueryInterface(__uuidof(IUnknown), (void**)&pObject);
			UNITTEST_TRUE(SUCCEEDED(hRes));
			pObject->Release();
			UNITTEST_TRUE(pObject == pBase);
			iRefCount = pBase.get_RefCount();
			UNITTEST_TRUE(iRefCount == 1);
		}
#endif

		CPtrTraceMgr::sm_bActive = bPrevActive;
		CPtrTraceMgr::I().TraceDump(*sm_pLog, nPrevCount);
	}
};
UNITTEST_REGISTER(CPtrTraceMgr, UNITTEST_LEVEL_Core);
#endif	// USE_UNITTESTS

