//
//! @file cHeap.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrT.h"
#include "cCodeProfiler.h"
#include "cHeap.h"
#include "cLogMgr.h"

#if !defined(UNDER_CE) && USE_CRT
#include <malloc.h>  // malloc_usable_size() or _msize()
#endif
#ifdef __linux__
#include <sys/sysinfo.h>
#endif

namespace Gray {
HANDLE cHeapCommon::g_hHeap = ::GetProcessHeap();  // Global singleton.
ITERATE_t cHeapCommon::sm_nAllocs = 0;
#ifdef USE_HEAP_STATS
size_t cHeapCommon::sm_nAllocTotalBytes = 0;
#endif

size_t GRAYCALL cHeapCommon::GetAlign(const void* pData) noexcept  // static
{
    // ASSUME >= k_SizeAlignDef. 1,2,4,8,16,32
    auto bits = cBits::Lowest1Bit(PtrCastToNum(pData));
    if (bits == 0) return 0;
    return cBits::Mask1<size_t>(bits - 1);
}

UINT64 GRAYCALL cHeapCommon::get_PhysTotal()  // static
{
#ifdef UNDER_CE
    ::MEMORYSTATUS ms;
    ms.dwLength = sizeof(ms);
    ::GlobalMemoryStatus(&ms);
    return ms.dwTotalPhys;
#elif defined(_WIN32)
    ::MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!::GlobalMemoryStatusEx(&ms)) {
        ASSERT(0);
        return 0;
    }
    return ms.ullTotalPhys;  // DWORDLONG
#else
    // like GREP MemTotal /proc/meminfo
    struct ::sysinfo ms;
    const int iRet = ::sysinfo(&ms);
    if (iRet != 0) {
        ASSERT(0);
        return 0;
    }
    return ms.totalram * (UINT64)ms.mem_unit;
#endif
}

UINT64 GRAYCALL cHeapCommon::get_PhysAvail() {  // static
#ifdef UNDER_CE
    MEMORYSTATUS ms;
    ms.dwLength = sizeof(ms);
    ::GlobalMemoryStatus(&ms);
    return ms.dwAvailPhys;
#elif defined(_WIN32)
    ::MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!::GlobalMemoryStatusEx(&ms)) {
        ASSERT(0);
        return 0;
    }
    return ms.ullAvailPhys;
#else
    // like grep MemTotal /proc/meminfo
    struct sysinfo ms;
    int iRet = ::sysinfo(&ms);
    if (iRet != 0) {
        ASSERT(0);
        return 0;
    }
    return ms.freeram * (UINT64)ms.mem_unit;
#endif
}

void GRAYCALL cHeapCommon::Init(int nFlags) {  // static
    //! Initialize the heap to debug if desired.
    //! @arg nFlags = _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF
    //!  _CRTDBG_CHECK_ALWAYS_DF = auto call _CrtCheckMemory on every alloc or free.
    //! _crtDbgFlag
#if defined(_MSC_VER) && defined(_DEBUG) && !defined(UNDER_CE) && USE_CRT
    ::_CrtSetDbgFlag(nFlags);
#else
    UNREFERENCED_PARAMETER(nFlags);
#endif
}

bool GRAYCALL cHeapCommon::Check() {  // static
    //! Explicitly check the heap for consistency, validity.
    //! Assert if the memory check fails.
    //! called automatically every so often if (_CRTDBG_CHECK_ALWAYS_DF,_CRTDBG_CHECK_EVERY_16_DF,_CRTDBG_CHECK_EVERY_128_DF,etc)
    //! @return false = failure.

#if defined(_MSC_VER) && defined(_DEBUG) && !defined(UNDER_CE) && USE_CRT
    bool bRet = ::_CrtCheckMemory();
    ASSERT(bRet);
#else
    bool bRet = true;
#endif
    return bRet;
}

bool GRAYCALL cHeap::IsValidInside(const void* pData, ptrdiff_t iOffset) noexcept {  // static
    //! Is this offset inside the valid heap block (pData).
    //! @note this should only ever be used in debug code. and only in an ASSERT.
    CODEPROFILEFUNC();
    if (iOffset < 0)  // never ok. even if aligned.
        return false;
    if (!cHeap::IsValidHeap(pData)) return false;
    const size_t nSize = cHeap::GetSize(pData);
    if ((size_t)iOffset >= nSize) return false;
    return true;
}

