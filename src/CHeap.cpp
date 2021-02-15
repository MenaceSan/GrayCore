//
//! @file cHeap.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cHeap.h"
#include "cCodeProfiler.h"
#include "StrT.h"
#include "cLogMgr.h"

#if ! defined(UNDER_CE) && USE_CRT
#include <malloc.h>		// malloc_usable_size() or _msize()
#endif
#ifdef __linux__
#include <sys/sysinfo.h>
#endif

namespace Gray
{
	ITERATE_t cHeap::sm_nAllocs = 0;
#ifdef USE_HEAP_STATS
	size_t cHeap::sm_nAllocTotalBytes = 0;
#endif

	UINT64 GRAYCALL cHeap::get_PhysTotal() // static
	{
		//! @return total physical memory for this system.
		//! UINT64 same as size_t for 64bit

#ifdef UNDER_CE
		::MEMORYSTATUS ms;
		ms.dwLength = sizeof(ms);
		::GlobalMemoryStatus(&ms);
		return ms.dwTotalPhys;
#elif defined(_WIN32)
		::MEMORYSTATUSEX ms;
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
	UINT64 GRAYCALL cHeap::get_PhysAvail() // static
	{
		//! get total physical memory that might be avail to this process.
		//! UINT64 same as size_t for 64bit
#ifdef UNDER_CE
		MEMORYSTATUS ms;
		ms.dwLength = sizeof(ms);
		::GlobalMemoryStatus(&ms);
		return ms.dwAvailPhys;
#elif defined(_WIN32)
		::MEMORYSTATUSEX ms;
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

	bool GRAYCALL cHeap::IsValidInside(const void* pData, ptrdiff_t iOffset) noexcept // static
	{
		//! Is this offset inside the valid heap block.
		//! @note this should only ever be used in debug code. and only in an ASSERT.
		CODEPROFILEFUNC();
		if (iOffset < 0)
			return false;
		if (!cHeap::IsValidHeap(pData))
			return false;
		size_t nSize = cHeap::GetSize(pData);
		if ((size_t)iOffset >= nSize)
			return false;
		return true;
	}

	void GRAYCALL cHeap::FreePtr(void* pData) noexcept // static
	{
		//! free a pointer to a block allocated on the heap.
		//! Same footprint as C free()
		CODEPROFILEFUNC();
		if (pData == nullptr)
			return;
#if defined(_DEBUG)
		DEBUG_CHECK(cHeap::IsValidHeap(pData));
#endif
		cHeap::sm_nAllocs--;
#ifdef USE_HEAP_STATS
		size_t nSizeAllocated = cHeap::GetSize(pData);
		cHeap::sm_nAllocTotalBytes -= nSizeAllocated;
#endif
#if defined(_WIN32) && ! USE_CRT
		::LocalFree(pData);
#else
		::free(pData);
#endif
	}

	void* GRAYCALL cHeap::AllocPtr(size_t iSize) // static
	{
		//! Allocate a block of memory on the application heap. assume nothing about its current contents. uninitialized.
		//! Same footprint as C malloc()
		//! 0 size is allowed for some reason. (maybe returns NON nullptr)

		CODEPROFILEFUNC();
#ifdef _DEBUG
		DEBUG_ASSERT(iSize < k_ALLOC_MAX, "AllocPtr"); // 256 * 64K - remove/change this if it becomes a problem
#endif
#if defined(_WIN32) && ! USE_CRT
		void* pData = ::LocalAlloc(LPTR, iSize);		// nh_malloc_dbg.
#else
		void* pData = ::malloc(iSize);		// nh_malloc_dbg.
#endif
		if (pData == nullptr)
		{
			// I asked for too much!
			DEBUG_ASSERT(0, "malloc");
			return nullptr;	// E_OUTOFMEMORY
		}
#if defined(_DEBUG)
		ASSERT(cHeap::IsValidHeap(pData));
#endif
		cHeap::sm_nAllocs++;
#ifdef USE_HEAP_STATS
		size_t nSizeAllocated = cHeap::GetSize(pData); // the actual size.
		ASSERT(nSizeAllocated >= iSize);
		cHeap::sm_nAllocTotalBytes += nSizeAllocated;	// actual alloc size may be diff from requested size.
#endif
		return pData;
	}

	void* GRAYCALL cHeap::ReAllocPtr(void* pData, size_t iSize) // static
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
			cHeap::sm_nAllocTotalBytes -= cHeap::GetSize(pData);
#endif
			cHeap::sm_nAllocs--;
		}
#if defined(_WIN32) && ! USE_CRT
		void* pData2 = ::LocalReAlloc(pData, iSize, LPTR);
#else
		void* pData2 = ::realloc(pData, iSize);		// nh_malloc_dbg.
#endif
		if (pData2 == nullptr)
		{
			// I asked for too much!
			DEBUG_ASSERT(iSize == 0, "realloc");	// 0 sized malloc may or may not return nullptr ? not sure why.
			return nullptr;
		}
		cHeap::sm_nAllocs++;
#ifdef USE_HEAP_STATS
		size_t nSizeAllocated = cHeap::GetSize(pData2);
		ASSERT(nSizeAllocated >= 0 && nSizeAllocated >= iSize);
		cHeap::sm_nAllocTotalBytes += nSizeAllocated;	// alloc size may be different than requested size.
#endif
		return pData2;
	}

