//! @file cOSHandleSet.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cOSHandleSet_H
#define _INC_cOSHandleSet_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArray.h"
#include "cOSHandle.h"

namespace Gray {
/// <summary>
/// A collection of cOSHandle
/// Wait on any of a set of OS handles to be signaled.
/// Similar to cNetSocketSet and ::select() especially for __linux__
/// Similar to _WIN32 WaitForMultipleObjects(). MAX = MAXIMUM_WAIT_OBJECTS or FD_SETSIZE
/// </summary>
class GRAYCORE_LINK cOSHandleSet {
 public:
    static const int k_nHandeMax =  //! not always the same as cNetSocketSet::k_nSocketSetSize
#ifdef _WIN32
        MAXIMUM_WAIT_OBJECTS;
#else
        FD_SETSIZE;
#endif

 private:
#ifdef _WIN32
    cArrayVal<::HANDLE> _fds;  /// an array of OS handles. like fd_set. but dynamic.
#elif defined(__linux__)
    ::HANDLE _hHandleMax;  /// Largest handle we have. <= FD_SETSIZE
    ::fd_set _fds;         /// array of FD_SETSIZE possible HANDLE(s). NOTE: sizeof(_fds) varies/fixed with FD_SETSIZE
#else
#error NOOS
#endif

 private:
    void InitHandles() noexcept {
#ifdef __linux__
        _hHandleMax = 0;
        FD_ZERO(&_fds);
#endif
    }

 public:
    cOSHandleSet() noexcept {
        InitHandles();
    }
    cOSHandleSet(HANDLE h) {
        // a handle set with a single handle.
#ifdef __linux__
        _hHandleMax = h;
        FD_ZERO(&_fds);
        FD_SET(h, &_fds);
#else
        InitHandles();
        AddHandle(h);
#endif
    }
    cOSHandleSet(const cOSHandleSet& nss) {
        InitHandles();
        SetCopy(nss);
    }
    ~cOSHandleSet() {}

    void operator=(const cOSHandleSet& nss) {
        SetCopy(nss);
    }
    void SetCopy(const cOSHandleSet& nss) {
#ifdef __linux__
        _hHandleMax = nss._hHandleMax;
        // FD_COPY(pfds,&_fds);
        cMem::Copy(&_fds, &nss._fds, sizeof(_fds));
#else
        _fds = nss._fds;
#endif
    }

    /// <summary>
    /// Add OSHandle to the set.
    /// @note can't add more than k_nHandeMax compile time constant.
    /// FD_SETSIZE on __linux__. MAXIMUM_WAIT_OBJECTS on _WIN32.
    /// </summary>
    bool AddHandle(HANDLE h) {
        if (h == INVALID_HANDLE_VALUE) return false;
#ifdef __linux__
        if (h > _hHandleMax) _hHandleMax = h;
        FD_SET(h, &_fds);
#else
        // Only add up to
        if (h == cOSHandle::kNULL) return false;
        if (_fds.GetSize() >= MAXIMUM_WAIT_OBJECTS) return false;
        _fds.Add(h);
#endif
        return true;
    }
    void RemoveHandle(::HANDLE h) {
        if (h == INVALID_HANDLE_VALUE) return;
#ifdef __linux__
        FD_CLR(h, &_fds);
#else
        _fds.RemoveArg(h);
#endif
    }
    void ClearHandles() {
#ifdef __linux__
        InitHandles();
#else
        _fds.RemoveAll();
#endif
    }

    HRESULT WaitForObjects(TIMESYSD_t dwMilliseconds, bool bWaitForAll = false);
};
}  // namespace Gray
#endif
