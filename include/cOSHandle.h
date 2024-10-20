//! @file cOSHandle.h
//! Wrap the OS kernel handle (scoped auto-close). for _WIN32 or __linux__.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cOSHandle_H
#define _INC_cOSHandle_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "cDebugAssert.h"  // ASSERT
#include "cMemSpan.h"
#include "cNonCopyable.h"
#include "cStreamProgress.h"  // STREAM_OFFSET_t , STREAM_POS_t, SEEK_t
#include "cTimeSys.h"         // TIMESYSD_t

#ifdef __linux__
typedef int HANDLE;                                       /// an OS Handle. int not INT_PTR and not void* like _WIN32.
static const HANDLE INVALID_HANDLE_VALUE = ((HANDLE)-1);  /// Invalid OS Handle. 0 is a valid OS handle in __linux__ but not in _WIN32. (0=stdin)

typedef void* HMODULE;  /// ::dlclose() is different for __linux__ than _WIN32
typedef void* HINSTANCE;
#endif

namespace Gray {
#define WINHANDLE_NULL NULL  /// HWND, HPEN, etc are NOT OS Handles. like HWND_DESKTOP. like HANDLEPTR_NULL. This is a WINAPI void* handle via DECLARE_HANDLE like HWND, HPEN, HINSTANCE, etc. can't call CloseHandle() on it.

/// <summary>
/// Wrap ownership of a OS Kernel HANDLE. (NOT a GUI or User handle)
/// Close on destruct.
/// Any OS system handles that might use CloseHandle()
/// Similar to CHandle in ATL
/// @note Don't use cHandlePtr because that is just for typed (not void) pointers and HANDLE is 'int' in __linux__
/// </summary>
class GRAYCORE_LINK cOSHandle : protected cNonCopyable {
 public:
    ::HANDLE _h;  /// The OS assigned handle to some object.
    static constexpr ::HANDLE kNULL = NULL;  /// Invalid OS handle for _WIN32. Not invalid OS handle for __linux__?

 protected:
    void CloseHandleLast() noexcept {
        // Assume destruction or my caller will clear _h
        if (!isValidHandle()) return;
        CloseHandle(_h);
    }

 public:
    explicit inline cOSHandle(::HANDLE h = INVALID_HANDLE_VALUE) noexcept : _h(h) {}
    cOSHandle(const cOSHandle& h) noexcept : _h(h.Duplicate()) {}
    cOSHandle(cOSHandle&& h) noexcept : _h(h._h) {
        h._h = INVALID_HANDLE_VALUE;
    }

    cOSHandle& operator=(const cOSHandle& Handle) {
        if (_h != Handle._h) {
            AttachHandle(Handle.Duplicate());
        }
        return *this;
    }

    inline ~cOSHandle() {
        CloseHandleLast();
    }

    operator ::HANDLE() const noexcept {
        return _h;
    }
    ::HANDLE get_Handle() const noexcept {
        return _h;
    }
    ::HANDLE& ref_Handle() noexcept {
        return _h;
    }

    /// <summary>
    /// Is handle valid?
    /// @note: 0 is a valid OS handle for __linux__ but NOT _WIN32
    /// </summary>
    /// <param name="h"></param>
    /// <returns></returns>
    static bool inline IsValidHandle(::HANDLE h) noexcept {
#ifdef _WIN32
        if (h == kNULL) return false;  /// 0 is never a valid handle value for _WIN32. (0=stdin for __linux__)
#endif
        return h != INVALID_HANDLE_VALUE;  //  -1
    }
    bool isValidHandle() const noexcept {
        return IsValidHandle(_h);
    }

