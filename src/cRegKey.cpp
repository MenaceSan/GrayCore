//
//! @file cRegKey.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cRegKey.h"

#ifdef __GNUC__
#define HKEY_PERFORMANCE_TEXT       (( HKEY ) (ULONG_PTR)((LONG)0x80000050) )
#define HKEY_PERFORMANCE_NLSTEXT    (( HKEY ) (ULONG_PTR)((LONG)0x80000060) )
#endif
#if(WINVER < 0x0400)
#define HKEY_CURRENT_CONFIG         (( HKEY ) (ULONG_PTR)((LONG)0x80000005) )
#define HKEY_DYN_DATA               (( HKEY ) (ULONG_PTR)((LONG)0x80000006) )
#endif


namespace Gray
{
	const cRegKeyName cRegKey ::k_aNames[] =	// map default HKEY values to names.
	{
		{ HKEY_CLASSES_ROOT, _FN("HKCR") },
		{ HKEY_CURRENT_USER, _FN("HKCU") },
		{ HKEY_LOCAL_MACHINE, _FN("HKLM") },
		{ HKEY_USERS, _FN("HKU") },
#ifndef UNDER_CE
		{ HKEY_PERFORMANCE_DATA, _FN("HKPD") },
		{ HKEY_PERFORMANCE_TEXT, _FN("HKPT") },
		{ HKEY_PERFORMANCE_NLSTEXT, _FN("HKPN") },
		{ HKEY_CURRENT_CONFIG, _FN("HKCC") },
		{ HKEY_DYN_DATA, _FN("HKDD") },
#endif
	};

	const FILECHAR_t* GRAYCALL cRegKey::GetNameBase(HKEY hKey) noexcept // static
	{
		//! Get a text name for a base registry HKEY.
		if (!IsKeyBase(hKey))
		{
			return nullptr;
		}
		for (UINT i = 0; i < _countof(k_aNames); i++)
		{
			if (hKey == k_aNames[i].m_hKey)
				return k_aNames[i].m_pszRegPath;
		}
		// NOT a valid base key.
		DEBUG_ASSERT(0, "GetNameBase");
		return _FN("H??");
	}
}
