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
#include "CThreadArray.h"
#include "cSingleton.h"

#ifdef __linux__
// _uuidof(IUnknown) = "00000000-0000-0000-C000-000000000046"
GRAYCORE_LINK GUID IID_IUnknown = { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };	// access the GUID value via ref to this.
#endif

namespace Gray
{
	void cPtrTraceMgr::TraceDump(cLogProcessor& log, ITERATE_t iCountExpected) // virtual
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

	//****************************************************

	bool cPtrTrace::sm_bActive = false;

	void cPtrTrace::TraceOpen(void* p)
	{
		UNREFERENCED_PARAMETER(p);
		if (!sm_bActive)	// not tracking this now.
			return;
		if (cAppState::isInCExit())	// can't track this here.
			return;
		cPtrTraceMgr& mgr = cPtrTraceMgr::I();
		cThreadGuard threadguard(mgr.m_Lock);	// thread sync critical section.
		mgr.m_aTraces.Add(this);
	}

	void cPtrTrace::TraceClose(void* p)
	{
		UNREFERENCED_PARAMETER(p);
		if (!sm_bActive)	// not tracking this now.
			return;
		if (cAppState::isInCExit())	// can't track this here.
			return;
		cPtrTraceMgr& mgr = cPtrTraceMgr::I();
		cThreadGuard threadguard(mgr.m_Lock);	// thread sync critical section.
		mgr.m_aTraces.RemoveArgKey(this);
	}
}
