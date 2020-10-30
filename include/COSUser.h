//
//! @file COSUser.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_COSUser_H
#define _INC_COSUser_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "COSHandle.h"
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
	class CSecurityId;

	class GRAYCORE_LINK COSUserToken : public COSHandle
	{
		//! @class GrayLib::COSUserToken
		//! _WIN32 User access token for secure access to some system object.
		//! SetPrivilege( SE_DEBUG_NAME ) to get debug type info like COSProcess.get_CommandLine().
		//! like ATL CAccessToken
	public:
		COSUserToken();
		COSUserToken(DWORD dwDesiredAccess, HANDLE hProcess = INVALID_HANDLE_VALUE);

		HRESULT OpenProcessToken(DWORD dwDesiredAccess, HANDLE hProcess = INVALID_HANDLE_VALUE);
		HRESULT LogonUserX(const char* pszName, const char* pszPass);

		bool SetPrivilege(const GChar_t* pszToken, DWORD dwAttr);
		bool SetPrivilege(const FILECHAR_t* pszToken);
		bool RemovePrivilege(const FILECHAR_t* pszToken);

		HRESULT GetIntegrityLevel();
		int get_IntegrityLevel();

		HRESULT GetSID(CSecurityId& sid);
		HRESULT GetStatistics(struct _TOKEN_STATISTICS* pStats);
	};
#endif
}

#endif
