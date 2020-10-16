//
//! @file CHookJump.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CHookJump.h"
#include "CLogMgr.h"

#ifdef _WIN32
#include "CArraySort.h"
#include "CSystemInfo.h"
#endif

namespace Gray
{

#ifdef _WIN32
	class CHookPage : public CSmartBase
	{
		//! @class GrayLib::CHookPage
		//! Track a protected memory page.
		//! _WIN32 ONLY allocates whole pages at a time, not just specified range of bytes. Pool these locked blocks.

		friend class CHookPageMgr;

	public:
		CHookPage(UINT_PTR nPageStart, size_t nPageSize)
			: m_nPageStart(nPageStart)
			, m_nPageSize(nPageSize)
			, m_dwOldProtectionFlags(0)
			, m_nRefCount2(1)
		{
			ASSERT(get_SortValue() != 0);
			ASSERT((get_SortValue() % m_nPageSize) == 0);
		}
		virtual ~CHookPage() noexcept
		{
		}

		UINT_PTR get_SortValue() const noexcept
		{
			return m_nPageStart;
		}
		bool IsOverlapped(UINT_PTR p, size_t n) const noexcept
		{
			if ((p + n) <= m_nPageStart)
				return false;
			if ((m_nPageStart + m_nPageSize) <= p)
				return false;
			return true;
		}
		bool SetProtect(bool bProtect) noexcept
		{
			//! @arg bProtect = false = allow PAGE_EXECUTE_READWRITE
#ifdef __linux__
			::mprotect((void*)m_nPageStart, m_nPageSize - 1, PROT_READ | PROT_WRITE | PROT_EXEC);
#else
			DWORD dwNewProtectionFlags = bProtect ? m_dwOldProtectionFlags : PAGE_EXECUTE_READWRITE;
			return ::VirtualProtect((void*)m_nPageStart, m_nPageSize - 1, dwNewProtectionFlags, &m_dwOldProtectionFlags);
#endif
		}

	public:
		UINT_PTR m_nPageStart;	//!< Always aligned to dwPageSize.
		size_t m_nPageSize;			//!< SystemInfo::dwPageSize
		DWORD m_dwOldProtectionFlags;	//!< original flags used/returned by _WIN32 VirtualProtect()
		int m_nRefCount2;				//!< ProtectPages count.
	};
	typedef CSmartPtr<CHookPage> CHookPagePtr;

	class CHookPageMgr : public CSingleton<CHookPageMgr>
	{
		//! @class GrayLib::CHookPageMgr
		//! Track my protected memory pages.
		//! _WIN32 ONLY allocates whole pages at a time, not just specified range of bytes. Pool these locked blocks.
		CHEAPOBJECT_IMPL;

	public:
		CHookPageMgr()
			: CSingleton<CHookPageMgr>(this, typeid(CHookPageMgr))
			, m_dwPageSize(0)
		{
		}

		HRESULT ProtectPages(const void* p, size_t nSize, bool bProtect)
		{
			//! Protect or un-protect these pages.
			if (m_dwPageSize == 0)
			{
				m_dwPageSize = CSystemInfo::I().m_SystemInfo.dwPageSize;
				ASSERT(m_dwPageSize);
			}

			const UINT_PTR nStart = ((UINT_PTR)p);
			const UINT_PTR nEnd = nStart + nSize;
			const UINT_PTR nPageOver = nStart % m_dwPageSize;
			UINT_PTR nPageStart = nStart - nPageOver;
			for (; nPageStart < nEnd; nPageStart += m_dwPageSize)
			{
				CHookPagePtr pPage = m_aPages.FindArgForKey(nPageStart);
				if (bProtect)
				{
					if (pPage == nullptr)
					{
						// odd
						DEBUG_ERR(("ProtectPages bProtect = nullptr"));
						continue;
					}
					if (--pPage->m_nRefCount2)
						continue;
					m_aPages.RemoveArg(pPage);
					if (!pPage->SetProtect(true))
					{
						DEBUG_ERR(("ProtectPages SetProtect true"));
						return E_FAIL;
					}
				}
				else
				{
					if (pPage == nullptr)
					{
						pPage = new CHookPage(nPageStart, m_dwPageSize);
						ASSERT(pPage);
						if (!pPage->SetProtect(false))
						{
							DEBUG_ERR(("ProtectPages SetProtect false"));
							return E_FAIL;
						}
						m_aPages.Add(pPage);
					}
					else
					{
						pPage->m_nRefCount2++;
					}
				}
			}
			return S_OK;
		}

