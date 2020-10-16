//
//! @file CHookJump.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CHookJump_H
#define _INC_CHookJump_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CThreadLock.h"
#include "CUnitTestDecl.h"
UNITTEST_PREDEF(CHookJump)

namespace Gray
{
	class GRAYCORE_LINK CHookJump
	{
		//! @class GrayLib::CHookJump
		//! Create/Define a relative jump to hook/replace an existing API call. Jump to new code is injected into old function start.
		//! @note NOT for use in hooking an interface or a vtable. those don't require a jump instruction.
		//! @note This ASSUMEs Intel x86 type CPU. 32 or 64 bit instructions

	public:
		static const BYTE k_I_NULL = 0x00;
#if USE_INTEL
		static const BYTE k_I_JUMP = 0xe9;		// X86
#endif
		static const int k_LEN_J = 1;	//!< size of the jump instruction. 0xe9 = k_JUMP_I

		static const int k_LEN_P = 4;	//!< size of the relative pointer. NOT same as sizeof(FARPROC) or INT_PTR
#ifdef USE_64BIT
		static const int k_LEN_A = 16;	//!< k_LEN_D aligned to 16 (for 64 bit code)
#else
		static const int k_LEN_A = 8;	//!< k_LEN_D aligned to 8 (for 32 bit code)
#endif
		static const int k_LEN_D = k_LEN_J + k_LEN_P;	//!< ( k_LEN_J+k_LEN_P ) = size of actual instruction

		friend class CHookSwapLock;
		friend class CHookSwapChain;

	protected:
		FARPROC m_pFuncOrig;			//!< Pointer to the original/old function. The one i will replace. Inject code here.
		BYTE m_OldCode[k_LEN_D];		//!< what was at m_pFuncOrig previously. NOTE: What if this uses k_LEN_J the m_Lock isn't needed ?!!!
		BYTE m_Jump[k_LEN_D];			//!< what do i want to replace m_pFuncOrig with. jump to pFuncNew
		CThreadLockFast m_Lock;			//!< prevent multiple threads from using this at the same time.

	protected:
		bool SwapOld() noexcept
		{
			//! put back saved code fragment. temporary to call previous version of the function.
			//! ASSUME use of CHookSwapLock m_Lock
			if (!isHookValid())
				return false;
			::memcpy((void*)m_pFuncOrig, m_OldCode, sizeof(m_OldCode));
			return true;
		}
		void SwapReset() noexcept
		{
			//! put back original JMP instruction again
			//! ASSUME use of CHookSwapLock m_Lock
			if (!isHookInstalled() || m_pFuncOrig == nullptr)	// hook has since been destroyed!
				return;
			::memcpy((void*)m_pFuncOrig, m_Jump, sizeof(m_Jump));
		}

	public:
		CHookJump() noexcept
			: m_pFuncOrig(nullptr)
		{
			m_OldCode[0] = k_I_NULL;
			m_Jump[0] = k_I_NULL;
		}
		~CHookJump()
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
			if (::memcmp((const void*)m_pFuncOrig, m_Jump, sizeof(m_Jump)))
				return false;	// NOT set !!
			return true;
		}

		bool isChainable() const noexcept
		{
			// The jump i inserted is just on top of another jump? 
			// I don't need to lock and swap to call the old code. I can just chain to it
			return m_OldCode[0] == k_I_JUMP;	// the old code was just a jump as well.
		}

		FARPROC GetChainFunc() const;

		bool InstallHook(FARPROC pFuncOrig, FARPROC pFuncNew);
		void RemoveHook();

		UNITTEST_FRIEND(CHookJump);
	};

	class GRAYCORE_LINK CHookSwapLock : public CThreadGuardFast
	{
		//! @class GrayLib::CHookSwapLock
		//! Stack based temporary lock for CHookJump. swap original call back so it may be used inside hook.
	public:
		CHookJump& m_rJump;	//!< The code we are locking for use.
		bool m_bSwapOld;	// has Old. Must be locked.

	public:
		CHookSwapLock(CHookJump& rJump, bool swap = true)
			: CThreadGuardFast(rJump.m_Lock)	// MUST lock while we do this. single thread.
			, m_rJump(rJump)
		{
			if (swap)
			{
				m_bSwapOld = m_rJump.SwapOld();
			}
		}
		~CHookSwapLock() noexcept
		{
			if (m_bSwapOld)
			{
				m_rJump.SwapReset();
			}
		}
	};

	class GRAYCORE_LINK CHookSwapChain : public CHookSwapLock
	{
	public:
		FARPROC m_pFuncChain;	// chained version of m_pFuncOrig. or fallback to m_pFuncOrig.
	public:
		CHookSwapChain(CHookJump& rJump)
			: CHookSwapLock(rJump, !rJump.isChainable())
		{
			m_pFuncChain = rJump.GetChainFunc();
		}
	};
}

#endif // _INC_CHookJump_H
