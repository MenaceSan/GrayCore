//
//! @file CHandlePtr.h
//! Wrap a general handle/pointer
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CHandlePtr_H
#define _INC_CHandlePtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CNonCopyable.h"

namespace Gray
{
#define HANDLEPTR_NULL	nullptr		//!< NOT the same as an OS HANDLE, Not an int in Linux. Always void* based.

	template< typename _TYPE_HAND = void* >
	class CHandlePtr : protected CNonCopyable
	{
		//! @class Gray::CHandlePtr
		//! Generic handle/pointer that requires an open/close operation. The underlying type is a pointer more or less.
		//! Assumes handle has been declared as a pointer type with DECLARE_HANDLE() and STRICT or similar.
		//! @note NOT the same as COSHandle. Might be GUI or User handle. NOT _WIN32 = CloseHandle(HANDLE).
		//! @note this can't be used with handles that don't declare a type using DECLARE_HANDLE. e.g. HCERTSTORE.
		//! e.g. _WIN32 types RegCloseKey(HKEY), SC_HANDLE, HDESK, HWINSTA, FreeLibrary(HMODULE),
		//!  UnhookWindowsHookEx(HHOOK)
		//! _WIN32 http://msdn.microsoft.com/en-us/library/ms724515(VS.85).aspx

	private:
		_TYPE_HAND m_h;	//!< nullptr or HANDLEPTR_NULL

	protected:
		void CloseHandleLast()
		{
			//! Assume destruction or my caller will clear m_h
			if (!isValidHandle())
				return;
			CloseHandle(m_h);
		}

	public:
		//! MUST Implement versions of this for each _TYPE_HAND.
		static void inline CloseHandle(_TYPE_HAND h); // Don't use/define a default implementation! This should fail at compile time if type is not implemented explicitly.

		explicit inline CHandlePtr(_TYPE_HAND h = nullptr)
		: m_h(h)
		{
		}
		inline ~CHandlePtr()
		{
			CloseHandleLast();
		}

		bool isValidHandle() const
		{
			return m_h != nullptr;
		}

		operator _TYPE_HAND () const
		{
			return m_h;
		}
		_TYPE_HAND operator -> () const
		{
			return m_h;
		}

		_TYPE_HAND get_Handle() const
		{
			return m_h;
		}
		_TYPE_HAND& ref_Handle()
		{
			return m_h;
		}

		void CloseHandle()
		{
			if (!isValidHandle())
				return;
			_TYPE_HAND h = m_h;
			m_h = nullptr;
			CloseHandle(h);
		}
		void AttachHandle(_TYPE_HAND h)
		{
			if (m_h != h)
			{
				CloseHandleLast();
				m_h = h;
			}
		}
		_TYPE_HAND DetachHandle()
		{
			_TYPE_HAND h = m_h;
			m_h = nullptr;
			return h;
		}
	};
};
#endif // _INC_CHandlePtr_H
