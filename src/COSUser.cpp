//
//! @file COSUser.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "COSUser.h"

#if defined(_WIN32) && ! defined(UNDER_CE)
#include <WinNT.h>	// TokenIntegrityLevel
#include "CSecurityAttributes.h"
#include "CAppState.h"

#ifndef SE_PRIVILEGE_REMOVED
#define SE_PRIVILEGE_REMOVED            (0X00000004L)
#endif
#if !defined(SE_GROUP_INTEGRITY) || defined(__linux__)
// Defined in Windows Vista only.
typedef struct _TOKEN_MANDATORY_LABEL
{
	SID_AND_ATTRIBUTES Label;
} TOKEN_MANDATORY_LABEL;
#define TokenIntegrityLevel ((TOKEN_INFORMATION_CLASS)25)	// normally part of enum
#endif

#ifdef __GNUC__
WINADVAPI BOOL WINAPI GetTokenInformation( IN HANDLE TokenHandle, IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT void* TokenInformation, IN DWORD TokenInformationLength,
	OUT DWORD* ReturnLength );
WINADVAPI BOOL WINAPI LogonUserA (
	IN        LPCSTR lpszUsername,
	IN    LPCSTR lpszDomain,
	IN        LPCSTR lpszPassword,
	IN        DWORD dwLogonType,
	IN        DWORD dwLogonProvider,
	OUT PHANDLE phToken
	);
WINADVAPI BOOL WINAPI LogonUserW (
	IN        LPCWSTR lpszUsername,
	IN    LPCWSTR lpszDomain,
	IN        LPCWSTR lpszPassword,
	IN        DWORD dwLogonType,
	IN        DWORD dwLogonProvider,
	OUT PHANDLE phToken
	);
#endif

namespace Gray
{
	COSUserToken::COSUserToken()
	{
		//! use call LoginUser or OpenProcessToken
	}
	COSUserToken::COSUserToken(DWORD dwDesiredAccess, HANDLE hProcess)
	{
		OpenProcessToken(dwDesiredAccess, hProcess);
	}

	HRESULT COSUserToken::OpenProcessToken(DWORD dwDesiredAccess, HANDLE hProcess)
	{
		//! Open the current process token for the current process user.
		//! dwDesiredAccess = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_QUERY_SOURCE
		if (this->isValidHandle())	// already open with dwDesiredAccess ??
			return S_FALSE;
		if (!COSHandle::IsValidHandle(hProcess))
		{
			hProcess = ::GetCurrentProcess();
		}
		if (!::OpenProcessToken(hProcess, dwDesiredAccess, &ref_Handle()))
		{
			return HResult::GetLastDef();
		}
		return S_OK;
	}

	HRESULT COSUserToken::LogonUserX(const char* pszName, const char* pszPass)
	{
		//! Try to authenticate a specific user

		if (this->isValidHandle())	// already open?
			return S_FALSE;

		// Get "@domain"

		if (!_GTN(::LogonUser)(StrArg<GChar_t>(pszName), nullptr, StrArg<GChar_t>(pszPass), 0, 0, &ref_Handle()))
			return HResult::GetLastDef();
		return S_OK;
	}

	bool COSUserToken::SetPrivilege(const GChar_t* pszToken, DWORD dwAttr)
	{
		//! @arg pszToken = SE_DEBUG_NAME, SE_RESTORE_NAME, SE_BACKUP_NAME
		//! @arg dwAttr = SE_PRIVILEGE_ENABLED or SE_PRIVILEGE_REMOVED

		if (!this->isValidHandle())
		{
			HRESULT hRes = OpenProcessToken(TOKEN_ADJUST_PRIVILEGES);
			if (FAILED(hRes))
			{
				return false;
			}
		}

		LUID luidDebug;
		if (!_GTN(::LookupPrivilegeValue)(_GT(""), pszToken, &luidDebug))
		{
			return false;
		}

		TOKEN_PRIVILEGES tokenPriv;
		tokenPriv.PrivilegeCount = 1;
		tokenPriv.Privileges[0].Luid = luidDebug;
		tokenPriv.Privileges[0].Attributes = dwAttr;
		if (!::AdjustTokenPrivileges(get_Handle(), false, &tokenPriv, sizeof(tokenPriv), nullptr, nullptr))
		{
			return false;
		}
		return true;
	}

	bool COSUserToken::SetPrivilege(const FILECHAR_t* pszToken)
	{
		return SetPrivilege(pszToken, SE_PRIVILEGE_ENABLED);
	}

	bool COSUserToken::RemovePrivilege(const FILECHAR_t* pszToken)
	{
		return SetPrivilege(pszToken, SE_PRIVILEGE_REMOVED);
	}