    /// <summary>
    /// default implementation for closing OS HANDLE.
    /// ASSUME IsValidHandle(h)
    /// </summary>
    /// <param name="h"></param>
    /// <returns>true = ok</returns>
    static inline bool CloseHandle(::HANDLE h) noexcept {
        DEBUG_CHECK(IsValidHandle(h));
#ifdef _WIN32
        const BOOL bRet = ::CloseHandle(h);  // ignored bool return.
        return bRet;
#elif defined(__linux__)
        const int iRet = ::close(h);
        return iRet == 0;
#endif
    }
    void CloseHandle() noexcept {
        if (!isValidHandle()) return;
        ::HANDLE h = _h;
        _h = INVALID_HANDLE_VALUE;
        CloseHandle(h);
    }

#ifdef __linux__
    /// <summary>
    /// Similar to _WIN32 AttachHandle( _FNF(::CreateFile)).
    /// @note call isValidHandle() to decide if it worked. HResult::GetLastDef(E_HANDLE)
    /// </summary>
    /// <param name="pszPath"></param>
    /// <param name="uFlags">O_RDWR | O_NOCTTY | O_NDELAY</param>
    /// <param name="uMode"></param>
    void OpenHandle(const char* pszPath, UINT uFlags, UINT uMode = 0) {
        CloseHandleLast();
        _h = ::open(pszPath, uFlags, uMode);
    }
#endif

    void AttachHandle(::HANDLE h) noexcept {
        if (_h != h) {
            CloseHandleLast();
            _h = h;
        }
    }
    ::HANDLE DetachHandle() noexcept {
        ::HANDLE h = _h;
        _h = INVALID_HANDLE_VALUE;
        return h;
    }

    /// <summary>
    /// Write to raw OS handle.
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="nDataSize"></param>
    /// <returns># of bytes written. -lte- nDataSize. -lt- 0 = error.
    /// ERROR_INVALID_USER_BUFFER = too many async calls ?? wait.
    /// ERROR_IO_PENDING = must wait!?</returns>
    HRESULT WriteX(const cMemSpan& m) const {
        if (m.isEmpty()) return S_OK;  // Do nothing.
#if defined(_WIN32)
        DWORD nLengthWritten = 0;
        const bool bRet = ::WriteFile(_h, m, (DWORD)m.get_SizeBytes(), &nLengthWritten, nullptr);
        if (!bRet)
#elif defined(__linux__)
        const int nLengthWritten = ::write(_h, m.GetTPtrC<char>(), (long)m.get_SizeBytes());
        if (nLengthWritten <= 0)
#endif
        {
            return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));  // E_HANDLE = file handle _h is bad.
        }
        return CastN(HRESULT, nLengthWritten);
    }

    /// <summary>
    /// Low level read from the OS file handle _h.
    /// </summary>
    /// <param name="m">max amount of data to read</param>
    /// <returns>-lt- 0 = error. 0=might be EOF?, length of data read lt nDataSize</returns>
    HRESULT ReadX(cMemSpan ret) const noexcept {
        // HRESULT_WIN32_C(ERROR_HANDLE_EOF) = No more data = EOF.
        // HRESULT_WIN32_C(ERROR_READ_FAULT), etc
        if (ret.isEmpty()) return S_OK;
#ifdef _WIN32
        DWORD nLengthRead = 0;
        const bool bRet = ::ReadFile(_h, ret.get_BytePtrW(), CastN(DWORD, ret.get_SizeBytes()), &nLengthRead, nullptr);
        if (!bRet)
#elif defined(__linux__)
        const int nLengthRead = ::read(_h, ret.GetTPtrW<char>(), CastN(long, ret.get_SizeBytes()));
        if (nLengthRead < 0)
#endif
        {
            return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_READ_FAULT));
        }
        return CastN(HRESULT, nLengthRead);
    }

    /// <summary>
    /// synchronous flush of write data to file.
    /// </summary>
    HRESULT FlushX() const noexcept {
#ifdef _WIN32
        if (!::FlushFileBuffers(_h))
#elif defined(__linux__)
        int iRet = ::fsync(_h);
        if (iRet != 0)
#endif
        {
            return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
        }
        return S_OK;
    }

    /// <summary>
    /// Change or get the current file position pointer.
    /// @note it is legal to seek beyond the end of the file to grow it !
    /// </summary>
    /// <param name="nOffset"></param>
    /// <param name="eSeekOrigin">SEEK_t::_Cur</param>
    /// <returns></returns>
    STREAM_POS_t SeekRaw(STREAM_OFFSET_t nOffset, SEEK_t eSeekOrigin) const noexcept {
#ifdef _WIN32
#ifdef USE_FILE_POS64
        ::LARGE_INTEGER NewFilePointer;
        NewFilePointer.QuadPart = nOffset;
        if (!::SetFilePointerEx(_h, NewFilePointer, &NewFilePointer, CastN(DWORD, eSeekOrigin))) {
            return k_STREAM_POS_ERR;  // HResult::GetLast()
        }
        return NewFilePointer.QuadPart;
#else
        DWORD dwRet = ::SetFilePointer(_h, CastN(LONG, nOffset), nullptr, CastN(DWORD, eSeekOrigin));
        if (dwRet == INVALID_SET_FILE_POINTER) return k_STREAM_POS_ERR;  // HResult::GetLast()
        return dwRet;
#endif  // USE_FILE_POS64
#else
        //! Use return _tell( _h ) for __linux__ ? off_t
        return CastN(STREAM_POS_t, ::lseek(_h, nOffset, CastN(int, eSeekOrigin)));
#endif
    }

    /// <summary>
    /// Change or get the current file position pointer.
    /// @note it is legal to seek beyond the end of the file to grow it !
    /// </summary>
    /// <param name="nOffset"></param>
    /// <param name="eSeekOrigin">SEEK_SET ?</param>
    /// <returns>the New position % int, -lt- 0=FAILED</returns>
    HRESULT SeekX(STREAM_OFFSET_t nOffset, SEEK_t eSeekOrigin = SEEK_t::_Set) const noexcept {
        if (!isValidHandle()) return E_HANDLE;
        const STREAM_POS_t nPos = SeekRaw(nOffset, eSeekOrigin);
        if (nPos == k_STREAM_POS_ERR) return HResult::GetLastDef();
        return CastN(HRESULT, CastN(INT32, nPos));  // truncated ?
    }

    HRESULT WaitForSingleObject(TIMESYSD_t dwMilliseconds) const;

