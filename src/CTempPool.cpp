//
//! @file CTempPool.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CTempPool.h"
#include "StrConst.h"
#include "StrU.h"
#include "StrT.h"

namespace Gray
{
	CThreadLocalSysNew<CTempPool> CTempPool::sm_ThreadLocalDefault;	// default Thread Local Mechanism. 
	IThreadLocal* CTempPool::sm_pThreadLocal = &CTempPool::sm_ThreadLocalDefault;	// can use CThreadLocalTypeNew<> instead

	CTempPool* GRAYCALL CTempPool::GetTempPool() // static
	{
		//! Get thread local CTempPool. create it if its not already allocated.
		ASSERT(sm_pThreadLocal != nullptr);
		return ((CTempPool*)(sm_pThreadLocal->GetDataNewV()));
	}
	void GRAYCALL CTempPool::FreeTempsForThreadManually() // static
	{
		// Manually free all stuff that would otherwise leak on this thread.
		if (sm_pThreadLocal == &sm_ThreadLocalDefault)
		{
			sm_ThreadLocalDefault.FreeDataManually();
		}
	}

	void CTempPool::CleanTemps()
	{
		//! reset contents for this pool. release element allocated memory. keep array.
		m_aBlocks.SetSize(0);
		m_iCountCur = 0;
	}

	void* CTempPool::GetTempV(size_t nLenNeed)
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

	void* CTempPool::GetTempV(size_t nLenNeed, const void* pData)
	{
		void* pDst = GetTempV(nLenNeed);
		if (pData != nullptr && pDst != nullptr)
		{
			CMem::Copy(pDst, pData, nLenNeed);
		}
		return pDst;
	}
}

