//
//! @file CMemPage.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CMemPage_H
#define _INC_CMemPage_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CSingleton.h"
#include "CArraySort.h"
#include "CSystemInfo.h"

namespace Gray
{
	class CMemPage : public CSmartBase
	{
		//! @class GrayLib::CMemPage
		//! Track a protected memory page.
		//! _WIN32 ONLY allocates whole pages at a time, not just specified range of bytes. Pool these locked blocks.

		friend class CMemPageMgr;

	public:
		CMemPage(UINT_PTR nPageStart, size_t nPageSize)
			: m_nPageStart(nPageStart)
			, m_nPageSize(nPageSize)
			, m_dwOldProtectionFlags(0)
			, m_nRefCount2(1)
		{
			ASSERT(get_SortValue() != 0);
			ASSERT((get_SortValue() % m_nPageSize) == 0);
		}
		virtual ~CMemPage() noexcept
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
	typedef CSmartPtr<CMemPage> CMemPagePtr;

	class CMemPageMgr : public CSingleton<CMemPageMgr>
	{
		//! @class GrayLib::CMemPageMgr
		//! Track my protected memory pages.
		//! _WIN32 ONLY allocates whole pages at a time, not just specified range of bytes. Pool these locked blocks.

	public:
		CMemPageMgr()
			: CSingleton<CMemPageMgr>(this, typeid(CMemPageMgr))
			, m_dwPageSize(0)
		{
		}
		virtual ~CMemPageMgr()
		{
			// Make sure this stuff doesnt get destroyed too early.
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
				CMemPagePtr pPage = m_aPages.FindArgForKey(nPageStart);
				if (bProtect)
				{
					if (pPage == nullptr)
					{
						// odd
						// DEBUG_ERR(("ProtectPages bProtect = nullptr"));
						continue;
					}
					if (--pPage->m_nRefCount2)
						continue;
					m_aPages.RemoveArg(pPage);
					if (!pPage->SetProtect(true))
					{
						// DEBUG_ERR(("ProtectPages SetProtect true"));
						return E_FAIL;
					}
				}
				else
				{
					if (pPage == nullptr)
					{
						pPage = new CMemPage(nPageStart, m_dwPageSize);
						ASSERT(pPage);
						if (!pPage->SetProtect(false))
						{
							// DEBUG_ERR(("ProtectPages SetProtect false"));
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
		CArraySortValue<CMemPage, UINT_PTR> m_aPages;
	};
}

#endif