void GRAYCALL cHeap::FreePtr(void* pData) noexcept {  // static
    if (pData == nullptr) return;
    CODEPROFILEFUNC();
#if defined(_DEBUG)
    DEBUG_CHECK(cHeap::IsValidHeap(pData));
#endif
    cHeap::sm_nAllocs--;
#ifdef USE_HEAP_STATS
    size_t nSizeAllocated = cHeap::GetSize(pData);
    sm_nAllocTotalBytes -= nSizeAllocated;
#endif
#if defined(_WIN32) && !USE_CRT
    ::HeapFree(g_Heap, 0, pData);
#else
    ::free(pData);
#endif
}

void* GRAYCALL cHeap::AllocPtr(size_t iSize) {  // static // throw(std::bad_alloc)
    //! Allocate a block of memory on the application heap. assume nothing about its current contents. uninitialized.
    //! Same footprint as C malloc()
    //! Will it throw if out of memory? (or the heap is invalid/corrupted)
    //! iSize = 0 is allowed for some reason. (maybe returns NON nullptr)

    CODEPROFILEFUNC();
#ifdef _DEBUG
    DEBUG_ASSERT(iSize < k_ALLOC_MAX, "AllocPtr");  // 256 * 64K - remove/change this if it becomes a problem
#endif
#if defined(_WIN32) && !USE_CRT
    void* pData = ::HeapAlloc(g_Heap, 0, iSize);  // nh_malloc_dbg.
#else
    void* pData = ::malloc(iSize);         // nh_malloc_dbg.
#endif
    if (pData == nullptr) {
        // I asked for too much!
        DEBUG_ASSERT(0, "malloc");
        return nullptr;  // E_OUTOFMEMORY
    }
#if defined(_DEBUG)
    ASSERT(cHeap::IsValidHeap(pData));
#endif
    cHeap::sm_nAllocs++;
#ifdef USE_HEAP_STATS
    size_t nSizeAllocated = cHeap::GetSize(pData);  // the actual size.
    ASSERT(nSizeAllocated >= iSize);
    sm_nAllocTotalBytes += nSizeAllocated;  // actual alloc size may be diff from requested size.
#endif
    return pData;
}

void* GRAYCALL cHeap::ReAllocPtr(void* pData, size_t iSize) {  // static
    //! allocate a different sized block but preserve existing content.
    //! Same footprint as C realloc()
    //! Should it throw if pData is invalid ? nullptr is ok.
    //! Will it throw if out of memory? (or the heap is invalid/corrupted)

    CODEPROFILEFUNC();
    ASSERT(iSize < k_ALLOC_MAX);  // 256 * 64K
    void* pData2 = nullptr;
    if (pData == nullptr) {
        if (iSize <= 0)  // just do nothing. this is ok.
            return nullptr;
#if defined(_WIN32) && !USE_CRT
        pData2 = ::HeapAlloc(g_hHeap, 0, iSize);  // nh_malloc_dbg.
#else
        pData2 = ::malloc(iSize);          // nh_malloc_dbg.
#endif
    } else {
#if defined(_WIN32) && !USE_CRT
        pData2 = ::HeapReAlloc(g_hHeap, 0, pData, iSize);
#else
        pData2 = ::realloc(pData, iSize);  // nh_malloc_dbg.
#endif
#ifdef USE_HEAP_STATS
        sm_nAllocTotalBytes -= cHeap::GetSize(pData);
#endif
        cHeap::sm_nAllocs--;
    }
    if (pData2 == nullptr) {
        // I asked for too much!
        DEBUG_ASSERT(iSize == 0, "realloc");  // 0 sized malloc may or may not return nullptr ? not sure why.
        return nullptr;
    }
    cHeap::sm_nAllocs++;
#ifdef USE_HEAP_STATS
    size_t nSizeAllocated = cHeap::GetSize(pData2);
    ASSERT(nSizeAllocated >= 0 && nSizeAllocated >= iSize);
    sm_nAllocTotalBytes += nSizeAllocated;  // alloc size may be different than requested size.
#endif
    return pData2;
}

