//
//! @file cHookJump.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cHookJump_H
#define _INC_cHookJump_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cThreadLock.h"
#include "cMem.h"

#if USE_INTEL
namespace Gray
{
	class GRAYCORE_LINK cHookJump
	{
		//! @class GrayLib::cHookJump
		//! Create/Define a relative jump to hook/replace an existing API call. Jump to new code is injected into old function start.
		//! @note This is NOT for use in hooking an interface or a vtable. those don't require a jump instruction.
		//! @note This ASSUMEs Intel x86 type CPU. 32 or 64 bit instructions. The CPU type must be defined as _M_IX86 or _M_X64
		//! https://www.felixcloutier.com/x86/jmp

	public:
		static const BYTE k_I_NULL = 0x00;		//!< Not a valid instruction.

		static const BYTE k_I_JUMP = 0xe9;		//!< X86 32 bit relative jump instruction (same on 64 bit system). NOTE: "48 ff 25" can act the same in 64 bit code. 3 byte jump prefix. or "ff 25" for 32 bit code. 
		static const size_t k_LEN_J = 1;	//!< size_t of the jump instruction. 0xe9 = k_JUMP_I
		static const size_t k_LEN_JO = 4;	//!< size_t of the relative 32 bit jump offset. NOT same as sizeof(FARPROC) or INT_PTR

#ifdef USE_64BIT
		static const size_t k_LEN_A = 16;	//!< size_t k_LEN_D page aligned to 16 (for 64 bit code)
#else
		static const size_t k_LEN_A = 8;	//!< size_t k_LEN_D page aligned to 8 (for 32 bit code)
#endif

		friend class cHookSwapLock;
 
	protected:
		FARPROC m_pFuncOrig;			//!< Pointer to the original/old function. The one i will replace. Inject code here.
		BYTE m_OldCode[k_LEN_A];		//!< What was at m_pFuncOrig previously. Take more than i actually need to account for isChainable() tests.
		BYTE m_Jump[k_LEN_J + k_LEN_JO];			//!< What do i want to replace m_pFuncOrig with. jump to pFuncNew
		mutable cThreadLockFast m_Lock;			//!< prevent multiple threads from using this at the same time.

	protected:
		bool SwapOld() noexcept
		{
			//! put back saved code fragment. temporary to call previous version of the function.
			//! ASSUME use of cHookSwapLock m_Lock, and SetProtectPages
			if (!isHookValid())
				return false;
			cMem::Copy((void*)m_pFuncOrig, m_OldCode, sizeof(m_Jump));
			return true;
		}
		void SwapReset() noexcept
		{
			//! put back original JMP instruction again
			//! ASSUME use of cHookSwapLock m_Lock, and SetProtectPages
			if (!isHookInstalled() || m_pFuncOrig == nullptr)	// hook has since been destroyed!
				return;
			cMem::Copy((void*)m_pFuncOrig, m_Jump, sizeof(m_Jump));
		}

		HRESULT SetProtectPages(bool isProtected);
		FARPROC GetChainFuncInt() const;

	public:
		cHookJump() noexcept
			: m_pFuncOrig(nullptr)
		{
			m_OldCode[0] = k_I_NULL;
			m_Jump[0] = k_I_NULL;
		}
		~cHookJump()
		{
			RemoveHook();
		}

		bool isHookInstalled() const noexcept
		{
			return m_Jump[0] == k_I_JUMP;
		}
		bool isHookValid() const noexcept
		{
			//! @note sometimes DLLs' can reload themselves and destroy our hook behind our backs.
			if (!isHookInstalled() || m_pFuncOrig == nullptr)
				return false;
			// ASSUME SetProtectPages()
			if (!cMem::IsEqual((const void*)m_pFuncOrig, m_Jump, sizeof(m_Jump)))
				return false;	// NOT set !!
			return true;
		}

		bool isChainable() const noexcept;
		FARPROC GetChainFunc() const;

		HRESULT InstallHook(FARPROC pFuncOrig, FARPROC pFuncNew, bool bSkipChainable = false);
		void RemoveHook();
	};

	class GRAYCORE_LINK cHookSwapLock : public cThreadGuardFast
	{
		//! @class GrayLib::cHookSwapLock
		//! Stack based temporary lock for cHookJump. swap original call back so it may be used inside hook.
	public:
		cHookJump& m_rJump;	//!< The code we are locking for use.
		bool m_bSwapOld;	//!< has Old swapped back in. Must be locked. NOT isChainable

	public:
		cHookSwapLock(cHookJump& rJump, bool swap = true)
			: cThreadGuardFast(rJump.m_Lock)	// MUST lock while we do this. single thread.
			, m_rJump(rJump)
		{
			m_bSwapOld = swap ? m_rJump.SwapOld() : false;
		}
		~cHookSwapLock() noexcept
		{
			if (m_bSwapOld)	// did i use the swap?
			{
				m_rJump.SwapReset();
			}
		}
	};

	template <class TYPE = FARPROC>
	class cHookSwapChain : public cHookSwapLock
	{
		//! @class GrayLib::cHookSwapLock
		//! Stack based temporary lock for cHookJump. Will chain if possible (isChainable()) else swap original call back so it may be used inside hook.
	public:
		TYPE m_pFuncChain;	//!< chained version of m_pFuncOrig. or fallback to m_pFuncOrig.
	public:
		cHookSwapChain(cHookJump& rJump)
			: cHookSwapLock(rJump, !rJump.isChainable())
		{
			m_pFuncChain = (TYPE)rJump.GetChainFunc();
		}
	};
}
#endif
#endif // _INC_cHookJump_H
