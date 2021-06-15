//
//! @file cHandlePtr.h
//! Wrap a general handle/pointer
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cHandlePtr_H
#define _INC_cHandlePtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cNonCopyable.h"

namespace Gray
{
#define HANDLEPTR_NULL	nullptr		//!< NOT the same as an OS HANDLE, Not an int in Linux. Always void* based.

	//! MUST Implement versions of this for each _TYPE_HAND.
	//! Assume destruction or my caller will clear m_h
	template< typename _TYPE_HAND >
	static void inline CloseHandleType(_TYPE_HAND h) noexcept; // Don't use/define a default implementation! This should fail at compile time if type is not implemented explicitly.

	template< typename _TYPE_HAND, void (*_CLOSER)(_TYPE_HAND) = CloseHandleType >
	class cHandlePtr : protected cNonCopyable
	{
		//! @class Gray::cHandlePtr
		//! Generic handle/pointer that requires an open/close operation. The underlying type is a pointer more or less.
		//! @note NOT the same as cOSHandle. Might be GUI or User handle. NOT _WIN32 = CloseHandle(HANDLE).
		//! @note if The pointer is void*. We MUST supply CloseHandleType manually!
		//! Assumes handle has been declared as a pointer type with DECLARE_HANDLE() and STRICT or similar.
		//! ASSSUME It has a corresponding cHandlePtr<TYPE*>::CloseHandleType(h). So TYPE MUST NOT by void!
		//! @note this can't be used with handles that don't declare a unique type using DECLARE_HANDLE. e.g. HCERTSTORE.
		//! e.g. _WIN32 types RegCloseKey(HKEY), SC_HANDLE, HDESK, HWINSTA, FreeLibrary(HMODULE)
		//!  UnhookWindowsHookEx(HHOOK)
		//! _WIN32 http://msdn.microsoft.com/en-us/library/ms724515(VS.85).aspx


	protected:
		_TYPE_HAND m_h;	//!< nullptr or HANDLEPTR_NULL

	public:
		explicit inline cHandlePtr(_TYPE_HAND h = nullptr) noexcept
			: m_h(h)
		{
		}

		inline bool isValidHandle() const noexcept
		{
			return m_h != HANDLEPTR_NULL;
		}
		void CloseHandle() noexcept
		{
			if (!isValidHandle())
				return;
			_TYPE_HAND h = m_h;
			m_h = HANDLEPTR_NULL;
			_CLOSER(h);
		}

		inline ~cHandlePtr() noexcept
		{
			CloseHandle();
		}

		void AttachHandle(_TYPE_HAND h) noexcept
		{
			if (m_h == h)
				return;
			CloseHandle();
			m_h = h;
		}
		_TYPE_HAND DetachHandle() noexcept
		{
			_TYPE_HAND h = m_h;
			m_h = HANDLEPTR_NULL;
			return h;
		}

		inline operator _TYPE_HAND () const noexcept
		{
			return m_h;
		}
	
		inline _TYPE_HAND get_Handle() const noexcept
		{
			return m_h;
		}

		inline _TYPE_HAND& ref_Handle() noexcept
		{
			//! DANGER. Expect the caller to modify the handle. It is responsible if it leaks an old value !
			return m_h;
		}
	};
} 
#endif // _INC_cHandlePtr_H
