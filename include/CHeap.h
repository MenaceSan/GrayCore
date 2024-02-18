//! @file cHeap.h
//! wrap a dynamically allocated (un-typed) block/blob of heap memory.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cHeap_H
#define _INC_cHeap_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"
#include "cMem.h"

#define USE_HEAP_STATS  // _DEBUG total allocation stats.

namespace Gray {

struct cHeapStats {
    ITERATE_t _Ops = 0;
    ITERATE_t _Allocs = 0;  /// count total allocations (i.e. Number of calls to malloc() minus calls to free())
#ifdef USE_HEAP_STATS
    size_t _Total = 0;  /// Keep running count of current total memory allocated.
    size_t _Max = 0;    /// max observed _Total
#endif
    void Alloc(size_t size = 1) {
        _Ops++;
        _Allocs++;
#ifdef USE_HEAP_STATS
        _Total += size;
        if (_Total > _Max) _Max = _Total;
#endif
    }
    void Free(size_t size = 1) {
        ASSERT(_Allocs > 0);
        _Allocs--;
#ifdef USE_HEAP_STATS
        ASSERT(size <= _Total);
        _Total -= size;
#endif
    }
};

struct GRAYCORE_LINK cHeapCommon : public cMem {  // static class
    /// Debug Heap fill bytes. enum? _WIN32 only?
    static const BYTE kFillAlloc = 0xCD;   /// filled to indicate malloc() memory in debug mode.
    static const BYTE kFillFreed = 0xDD;   /// filled to indicate free() has been called on this.
    static const BYTE kFillPrefix = 0xFD;  /// Fills the gap before the returned memory block. _DEBUG ONLY

    static const size_t k_ALLOC_MAX = 0x2000000;  /// (arbitrary) largest reasonable single malloc. e.g. Single frame allocation of big screen

#ifdef USE_64BIT
    static const size_t k_SizeAlignDef = 16;  /// default/min heap alignment for the arch.
#else
    static const size_t k_SizeAlignDef = 8;  /// default/min heap alignment for the arch.
#endif

    static HANDLE g_hHeap;  /// ::GetProcessHeap() _WIN32
    static cHeapStats g_Stats;

    /// <summary>
    /// What is the alignment of this pointer?
    /// </summary>
    /// <param name="pData"></param>
    /// <returns>2,4,8,16?</returns>
    static size_t GRAYCALL GetAlign(const void* pData) noexcept;

    /// <summary>
    /// Get total physical memory for this system/machine (not just process?).
    /// UINT64 same as size_t for 64bit
    /// </summary>
    static UINT64 GRAYCALL get_PhysTotal();

    /// <summary>
    /// get total physical memory that might be avail to this process.
    /// UINT64 same as size_t for 64bit
    /// </summary>
    static UINT64 GRAYCALL get_PhysAvail();

    static void GRAYCALL Init(int nFlags = 0);
    static bool GRAYCALL Check();
};

/// <summary>
/// A static name space for applications main heap allocation/free related functions.
/// @note Turning on the _DEBUG heap automatically uses _malloc_dbg()
/// _DEBUG will put header and footer info on each heap allocation.
/// malloc, by default, align address to 8 bytes (x86) or 16 bytes (x64)
/// </summary>
struct GRAYCORE_LINK cHeap : public cHeapCommon {  // static class
    static bool GRAYCALL IsValidHeap(const void* pData) noexcept;
    static size_t GRAYCALL GetSize(const void* pData) noexcept;
    static bool GRAYCALL IsValidInside(const void* pData, ptrdiff_t index) noexcept;

    static void* GRAYCALL AllocPtr(size_t nSize);

    /// <summary>
    /// free a pointer to a block allocated on the process heap.
    /// Same footprint as C free(). Should it throw if pData is invalid ? nullptr is ok.
    /// </summary>
    static void GRAYCALL FreePtr(void* pData) noexcept;

    static void* GRAYCALL ReAllocPtr(void* pData, size_t nSize);

    /// helpers

    /// <summary>
    /// is this NOT a valid malloc() heap pointer?
    /// @note this should only ever be used in debug code. and only in an ASSERT.
    /// </summary>
    static inline bool IsCorruptHeap(const void* pData) noexcept {
        if (pData == nullptr)  // nullptr is not corrupt. freeing it does nothing.
            return false;
        return !IsValidHeap(pData);
    }
    /// <summary>
    /// Allocate memory then copy stuff into it.
    /// </summary>
    static inline void* AllocPtr(size_t nSize, const void* pDataInit) {
        void* pData = AllocPtr(nSize);
        if (pData != nullptr && pDataInit != nullptr) {
            cMem::Copy(pData, pDataInit, nSize);
        }
        return pData;
    }
};

#if defined(_MSC_VER) && (_MSC_VER >= 1300)
struct cHeapAlignHeader;

/// <summary>
/// Allocate a block/blob of memory that starts on a certain alignment. will/may have padded prefix. allocate Align-1 more.
/// Linux might use posix_memalign() http://linux.about.com/library/cmd/blcmdl3_posix_memalign.htm
/// size align must be a power of two and a multiple of sizeof(void *). 16(32 bit only), 32, 64, 128
/// WARNING: dont mix this with cHeap calls! Use cHeapAlign consistently on the same pointer.
/// </summary>
struct GRAYCORE_LINK cHeapAlign : public cHeapCommon {  // static
#ifdef _DEBUG
#if (_MSC_VER < 1400)
    static const DWORD kTailGap = 0xBDBDBDBD;  /// Fills the m_TailGap _DEBUG ONLY
#else
    static const DWORD kTailGap = 0xEDEDEDED;  /// Fills the m_TailGap _DEBUG ONLY
#endif
#endif
    static const size_t k_SizeAlignMax = 128;  /// max reasonable size for alignment. Why would you align to more than this ?

    static const cHeapAlignHeader* GRAYCALL GetHeader(const void* pData) noexcept;
    static bool GRAYCALL IsHeapAlign(const void* pData) noexcept;

    // Override cHeap

    static bool GRAYCALL IsValidHeap(const void* pData) noexcept;
    static size_t GRAYCALL GetSize(const void* pData) noexcept;
    static bool GRAYCALL IsValidInside(const void* pData, INT_PTR index) noexcept;

    static void* GRAYCALL AllocPtr(size_t nSize, size_t iAligned);
    static void GRAYCALL FreePtr(void* pData);

    static void* GRAYCALL ReAllocPtr(void* pData, size_t nSize) = delete;  // NOT supported!
};
#endif  // _MSC_VER
}  // namespace Gray
#endif  // _INC_cHeap_H