size_t GRAYCALL cHeap::GetSize(const void* pData) noexcept {  // static
    //! get the actual allocated size of a memory block in bytes.
    //! @note __linux__ = Not always the size of the allocation request. maybe greater.
    //! _WIN32 = always exact same size as requested malloc(),

    if (pData == nullptr) return 0;
#if defined(__linux__)
    return ::malloc_usable_size((void*)pData);
#elif defined(_WIN32) && !USE_CRT
    return ::HeapSize(g_Heap, 0, (void*)pData);
#elif defined(_WIN32)
    return ::_msize((void*)pData);
#else
#error NOOS
#endif
}

bool GRAYCALL cHeap::IsValidHeap(const void* pData) noexcept {  // static
    //! is this a valid malloc() heap pointer? ::GlobalAlloc does not validate ???
    //! @note this should only ever be used in debug code. and only in an ASSERT.
    if (!cMem::IsValidApp(pData)) return false;
#if defined(_DEBUG)
#if defined(_WIN32) && !defined(UNDER_CE) && !defined(__GNUC__) && USE_CRT
    return ::_CrtIsValidHeapPointer(pData) ? true : false;
#else
    //! @todo validate the heap block vs static memory for __linux__?
    return !cMem::IsCorruptApp(pData, 1);
#endif
#else
    return !cMem::IsCorruptApp(pData, 1);
#endif
}

//********************************************

#if defined(_MSC_VER) && (_MSC_VER >= 1300)
// #pragma pack(1)
/// <summary>
/// Internal prefix for Aligned block of memory. allocated using _aligned_malloc or malloc.
/// VERY DANGEROUS to use internal structure like this!
/// FROM MSVC.NET 2003 CRT - MAY NEED CHANGING FOR OTHER COMPILER
/// ASSUME: alignment empty memory is here. contains 0x0BADF00D repeated.
/// </summary>
struct CATTR_PACKED cHeapAlignHeader {
    void* m_pMallocHead;  /// pointer back to the returned malloc() memory. may point at self?!
#ifdef _DEBUG
    DWORD m_TailGap;  /// filled with kTailGap IN _DEBUG ONLY HEAP. cMemSignature kTailGap
#endif
};
// #pragma pack()

/// <summary>
/// Get the header prefix for the align memory block. maybe IsAlignedAlloc().
/// </summary>
/// <param name="pData">the pointer returned by cHeapAlign::Alloc</param>
/// <returns></returns>
const cHeapAlignHeader* GRAYCALL cHeapAlign::GetHeader(const void* pData) noexcept {  // static
    if (pData == nullptr) return nullptr;
    UINT_PTR uDataPtr = PtrCastToNum(pData);
    uDataPtr &= ~(sizeof(UINT_PTR) - 1);
    auto pHdr = CastNumToPtr<const cHeapAlignHeader>(uDataPtr - sizeof(cHeapAlignHeader));
    // is pHdr valid ?
#ifdef _DEBUG
    if (pHdr->m_TailGap != kTailGap) return nullptr;
#endif
    const BYTE* pMallocHead = PtrCast<const BYTE>(pHdr->m_pMallocHead);
    // check small valid range. no point to align > HEAP_BYTE_SizeAlignMax !? iAligned vs k_SizeAlignMax
    if (pMallocHead > (const void*)pHdr || pMallocHead + k_SizeAlignMax < pData) {
        return nullptr;
    }
    // DEBUG_CHECK(cHeap::IsValidHeap(pMallocHead));
    return pHdr;
}

bool GRAYCALL cHeapAlign::IsHeapAlign(const void* pData) noexcept {  // static
    //! Was pData created using cHeapAlign::Alloc() ?
    //! @note _DEBUG heap is VERY different from release heap structure.
    return GetHeader(pData) != nullptr;
}

