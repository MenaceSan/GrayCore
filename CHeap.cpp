//
//! @file CHeap.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#include "StdAfx.h"
#include "CHeap.h"
#include "CCodeProfiler.h"
#include "StrT.h"
#include "CLogMgr.h"

#ifndef UNDER_CE
#include <malloc.h>		// malloc_usable_size() or _msize()
#endif
#ifdef __linux__
#include <sys/sysinfo.h>
#endif

namespace Gray
{
	ITERATE_t CHeap::sm_nAllocs = 0;
#ifdef USE_HEAP_STATS
	size_t CHeap::sm_nAllocTotalBytes = 0;
#endif

	UINT64 GRAYCALL CHeap::get_PhysTotal() // static
	{
		//! @return total physical memory for this system.
		//! UINT64 same as size_t for 64bit

#ifdef UNDER_CE
		MEMORYSTATUS ms;
		ms.dwLength = sizeof(ms);
		::GlobalMemoryStatus(&ms);
		return ms.dwTotalPhys;
#elif defined(_WIN32)
		MEMORYSTATUSEX ms;
		ms.dwLength = sizeof(ms);
		if (!::GlobalMemoryStatusEx(&ms))
		{
			ASSERT(0);
			return 0;
		}
		return ms.ullTotalPhys;
#else
	// like GREP MemTotal /proc/meminfo
		struct sysinfo ms;
		int iRet = ::sysinfo(&ms);
		if (iRet != 0)
		{
			ASSERT(0);
			return 0;
		}
		return ms.totalram * (UINT64)ms.mem_unit;
#endif
	}
	UINT64 GRAYCALL CHeap::get_PhysAvail() // static
	{
		//! get total physical memory that might be avail to this process.
		//! UINT64 same as size_t for 64bit
#ifdef UNDER_CE
		MEMORYSTATUS ms;
		ms.dwLength = sizeof(ms);
		::GlobalMemoryStatus(&ms);
		return ms.dwAvailPhys;
#elif defined(_WIN32)
		MEMORYSTATUSEX ms;
		ms.dwLength = sizeof(ms);
		if (!::GlobalMemoryStatusEx(&ms))
		{
			ASSERT(0);
			return 0;
		}
		return ms.ullAvailPhys;
#else
	// like grep MemTotal /proc/meminfo
		struct sysinfo ms;
		int iRet = ::sysinfo(&ms);
		if (iRet != 0)
		{
			ASSERT(0);
			return 0;
		}
		return ms.freeram * (UINT64)ms.mem_unit;
#endif
	}

	bool GRAYCALL CHeap::IsValidInside(const void* pData, ptrdiff_t iOffset)// static
	{
		//! Is this offset inside the valid heap block.
		//! @note this should only ever be used in debug code. and only in an ASSERT.
		CODEPROFILEFUNC();
		if (iOffset < 0)
			return false;
		if (!CHeap::IsValidHeap(pData))
			return false;
		size_t nSize = CHeap::GetSize(pData);
		if ((size_t)iOffset >= nSize)
			return false;
		return true;
	}

	void GRAYCALL CHeap::FreePtr(void* pData) // static
	{
		//! free a pointer to a block allocated on the heap.
		//! Same footprint as C free()
		CODEPROFILEFUNC();
		if (pData == nullptr)
			return;
#if defined(_DEBUG)
		ASSERT(CHeap::IsValidHeap(pData));
#endif
		CHeap::sm_nAllocs--;
#ifdef USE_HEAP_STATS
		size_t nSizeAllocated = CHeap::GetSize(pData);
		CHeap::sm_nAllocTotalBytes -= nSizeAllocated;
#endif
		::free(pData);
	}

	void* GRAYCALL CHeap::AllocPtr(size_t iSize) // static
	{
		//! Allocate a block of memory on the application heap. assume nothing about its current contents. uninitialized.
		//! Same footprint as C malloc()
		//! 0 size is allowed for some reason. (maybe returns NON nullptr)

		CODEPROFILEFUNC();
#ifdef _DEBUG
		DEBUG_ASSERT(iSize < k_ALLOC_MAX, "AllocPtr"); // 256 * 64K - remove/change this if it becomes a problem
#endif
		void* pData = ::malloc(iSize);		// nh_malloc_dbg.
		if (pData == nullptr)
		{
			// I asked for too much!
			DEBUG_ASSERT(0, "malloc");
			return nullptr;	// E_OUTOFMEMORY
		}
#if defined(_DEBUG)
		ASSERT(CHeap::IsValidHeap(pData));
#endif
		CHeap::sm_nAllocs++;
#ifdef USE_HEAP_STATS
		size_t nSizeAllocated = CHeap::GetSize(pData); // the actual size.
		ASSERT(nSizeAllocated >= iSize);
		CHeap::sm_nAllocTotalBytes += nSizeAllocated;	// actual alloc size may be diff from requested size.
#endif
		return pData;
	}

