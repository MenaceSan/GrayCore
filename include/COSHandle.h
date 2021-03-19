//
//! @file cOSHandle.h
//! Wrap the OS kernel handle (scoped auto-close). for _WIN32 or __linux__.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cOSHandle_H
#define _INC_cOSHandle_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "cTimeSys.h" // TIMESYSD_t
#include "cNonCopyable.h"
#include "cUnitTestDecl.h"
#include "cDebugAssert.h"

namespace Gray
{
#define HANDLE_NULL		NULL	//!< Invalid OS handle for _WIN32. Not invalid OS handle for __linux__.

#define WINHANDLE_NULL	NULL	//!< HWND, HPEN, etc are NOT OS Handles. like HWND_DESKTOP. like HANDLEPTR_NULL. This is a WINAPI void* handle via DECLARE_HANDLE like HWND, HPEN, HINSTANCE, etc. can't call CloseHandle() on it.

#ifdef __linux__
	typedef int HANDLE;			//!< an OS Handle. int not INT_PTR and not void* like _WIN32.
	static const HANDLE INVALID_HANDLE_VALUE = ((HANDLE)-1); //!< Invalid OS Handle. 0 is a valid OS handle in __linux__ but not in _WIN32. (0=stdin)

	typedef void* HMODULE;		//!< ::dlclose() is different for __linux__ than _WIN32
	typedef void* HINSTANCE;
#endif

	enum SEEK_ORIGIN_TYPE
	{
		//! @enum Gray::SEEK_ORIGIN_TYPE
		//! What are we moving relative to ? SEEK_SET,SEEK_CUR,SEEK_END or FILE_BEGIN,FILE_CURRENT,FILE_END
		//! SEEK_SET defined for both __linux__ and _WIN32
		//! same as enum tagSTREAM_SEEK

		SEEK_Set = 0,		//!< SEEK_SET = FILE_BEGIN = STREAM_SEEK_SET = 0 = relative to the start of the file.
		SEEK_Cur = 1,		//!< SEEK_CUR = FILE_CURRENT = STREAM_SEEK_CUR = 1 = relative to the current position.
		SEEK_End = 2,		//!< SEEK_END = FILE_END = STREAM_SEEK_END = 2 = relative to the end of the file.

		SEEK_MASK = 0x0007,		//!< | _BITMASK(SEEK_Set) allow extra bits above SEEK_ORIGIN_TYPE ?
	};

#if defined(_MFC_VER) && ( _MFC_VER > 0x0600 )
	typedef LONGLONG	STREAM_OFFSET_t;
	typedef ULONGLONG	STREAM_POS_t;		// same as FILE_SIZE_t.
#define USE_FILE_POS64

#else
	typedef LONG_PTR	STREAM_OFFSET_t;	//!< Might be 64 or 32 bit relative value (signed). TODO SET USE_FILE_POS64
	typedef ULONG_PTR	STREAM_POS_t;		//!< NOT same as FILE_SIZE_t in 32 bit? Why not ?

#endif	// ! _MFC_VER

	constexpr STREAM_POS_t k_STREAM_POS_ERR = (STREAM_POS_t)(-1);	// like INVALID_SET_FILE_POINTER

	class GRAYCORE_LINK cOSHandle : protected cNonCopyable
	{
		//! @class Gray::cOSHandle
		//! Wrap ownership of a OS Kernel HANDLE. (NOT a GUI or User handle)
		//! Close on destruct.
		//! Any OS system handles that might use CloseHandle()
		//! Similar to CHandle in ATL
		//! @note Don't use cHandlePtr<> because that is just for typed (not void) pointers and HANDLE is 'int' in __linux__

	public:
		HANDLE m_h;

	protected:
		void CloseHandleLast() noexcept
		{
			// Assume destruction or my caller will clear m_h
			if (!isValidHandle())
				return;
			CloseHandle(m_h);
		}

	public:
		explicit inline cOSHandle(HANDLE h = INVALID_HANDLE_VALUE) noexcept
			: m_h(h)
		{
		}

		cOSHandle(const cOSHandle& Handle) noexcept
			: m_h(Handle.Duplicate())
		{
		}
		cOSHandle& operator=(const cOSHandle& Handle)
		{
			if (m_h != Handle.m_h)
			{
				AttachHandle(Handle.Duplicate());
			}
			return (*this);
		}