#ifdef __linux__
    int IOCtl(int nCmd, void* pArgs) const;
    int IOCtl(int nCmd, int nArgs) const;
#endif

#if defined(_WIN32) && !defined(UNDER_CE)
    DWORD GetInformation() const {
        //! @return HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE
        ASSERT(isValidHandle());
        DWORD dwHandleInfo = 0;
        if (!::GetHandleInformation(_h, &dwHandleInfo)) {
            return 0;
        }
        return dwHandleInfo;
    }
    bool SetInformation(DWORD dwMask, DWORD dwFlags) const {
        ASSERT(isValidHandle());
        const bool bRet = ::SetHandleInformation(_h, dwMask, dwFlags);
        return bRet;
    }
#endif

#ifdef _WIN32
    /// <summary>
    /// Create a Dupe of this handle with special properties.
    /// </summary>
    /// <param name="hTargetProcess"></param>
    /// <param name="dwDesiredAccess"></param>
    /// <param name="bInheritHandle"></param>
    /// <param name="dwOptions">DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS (ignore dwDesiredAccess)</param>
    /// <returns></returns>
    ::HANDLE Duplicate(HANDLE hTargetProcess = INVALID_HANDLE_VALUE, DWORD dwDesiredAccess = DUPLICATE_SAME_ACCESS, bool bInheritHandle = false, DWORD dwOptions = DUPLICATE_SAME_ACCESS) const {
        ASSERT(isValidHandle());
        HANDLE hNewHandle = INVALID_HANDLE_VALUE;
        HANDLE hCurrentProcess = ::GetCurrentProcess();
        if (hTargetProcess == INVALID_HANDLE_VALUE) hTargetProcess = hCurrentProcess;
        const bool bRet = ::DuplicateHandle(hCurrentProcess, _h, hTargetProcess, &hNewHandle, dwDesiredAccess, bInheritHandle, dwOptions);
        UNREFERENCED_PARAMETER(bRet);
        return hNewHandle;
    }
#elif defined(__linux__)
    ::HANDLE Duplicate() const {
        // http://linux.about.com/library/cmd/blcmdl2_dup.htm
        return ::dup(_h);
    }
#endif
};
}  // namespace Gray
#endif  // _INC_cOSHandle_H