	void GRAYCALL cHeap::Init(int nFlags) // static
	{
		//! Initialize the heap to debug if desired.
		//! @arg nFlags = _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF
		//!  _CRTDBG_CHECK_ALWAYS_DF = auto call _CrtCheckMemory on every alloc or free.
		//! _crtDbgFlag
#if defined(_MSC_VER) && defined(_DEBUG) && ! defined(UNDER_CE) && USE_CRT
		::_CrtSetDbgFlag(nFlags);
#else
		UNREFERENCED_PARAMETER(nFlags);
#endif
	}

	bool GRAYCALL cHeap::Check() // static
	{
		//! Explicitly check the heap for consistency, validity.
		//! Assert if the memory check fails.
		//! called automatically every so often if (_CRTDBG_CHECK_ALWAYS_DF,_CRTDBG_CHECK_EVERY_16_DF,_CRTDBG_CHECK_EVERY_128_DF,etc)
		//! @return false = failure.

#if defined(_MSC_VER) && defined(_DEBUG) && ! defined(UNDER_CE) && USE_CRT
		bool bRet = ::_CrtCheckMemory();
		ASSERT(bRet);
#else
		bool bRet = true;
#endif
		return bRet;
	}

	size_t GRAYCALL cHeap::GetSize(const void* pData) noexcept // static
	{
		//! get the actual allocated size of a memory block in bytes.
		//! @note __linux__ = Not always the size of the allocation request. maybe greater.
		//! _WIN32 = exact same size as requested malloc(),

		if (pData == nullptr)
			return 0;
#if defined(__linux__)
		return ::malloc_usable_size((void*)pData);
#elif defined(_WIN32) && ! USE_CRT
		return ::LocalSize((void*)pData);
#elif defined(_WIN32)
		return ::_msize((void*)pData);
#else
#error NOOS
#endif
	}

	bool GRAYCALL cHeap::IsValidHeap(const void* pData) noexcept // static
	{
		//! is this a valid malloc() heap pointer?
		//! @note this should only ever be used in debug code. and only in an ASSERT.
		if (!cMem::IsValidApp(pData))
			return false;
#if defined(_DEBUG)
#if defined(_WIN32) && ! defined(UNDER_CE) && ! defined(__GNUC__) && USE_CRT
		return ::_CrtIsValidHeapPointer(pData) ? true : false;
#else
		//! @todo validate the heap block vs static memory for __linux__?
		return !cMem::IsCorrupt(pData, 1);
#endif
#else
		return !cMem::IsCorrupt(pData, 1);
#endif
	}

	//********************************************

#if defined(_MSC_VER) && (_MSC_VER >= 1300)

	const cHeapAlign::cHeapHeader* GRAYCALL cHeapAlign::GetHeader(const void* pData) noexcept// static
	{
		//! Get the header prefix for the align memory block.
		//! pData = the pointer returned by cHeapAlign::Alloc
		//! ASSUME IsAlignedAlloc()
		if (pData == nullptr)
			return nullptr;
		UINT_PTR uDataPtr = (UINT_PTR)pData;
		uDataPtr &= ~(sizeof(UINT_PTR) - 1);
		return((const cHeapHeader*)(uDataPtr - sizeof(cHeapHeader)));
	}