		inline ~cOSHandle()
		{
			CloseHandleLast();
		}

		operator HANDLE () const noexcept
		{
			return m_h;
		}
		HANDLE get_Handle() const noexcept
		{
			return m_h;
		}
		HANDLE& ref_Handle() noexcept
		{
			return m_h;
		}

		static bool inline IsValidHandle(HANDLE h) noexcept
		{
			//! 0 is a valid OS handle for __linux__ but NOT _WIN32
#ifdef _WIN32
			if (h == HANDLE_NULL)	//!< 0 is never a valid handle value for _WIN32. (0=stdin for __linux__)
				return false;
#endif
			return(h != INVALID_HANDLE_VALUE);	//  -1
		}
		bool isValidHandle() const noexcept
		{
			return IsValidHandle(m_h);
		}

		static inline bool CloseHandle(HANDLE h) noexcept // static
		{
			//! default implementation for closing OS HANDLE.
			//! ASSUME IsValidHandle(h)
			//! @return true = ok;
#ifdef _WIN32
			const BOOL bRet = ::CloseHandle(h); // ignored bool return.
			return bRet;
#elif defined(__linux__)
			const int iRet = ::close(h);
			return iRet == 0;
#endif
		}
		void CloseHandle() noexcept
		{
			if (!isValidHandle())
				return;
			HANDLE h = m_h;
			m_h = INVALID_HANDLE_VALUE;
			CloseHandle(h);
		}

#ifdef __linux__
		void OpenHandle(const char* pszPath, UINT uFlags, UINT uMode = 0)
		{
			//! Similar to _WIN32 AttachHandle( _FNF(::CreateFile))
			//! @arg uFlags = O_RDWR | O_NOCTTY | O_NDELAY
			//! @note call isValidHandle() to decide if it worked. HResult::GetLastDef(E_HANDLE)
			CloseHandleLast();
			m_h = ::open(pszPath, uFlags, uMode);
		}
#endif

		void AttachHandle(HANDLE h) noexcept
		{
			if (m_h != h)
			{
				CloseHandleLast();
				m_h = h;
			}
		}
		HANDLE DetachHandle() noexcept
		{
			HANDLE h = m_h;
			m_h = INVALID_HANDLE_VALUE;
			return h;
		}