	void* GRAYCALL CHeap::AllocPtr(size_t nSize, const void* pDataInit)
	{
		//! Allocate memory then copy stuff into it.
		void* pData = AllocPtr(nSize);
		if (pData != nullptr && pDataInit != nullptr)
		{
			::memcpy(pData, pDataInit, nSize);
		}
		return pData;
	}

	void* GRAYCALL CHeap::ReAllocPtr(void* pData, size_t iSize) // static
	{
		//! allocate a different sized block but preserve existing content.
		//! Same footprint as C realloc()

		CODEPROFILEFUNC();
		ASSERT(iSize < k_ALLOC_MAX); // 256 * 64K
		if (pData == nullptr)
		{
			if (iSize <= 0)	// just do nothing. this is ok.
				return nullptr;
		}
		else
		{
#ifdef USE_HEAP_STATS
			CHeap::sm_nAllocTotalBytes -= CHeap::GetSize(pData);
#endif
			CHeap::sm_nAllocs--;
		}
		void* pData2 = ::realloc(pData, iSize);		// nh_malloc_dbg.
		if (pData2 == nullptr)
		{
			// I asked for too much!
			DEBUG_ASSERT(iSize == 0, "realloc");	// 0 sized malloc may or may not return nullptr ? not sure why.
			return nullptr;
		}
		CHeap::sm_nAllocs++;
#ifdef USE_HEAP_STATS
		size_t nSizeAllocated = CHeap::GetSize(pData2);
		ASSERT(nSizeAllocated >= 0 && nSizeAllocated >= iSize);
		CHeap::sm_nAllocTotalBytes += nSizeAllocated;	// alloc size may be different than requested size.
#endif
		return pData2;
	}

	void GRAYCALL CHeap::Init(int nFlags) // static
	{
		//! Initialize the heap to debug if desired.
		//! @arg nFlags = _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF
		//!  _CRTDBG_CHECK_ALWAYS_DF = auto call _CrtCheckMemory on every alloc or free.
		//! _crtDbgFlag
#if defined(_MSC_VER) && defined(_DEBUG) && ! defined(UNDER_CE)
		::_CrtSetDbgFlag(nFlags);
#else
		UNREFERENCED_PARAMETER(nFlags);
#endif
	}

	bool GRAYCALL CHeap::Check() // static
	{
		//! Explicitly check the heap for consistency, validity.
		//! Assert if the memory check fails.
		//! called automatically every so often if (_CRTDBG_CHECK_ALWAYS_DF,_CRTDBG_CHECK_EVERY_16_DF,_CRTDBG_CHECK_EVERY_128_DF,etc)
		//! @return false = failure.

#if defined(_MSC_VER) && defined(_DEBUG) && ! defined(UNDER_CE)
		bool bRet = ::_CrtCheckMemory();
		ASSERT(bRet);
#else
		bool bRet = true;
#endif
		return bRet;
	}

	size_t GRAYCALL CHeap::GetSize(const void* pData) // static
	{
		//! get the actual allocated size of a memory block in bytes.
		//! @note __linux__ = Not always the size of the allocation request. maybe greater.
		//! _WIN32 = exact same size as requested malloc(),

		if (pData == nullptr)
			return 0;
#if defined(__linux__)
		return ::malloc_usable_size((void*)pData);
#elif defined(_WIN32)
		return ::_msize((void*)pData);
#else
#error NOOS
#endif
	}

	bool GRAYCALL CHeap::IsValidHeap(const void* pData) // static
	{
		//! is this a valid malloc() heap pointer?
		//! @note this should only ever be used in debug code. and only in an ASSERT.
		if (pData == nullptr)
			return false;
#if defined(_DEBUG)
#if defined(_WIN32) && ! defined(UNDER_CE) && ! defined(__GNUC__)
		return(::_CrtIsValidHeapPointer(pData) ? true : false);
#else
		//! @todo validate the heap block vs static memory for __linux__?
		return CMem::IsValid(pData, 1);
#endif
#else
		return CMem::IsValid(pData, 1);
#endif
	}

	//********************************************

#if defined(_MSC_VER) && (_MSC_VER >= 1300)