	bool GRAYCALL cHeapAlign::IsAlignedAlloc(const void* pData, size_t iAligned) noexcept // static
	{
		//! Was pData created using cHeapAlign::Alloc() ?
		//! @note _DEBUG heap is VERY different from release heap structure.
		if (pData == nullptr)
			return false;
		UNREFERENCED_PARAMETER(iAligned);
#ifdef _DEBUG
		return cValArray::IsFilledSize<BYTE>(((BYTE*)pData) - k_SizeGap, k_SizeGap, FILL_AlignTail);
#else
		const cHeapHeader* pAlign = GetHeader(pData);
		BYTE* pAlloc = (BYTE*)(pAlign->m_pMallocHead);
		if (pAlloc > pData)
			return false;
		if (pAlloc + k_SizeAlignMax < pData)	// no point to align > HEAP_BYTE_SizeAlignMax !?
			return false;
		return true;
#endif
	}

	bool GRAYCALL cHeapAlign::IsValidInside(const void* pData, INT_PTR iOffset) noexcept // static
	{
		CODEPROFILEFUNC();
		if (!IsAlignedAlloc(pData, sizeof(void*)))
		{
			return SUPER_t::IsValidInside(pData, iOffset);
		}

		const cHeapHeader* pHdr = GetHeader(pData);

#ifdef _DEBUG
		if (!cValArray::IsFilledSize<BYTE>(pHdr->m_TailGap, sizeof(pHdr->m_TailGap), FILL_AlignTail)) //  seem to be aligned block
			return false;
#endif

		void* pMallocHead = pHdr->m_pMallocHead;
		if (!SUPER_t::IsValidHeap(pMallocHead))
			return false;

		size_t nSize = SUPER_t::GetSize(pMallocHead);
		return((size_t)iOffset < nSize);
	}

	bool GRAYCALL cHeapAlign::IsValidHeap(const void* pData) noexcept // static
	{
		//! is this a valid malloc() or _aligned_malloc() pointer?
		//! May or may not be aligned.
		CODEPROFILEFUNC();

		// check if this is an aligned alloc first.
		if (!IsAlignedAlloc(pData, sizeof(void*)))
		{
			return SUPER_t::IsValidHeap(pData);
		}

		// get the cHeapHeader pointer.

		const cHeapHeader* pHdr = GetHeader(pData);

#ifdef _DEBUG
		if (!cValArray::IsFilledSize<BYTE>(pHdr->m_TailGap, sizeof(pHdr->m_TailGap), FILL_AlignTail)) //  seem to be aligned block
		{
			return false;
		}
#endif

		return SUPER_t::IsValidHeap(pHdr->m_pMallocHead);
	}

	size_t GRAYCALL cHeapAlign::GetSize(const void* pData) noexcept // static
	{
		CODEPROFILEFUNC();
		if (!IsAlignedAlloc(pData, sizeof(void*)))
		{
			return SUPER_t::GetSize(pData);
		}

		// get the cHeapHeader pointer.
		const cHeapHeader* pHdr = GetHeader(pData);
		DEBUG_CHECK(SUPER_t::IsValidHeap(pHdr->m_pMallocHead));
		return SUPER_t::GetSize(pHdr->m_pMallocHead);
	}

	void GRAYCALL cHeapAlign::FreePtr(void* pData)// static
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

#if defined(_WIN32) && ! defined(UNDER_CE) && USE_CRT
		::_aligned_free(pData);	// CAN'T just use free() ! we need to undo the padding.
#elif defined(_WIN32) && ! USE_CRT
		::LocalFree(pData);
#else
		::free(pData); // Linux just used free() for memalign() and malloc().
#endif
	}

	void* GRAYCALL cHeapAlign::AllocPtr(size_t iSize, size_t iAlignment)// static
	{
		//! @note _aligned_malloc is based on malloc(); see malloc for more information on using _aligned_malloc
		//! @arg
		//!  iAlignment = The alignment value, which must be an integer power of 2.
		CODEPROFILEFUNC();
		ASSERT(iAlignment > 0 && iAlignment <= k_SizeAlignMax);

		// 0 size is allowed for some reason. (returns NON nullptr)
#if defined(UNDER_CE)
		void* pData = ::malloc(iSize);	// No special call for this in CE.
#elif defined(__linux__)
		void* pData = ::memalign(iAlignment, iSize);
#elif defined(_WIN32) && ! USE_CRT
		void* pData = ::LocalAlloc(LPTR, iSize);
#elif defined(_WIN32) && USE_CRT
		void* pData = ::_aligned_malloc(iSize, iAlignment);		// nh_malloc_dbg.
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
