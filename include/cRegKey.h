//
//! @file cRegKey.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_cRegKey_H
#define _INC_cRegKey_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cHandlePtr.h"
#include "FileName.h"
#include "HResult.h"

#ifdef _WIN32

#ifndef HKEY_LOCAL_MACHINE	//!< Not defined in normal 16 bit API
#define HKEY_LOCAL_MACHINE	((HKEY)(ULONG_PTR)((LONG)0x80000002))
#endif	// HKEY_LOCAL_MACHINE
#ifndef HKEY_CURRENT_USER
#define HKEY_CURRENT_USER	((HKEY)(ULONG_PTR)((LONG)0x80000001))
#endif	// HKEY_LOCAL_MACHINE

namespace Gray
{
	struct cRegKeyName
	{
		//! Bind a hard name to the default HKEY values.
		HKEY m_hKey; //!< e.g. HKEY_CLASSES_ROOT, HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, HKEY_USERS
		const FILECHAR_t* m_pszRegPath;	//!< e.g. _FN("SOFTWARE\\Menasoft"), nullptr = use previous in array.
	};
	class cRegKeyInit
	{
		//! @class GrayLib::cRegKeyInit
		//! Declare a static starting/init value for a registry/config key.
	public:
		cRegKeyName m_Key;
		const FILECHAR_t* m_pszKeyName;	//!< value name. can be nullptr (default) or _FN("MyValue")
		void* m_pValue;		//!< (union) May be a string pointer or DWORD. depends on m_dwType
		DWORD m_dwType;		//!< REG_SZ, REG_DWORD, omitted for default key. (since default must always be a string)

	public:
		bool isEndMarker() const noexcept
		{
			return m_Key.m_hKey == HANDLEPTR_NULL;	// end of an array of values.
		}
		bool isRegValue() const noexcept
		{
			// ASSUME m_dwType is set as well.
			return m_pszKeyName != nullptr;
		}
	};

