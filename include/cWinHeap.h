//! @file cWinHeap.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cWinHeap_H
#define _INC_cWinHeap_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cOSHandle.h"

#if defined(_WIN32) && !defined(UNDER_CE)

namespace Gray {
/// <summary>
/// Create a private heap for the app. or access the process default heap.
/// </summary>
class GRAYCORE_LINK cWinHeap {
 private:
    ::HANDLE _hHeap;   /// Handle to the _WIN32 heap. GetProcessHeap()
    bool _isManaged = false;  /// I manage its lifetime? use HeapDestroy() ?

 public:
    cWinHeap() noexcept
        : _hHeap(::GetProcessHeap()),
          _isManaged(false)  // This belongs to the process. leave it.
    {
        //! Use the current/default process heap.
    }
    cWinHeap(HANDLE hHeap, bool bManaged = true) noexcept : _hHeap(hHeap), _isManaged(bManaged) {
        //! Attach existing handle to this class.
    }
    cWinHeap(DWORD flOptions, SIZE_T dwInitialSize = 0, SIZE_T dwMaximumSize = 0) noexcept : _hHeap(::HeapCreate(flOptions, dwInitialSize, dwMaximumSize)), _isManaged(true) {
        //! flOptions = HEAP_GENERATE_EXCEPTIONS
    }
    ~cWinHeap() noexcept {
        if (_isManaged) {
            ::HeapDestroy(_hHeap);
        }
    }

    HANDLE get_Handle() const noexcept {
        return _hHeap;
    }
    bool isValidHeap() const noexcept {
        //! did HeapCreate() work?
        return _hHeap != cOSHandle::kNULL;
    }
    SIZE_T Compact(DWORD dwFlags = 0) const noexcept {
        //! dwFlags = HEAP_NO_SERIALIZE
        //! @return Size of the largest block.
        return ::HeapCompact(_hHeap, dwFlags);
    }

#ifndef UNDER_CE
    bool Lock() {
        //! Thread lock this. SERIALIZE
        return ::HeapLock(_hHeap);
    }
    bool Unlock() {
        //! Thread release this. SERIALIZE
        return ::HeapUnlock(_hHeap);
    }
    bool QueryInformation(HEAP_INFORMATION_CLASS HeapInformationClass, void* pData, SIZE_T* pnSizeData) noexcept {
        // HeapInformationClass = HeapEnableTerminationOnCorruption, HeapCompatibilityInformation
        return ::HeapQueryInformation(_hHeap, HeapInformationClass, pData, *pnSizeData, pnSizeData);
    }
    bool SetInformation(HEAP_INFORMATION_CLASS HeapInformationClass, const void* pData, SIZE_T nSizeData) noexcept {
        // HeapInformationClass = HeapEnableTerminationOnCorruption, HeapCompatibilityInformation
        return ::HeapSetInformation(_hHeap, HeapInformationClass, const_cast<void*>(pData), nSizeData);
    }
#endif

#if 0
		// HeapWalk
#endif

    bool IsValidHeapPtr(void* pMem = nullptr, DWORD dwFlags = 0) const noexcept {
        //! Is this a valid heap pointer?
        //! dwFlags = HEAP_NO_SERIALIZE
        return ::HeapValidate(_hHeap, dwFlags, pMem);
    }

    void* AllocPtr(SIZE_T dwBytes, DWORD dwFlags = 0) {
        //! dwFlags= HEAP_ZERO_MEMORY, HEAP_GENERATE_EXCEPTIONS, HEAP_NO_SERIALIZE
        return ::HeapAlloc(_hHeap, dwFlags, dwBytes);
    }
    void* ReAllocPtr(void* pMem, SIZE_T dwBytes, DWORD dwFlags = 0) {
        //! dwFlags= HEAP_ZERO_MEMORY, HEAP_GENERATE_EXCEPTIONS, HEAP_NO_SERIALIZE
        return ::HeapReAlloc(_hHeap, dwFlags, pMem, dwBytes);
    }
    bool FreePtr(void* pMem, DWORD dwFlags = 0) {
        //! dwFlags=HEAP_NO_SERIALIZE
        return ::HeapFree(_hHeap, dwFlags, pMem);
    }
    SIZE_T GetAllocPtrSize(const void* pMem, DWORD dwFlags = 0) const noexcept {
        //! dwFlags=HEAP_NO_SERIALIZE
        //! @return (SIZE_T) -1 = FAILURE.
        return ::HeapSize(_hHeap, dwFlags, pMem);
    }
};
}  // namespace Gray
#endif
#endif  // _INC_cWinHeap_H