		HRESULT WriteX(const void* pData, size_t nDataSize) const
		{
			//! @return # of bytes written. <0 = error.
			//!   ERROR_INVALID_USER_BUFFER = too many async calls ?? wait
			//!   ERROR_IO_PENDING = must wait!?

			if (nDataSize <= 0)	// Do nothing.
				return S_OK;
#if defined(_WIN32)
			DWORD nLengthWritten = 0;
			const bool bRet = ::WriteFile(m_h, pData, (DWORD)nDataSize, &nLengthWritten, nullptr);
			if (!bRet)
#elif defined(__linux__)
			int nLengthWritten = ::write(m_h, (const char*)pData, (long)nDataSize);
			if (nLengthWritten <= 0)
#endif
			{
				return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));	// E_HANDLE = file handle m_h is bad. 
			}
			return (HRESULT)nLengthWritten;
		}

		HRESULT ReadX(void* pData, size_t nDataSize) const
		{
			//! @return HRESULT_WIN32_C(ERROR_HANDLE_EOF) = EOF.
			if (nDataSize <= 0)
				return S_OK;
#ifdef _WIN32
			DWORD nLengthRead;
			const bool bRet = ::ReadFile(m_h, pData, (DWORD)nDataSize, &nLengthRead, nullptr);
			if (!bRet)
#elif defined(__linux__)
			int nLengthRead = ::read(m_h, pData, (long)nDataSize);
			if (nLengthRead == 0)	// EOF 
				return HRESULT_WIN32_C(ERROR_HANDLE_EOF);
			if (nLengthRead < 0)
#endif
			{
				// ERROR_HANDLE_EOF = No more data = EOF.
				return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_READ_FAULT));
			}
			return (HRESULT)nLengthRead;
		}

		HRESULT FlushX() const noexcept
		{
			//! synchronous flush of write data to file.
#ifdef _WIN32
			if (!::FlushFileBuffers(m_h))
#elif defined(__linux__)
			int iRet = ::fsync(m_h);
			if (iRet != 0)
#endif
			{
				return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
			}
			return S_OK;
		}

		STREAM_POS_t SeekRaw(STREAM_OFFSET_t lOffset, SEEK_ORIGIN_TYPE eSeekOrigin) const noexcept
		{
			//! Change or get the current file position pointer.
			//! @note it is legal to seek beyond the end of the file to grow it !
#ifdef _WIN32
#ifdef USE_FILE_POS64
			LARGE_INTEGER NewFilePointer;
			NewFilePointer.QuadPart = lOffset;
			if (!::SetFilePointerEx(m_h, NewFilePointer, &NewFilePointer, eSeekOrigin))
			{
				return k_STREAM_POS_ERR;	// HResult::GetLast()
			}
			return NewFilePointer.QuadPart;
#else
			DWORD dwRet = ::SetFilePointer(m_h, (LONG)lOffset, nullptr, eSeekOrigin);
			if (dwRet == INVALID_SET_FILE_POINTER)
				return k_STREAM_POS_ERR;	// HResult::GetLast()
			return dwRet;
#endif // USE_FILE_POS64
#else
			//! Use _tell( m_hFile ) for __linux__ ?
			return (STREAM_POS_t) ::lseek(m_h, 0, SEEK_CUR);
#endif
		}

		HRESULT SeekX(STREAM_OFFSET_t lOffset, SEEK_ORIGIN_TYPE eSeekOrigin) const
		{
			//! Change or get the current file position pointer.
			//! @arg eSeekOrigin = // SEEK_SET ?
			//! @return the New position % int, <0=FAILED
			//! @note it is legal to seek beyond the end of the file to grow it !

			const STREAM_POS_t nPos = SeekRaw(lOffset, eSeekOrigin);
			if (nPos == k_STREAM_POS_ERR)
			{
				return HResult::GetLastDef();
			}

			return (HRESULT)(INT32)nPos;	// truncated ?
		}

		HRESULT WaitForSingleObject(TIMESYSD_t dwMilliseconds) const;

#ifdef __linux__
		int IOCtl(int nCmd, void* pArgs) const;
		int IOCtl(int nCmd, int nArgs) const;
#endif

#if defined(_WIN32) && ! defined(UNDER_CE)
		DWORD GetInformation() const
		{
			//! @return HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE
			ASSERT(isValidHandle());
			DWORD dwHandleInfo = 0;
			if (!::GetHandleInformation(m_h, &dwHandleInfo))
			{
				return 0;
			}
			return dwHandleInfo;
		}
		bool SetInformation(DWORD dwMask, DWORD dwFlags) const
		{
			ASSERT(isValidHandle());
			const bool bRet = ::SetHandleInformation(m_h, dwMask, dwFlags);
			return bRet;
		}
#endif

#ifdef _WIN32
		HANDLE Duplicate(HANDLE hTargetProcess = INVALID_HANDLE_VALUE, DWORD dwDesiredAccess = DUPLICATE_SAME_ACCESS, bool bInheritHandle = false, DWORD dwOptions = DUPLICATE_SAME_ACCESS) const
		{
			//! Create a Dupe of this handle with special properties.
			//! dwOptions = DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS (ignore dwDesiredAccess)
			ASSERT(isValidHandle());
			HANDLE hNewHandle = INVALID_HANDLE_VALUE;
			HANDLE hCurrentProcess = ::GetCurrentProcess();
			if (hTargetProcess == INVALID_HANDLE_VALUE)
				hTargetProcess = hCurrentProcess;
			const bool bRet = ::DuplicateHandle(hCurrentProcess, m_h, hTargetProcess,
				&hNewHandle, dwDesiredAccess, bInheritHandle, dwOptions);
			UNREFERENCED_PARAMETER(bRet);
			return hNewHandle;
		}
#elif defined(__linux__)
		HANDLE Duplicate() const
		{
			// http://linux.about.com/library/cmd/blcmdl2_dup.htm
			return ::dup(m_h);
		}
#endif

		UNITTEST_FRIEND(cOSHandle);
	};

};
#endif // _INC_cOSHandle_H