	CHeapAlign::CHeader* GRAYCALL CHeapAlign::GetHeader(const void* pData) // static
	{
		//! pData = the pointer returned by CHeapAlign::Alloc
		//! ASSUME IsAlignedAlloc()
		if (pData == nullptr)
			return nullptr;
		UINT_PTR uDataPtr = (UINT_PTR)pData;
		uDataPtr &= ~(sizeof(UINT_PTR) - 1);
		return((CHeader*)(uDataPtr - sizeof(CHeader)));
	}

	bool GRAYCALL CHeapAlign::IsAlignedAlloc(const void* pData, size_t iAligned) // static
	{
		//! Was pData created using CHeapAlign::Alloc() ?
		//! @note _DEBUG heap is VERY different from release heap structure.
		if (pData == nullptr)
			return false;
		UNREFERENCED_PARAMETER(iAligned);
#ifdef _DEBUG
		return CValArray::IsFilledSize<BYTE>(((BYTE*)pData) - k_SizeGap, k_SizeGap, FILL_AlignTail);
#else
		CHeader* pAlign = GetHeader(pData);
		BYTE* pAlloc = (BYTE*)(pAlign->m_pMallocHead);
		if (pAlloc > pData)
			return false;
		if (pAlloc + k_SizeAlignMax < pData)	// no point to align > HEAP_BYTE_SizeAlignMax !?
			return false;
		return true;
#endif
	}

	bool GRAYCALL CHeapAlign::IsValidInside(const void* pData, INT_PTR iOffset)// static
	{
		CODEPROFILEFUNC();
		if (!IsAlignedAlloc(pData, sizeof(void*)))
		{
			return SUPER_t::IsValidInside(pData, iOffset);
		}

		CHeader* pHdr = GetHeader(pData);

#ifdef _DEBUG
		if (!CValArray::IsFilledSize<BYTE>(pHdr->m_TailGap, sizeof(pHdr->m_TailGap), FILL_AlignTail)) //  seem to be aligned block
			return false;
#endif

		void* pMallocHead = pHdr->m_pMallocHead;
		if (!SUPER_t::IsValidHeap(pMallocHead))
			return false;

		size_t nSize = SUPER_t::GetSize(pMallocHead);
		return((size_t)iOffset < nSize);
	}

	bool GRAYCALL CHeapAlign::IsValidHeap(const void* pData)// static
	{
		//! is this a valid malloc() or _aligned_malloc() pointer?
		//! May or may not be aligned.
		CODEPROFILEFUNC();

		// check if this is an aligned alloc first.
		if (!IsAlignedAlloc(pData, sizeof(void*)))
		{
			return SUPER_t::IsValidHeap(pData);
		}

		// get the CHeader pointer.

		CHeader* pHdr = GetHeader(pData);

#ifdef _DEBUG
		if (!CValArray::IsFilledSize<BYTE>(pHdr->m_TailGap, sizeof(pHdr->m_TailGap), FILL_AlignTail)) //  seem to be aligned block
		{
			return false;
		}
#endif

		return(SUPER_t::IsValidHeap(pHdr->m_pMallocHead));
	}

	size_t GRAYCALL CHeapAlign::GetSize(const void* pData)// static
	{
		CODEPROFILEFUNC();
		if (!IsAlignedAlloc(pData, sizeof(void*)))
		{
			return SUPER_t::GetSize(pData);
		}

		// get the CHeader pointer.
		const CHeader* pHdr = GetHeader(pData);
		ASSERT(SUPER_t::IsValidHeap(pHdr->m_pMallocHead));
		return SUPER_t::GetSize(pHdr->m_pMallocHead);
	}

	void GRAYCALL CHeapAlign::FreePtr(void* pData)// static
	{
		//! NOTE: Will work if ! IsAlignedAlloc
		CODEPROFILEFUNC();
		if (pData == nullptr)
			return;

#ifdef USE_HEAP_STATS
		size_t nSizeAllocated = GetSize(pData); // can't call _msize for aligned directly.
		SUPER_t::sm_nAllocTotalBytes -= nSizeAllocated;
#endif
		SUPER_t::sm_nAllocs--;

#if defined(_WIN32) && ! defined(UNDER_CE)
		::_aligned_free(pData);	// CAN'T just use free() ! we need to undo the padding.
#else
		::free(pData); // Linux just used free() for memalign() and malloc().
#endif
	}