		DWORD m_dwPageSize;
		CArraySortValue<CHookPage, UINT_PTR> m_aPages;
	};
#endif

	//*****************************************************

	FARPROC CHookJump::GetChainFunc() const
	{
		if (isChainable())
		{
			// Get a callable function from this.
			int lRelAddr;
			STATIC_ASSERT(sizeof(lRelAddr) == k_LEN_P, lRelAddr);
 			::memcpy(&lRelAddr, m_OldCode + k_LEN_J, k_LEN_P);
			return (FARPROC)(((UINT_PTR)m_pFuncOrig) + lRelAddr + sizeof(m_OldCode));
		}
		return m_pFuncOrig;
	}

	bool CHookJump::InstallHook(FARPROC pFuncOrig, FARPROC pFuncNew)
	{
		//! @note X86 ONLY!! 32 or 64 bit.

		CThreadGuardFast guard(m_Lock);
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
		const HRESULT hRes = CHookPageMgr::I().ProtectPages((void*)pFuncOrig, k_LEN_A, false);
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

		::memcpy((void*)pFuncOrig, m_Jump, sizeof(m_Jump));	// inject jump. we are armed!

		DEBUG_MSG(("InstallHook: JMP-hook planted."));
		return true;
	}

	void CHookJump::RemoveHook()
	{
		CThreadGuardFast guard(m_Lock);
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
			CHookPageMgr::I().ProtectPages((void*)m_pFuncOrig, k_LEN_A, true);
	#endif
		}
			GRAY_TRY_CATCHALL
		{
			// UNREFERENCED_PARAMETER(ex);
			DEBUG_ERR(("CHookJump::RemoveHook FAIL"));
		}
			GRAY_TRY_END
	}
}

//*********************************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"

// Make sure this code is not optimized out !
#ifdef _MSC_VER
#pragma optimize( "", off )
#endif
INT_PTR GRAYCALL CUnitTest_HookJump1() // never inline optimize this
{
	CUnitTests::sm_pLog->addDebugInfoF("CUnitTest_HookJump1");
	return 1;
}
INT_PTR GRAYCALL CUnitTest_HookJump2() // never inline optimize this
{
	// replace CUnitTest_HookJump1 with this.
	CUnitTests::sm_pLog->addDebugInfoF("CUnitTest_HookJump2");
	return 2;
}
#ifdef _MSC_VER
#pragma optimize( "", on )	// restore old params.
#endif

UNITTEST_CLASS(CHookJump)
{
	UNITTEST_METHOD(CHookJump)
	{
		//! hook a API function for one call.

		UNITTEST_TRUE(CUnitTests::sm_pLog != nullptr);

		INT_PTR iRet = CUnitTest_HookJump1();
		UNITTEST_TRUE(iRet == 1);

		CHookJump tester;
		tester.InstallHook((FARPROC)CUnitTest_HookJump1, (FARPROC)CUnitTest_HookJump2);
		UNITTEST_TRUE(tester.isHookInstalled());

		iRet = CUnitTest_HookJump1();	// really calls CUnitTest_HookJump2
		UNITTEST_TRUE(iRet == 2);

		iRet = CUnitTest_HookJump2();
		UNITTEST_TRUE(iRet == 2);

		{
			CHookSwapLock lock(tester);
			iRet = CUnitTest_HookJump1();	// CUnitTest_HookJump1 was restored globally.
			UNITTEST_TRUE(iRet == 1);
		}

		iRet = CUnitTest_HookJump1();
		UNITTEST_TRUE(iRet == 2);

		{
			CHookSwapChain lock(tester);
			iRet = CUnitTest_HookJump1();	// CUnitTest_HookJump1 still as CUnitTest_HookJump2.
			UNITTEST_TRUE(iRet == 2);
			iRet = lock.m_pFuncChain();	// CUnitTest_HookJump1 was restored just for Chain.
			UNITTEST_TRUE(iRet == 1);
		}

		iRet = CUnitTest_HookJump1();	// CUnitTest_HookJump1 as CUnitTest_HookJump2.
		UNITTEST_TRUE(iRet == 2);

		tester.RemoveHook();
		UNITTEST_TRUE(!tester.isHookInstalled());

		iRet = CUnitTest_HookJump1();
		UNITTEST_TRUE(iRet == 1);	// was restored.
		iRet = CUnitTest_HookJump2();	
		UNITTEST_TRUE(iRet == 2);		// still works as expected.
	}
};
UNITTEST_REGISTER(CHookJump, UNITTEST_LEVEL_Lib);
#endif