	HRESULT COSUserToken::GetSID(CSecurityId& sid)
	{
		//! Open the current process token if not already open.
		if (!this->isValidHandle())
		{
			HRESULT hRes = OpenProcessToken(TOKEN_QUERY);
			if (FAILED(hRes))
			{
				return hRes;
			}
		}

		BYTE Buffer[256 + SECURITY_MAX_SID_SIZE];
		DWORD dwLengthNeeded = 0;
		if (!::GetTokenInformation(get_Handle(), TokenUser,
			Buffer, sizeof(Buffer), &dwLengthNeeded))
		{
			return HResult::GetLastDef();
		}

		TOKEN_USER* pTokenUser = (TOKEN_USER*)Buffer;
		if (!sid.SetSID((SID*)pTokenUser->User.Sid))
		{
			return E_FAIL;
		}

		return S_OK;
	}

	HRESULT COSUserToken::GetIntegrityLevel()
	{
		//! Get the IntegrityLevel of the current process and user.
		//! WINE and XP fails this call. so use CAppState::isUserAdmin() ??
		//! @note WINE fails this call differently than XP. 0x80070001 = ERROR_INVALID_FUNCTION
		//! @return
		//!  <= SECURITY_MANDATORY_MEDIUM_RID = Low
		//!  ERROR_INVALID_PARAMETER = XP.
		//!  ERROR_INVALID_FUNCTION = WINE.

		if (!this->isValidHandle())
		{
			HRESULT hRes = OpenProcessToken(TOKEN_QUERY | TOKEN_QUERY_SOURCE);
			if (FAILED(hRes))
			{
				return hRes;
			}
		}

		// Get the Integrity level.
		DWORD dwLengthNeeded = 0;
		if (::GetTokenInformation(get_Handle(), TokenIntegrityLevel,
			nullptr, 0, &dwLengthNeeded))
		{
			// SHOULD NOT Succeed!
			return HResult::GetLastDef();
		}

		HRESULT hRes = HResult::GetLast();
		if (hRes != HRESULT_WIN32_C(ERROR_INSUFFICIENT_BUFFER))
		{
			// ERROR_INVALID_PARAMETER = XP. only supported in Vista+
			// ERROR_INVALID_FUNCTION = WINE fails this call differently than XP. 0x80070001
			return hRes;
		}

		CHeapBlock til((size_t)dwLengthNeeded);
		TOKEN_MANDATORY_LABEL* pTIL = (TOKEN_MANDATORY_LABEL*)til.get_Data();
		if (pTIL == nullptr)
		{
			return E_OUTOFMEMORY;
		}
		if (!::GetTokenInformation(get_Handle(), TokenIntegrityLevel,
			pTIL, dwLengthNeeded, &dwLengthNeeded))
		{
			return E_NOTIMPL;	// HResult::GetLastDef()
		}

		DWORD dwIntegrityLevel = *::GetSidSubAuthority(pTIL->Label.Sid,
			(DWORD)(UCHAR)(*::GetSidSubAuthorityCount(pTIL->Label.Sid) - 1));
#ifdef _DEBUG
		if (dwIntegrityLevel < SECURITY_MANDATORY_MEDIUM_RID)
		{
			DEBUG_MSG(("SECURITY_MANDATORY_LOW_RID"));
		}
		else if (dwIntegrityLevel < SECURITY_MANDATORY_HIGH_RID)
		{
			DEBUG_MSG(("SECURITY_MANDATORY_MEDIUM_RID"));
		}
		else
		{
			DEBUG_MSG(("SECURITY_MANDATORY_HIGH_RID"));
		}
#endif
		return dwIntegrityLevel;
	}

	int COSUserToken::get_IntegrityLevel()
	{
		//! Get the IntegrityLevel of the current process user.
		//! @return
		//!  SECURITY_MANDATORY_HIGH_RID

		HRESULT hRes = GetIntegrityLevel();
		if (SUCCEEDED(hRes))
		{
			return hRes;
		}
		switch (hRes)
		{
		case E_NOTIMPL:		// weird.
		case HRESULT_WIN32_C(ERROR_INVALID_PARAMETER):	// XP
		case HRESULT_WIN32_C(ERROR_INVALID_FUNCTION):	// WINE
			return CAppState::isCurrentUserAdmin() ? SECURITY_MANDATORY_HIGH_RID : SECURITY_MANDATORY_MEDIUM_RID;
		}

		return SECURITY_MANDATORY_UNTRUSTED_RID;	// no idea.
	}

	HRESULT COSUserToken::GetStatistics(struct _TOKEN_STATISTICS* pStats)
	{
		//! get TokenStatistics for the user.

		if (!this->isValidHandle())
		{
			HRESULT hRes = OpenProcessToken(TOKEN_QUERY);
			if (FAILED(hRes))
			{
				return hRes;
			}
		}

		DWORD dwLengthNeeded = 0;
		if (!::GetTokenInformation(get_Handle(), TokenStatistics,
			pStats, sizeof(struct _TOKEN_STATISTICS), &dwLengthNeeded))
		{
			return HResult::GetLastDef();
		}

		return S_OK;
	}
}

#endif
