//
//! @file cPtrTrace.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cPtrTrace.h"
#include "cPtrTraceMgr.h"
#include "cIUnkPtr.h"
#include "cLogMgr.h"
#include "cAppState.h"
#include "cThreadArray.h"
#include "cSingleton.h"

#ifdef __linux__
// _uuidof(IUnknown) = "00000000-0000-0000-C000-000000000046"
GRAYCORE_LINK GUID IID_IUnknown = { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };	// access the GUID value via ref to this.
#endif

namespace Gray
{
	int cPtrTraceMgr::TraceDump(cLogProcessor* pLog, ITERATE_t iCountExpected) // virtual
	{
		//! Dump all the IUnks that are left not released !!!

		cThreadGuard threadguard(m_Lock);	// thread sync critical section.
		const ITERATE_t iCount = m_aTraces.GetSize();
		int iLockCountTotal = 0;

		for (ITERATE_t i = 0; i < iCount; i++)
		{
			cPtrTrace* pTrace = m_aTraces.GetAt(i);
			if (pTrace == nullptr)
				break;

			IUnknown* p1 = pTrace->m_pIUnk;	// The stored pointer. should be valid!
			ASSERT(p1 != nullptr);

			// pTrace -> cPtrFacade
			const int iLockCount2 = p1->AddRef() - 1;
			ASSERT(iLockCount2 >= 1);
			p1->Release();

			if (pLog != nullptr)
			{
				pLog->addInfoF("IUnknown=0%x, Locks=%d, Type=%s, File='%s',%d",
					(UINT_PTR)p1, iLockCount2,
					LOGSTR(pTrace->m_TypeInfo.name()),
					LOGSTR(pTrace->m_Src.m_pszFile), pTrace->m_Src.m_uLine);
			}

			iLockCountTotal += iLockCount2;
		}
		if (pLog != nullptr)
		{
			pLog->addEventF(LOG_ATTR_DEBUG, (iCount == iCountExpected) ? LOGLEV_INFO : LOGLEV_ERROR,
				"IUnk Dump of %d objects %d locks (of %d expected).",
				iCount, iLockCountTotal, iCountExpected);
		}
		return iLockCountTotal;
	}

	//****************************************************

	bool cPtrTrace::sm_bActive = false;

	void cPtrTrace::TraceAttach(IUnknown* p)
	{
		//! Called when cPtrTrace is created.
		if (!sm_bActive)	// not tracking this now.
			return;
		if (cAppState::isInCExit())	// can't track this here.
			return;
		ASSERT(m_pIUnk == nullptr || m_pIUnk == p);
		cPtrTraceMgr& mgr = cPtrTraceMgr::I();
		cThreadGuard threadguard(mgr.m_Lock);	// thread sync critical section.
		ASSERT(mgr.m_aTraces.FindIForKey(this) <= k_ITERATE_BAD); // no dupe.
		mgr.m_aTraces.Add(this);
		m_pIUnk = p;
	}

	void cPtrTrace::TraceRelease(IUnknown* p)
	{
		//! Called when cPtrTrace is destroyed. 
		if (!sm_bActive)	// not tracking this now. m_aTraces MUST be EMPTY!
			return;
		if (cAppState::isInCExit())	// can't track this here.
		{
			sm_bActive = false;
			return;
		}
		ASSERT(m_pIUnk != nullptr);
		cPtrTraceMgr& mgr = cPtrTraceMgr::I();
		cThreadGuard threadguard(mgr.m_Lock);	// thread sync critical section.
		bool bRet = mgr.m_aTraces.RemoveArgKey(this);
		ASSERT(bRet);	// Must succeed
		m_pIUnk = nullptr;
	}
}