bool GRAYCALL cHeapAlign::IsValidInside(const void* pData, INT_PTR iOffset) noexcept {  // static
    CODEPROFILEFUNC();
    const cHeapAlignHeader* pHdr = GetHeader(pData);
    if (pHdr == nullptr) {
        return cHeap::IsValidInside(pData, iOffset);
    }

    const void* pMallocHead = pHdr->m_pMallocHead;
    if (!cHeap::IsValidHeap(pMallocHead)) return false;
    const size_t nSize = cHeap::GetSize(pMallocHead);
    return CastN(size_t, iOffset) < nSize;
}

bool GRAYCALL cHeapAlign::IsValidHeap(const void* pData) noexcept {  // static
    //! is this a valid malloc() or _aligned_malloc() pointer?
    //! May or may not be aligned.
    CODEPROFILEFUNC();

    // check if this is an aligned alloc first.
    const cHeapAlignHeader* pHdr = GetHeader(pData);
    if (pHdr == nullptr) {
        return cHeap::IsValidHeap(pData);
    }

    // get the cHeapHeader pointer.
    return cHeap::IsValidHeap(pHdr->m_pMallocHead);
}

size_t GRAYCALL cHeapAlign::GetSize(const void* pData) noexcept {  // static
    CODEPROFILEFUNC();
    const cHeapAlignHeader* pHdr = GetHeader(pData);
    if (pHdr == nullptr) {
        return cHeap::GetSize(pData);
    }

    // get the cHeapHeader pointer.
    DEBUG_CHECK(cHeap::IsValidHeap(pHdr->m_pMallocHead));
    return cHeap::GetSize(pHdr->m_pMallocHead);
}

void GRAYCALL cHeapAlign::FreePtr(void* pData) {  // static
    //! NOTE: Will work if ! IsHeapAlign
    CODEPROFILEFUNC();
    if (pData == nullptr) return;

#ifdef USE_HEAP_STATS
    size_t nSizeAllocated = GetSize(pData);  // can't call _msize for aligned directly.
    sm_nAllocTotalBytes -= nSizeAllocated;
#endif
    sm_nAllocs--;

#if defined(_WIN32) && !defined(UNDER_CE) && USE_CRT
    ::_aligned_free(pData);  // CAN'T just use free() ! we need to undo the padding.
#elif defined(_WIN32) && !USE_CRT
    ::HeapFree(g_hHeap, 0, pData);
#else
    ::free(pData);                                // Linux just used free() for memalign() and malloc().
#endif
}

void* GRAYCALL cHeapAlign::AllocPtr(size_t iSize, size_t iAlignment) {  // static
    //! @note _aligned_malloc is based on malloc(); see malloc for more information on using _aligned_malloc
    //! @arg
    //!  iAlignment = The alignment value, which must be an integer power of 2.
    CODEPROFILEFUNC();
    ASSERT(iAlignment > 0 && iAlignment <= k_SizeAlignMax);

    // 0 size is allowed for some reason. (returns NON nullptr)
#if defined(UNDER_CE)
    void* pData = ::malloc(iSize);  // No special call for this in CE. WRONG!!!
#elif defined(__linux__)
    void* pData = ::memalign(iAlignment, iSize);
#elif defined(_WIN32) && !USE_CRT
    void* pData = ::HeapAlloc(g_Heap, 0, iSize);  // WRONG!!!
#elif defined(_WIN32) && USE_CRT
    void* pData = ::_aligned_malloc(iSize, iAlignment);  // nh_malloc_dbg.
#else
#error NOOS
#endif
    if (pData == nullptr) {
        // I asked for too much!
        DEBUG_ASSERT(0, "_aligned_malloc");
        return nullptr;
    }
    ASSERT(cHeapAlign::IsHeapAlign(pData));

#ifdef USE_HEAP_STATS
    size_t nSizeAllocated = GetSize(pData);  // can't call _msize for aligned directly.
    ASSERT(nSizeAllocated >= iSize);
    ASSERT(nSizeAllocated >= iAlignment);
    sm_nAllocTotalBytes += nSizeAllocated;  // alloc size may be different than requested size.
#endif
    sm_nAllocs++;
    return pData;
}
#endif  // defined(_MSC_VER) && _MSC_VER >= 1300
}  // namespace Gray
