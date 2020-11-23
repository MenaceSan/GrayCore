//
//! @file cHookJump.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cHookJump.h"
#include "cLogMgr.h"

#ifdef _WIN32
#include "cMemPage.h"
#endif

namespace Gray
{
	FARPROC cHookJump::GetChainFunc() const
	{
		if (isChainable())
		{
			// Get a callable function from this.
			int lRelAddr; // not int_ptr
			STATIC_ASSERT(sizeof(lRelAddr) == k_LEN_P, lRelAddr);
 			::memcpy(&lRelAddr, m_OldCode + k_LEN_J, k_LEN_P);
			return (FARPROC)(((UINT_PTR)m_pFuncOrig) + lRelAddr + sizeof(m_OldCode));
		}
		return m_pFuncOrig;
	}

	bool cHookJump::InstallHook(FARPROC pFuncOrig, FARPROC pFuncNew)
	{
		//! @note X86 ONLY!! 32 or 64 bit.

		cThreadGuardFast guard(m_Lock);
		if (pFuncOrig == nullptr || pFuncNew == nullptr)
		{
			ASSERT(pFuncNew != nullptr);
			DEBUG_ERR(("InstallHook: nullptr."));
			return false;
		}
		if (isHookInstalled())
		{
			ASSERT(m_pFuncOrig != nullptr);
			DEBUG_MSG(("InstallHook: already has JMP-implant."));
			return true;
		}

		// DEBUG_TRACE(("InstallHook: pFuncOrig = %08x, pFuncNew = %08x", (UINT_PTR)pFuncOrig, (UINT_PTR)pFuncNew ));

#ifdef _WIN32
		// Remove code protection. so i can read/write to code space.
		const HRESULT hRes = cMemPageMgr::I().ProtectPages((void*)pFuncOrig, k_LEN_A, false);
		if (FAILED(hRes))
		{
			return false;
		}
#endif

		m_pFuncOrig = pFuncOrig;		 
		::memcpy(m_OldCode, (void*)pFuncOrig, sizeof(m_OldCode));		// save old code.

		// create unconditional JMP to relative address is 5 bytes. X86/64 ONLY!!
#if USE_INTEL
		m_Jump[0] = k_I_JUMP;
#else
#error The CPU type must be defined as _M_IX86 or _M_X64
#endif

		const int lRelAddr = (int)((UINT_PTR)pFuncNew - (UINT_PTR)pFuncOrig) - sizeof(m_Jump);
		STATIC_ASSERT(sizeof(lRelAddr) == k_LEN_P, lRelAddr);
		DEBUG_TRACE(("InstallHook JMP %08x", lRelAddr));
		::memcpy(m_Jump + k_LEN_J, &lRelAddr, k_LEN_P);

		if (::memcmp(m_Jump, m_OldCode, sizeof(m_OldCode)) == 0)
		{
			// We already injected this with some other cHookJump instance! This is bad. We are fighting ourselfs with duplicated code !! why?

		}

		::memcpy((void*)pFuncOrig, m_Jump, sizeof(m_Jump));	// inject jump. we are armed!

		DEBUG_MSG(("InstallHook: JMP-hook planted."));
		return true;
	}

	void cHookJump::RemoveHook()
	{
		cThreadGuardFast guard(m_Lock);
		if (!isHookInstalled())	// was never set?
			return;
		ASSERT(m_pFuncOrig != nullptr);
		GRAY_TRY
		{
			::memcpy((void*)m_pFuncOrig, m_OldCode, sizeof(m_OldCode));	// SwapOld(pFuncOrig)
			m_Jump[0] = k_I_NULL;	// destroy my jump. (must reconstruct it)
			ASSERT(!isHookInstalled());

	#ifdef _WIN32
			// Restore code protection.
			cMemPageMgr::I().ProtectPages((void*)m_pFuncOrig, k_LEN_A, true);
	#endif
		}
			GRAY_TRY_CATCHALL
		{
			// UNREFERENCED_PARAMETER(ex);
			DEBUG_ERR(("cHookJump::RemoveHook FAIL"));
		}
			GRAY_TRY_END
	}
}
 