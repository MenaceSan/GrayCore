//
//! @file CWinHeap.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CWinHeap_H
#define _INC_CWinHeap_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "COSHandle.h"
#include "CUnitTestDecl.h"

#ifdef _WIN32
UNITTEST_PREDEF(CWinHeap)

namespace Gray
{
	class GRAYCORE_LINK CWinHeap
	{
		//! @class GrayLib::CWinHeap
		//! Create a private heap for the app. or access the process default heap.

	private:
		HANDLE m_hHeap;		//!< Handle to the _WIN32 heap. GetProcessHeap()
		bool m_bManaged;	//!< I manage its lifetime? use HeapDestroy() ?

	public:
		CWinHeap()
		: m_hHeap(::GetProcessHeap())
		, m_bManaged(false)		// This belongs to the process. leave it.
		{
			//! Use the current/default process heap.
		}
		CWinHeap(HANDLE hHeap, bool bManaged = true)
		: m_hHeap(hHeap)
		, m_bManaged(bManaged)
		{
			//! Attach existing handle to this class.
		}
		CWinHeap(DWORD flOptions, SIZE_T dwInitialSize = 0, SIZE_T dwMaximumSize = 0)
		: m_hHeap(::HeapCreate(flOptions, dwInitialSize, dwMaximumSize))
		, m_bManaged(true)
		{
			//! flOptions = HEAP_GENERATE_EXCEPTIONS
		}
		~CWinHeap()
		{
			if (m_bManaged)
			{
				::HeapDestroy(m_hHeap);
			}
		}

		HANDLE get_Handle() const noexcept
		{
			return m_hHeap;
		}
		bool isValidHeap() const noexcept
		{
			//! did HeapCreate() work?
			return m_hHeap != HANDLE_NULL;
		}
		SIZE_T Compact(DWORD dwFlags = 0) const
		{
			//! dwFlags = HEAP_NO_SERIALIZE
			//! @return Size of the largest block.
			return ::HeapCompact(m_hHeap, dwFlags);
		}

#ifndef UNDER_CE
		bool Lock()
		{
			//! Thread lock this. SERIALIZE
			return ::HeapLock(m_hHeap);
		}
		bool Unlock()
		{
			//! Thread release this. SERIALIZE
			return ::HeapUnlock(m_hHeap);
		}
		bool QueryInformation(HEAP_INFORMATION_CLASS HeapInformationClass, void* pData, SIZE_T* pnSizeData)
		{
			// HeapInformationClass = HeapEnableTerminationOnCorruption, HeapCompatibilityInformation
			return ::HeapQueryInformation(m_hHeap, HeapInformationClass, pData, *pnSizeData, pnSizeData);
		}
		bool SetInformation(HEAP_INFORMATION_CLASS HeapInformationClass, const void* pData, SIZE_T nSizeData)
		{
			// HeapInformationClass = HeapEnableTerminationOnCorruption, HeapCompatibilityInformation
			return ::HeapSetInformation(m_hHeap, HeapInformationClass, const_cast<void*>(pData), nSizeData);
		}
#endif

#if 0
		// HeapWalk
#endif

		bool IsValidHeapPtr(void* pMem = nullptr, DWORD dwFlags = 0) const
		{
			//! Is this a valid heap pointer?
			//! dwFlags = HEAP_NO_SERIALIZE
			return ::HeapValidate(m_hHeap, dwFlags, pMem);
		}

		void* AllocPtr(SIZE_T dwBytes, DWORD dwFlags = 0)
		{
			//! dwFlags= HEAP_ZERO_MEMORY, HEAP_GENERATE_EXCEPTIONS, HEAP_NO_SERIALIZE
			return ::HeapAlloc(m_hHeap, dwFlags, dwBytes);
		}
		void* ReAllocPtr(void* pMem, SIZE_T dwBytes, DWORD dwFlags = 0)
		{
			//! dwFlags= HEAP_ZERO_MEMORY, HEAP_GENERATE_EXCEPTIONS, HEAP_NO_SERIALIZE
			return ::HeapReAlloc(m_hHeap, dwFlags, pMem, dwBytes);
		}
		bool FreePtr(void* pMem, DWORD dwFlags = 0)
		{
			//! dwFlags=HEAP_NO_SERIALIZE
			return ::HeapFree(m_hHeap, dwFlags, pMem);
		}
		SIZE_T GetAllocPtrSize(const void* pMem, DWORD dwFlags = 0)
		{
			//! dwFlags=HEAP_NO_SERIALIZE
			//! @return (SIZE_T) -1 = FAILURE.
			return ::HeapSize(m_hHeap, dwFlags, pMem);
		}

		UNITTEST_FRIEND(CWinHeap);
	};

 
};

#endif
#endif // _INC_CWinHeap_H
