//
//! @file cHookJump.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cHookJump.h"
#include "cLogMgr.h"

#if USE_INTEL

#ifdef _WIN32
#include "cMemPage.h"
#endif

namespace Gray
{
	bool cHookJump::isChainable() const noexcept
	{
		// The k_I_JUMP i inserted is just on top of another k_I_JUMP? Normal fixup jump table.
		// I don't need to lock and swap to call the old code. I can just chain jump to it

		// Look for other forms of chainable jmp commands. e.g. 48 ff 25
		if (m_OldCode[0] == 0x48 && m_OldCode[1] == 0xff && m_OldCode[2] == 0x25)
		{
			return true;
		}
		if (m_OldCode[0] == 0xff && m_OldCode[1] == 0x25)
		{
			// return true; // TODO? 
		}

		return m_OldCode[0] == k_I_JUMP;	// the old code was just a jump as well.
	}

	FARPROC cHookJump::GetChainFuncInt() const
	{
		// Get a chain callable function from this JMP.
		// ASSUME isChainable()

		int lRelAddr = 0; // not int_ptr
		STATIC_ASSERT(sizeof(lRelAddr) == k_LEN_JO, lRelAddr);

		// for k_I_JUMP
		if (m_OldCode[0] == k_I_JUMP)
		{
			::memcpy(&lRelAddr, m_OldCode + k_LEN_J, sizeof(lRelAddr));
			return (FARPROC)(((UINT_PTR)m_pFuncOrig) + lRelAddr + k_LEN_J + k_LEN_JO);
		}

		// Look for other forms of chainable jmp commands. e.g.  0xFF
		if (m_OldCode[0] == 0x48 && m_OldCode[1] == 0xff && m_OldCode[2] == 0x25)
		{
			::memcpy(&lRelAddr, m_OldCode + 3, sizeof(lRelAddr));
			return *((FARPROC*)(((UINT_PTR)m_pFuncOrig) + lRelAddr + 3 + k_LEN_JO));
		}
		if (m_OldCode[0] == 0xff && m_OldCode[1] == 0x25)
		{
			// TODO ?
		}

		return nullptr;
	}

	FARPROC cHookJump::GetChainFunc() const
	{
		// Get a callable function from this.
		FARPROC p = GetChainFuncInt();
		if (p == nullptr)
		{
			return m_pFuncOrig;		// assume it is swapped back in.
		}
		return p;
	}

	HRESULT cHookJump::SetProtectPages(bool isProtected)
	{
		//! Set/Remove code protection. so i can read/write to code space.
		//! ASSUME locked m_Lock.
#ifdef _WIN32
		return cMemPageMgr::I().ProtectPages((void*)m_pFuncOrig, k_LEN_A, isProtected);
#else
		return S_OK;
#endif
	}

	HRESULT cHookJump::InstallHook(FARPROC pFuncOrig, FARPROC pFuncNew, bool bSkipChainable)
	{
		//! Install my hook jump.
		//! @note X86 ONLY!! 32 or 64 bit.
		//! bSkipChainable = if chainable code exists, we should skip over it. because we assume other callers might do this too.
		//! @todo i could insert a value in an address table if the jump uses that format ? jmp [XXX]

		cThreadGuardFast guard(m_Lock);
		if (pFuncOrig == nullptr || pFuncNew == nullptr)
		{
			ASSERT(pFuncNew != nullptr);
			DEBUG_ERR(("InstallHook: nullptr."));
			return E_POINTER;
		}
		if (isHookInstalled())
		{
			ASSERT(m_pFuncOrig != nullptr);
			DEBUG_MSG(("InstallHook: already has JMP-implant."));
			return S_FALSE;
		}

		// DEBUG_TRACE(("InstallHook: pFuncOrig = %08x, pFuncNew = %08x", (UINT_PTR)pFuncOrig, (UINT_PTR)pFuncNew ));
		m_pFuncOrig = pFuncOrig;
		SetProtectPages(false);
		::memcpy(m_OldCode, (void*)pFuncOrig, sizeof(m_OldCode));		// save old code.

		if (bSkipChainable && isChainable())
		{
			SetProtectPages(true);
			return cHookJump::InstallHook(GetChainFuncInt(), pFuncNew, true);
		}

		const INT_PTR lRelPtr = ((UINT_PTR)pFuncNew - (UINT_PTR)pFuncOrig) - sizeof(m_Jump);
#ifdef USE_64BIT
		// Possible problem with 64 bit system. // assume this does not overflow 32 bits !!!
		if (lRelPtr < INT_MIN || lRelPtr > INT_MAX)
		{
			SetProtectPages(true);
			DEBUG_ERR(("InstallHook: 64 bit overflow."));
			return HRESULT_FROM_WIN32(ERROR_INVALID_HOOK_HANDLE);
		}
#endif
		const int lRelAddr = (int)lRelPtr;	// 32 bit.
		STATIC_ASSERT(sizeof(lRelAddr) == k_LEN_JO, lRelAddr);

		// DEBUG_TRACE(("InstallHook JMP %08x", lRelAddr));
		// create unconditional JMP to relative address is 5 bytes. X86/64 ONLY!!
		m_Jump[0] = k_I_JUMP;
		::memcpy(m_Jump + k_LEN_J, &lRelAddr, k_LEN_JO);

		if (::memcmp(m_Jump, m_OldCode, sizeof(m_Jump)) == 0)
		{
			// We already injected this with some other cHookJump instance! This is bad. We are fighting ourselfs with duplicated code !! why?
			// We must unwind in proper order or it will fail.
			m_Jump[0] = k_I_NULL;
			SetProtectPages(true);
			DEBUG_ERR(("InstallHook: duplicated hook."));
			return E_FAIL;
		}

		::memcpy((void*)pFuncOrig, m_Jump, sizeof(m_Jump));	// inject jump. we are armed!

		// DEBUG_MSG(("InstallHook: JMP-hook planted."));
		return S_OK;
	}

	void cHookJump::RemoveHook()
	{
		cThreadGuardFast guard(m_Lock);
		if (!isHookInstalled())	// was never set?
			return;
		ASSERT(m_pFuncOrig != nullptr);
		GRAY_TRY
		{
			::memcpy((void*)m_pFuncOrig, m_OldCode, sizeof(m_Jump));	// SwapOld(pFuncOrig)
			m_Jump[0] = k_I_NULL;	// destroy my jump. (must reconstruct it)
			SetProtectPages(true);
		}
			GRAY_TRY_CATCHALL
		{
			// UNREFERENCED_PARAMETER(ex);
			DEBUG_ERR(("cHookJump::RemoveHook FAIL"));
		}
			GRAY_TRY_END
	}
}
#endif
