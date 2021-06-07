//
//! @file cOSUser.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cOSUser_H
#define _INC_cOSUser_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cOSHandle.h"
#include "FileName.h"

//! Integrity levels added to WinSDK v6.1 (Vista)
#ifndef SECURITY_MANDATORY_UNTRUSTED_RID
#define SECURITY_MANDATORY_UNTRUSTED_RID            (0x00000000L)
#define SECURITY_MANDATORY_LOW_RID                  (0x00001000L)	// Low Integrity
#define SECURITY_MANDATORY_MEDIUM_RID               (0x00002000L)	// Normal User
#define SECURITY_MANDATORY_HIGH_RID                 (0x00003000L)	// Admin
#define SECURITY_MANDATORY_SYSTEM_RID               (0x00004000L)
#define SECURITY_MANDATORY_PROTECTED_PROCESS_RID    (0x00005000L)
#endif
#if defined(_WIN32) && ! defined(UNDER_CE)
struct _TOKEN_STATISTICS;
#endif

namespace Gray 
{
#if defined(_WIN32) && ! defined(UNDER_CE)
	class cSecurityId;

	class GRAYCORE_LINK cOSUserToken : public cOSHandle
	{
		//! @class GrayLib::cOSUserToken
		//! _WIN32 User access token for secure access to some system object.
		//! SetPrivilege( SE_DEBUG_NAME ) to get debug type info like cOSProcess.get_CommandLine().
		//! like ATL CAccessToken
	public:
		cOSUserToken();
		cOSUserToken(DWORD dwDesiredAccess, HANDLE hProcess = INVALID_HANDLE_VALUE);

		HRESULT OpenProcessToken(DWORD dwDesiredAccess, HANDLE hProcess = INVALID_HANDLE_VALUE);
		HRESULT LogonUserX(const char* pszName, const char* pszPass);

		bool SetPrivilege(::LUID luidDebug, DWORD dwAttr);

		bool SetPrivilege(const wchar_t* pszToken, DWORD dwAttr = SE_PRIVILEGE_ENABLED);
		bool SetPrivilege(const char* pszToken, DWORD dwAttr = SE_PRIVILEGE_ENABLED);

		HRESULT GetIntegrityLevel();
		int get_IntegrityLevel();

		HRESULT GetSID(cSecurityId& sid);
		HRESULT GetStatistics(struct _TOKEN_STATISTICS* pStats);
	};
#endif
}

#endif
