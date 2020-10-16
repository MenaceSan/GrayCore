//
//! @file COSHandle.h
//! Wrap the OS kernel handle. for _WIN32 or __linux__.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_COSHandle_H
#define _INC_COSHandle_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "CTimeSys.h"
#include "CNonCopyable.h"
#include "CUnitTestDecl.h"
#include "CDebugAssert.h"

UNITTEST_PREDEF(COSHandle)

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
		//! SEEK_SET defined for both __linux__ and _WIN32
		//! same as enum tagSTREAM_SEEK
		//! Seek Origin Type flag. SEEK_SET,SEEK_CUR,SEEK_END or FILE_BEGIN,FILE_CURRENT,FILE_END
		SEEK_Set = SEEK_SET,		//!< FILE_BEGIN, STREAM_SEEK_SET = 0 = relative to the start of the file.
		SEEK_Cur = SEEK_CUR,		//!< FILE_CURRENT, STREAM_SEEK_CUR = 1 = relative to the current position.
		SEEK_End = SEEK_END,		//!< FILE_END, STREAM_SEEK_END = 2 = relative to the end of the file.
		SEEK_MASK = 0x0007,		//!< allow extra bits above SEEK_ORIGIN_TYPE ?
	};

#if defined(_MFC_VER) && ( _MFC_VER > 0x0600 )
	typedef LONGLONG	STREAM_OFFSET_t;
	typedef ULONGLONG	STREAM_SEEKRET_t;
	typedef ULONGLONG	STREAM_POS_t;		// same as FILE_SIZE_t.
#define USE_FILE_POS64

#else
	typedef LONG_PTR	STREAM_OFFSET_t;	//!< Might be 64 or 32 bit. TODO SET USE_FILE_POS64
	typedef LONG_PTR	STREAM_SEEKRET_t;	//!< return from Seek()
	typedef ULONG_PTR	STREAM_POS_t;		//!< NOT same as FILE_SIZE_t in 32 bit. Why not ?

#endif	// ! _MFC_VER

	class GRAYCORE_LINK COSHandle : protected CNonCopyable
	{
		//! @class Gray::COSHandle
		//! Wrap ownership of a OS Kernel HANDLE. (NOT a GUI or User handle)
		//! Close on destruct.
		//! Any OS system handles that might use CloseHandle()
		//! Similar to CHandle in ATL
		//! @note Don't use CHandlePtr<> because that is just for typed (not void) pointers and HANDLE is 'int' in __linux__

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
		explicit inline COSHandle(HANDLE h = INVALID_HANDLE_VALUE) noexcept
		: m_h(h)
		{
		}

		COSHandle(const COSHandle& Handle) noexcept
		: m_h(Handle.Duplicate())
		{
		}
		COSHandle& operator=(const COSHandle& Handle)
		{
			if (m_h != Handle.m_h)
			{
				AttachHandle(Handle.Duplicate());
			}
			return (*this);
		}

		inline ~COSHandle()
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
		void OpenHandle( const char* pszPath, UINT uFlags, UINT uMode=0)
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
			DWORD nLengthWritten;
			bool bRet = ::WriteFile(m_h, pData, (DWORD)nDataSize, &nLengthWritten, nullptr);
			if (!bRet)
#elif defined(__linux__)
			int nLengthWritten = ::write(m_h, (const char*)pData, (long)nDataSize);
			if (nLengthWritten <= 0)
#endif
			{
				return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));	// E_HANDLE = file handle m_h is bad. 
			}
			return (HRESULT) nLengthWritten;
		}

		HRESULT ReadX(void* pData, size_t nDataSize) const
		{
			//! @return HRESULT_WIN32_C(ERROR_HANDLE_EOF) = EOF.
			if (nDataSize <= 0)
				return S_OK;
#ifdef _WIN32
			DWORD nLengthRead;
			bool bRet = ::ReadFile(m_h, pData, (DWORD)nDataSize, &nLengthRead, nullptr);
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
			return (HRESULT) nLengthRead;
		}

		HRESULT FlushX() const
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

		STREAM_SEEKRET_t Seek(STREAM_OFFSET_t lOffset, SEEK_ORIGIN_TYPE eSeekOrigin) const
		{
			//! Change or get the current file position pointer.
			//! @arg eSeekOrigin = // SEEK_SET ?
			//! @return the New position, -1=FAILED
#ifdef _WIN32
#ifdef USE_FILE_POS64
			LARGE_INTEGER NewFilePointer;
			NewFilePointer.QuadPart = lOffset;
			const bool bRet = ::SetFilePointerEx(m_h, NewFilePointer, &NewFilePointer, eSeekOrigin);
			if (!bRet)
			{
				return((STREAM_POS_t)-1);
			}
			return((STREAM_POS_t)NewFilePointer.QuadPart);
#else
			return ::SetFilePointer(m_h, (LONG)lOffset, nullptr, eSeekOrigin);
#endif // USE_FILE_POS64
#else
			return ::lseek(m_h, lOffset, eSeekOrigin);
#endif
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

		UNITTEST_FRIEND(COSHandle);
	};

};
#endif // _INC_COSHandle_H