	class cRegKey
		: public cHandlePtr < HKEY >
	{
		//! @class GrayLib::cRegKey
		//! read and write to/from the Windows registry hive by its handle. advapi32.dll
		//! Similar to the MFC/ATL CRegKey. default = HKEY_LOCAL_MACHINE. ::ATL::CRegKey
		//! can use IIniBaseSetter and IIniBaseGetter ?
		//! @note Key names are not case sensitive.
		typedef cHandlePtr<HKEY> SUPER_t;

	public:
		cRegKey(HKEY hKey = HKEY_LOCAL_MACHINE) noexcept
			: cHandlePtr<HKEY>(hKey)
		{
			//! hKey = HKEY_LOCAL_MACHINE default
			//! not HKEY_CLASSES_ROOT
		}
		~cRegKey() noexcept
		{
		}

		HKEY get_HKey() const noexcept
		{
			return get_Handle();
		}
 
		static inline bool IsKeyBase(HKEY hKey) noexcept
		{
			//! is it a base HKEY_* predefined key?
			//! e.g. HKEY_CLASSES_ROOT, HKEY_LOCAL_MACHINE
			return (((size_t)hKey) & ((size_t)HKEY_CLASSES_ROOT)) == ((size_t)HKEY_CLASSES_ROOT);
		}
		bool isKeyBase() const noexcept
		{
			//! is it a base HKEY_* predefined key?
			//! e.g. HKEY_CLASSES_ROOT, HKEY_LOCAL_MACHINE
			return IsKeyBase(get_HKey());
		}

		bool isKeyOpen() const noexcept
		{
			//! was RegOpenKeyEx() used ?
			if (isKeyBase())
				return false;
			return SUPER_t::isValidHandle();
		}

		void Attach(HKEY hKey)
		{
			SUPER_t::AttachHandle(hKey);
		}
		HKEY Detach()
		{
			//! like SUPER_t::DetachHandle()
			HKEY h = get_HKey();
			ref_Handle() = HKEY_LOCAL_MACHINE;
			return h;
		}

		HRESULT Open(HKEY hKeyBase, const FILECHAR_t* pszSubKey, REGSAM samDesired = KEY_READ)
		{
			//! Open the key for reading (typically). Don't create it if not exist.
			//! @arg samDesired = KEY_READ, KEY_QUERY_VALUE, KEY_ALL_ACCESS (if i want to delete tags?)
			//! @note
			//!  m_hKey = will always be changed!!! set to HANDLE_NULL on fail
			//! @return
			//!  0 = S_OK, 2=ERROR_FILE_NOT_FOUND
			CloseHandle();
			const LSTATUS lRet = _FNF(::RegOpenKeyEx)(hKeyBase, pszSubKey, 0, samDesired, &ref_Handle());
			return HResult::FromWin32(lRet);
		}

		HRESULT OpenBase(const FILECHAR_t* pszSubKey, REGSAM samDesired = KEY_READ)
		{
			//! open sub key from base key. Replaces get_HKey which is usually a base.
			ASSERT(isKeyBase());
			return Open(get_HKey(), pszSubKey, samDesired);
		}

		HRESULT FlushX()
		{
			const LSTATUS lRet = ::RegFlushKey(get_Handle());
			return HResult::FromWin32(lRet);
		}

		// Keys

		HRESULT DeleteKey(const FILECHAR_t* pszSubKey)
		{
			//! @return 0 = S_OK, 2=ERROR_FILE_NOT_FOUND
			//! @note This is for keys not values. delete values using RegDeleteValue.
			//! this does not delete subkeys, use DeleteKeyTree().
			const LSTATUS lRet = _FNF(::RegDeleteKey)(get_HKey(), pszSubKey);
			return HResult::FromWin32(lRet);
		}

		HRESULT EnumKey(DWORD dwIndex, OUT FILECHAR_t* pszNameRet, DWORD& dwSizeName)
		{
			//! Walk the list of child keys by name for a registry key.
			//! @arg dwSizeName = the max size of the buffer and the size of the name returned.
			//! @return
			//!  0 = S_OK
			//!  ERROR_NO_MORE_ITEMS = no more entries.
			const LSTATUS lRet = _FNF(::RegEnumKeyEx)(get_HKey(), dwIndex, pszNameRet, &dwSizeName,
				nullptr, nullptr, nullptr, nullptr);
			return HResult::FromWin32(lRet);
		}

		// Values

		HRESULT EnumValue(DWORD dwIndex, FILECHAR_t* pszNameRet, DWORD& dwSizeName, DWORD* pdwTypeRet = nullptr, void* pDataRet = nullptr, DWORD* pdwSizeData = nullptr)
		{
			//! Walk the list of values for a registry key.
			//! @note strings will always be of type FILECHAR_t
			//! @return 0 = S_OK
			//!  HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) = no more entries. (0x80070103)
			const LSTATUS lRet = _FNF(::RegEnumValue)(get_HKey(), dwIndex, pszNameRet, &dwSizeName, nullptr,
				pdwTypeRet, (LPBYTE)pDataRet, pdwSizeData);
			return HResult::FromWin32(lRet);
		}
		HRESULT DeleteValue(const FILECHAR_t* pszSubKey)
		{
			//! @return 0 = S_OK, 2=ERROR_FILE_NOT_FOUND
			const LSTATUS lRet = _FNF(::RegDeleteValue)(get_HKey(), pszSubKey);
			return HResult::FromWin32(lRet);
		}
		HRESULT SetValue(const FILECHAR_t* pszValueName, DWORD dwType, const void* pData, DWORD dwDataSize)
		{
			//! Raw Write.
			//! @arg pszValueName = nullptr = default value for the key.
			//! @return 0 = S_OK
			//! @note strings will always be of type FILECHAR_t

			const LSTATUS lRet = _FNF(::RegSetValueEx)(get_HKey(), pszValueName,
				0, dwType, (LPBYTE)pData, dwDataSize);
			return HResult::FromWin32(lRet);
		}

		HRESULT QueryValue(const FILECHAR_t* pszValueName, OUT DWORD& rdwType, OUT void* pData, OUT DWORD& rdwDataSize)
		{
			//! Raw Read.
			//! @arg pszValueName = key name. nullptr = default key.
			//! @arg pData = nullptr = just return the size we need.
			//! @note strings will always be of type FILECHAR_t
			//! @return
			//!  0 = S_OK, 2=ERROR_FILE_NOT_FOUND
			//!  rdwType=REG_DWORD, REG_SZ, etc.
			LSTATUS lRet = _FNF(::RegQueryValueEx)(get_HKey(), pszValueName, nullptr, &rdwType, (LPBYTE)pData, &rdwDataSize);
			return HResult::FromWin32(lRet);
		}

		HRESULT SetValueDWORD(const FILECHAR_t* lpszValueName, DWORD dwValue)
		{
			//! set DWORD value to the registry value.
			//! @return
			//!  0 = S_OK
			ASSERT(lpszValueName != nullptr);
			return SetValue(lpszValueName, REG_DWORD, &dwValue, sizeof(DWORD));
		}

		// Helper Combos
		HRESULT OpenQuerySubKey(HKEY hKeyBase, const FILECHAR_t* pszSubKey, FILECHAR_t* pData, DWORD dwDataSize = _MAX_PATH - 1)
		{
			//! This is always a string type data.
			HRESULT hRes = Open(hKeyBase, pszSubKey, KEY_QUERY_VALUE);
			if (FAILED(hRes))
				return hRes;
			DWORD dwType = REG_SZ;	// always REG_SZ or REG_EXPAND_SZ
			return QueryValue(nullptr, dwType, pData, dwDataSize);
		}
	};

	template <> inline void cHandlePtr<HKEY>::CloseHandle(HKEY h) // static
	{
		//! ASSUME IsValidHandle(h)
		if (cRegKey::IsKeyBase(h)) // never close base keys
			return;
		::RegCloseKey(h); // ignored BOOL return.
	}
}

#endif
#endif
