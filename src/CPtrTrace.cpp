//
//! @file cPtrTrace.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cPtrFacade.h"
#include "cIUnkPtr.h"

#ifdef __linux__
// _uuidof(IUnknown) = "00000000-0000-0000-C000-000000000046"
GRAYCORE_LINK GUID IID_IUnknown = { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };	// access the GUID value via ref to this.
#endif

#include "cLogMgr.h"
#include "cAppState.h"
#include "CThreadArray.h"
#include "cSingleton.h"

namespace Gray
{
	class GRAYCORE_LINK cPtrTraceMgr
		: public cSingleton < cPtrTraceMgr >
	{
		//! @class Gray::cPtrTraceMgr
		//! USE_IUNK_TRACE = We are tracing all calls to cIUnkPtr<> so we can figure out who is not releasing their ref.
		friend class cSingleton < cPtrTraceMgr >;
		friend class cPtrTrace;

	public:
		static bool sm_bActive;		//!< Turn on/off cPtrTraceMgr
		mutable cThreadLockCount m_Lock;
		cArraySortVal<cPtrTrace*> m_aTraces;

	protected:
		cPtrTraceMgr();
		~cPtrTraceMgr();

	public:
		void GRAYCALL TraceDump(cLogProcessor& log, ITERATE_t iCountExpected);
		static void GRAYCALL TracePtr(cPtrTrace* p2, void* p, bool bAdd);

		CHEAPOBJECT_IMPL;
	};

	void GRAYCALL cPtrTrace::TraceDump(cLogProcessor& log, ITERATE_t iCountExpected) // static
	{
		cPtrTraceMgr::I().TraceDump(log, iCountExpected);
	}

	void cPtrTrace::TraceOpen(void* p)
	{
		cPtrTraceMgr::TracePtr(this, p, true);
	}

	void cPtrTrace::TraceClose(void* p)
	{
		cPtrTraceMgr::TracePtr(this, p, false);
	}

	//****************************************************

	bool cPtrTraceMgr::sm_bActive = false;

	cPtrTraceMgr::cPtrTraceMgr()
		: cSingleton<cPtrTraceMgr>(this, typeid(cPtrTraceMgr))
	{
	}
	cPtrTraceMgr::~cPtrTraceMgr()
	{
	}

	void GRAYCALL cPtrTraceMgr::TracePtr(cPtrTrace* pIUnkTrace, void* p, bool bAdd) // static
	{
		//! find the tracking record for this lock.

		if (!sm_bActive)	// not tracking this now.
			return;
		if (cAppState::isInCExit())	// can't track this here.
			return;

		UNREFERENCED_PARAMETER(p);
		cPtrTraceMgr& mgr = cPtrTraceMgr::I();

		cThreadGuard threadguard(mgr.m_Lock);	// thread sync critical section.
		if (bAdd)
		{
			mgr.m_aTraces.Add(pIUnkTrace);
		}
		else
		{
			mgr.m_aTraces.RemoveArgKey(pIUnkTrace);
		}
	}

	void cPtrTraceMgr::TraceDump(cLogProcessor& log, ITERATE_t iCountExpected)
	{
		//! Dump all the IUnks that are left not released !!!

		cThreadGuard threadguard(m_Lock);	// thread sync critical section.
		ITERATE_t iCount = m_aTraces.GetSize();
		int iLockCountTotal = 0;

		for (ITERATE_t i = 0; i < iCount; i++)
		{
			cPtrTrace* pIUnkTrace = m_aTraces.GetAt(i);
			if (pIUnkTrace == nullptr)
				break;
			cIUnkBasePtr* p2 = (cIUnkBasePtr*)pIUnkTrace;
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

#if USE_UNITTESTS 
#include "cUnitTest.h"
#include "cLogEvent.h"

UNITTEST_CLASS(cPtrTraceMgr)
{
	UNITTEST_METHOD(cPtrTraceMgr)
	{
		bool bPrevActive = cPtrTraceMgr::sm_bActive;
		ITERATE_t nPrevCount = cPtrTraceMgr::I().m_aTraces.GetSize();
		cPtrTraceMgr::sm_bActive = true;

#ifdef USE_IUNK_TRACE
		{
			int iRefCount = 0;
			cIUnkPtr<cLogEvent> p1(new cLogEvent(LOG_ATTR_0, LOGLEV_ANY, "UnitTest", ""));
			IUNK_ATTACH(p1);
			iRefCount = p1.get_RefCount();
			UNITTEST_TRUE(iRefCount == 1);
			cIUnkPtr<cLogEvent> p2(p1);
			IUNK_ATTACH(p2);
			iRefCount = p1.get_RefCount();
			UNITTEST_TRUE(iRefCount == 2);
			cPtrTraceMgr::I().TraceDump(*sm_pLog, 2 + nPrevCount);
			p2 = nullptr;	// ReleasePtr
			iRefCount = p1.get_RefCount();
			UNITTEST_TRUE(iRefCount == 1);

			cRefBasePtr pBase(new cRefBase());
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

		cPtrTraceMgr::sm_bActive = bPrevActive;
		cPtrTraceMgr::I().TraceDump(*sm_pLog, nPrevCount);
	}
};
UNITTEST_REGISTER(cPtrTraceMgr, UNITTEST_LEVEL_Core);
#endif	// USE_UNITTESTS