	void* GRAYCALL CHeapAlign::AllocPtr(size_t iSize, size_t iAlignment)// static
	{
		//! @note _aligned_malloc is based on malloc(); see malloc for more information on using _aligned_malloc
		//! @arg
		//!  iAlignment = The alignment value, which must be an integer power of 2.
		CODEPROFILEFUNC();
		ASSERT(iAlignment > 0 && iAlignment <= k_SizeAlignMax);

		// 0 size is allowed for some reason. (returns NON nullptr)
#if defined(UNDER_CE)
		void* pData = ::malloc(iSize);
#elif defined(_WIN32)
		void* pData = ::_aligned_malloc(iSize, iAlignment);		// nh_malloc_dbg.
#elif defined(__linux__)
		void* pData = ::memalign(iAlignment, iSize);
#else
#error NOOS
#endif
		if (pData == nullptr)
		{
			// I asked for too much!
			DEBUG_ASSERT(0, "_aligned_malloc");
			return nullptr;
		}

#ifdef USE_HEAP_STATS
		size_t nSizeAllocated = GetSize(pData); // can't call _msize for aligned directly.
		ASSERT(nSizeAllocated >= iSize);
		ASSERT(nSizeAllocated >= iAlignment);
		SUPER_t::sm_nAllocTotalBytes += nSizeAllocated;	// alloc size may be different than requested size.
#endif
		SUPER_t::sm_nAllocs++;

		return pData;
	}

#endif // defined(_MSC_VER) && _MSC_VER >= 1300

}

//***************************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"

UNITTEST_CLASS(CHeap)
{
	UNITTEST_METHOD(CHeap)
	{
		// test physical memory and the heap.
		UNITTEST_TRUE(CHeap::Check());

		UINT64 uPhysTotal = CHeap::get_PhysTotal();
		UNITTEST_TRUE(uPhysTotal);
		UINT64 uPhysAvail = CHeap::get_PhysAvail();
		UNITTEST_TRUE(uPhysAvail);
		sm_pLog->addInfoF("Heap %s free of %s total", LOGSTR(cString::GetSizeK(uPhysAvail)), LOGSTR(cString::GetSizeK(uPhysTotal)));

		// Allocate a bunch of blocks and make sure they stay put til freed
		CHeapBlock test[32];
		for (ITERATE_t i = 0; i < (ITERATE_t)_countof(test) && !k_asTextLines[i].isNull(); i++)
		{
			const GChar_t* pszLine = k_asTextLines[i];
			StrLen_t iLen = StrT::Len(pszLine);
			test[i].Alloc(pszLine, iLen);
			UNITTEST_TRUE(test[i].isValidRead());
		}
		for (ITERATE_t j = 0; j < (ITERATE_t)_countof(test) && !k_asTextLines[j].isNull(); j++)
		{
			UNITTEST_TRUE(test[j].isValidRead());
			const GChar_t* pszLine = k_asTextLines[j];
			StrLen_t iLen = StrT::Len(pszLine);
			UNITTEST_TRUE(!CMem::Compare(pszLine, test[j].get_DataBytes(), iLen));
			test[j].Free();
		}

		UNITTEST_TRUE(CHeap::Check());

		// Test GetSize. NOTE: _MSC_VER always returns the exact size of alloc requested.
		for (size_t nSizeAlloc = 0; nSizeAlloc < 1024; nSizeAlloc++)
		{
			CHeapBlock heapblock(nSizeAlloc);
			size_t nSizeTest = heapblock.get_AllocSize();
			if (nSizeAlloc == 0)
			{
				// Alloc 0 ? may return nullptr or not.
				sm_pLog->addInfoF("Heap alloc(%d) = ptr 0%x, size=%d", (int)nSizeAlloc, (int)(INT_PTR)heapblock.get_DataBytes(), (int)nSizeTest);
			}
			UNITTEST_TRUE(nSizeTest >= nSizeAlloc);
		}

		// NOT Aligned allocate.
		void* pData1N = CHeap::AllocPtr(100);
		CValArray::FillSize<BYTE>(pData1N, 100, 0x11);
		UNITTEST_TRUE(!CHeapAlign::IsAlignedAlloc(pData1N, 16)); // Should NOT report it is aligned.
		CHeap::FreePtr(pData1N);

		// Aligned allocate.
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
		void* pData1A = CHeapAlign::AllocPtr(100, 16);
		CValArray::FillSize<BYTE>(pData1A, 100, 0x22);
		UNITTEST_TRUE(CHeapAlign::IsAlignedAlloc(pData1A, 16)); // Should report it is aligned.
		CHeapAlign::FreePtr(pData1A);
#endif

		UNITTEST_TRUE(CHeap::Check());
	}
};
UNITTEST_REGISTER(CHeap, UNITTEST_LEVEL_Core);
#endif
