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

		static inline bool IsKeyBase(HKEY hKey) noexcept
		{
			//! is it a base HKEY_* predefined key?
			//! e.g. HKEY_CLASSES_ROOT, HKEY_LOCAL_MACHINE
			return (((size_t)hKey) & ((size_t)HKEY_CLASSES_ROOT)) == ((size_t)HKEY_CLASSES_ROOT);
		}
	};

	template <> inline void CloseHandleType(HKEY h) // static
	{
		//! ASSUME IsValidHandle(h)
		if (cRegKeyName::IsKeyBase(h)) // never close base keys
			return;
		::RegCloseKey(h); // ignored BOOL return.
	}

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

	class GRAYCORE_LINK cRegKey	 
		: public cHandlePtr < HKEY >
	{
		//! @class GrayLib::cRegKey
		//! read and write to/from the Windows registry hive by its handle. advapi32.dll
		//! Similar to the MFC/ATL CRegKey. default = HKEY_LOCAL_MACHINE. ::ATL::CRegKey
		//! can use IIniBaseSetter and IIniBaseGetter ?
		//! @note Key names are not case sensitive.
		typedef cHandlePtr<HKEY> SUPER_t;

	public:
		static const cRegKeyName k_aNames[];	// map default HKEY values to names.

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

 		inline bool isKeyBase() const noexcept
		{
			//! is it a base HKEY_* predefined key?
			//! e.g. HKEY_CLASSES_ROOT, HKEY_LOCAL_MACHINE
			return cRegKeyName::IsKeyBase(get_Handle());
		}

		bool isKeyOpen() const noexcept
		{
			//! was RegOpenKeyEx() used ?
			if (isKeyBase())
				return false;
			return SUPER_t::isValidHandle();
		}

 		HKEY DetachHandle() noexcept
		{
			//! like SUPER_t::DetachHandle() but use HKEY_LOCAL_MACHINE not NULL
			HKEY h = get_Handle();
			ref_Handle() = HKEY_LOCAL_MACHINE;
			return h;
		}

		static const FILECHAR_t* GRAYCALL GetNameBase(HKEY hKey) noexcept;

		const FILECHAR_t* get_NameBase() const noexcept
		{
			return GetNameBase(get_Handle());
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

		HRESULT OpenCreate(HKEY hKeyBase, const FILECHAR_t* pszSubKey, DWORD dwOptions = REG_OPTION_NON_VOLATILE,
			REGSAM samDesired = KEY_ALL_ACCESS, SECURITY_ATTRIBUTES* pSa = nullptr)
		{
			//! Open the key for writing. Create it if it does not exist.
			//! @arg hKeyBase = HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER
			//! @arg dwOptions = REG_OPTION_NON_VOLATILE
			//! @arg samDesired = 0
			//! @arg pSa = the security attributes to be assigned to the new key i am creating.
			//! @return
			//!  0 = S_OK, 1=S_FALSE=was already existing.
			//!  E_INVALIDARGS = you used samDesired in place of dwOptions
			//!  HRESULT_WIN32_C(ERROR_INVALID_HANDLE)
			//! @note function creates all missing keys in the specified path.

			DWORD dwDisposition = 0;
			// bool bRetried = false;	do_retry:
			CloseHandle();
			const LSTATUS lRet = _FNF(::RegCreateKeyEx)(hKeyBase, pszSubKey, 0,
				nullptr,	// class
				dwOptions,
				samDesired,
				pSa,
				&ref_Handle(),
				&dwDisposition	// pointer to return disposition.
				);
			if (lRet == NO_ERROR)
			{
				return (dwDisposition == REG_OPENED_EXISTING_KEY) ? S_FALSE : S_OK;
			}
			return HResult::FromWin32(lRet);
		}

		HRESULT OpenBase(const FILECHAR_t* pszSubKey, REGSAM samDesired = KEY_READ)
		{
			//! open sub key from base key. Replaces get_HKey which is usually a base.
			ASSERT(isKeyBase());
			return Open(get_Handle(), pszSubKey, samDesired);
		}

		HRESULT FlushX() noexcept
		{
			const LSTATUS lRet = ::RegFlushKey(get_Handle());
			return HResult::FromWin32(lRet);
		}

		// Keys

		HRESULT DeleteKey(const FILECHAR_t* pszSubKey) noexcept
		{
			//! @return 0 = S_OK, 2=ERROR_FILE_NOT_FOUND
			//! @note This is for keys not values. delete values using RegDeleteValue.
			//! this does not delete subkeys, use DeleteKeyTree().
			const LSTATUS lRet = _FNF(::RegDeleteKey)(get_Handle(), pszSubKey);
			return HResult::FromWin32(lRet);
		}

		HRESULT EnumKey(DWORD dwIndex, OUT FILECHAR_t* pszNameRet, DWORD& dwSizeName) noexcept
		{
			//! Walk the list of child keys by name for a registry key.
			//! @arg dwSizeName = the max size of the buffer and the size of the name returned.
			//! @return
			//!  0 = S_OK
			//!  ERROR_NO_MORE_ITEMS = no more entries.
			const LSTATUS lRet = _FNF(::RegEnumKeyEx)(get_Handle(), dwIndex, pszNameRet, &dwSizeName,
				nullptr, nullptr, nullptr, nullptr);
			return HResult::FromWin32(lRet);
		}

		// Values

		HRESULT EnumValue(DWORD dwIndex, FILECHAR_t* pszNameRet, DWORD& dwSizeName, DWORD* pdwTypeRet = nullptr, void* pDataRet = nullptr, DWORD* pdwSizeData = nullptr) noexcept
		{
			//! Walk the list of values for a registry key.
			//! @note strings will always be of type FILECHAR_t
			//! @return 0 = S_OK
			//!  HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) = no more entries. (0x80070103)
			const LSTATUS lRet = _FNF(::RegEnumValue)(get_Handle(), dwIndex, pszNameRet, &dwSizeName, nullptr,
				pdwTypeRet, (LPBYTE)pDataRet, pdwSizeData);
			return HResult::FromWin32(lRet);
		}
		HRESULT DeleteValue(const FILECHAR_t* pszSubKey) noexcept
		{
			//! @return 0 = S_OK, 2=ERROR_FILE_NOT_FOUND
			const LSTATUS lRet = _FNF(::RegDeleteValue)(get_Handle(), pszSubKey);
			return HResult::FromWin32(lRet);
		}
		HRESULT SetValue(const FILECHAR_t* pszValueName, DWORD dwType, const void* pData, DWORD dwDataSize) noexcept
		{
			//! Raw Write. REG_SZ must include size for '\0'.
			//! @arg pszValueName = nullptr = default value for the key.
			//! @return 0 = S_OK
			//! @note strings will always be of type FILECHAR_t

			const LSTATUS lRet = _FNF(::RegSetValueEx)(get_Handle(), pszValueName,
				0, dwType, (LPBYTE)pData, dwDataSize);
			return HResult::FromWin32(lRet);
		}

		HRESULT QueryValue(const FILECHAR_t* pszValueName, OUT DWORD& rdwType, OUT void* pData, OUT DWORD& rdwDataSize) noexcept
		{
			//! Raw Read.
			//! @arg pszValueName = key name. nullptr = default key.
			//! @arg pData = nullptr = just return the size we need.
			//! @note strings will always be of type FILECHAR_t
			//! @return
			//!  0 = S_OK, 2=ERROR_FILE_NOT_FOUND
			//!  rdwType=REG_DWORD, REG_SZ, etc.
			const LSTATUS lRet = _FNF(::RegQueryValueEx)(get_Handle(), pszValueName, nullptr, &rdwType, (LPBYTE)pData, &rdwDataSize);
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
			const HRESULT hRes = Open(hKeyBase, pszSubKey, KEY_QUERY_VALUE);
			if (FAILED(hRes))
				return hRes;
			DWORD dwType = REG_SZ;	// always REG_SZ or REG_EXPAND_SZ
			return QueryValue(nullptr, OUT dwType, pData, OUT dwDataSize);
		}
	};
}

#endif
#endif
