//
//! @file cTempPool.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cTempPool.h"
#include "StrConst.h"
#include "StrU.h"
#include "StrT.h"

namespace Gray
{
	cThreadLocalSysNew<cTempPool> cTempPool::sm_ThreadLocalDefault;	// default Thread Local Mechanism. 
	IThreadLocal* cTempPool::sm_pThreadLocal = &cTempPool::sm_ThreadLocalDefault;	// can use CThreadLocalTypeNew<> instead

	cTempPool* GRAYCALL cTempPool::GetTempPool() // static
	{
		//! Get thread local cTempPool. create it if its not already allocated.
		ASSERT(sm_pThreadLocal != nullptr);
		return ((cTempPool*)(sm_pThreadLocal->GetDataNewV()));
	}
	void GRAYCALL cTempPool::FreeTempsForThreadManually() // static
	{
		// Manually free all stuff that would otherwise leak on this thread.
		if (sm_pThreadLocal == &sm_ThreadLocalDefault)
		{
			sm_ThreadLocalDefault.FreeDataManually();
		}
	}

	void cTempPool::CleanTemps()
	{
		//! reset contents for this pool. release element allocated memory. keep array.
		m_aBlocks.SetSize(0);
		m_iCountCur = 0;
	}

	void* cTempPool::GetTempV(size_t nLenNeed)
	{
		//! Get a temporary/scratch memory space for random uses. Non leaking pointer return. beware of k_iCountMax.
		//! Ideally we use should CString(x).get_CPtr() instead.
		//! Typically used to hold "%s" argument conversions for StrT::sprintfN() type operations.
		//! @arg nLenNeed = exact size (in bytes) including space for '\0'

		if (m_aBlocks.GetSize() == 0)	// first time alloc.
		{
			m_aBlocks.SetSize(k_iCountMax);
			m_iCountCur = 0;
		}
		else
		{
			if (++m_iCountCur >= k_iCountMax)
				m_iCountCur = 0;
		}
		m_aBlocks[m_iCountCur].Alloc(nLenNeed);	// alloc to the size we need.
		return(m_aBlocks[m_iCountCur].get_Data());
	}

	void* cTempPool::GetTempV(size_t nLenNeed, const void* pData)
	{
		void* pDst = GetTempV(nLenNeed);
		if (pData != nullptr && pDst != nullptr)
		{
			cMem::Copy(pDst, pData, nLenNeed);
		}
		return pDst;
	}
}

