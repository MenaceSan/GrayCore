//
//! @file CAppState.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#include "StdAfx.h"
#include "CAppState.h"
#include "StrT.h"
#include "CFileDir.h"
#include "CLogMgr.h"
#include "COSModule.h"
#include "CExceptionAssert.h"
#include "CUnitTest.h"
#include "CRandomDef.h"
#include "SysTypes.h"	// _MAX_PATH

#if defined(_WIN32) && ! defined(UNDER_CE)
#include <shlobj.h>		// M$ documentation says this nowhere, but this is the header file for SHGetPathFromIDList shfolder.h
#elif defined(__linux__)
#include <unistd.h>		// char *getlogin(void);
#endif

namespace Gray
{
	HMODULE CAppState::sm_hInstance = HMODULE_NULL; //!< the current applications HINSTANCE handle/base address. _IMAGE_DOS_HEADER

	CStringF CAppArgs::get_ArgsStr() const
	{
		//! Unparsed Command line args as a single line/string. might be used for COSProcess.
		//! Does not contain App.exe name.
		return m_sArguments;
	}

	ITERATE_t CAppArgs::get_ArgsQty() const
	{
		//! @return 1 = just app path. 2 = app has 1 argument value. etc.
		return m_asArgs.GetSize();
	}

	CStringF CAppArgs::GetArgsEnum(ITERATE_t i) const					// command line arg.
	{
		//! Get a command line argument parsed param by index.
		//! Command line arguments honor "quoted strings" as a single argument.
		//! @arg i [0]=app path.
		//! @return
		//!  "" = end or array of args.
		return m_asArgs.GetAtCheck(i);
	}

	void CAppArgs::InitArgsInt(ITERATE_t argc, APP_ARGS_t ppszArgs)
	{
		//! set pre-parsed arguments
		//! m_asArgs[0] = app name.
		m_asArgs.SetSize(argc);
		for (int i = 0; i < argc; i++)
		{
			m_asArgs.SetAt(i, ppszArgs[i]);
		}
	}

	void CAppArgs::InitArgs2(int argc, APP_ARGS_t ppszArgs)
	{
		//! Posix, _CONSOLE or DOS style arguments. main() style init.
		//! set pre-parsed arguments from console style start. ppszArgs[0] = app path
		//! @note USE_UNITTESTS_MS will block arguments ??
		ASSERT_N(ppszArgs != nullptr);

		// build m_sArguments
		m_sArguments.Empty();
		for (int i = 1; i < argc; i++)
		{
			if (i > 1)
				m_sArguments += _GT(" ");
			m_sArguments += ppszArgs[i];
		}

		InitArgsInt(argc, ppszArgs);
	}

	void CAppArgs::InitArgsW(const FILECHAR_t* pszCommandArgs, const FILECHAR_t* pszSep)
	{
		//! set m_sArguments and parse pszCommandArgs to CArrayString. Windows WinMain() style init.
		//! @arg pszCommandArgs = assumed to NOT contain the app path name.
		//! Similar to _WIN32  CommandLineToArgvW()
		//! Honor quotes.

		if (pszCommandArgs == nullptr)
			return;

		m_sArguments = pszCommandArgs;	// Raw unparsed.

		FILECHAR_t* apszArgs[128];	// arbitrary max.
		FILECHAR_t szNull[1];
		int iSkip = 0;

		if (pszSep == nullptr)
		{
			pszSep = _FN("\t ");
			iSkip = 1;
			szNull[0] = '\0';
			apszArgs[0] = szNull;		// app name to be filled in later.
		}

		// skip first argument as it is typically the name of my app EXE.
		FILECHAR_t szTmp[StrT::k_LEN_MAX];
		ITERATE_t iArgsQty = StrT::ParseCmdsTmp<FILECHAR_t>(szTmp, STRMAX(szTmp), pszCommandArgs, apszArgs + iSkip, _countof(apszArgs) - iSkip, pszSep, STRP_DEF);

		InitArgsInt(iArgsQty + iSkip, apszArgs);	// skip first.
	}

	ITERATE_t CAppArgs::FindCommandArg(const FILECHAR_t* pszCommandArgFind, bool bRegex, bool bIgnoreCase) const
	{
		//! Find a command line arg as regex or ignoring case.
		//! @arg bRegex = Search for a wildcard prefix.

		ITERATE_t iArgsQty = get_ArgsQty();
		for (ITERATE_t i = 0; i < iArgsQty; i++)
		{
			CStringF sArg = GetArgsEnum(i);
			const FILECHAR_t* pszArg = sArg;
			while (IsArgSwitch(*pszArg))
			{
				pszArg++;
			}

			// Match?
			if (bRegex)
			{
				if (StrT::MatchRegEx(pszArg, pszCommandArgFind, bIgnoreCase) > 0)
					return i;
			}
			else if (bIgnoreCase)
			{
				if (StrT::CmpI(pszArg, pszCommandArgFind) == 0)	// match all.
					return i;
			}
			else
			{
				if (StrT::Cmp(pszArg, pszCommandArgFind) == 0)	// match all.
					return i;
			}
		}
		return k_ITERATE_BAD;
	}

	ITERATE_t CAppArgs::FindCommandArgs(bool bIgnoreCase, const FILECHAR_t* pszCommandArgFind, ...) const
	{
		//! Find one of several possible command line args maybe ignoring case. nullptr terminated list.
		//! @return index of the first one.

		ITERATE_t iArgsQty = get_ArgsQty();
		for (ITERATE_t i = 0; i < iArgsQty; i++)
		{
			CStringF sArg = GetArgsEnum(i);
			const FILECHAR_t* pszArg = sArg.get_CPtr();
			while (CAppArgs::IsArgSwitch(pszArg[0]))
				pszArg++;

			// Match? nullptr terminated.
			va_list vargs;
			va_start(vargs, pszCommandArgFind);
			for (;;)
			{
				const FILECHAR_t* pszFind = va_arg(vargs, const FILECHAR_t*);
				if (pszFind == nullptr)
					break;
				if (bIgnoreCase)
				{
					if (StrT::CmpI(pszArg, pszFind) == 0)	// match all.
						return i;
				}
				else
				{
					if (StrT::Cmp(pszArg, pszFind) == 0)	// match all.
						return i;
				}
			}
			va_end(vargs);
		}
		return k_ITERATE_BAD;
	}


	//*********************************************************

	CAppState::CAppState()
		: CSingleton<CAppState>(this, typeid(CAppState))
		, m_Sig(_INC_GrayCore_H, sizeof(CAppState)) // help with debug versioning and DLL usage.
		, m_nMainThreadId(CThreadId::k_NULL)
		, m_eAppState(APPSTATE_Init)
		, m_bTempDirWritable(false)
	{
		//! Cache the OS params for this process/app ?
		ASSERT(m_ThreadModuleLoading.isInit());
	}

	CAppState::~CAppState()
	{
	}

	UINT GRAYCALL CAppState::get_LibVersion() // static
	{
		return _INC_GrayCore_H;
	}

	bool GRAYCALL CAppState::isDebuggerPresent() // static
	{
		//! @note kernel debuggers like SoftIce can fool this.
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0400) && ! defined(UNDER_CE)
		return(::IsDebuggerPresent() ? true : false);
#else
		return false;
#endif
#elif defined(__linux__)
		//! @todo __linux__ is there a debugger attached?
		return false;
#endif
	}

	bool GRAYCALL CAppState::isRemoteSession() // static
	{
		//! Should we act different if this is a remote terminal?
#ifdef _WIN32
		// Equiv to .NET System.Windows.Forms.SystemInformation.TerminalServerSession;
		return ::GetSystemMetrics(SM_REMOTESESSION);
#elif defined(__linux__)
		return false;
#endif
	}

	CStringF GRAYCALL CAppState::get_AppFilePath() // static
	{
		//! like _pgmptr in POSIX
		//! @return The full path of the app EXE now
#ifdef _WIN32
		FILECHAR_t szPath[_MAX_PATH];
		DWORD dwRetLen = _FNF(::GetModuleFileName)(HMODULE_NULL, szPath, STRMAX(szPath));
		if (dwRetLen <= 0)
		{
			return "";
		}
		return CStringF(szPath, dwRetLen);
#elif defined(__linux__)
		return I().m_Args.GetArgsEnum(0);	// The name of the current app.
#else
#error NOOS
#endif
	}

	CStringF GRAYCALL CAppState::get_AppFileTitle() // static
	{
		//! Get the title of the app EXE file. No extension.
		return CFilePath::GetFileNameNE(CAppState::get_AppFilePath());
	}
	CStringF GRAYCALL CAppState::get_AppFileDir() // static
	{
		//! Get the directory the app EXE is in.
		return CFilePath::GetFileDir(CAppState::get_AppFilePath());
	}

	APPSTATE_TYPE_ GRAYCALL CAppState::GetAppState() // static
	{
		if (CAppState::isSingleCreated())
		{
			return CAppState::I().get_AppState();
		}
		else
		{
			return APPSTATE_Exit;
		}
	}
	void CAppState::put_AppState(APPSTATE_TYPE_ eAppState)
	{
		//! Indicate the process/app has changed state.
		//! use CAppStateModuleLoad for DLL/SO loading.
		m_eAppState = eAppState;
	}
	void CAppState::InitAppState()
	{
		//! The main app thread has started. often called by CAppStateMain or via InitInstance() in _MFC_VER.
		ASSERT(m_eAppState == APPSTATE_Init);	//! Only call this once.
		this->m_nMainThreadId = CThreadId::GetCurrentId();
		put_AppState(APPSTATE_Run);
	}

	GRAYCORE_LINK bool GRAYCALL CAppState::isInCInit() // static
	{
		//! Indicate the process/app is currently initializing static variables. not yet reached main()
		//! Also set for a thread loading a DLL/SO.
		CAppState& app = I();
		APPSTATE_TYPE_ eAppState = app.m_eAppState;
		if (eAppState == APPSTATE_Init)
			return true;
		if (app.m_ThreadModuleLoading.GetData())
		{
			return true;	// this thread is in init loading a DLL/SO.
		}
		return false;
	}
	GRAYCORE_LINK bool GRAYCALL CAppState::isAppRunning() // static
	{
		//! Not in static init or destruct.
		//! Indicate the process/app is DONE initializing static variables. 
		//! Thought it may be setting up or tearing down. Almost exit.
		//! Use CAppStateMain inmain;
		APPSTATE_TYPE_ eAppState = I().m_eAppState;
		return(eAppState == APPSTATE_RunInit || eAppState == APPSTATE_Run || eAppState == APPSTATE_RunExit);
	}
	GRAYCORE_LINK bool GRAYCALL CAppState::isAppStateRun() // static
	{
		//! the process/app is in APPSTATE_Run?
		//! Use CAppStateMain inmain;
		APPSTATE_TYPE_ eAppState = I().m_eAppState;
		return(eAppState == APPSTATE_Run);
	}
	GRAYCORE_LINK bool GRAYCALL CAppState::isInCExit() // static
	{
		//! is the app exiting right now ? outside main()
		//! extern "C" int _C_Termination_Done; // undocumented C runtime variable - set to true during auto-finalization
		//! return _C_Termination_Done;	// undocumented symbol is not good in DLL.
		//! @note _C_Termination_Done wouldn't work properly in a DLL.
		APPSTATE_TYPE_ eAppState = I().m_eAppState;
		return(eAppState == APPSTATE_Exit);
	}

	StrLen_t GRAYCALL CAppState::GetEnvironStr(const FILECHAR_t* pszVarName, FILECHAR_t* pszValue, StrLen_t iLenMax)	// static
	{
		//! Get a named environment variable by name.
		//! @return
		//!  pszValue = the value for the Tag name
		//!  Length of the pszValue string. 0 = none
#ifdef UNDER_CE
		ASSERT(0);
		return 0;	// no such thing.
#elif defined(_WIN32)
		DWORD nReturnSize = _FNF(::GetEnvironmentVariable)(pszVarName, pszValue, iLenMax);
		if (nReturnSize == 0)
		{
			return 0;	// HResult::GetLast() to get real error.
		}
		return nReturnSize;
#elif defined(__linux__)
		return StrT::CopyLen(pszValue, ::getenv(pszVarName), iLenMax);
#endif
	}

	CStringF GRAYCALL CAppState::GetEnvironStr(const FILECHAR_t* pszVarName)	// static
	{
		//! Get a named environment variable by name.
		//! @arg pszVarName = nullptr = get a list of all variable names for the process?
		//! @note
		//!  environment variables can be cascaded for System:User:Process. no way to tell which level by name.
		//! @note
		//!  environment variables are very similar to a default block AppProfile.ini or registry entry.
#ifdef _WIN32
		FILECHAR_t szValue[_MAX_PATH];
		if (GetEnvironStr(pszVarName, szValue, STRMAX(szValue)) <= 0)
		{
			return "";
		}
		return szValue;
#elif defined(__linux__)
		return ::getenv(pszVarName);
#endif
	}

#if 0
	CStringF CAppState::ExpandEnvironmentString()
	{
		//! Expand things like %PATH% in Environment strings or REG_EXPAND_SZ
		//!

		return ::ExpandEnvironmentStrings();
	}
#endif

	ITERATE_t GRAYCALL CAppState::GetEnvironArray(CArrayString<FILECHAR_t>& a) // static
	{
		//! Get the full block of environ strings for this process.
		//! similar to CVarMap or CIniSectionData
		//! Each entry is in the form "Var1=Value1"
		//! http://linux.die.net/man/7/environ
		ITERATE_t i = 0;

#ifdef UNDER_CE
		ASSERT(0);	// no such thing.
#elif defined(_WIN32)
		FILECHAR_t* pszEnv0 = _FNFW(::GetEnvironmentStrings)();
		if (pszEnv0 == nullptr)
			return 0;
		FILECHAR_t* pszEnv = pszEnv0;
		for (;; i++)
		{
			if (pszEnv[0] == '\0')
			{
				_FNF(::FreeEnvironmentStrings)(pszEnv0);
				break;
			}
			a.Add(pszEnv);
			pszEnv += StrT::Len(pszEnv) + 1;
		}
#elif defined(__linux__)
		for (;; i++)
		{
			const FILECHAR_t* pszEnv = ::environ[i];
			if (pszEnv == nullptr)
				break;
			a.Add(pszEnv);
		}
#endif
		return i;
	}

	bool CAppState::SetEnvironStr(const FILECHAR_t* pszVarName, const FILECHAR_t* pszVal) // static
	{
		//! ASSUME pszVarName is valid format.
		//! @arg pszVal = nullptr = (or "") to erase it.
#ifdef UNDER_CE
		ASSERT(0);
		return false; // no such thing
#elif defined(_WIN32)
		return _FNF(::SetEnvironmentVariable)(pszVarName, pszVal) ? true : false;
#elif defined(__linux__)
		int iErrNo;
		if (pszVal == nullptr)
		{
			iErrNo = ::unsetenv(pszVarName);	// NOT ::clearenv()
		}
		else
		{
			iErrNo = ::setenv(pszVarName, pszVal, true);
		}
		if (iErrNo == 0)
			return true;
		return false;
#endif
	}

	StrLen_t GRAYCALL CAppState::GetCurrentDir(FILECHAR_t* pszDir, StrLen_t iSizeMax) // static
	{
		//! return the current directory for the process.
		//! In __linux__ and _WIN32 the Process has a current/default directory. UNDER_CE does not.
		//! @note Windows services start with current directory = windows system directory.
		//! @return Length of the directory string.

		if (iSizeMax <= 0)
			return 0;
#if defined(UNDER_CE)
		pszDir[0] = '\0';
		return 0;	// no concept of current directory in UNDER_CE. just use the root. (or get_AppFileDir()??)
#elif defined(_WIN32)
		DWORD dwRetLen = _FNF(::GetCurrentDirectory)(iSizeMax - 1, pszDir);
		return (StrLen_t)dwRetLen;
#elif defined(__linux__)
		if (::getcwd(pszDir, iSizeMax - 1) == nullptr)
		{
			return 0;
		}
		return StrT::Len(pszDir);
#else
#error NOOS
		return 0;
#endif
	}

	CStringF GRAYCALL CAppState::get_CurrentDir() // static
	{
		//! @return the current directory path for the process.
		FILECHAR_t szPath[_MAX_PATH];
		StrLen_t iLen = GetCurrentDir(szPath, STRMAX(szPath));
		if (iLen <= 0)
			return "";
		return szPath;
	}

	bool GRAYCALL CAppState::SetCurrentDir(const FILECHAR_t* pszDir) // static
	{
		//! set the current directory path for the current app.
		//! like chdir() or _chdir()
		//! @return true = OK
#ifdef UNDER_CE
		return false;		// Cant do this UNDER_CE.
#elif defined(_WIN32)
		return _FNF(::SetCurrentDirectory)(pszDir) ? true : false;
#elif defined(__linux__)
		int iRet = ::chdir(pszDir);
		return(iRet == 0);
#endif
	}

	CStringF CAppState::get_TempDir()
	{
		//! Get a directory i can place temporary files. ends with '\'
		//! This is decided based on the OS,User,App,
		//! Similar to _FNF(::SHGetFolderPath)(CSIDL_INTERNET_CACHE)
		//! Assume CInstallDir::IsInstallDirRestricted()

		if (!m_sTempDir.IsEmpty())
		{
			return m_sTempDir;	// cached value.
		}

		FILECHAR_t szTmp[_MAX_PATH];
#ifdef UNDER_CE
		DWORD nLenRet1 = _FNF(::GetTempPath)(STRMAX(szTmp), szTmp);
		UNREFERENCED_PARAMETER(nLenRet1);

#elif defined(_WIN32)
		// GetTempPath return is PathGetShortPath() with ~1. fix that.
		FILECHAR_t szTmp1[_MAX_PATH];
		DWORD nLenRet1 = _FNF(::GetTempPath)(STRMAX(szTmp1), szTmp1);
		UNREFERENCED_PARAMETER(nLenRet1);

		DWORD nLenRet2 = _FNF(::GetLongPathName)(szTmp1, szTmp, STRMAX(szTmp));
		UNREFERENCED_PARAMETER(nLenRet2);

#elif defined(__linux__)
		StrLen_t iLen = GetEnvironStr(_FN("TMP"), szTmp, STRMAX(szTmp));
		if (iLen <= 0)
		{
			iLen = GetEnvironStr(_FN("TEMP"), szTmp, STRMAX(szTmp));
			if (iLen <= 0)
			{
				iLen = GetEnvironStr(_FN("USERPROFILE"), szTmp, STRMAX(szTmp));
				if (iLen <= 0)
				{
					iLen = StrT::CopyLen(szTmp, _FN("/tmp"), STRMAX(szTmp));	// append tmp
				}
			}
		}
		CFilePath::AddFileDirSep(szTmp, iLen);
#endif

		m_sTempDir = szTmp;	// cache this.
		return m_sTempDir;
	}

	CStringF CAppState::GetTempFile(const FILECHAR_t* pszFileTitle)
	{
		//! Create a temporary file to store stuff. Make sure its not colliding.
		//! @note if pszFileTitle == nullptr then just make a new random named file.

		CStringF sTmp;
		if (pszFileTitle == nullptr)
		{
			// make up a random unused file name. ALA _WIN32 GetTempFileName()
			// TODO: Test if the file already exists? use base64 name ?
			BYTE noise[8];
			g_Rand.GetNoise(noise, sizeof(noise));
			char szNoise[(sizeof(noise) * 2) + 1];	// GetHexDigestSize
			StrLen_t nLen = CMem::GetHexDigest(szNoise, noise, sizeof(noise));
			ASSERT(nLen == STRMAX(szNoise));
			sTmp = szNoise;
			pszFileTitle = sTmp;
		}
		// TODO: m_bTempDirWritable = Test if we can really write to it?
		return CFilePath::CombineFilePathX(get_TempDir(), pszFileTitle);
	}

	CStringF CAppState::GetTempDir(const FILECHAR_t* pszFileDir, bool bCreate)
	{
		//! Get/Create a temporary folder in temporary folder space.
		//! @arg bCreate = create the sub dir if it doesn't already exist. TODO

		CStringF sTempDir = GetTempFile(pszFileDir);
		if (bCreate)
		{
			HRESULT hRes = CFileDir::CreateDirectoryX(sTempDir);
			if (FAILED(hRes))
			{
				return "";
			}
		}
		return sTempDir;
	}

	void CAppState::SetArgValid(ITERATE_t i)
	{
		m_ArgsValid.SetBit((BIT_ENUM_t)i);
	}

	CStringF CAppState::get_InvalidArgs() const
	{
		//! Get a list of args NOT marked as valid. Not IN m_ValidArgs
		CStringF sInvalidArgs;
		ITERATE_t iArgsQty = m_Args.get_ArgsQty();
		for (ITERATE_t i = 1; i < iArgsQty; i++)
		{
			if (m_ArgsValid.IsSet((BIT_ENUM_t)i))
				continue;
			if (!sInvalidArgs.IsEmpty())
				sInvalidArgs += ",";
			sInvalidArgs += m_Args.GetArgsEnum(i);
		}
		return sInvalidArgs;
	}

	void CAppState::InitArgsW(const FILECHAR_t* pszCommandArgs)
	{
		//! Windows style (unparsed) arguments. WinMain()
		//! Command line arguments honor "quoted strings" as a single argument.
		//! can get similar results from the win32 GetCommandLine(); (which includes the app path as arg 0)
		//! similar to _WIN32 shell32 CommandLineToArgvW( pszCommandArgs, &(dwArgc));

		if (pszCommandArgs == nullptr)
		{
			// Get command line from the OS ? 
#ifdef _WIN32
			m_Args.InitArgsW(_FNF(::GetCommandLine)());
#elif defined(__linux__)
			// Use "/proc/self/cmdline"
			return;
#endif
		}
		else
		{
			m_Args.InitArgsW(pszCommandArgs);
		}

		CStringF sAppPath = get_AppFilePath();
		m_Args.m_asArgs[0] = const_cast<FILECHAR_t*>(sAppPath.get_CPtr());
	}

	void CAppState::InitArgs2(int argc, APP_ARGS_t argv)
	{
		//! POSIX main() style init.
		//! If called by ServiceMain this might be redundant.
		m_Args.InitArgs2(argc, argv);
	}

	void GRAYCALL CAppState::AbortApp(APP_EXITCODE_t uExitCode)	// static
	{
		//! Abort the application from some place other than the main() or WinMain() fall through.
		//! Call this instead of abort() or exit() to preclude naughty libraries from exiting badly.
		//! @arg uExitCode = APP_EXITCODE_t like return from "int main()"
		//!		APP_EXITCODE_ABORT = 3 = like abort()
		if (CAppState::isSingleCreated())
		{
			// CAppExitCatcher should not block this now.
			CAppState::I().put_AppState(APPSTATE_Exit);
		}
#ifdef _WIN32
		::ExitProcess(uExitCode);
#elif defined(__linux__)
		::exit(uExitCode);
#endif
	}

	void GRAYCALL CAppState::SetExecutionState(bool bActiveCPU, bool bActiveGUI) // static
	{
		//! Tell the system it should not sleep/hibernate if it is active. I have a big task to complete.
		//! try this for Vista, it will fail on XP

		UNREFERENCED_PARAMETER(bActiveGUI);
#if defined(_WIN32)
		if (bActiveCPU)
		{
			if (::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED) == 0)
			{
				// try XP variant as well just to make sure
				::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
			}
		}
		else
		{
			::SetThreadExecutionState(ES_CONTINUOUS);
		}
#elif defined(__linux__)
		// __linux__ ?

#endif
	}

	cString GRAYCALL CAppState::GetCurrentUserName(bool bForce) // static
	{
		//! Get the current system user name for the process/app.
		//! @note Can't call this "GetUserName" because _WIN32 has a "#define" on that.
		//! @arg bForce = Read the UserName from the OS, It may change by impersonation.
		//! (i have this users accounts privs)

		CAppState* pThis = CAppState::get_Single();
		ASSERT_N(pThis != nullptr);
		if (!bForce && !pThis->m_sUserName.IsEmpty())	// cached name,.
		{
			return pThis->m_sUserName;
		}

#if defined(_WIN32)
		GChar_t szUserName[256];
		DWORD dwLength = STRMAX(szUserName);	// ::GetUserName
#if defined(UNDER_CE)
		if (!_GTF(::GetUserNameEx)(NameUnknown, szUserName, &dwLength))
#else
		if (!_GTF(::GetUserName)(szUserName, &dwLength))
#endif
		{
			return "";
		}
		pThis->m_sUserName = szUserName;
#elif defined(__linux__)
		// getlogin() = the session/desktop user.
		// cuserid() in <stdio.h> for the current process user.
		// GetEnvironStr("LOGNAME"); similar to GetEnvironStr("uid")
		pThis->m_sUserName = ::getlogin();
#endif
		return pThis->m_sUserName;
	}

	bool GRAYCALL CAppState::isCurrentUserAdmin() // static
	{
		//! This routine returns 'true' if the caller's process
		//! is a member of the Administrators local group. Caller is NOT expected
		//! to be impersonating anyone and is expected to be able to open its own
		//! process and process token.
		//! @return
		//!  true - Caller has Administrators local group.
		//!  false - Caller does not have Administrators local group. --

#if defined(UNDER_CE)
		return true;
#elif defined(_WIN32)
	// _WIN32 shell has IsUserAnAdmin()
	// CAUSES PROGRAM TO NOT WORK ON WIN9X (STATIC LINKAGE TO CheckTokenMembership)
		SID_IDENTIFIER_AUTHORITY NtAuthority = { SECURITY_NT_AUTHORITY };
		PSID AdministratorsGroup;
		BOOL b = ::AllocateAndInitializeSid(
			&NtAuthority,
			2,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS,
			0, 0, 0, 0, 0, 0,
			&AdministratorsGroup);
		if (!b)
		{
			return false;
		}
		if (!::CheckTokenMembership(HANDLE_NULL, AdministratorsGroup, &b))
		{
			b = false;
		}
		::FreeSid(AdministratorsGroup);
		return(b);
#elif defined(__linux__)
		if (!StrT::CmpI<GChar_t>(GetCurrentUserName(), _GT("root")))
			return true;
		if (::geteuid() == 0)
			return true;
		// TODO __linux__ user is admin group ??
		return false;
#endif
	}

	CStringF GRAYCALL CAppState::GetCurrentUserDir(const FILECHAR_t* pszSubFolder, bool bCreate) // static
	{
		//! get a folder the user has write access to. for placing log files and such.
		//! @arg pszSubFolder = create the sub folder if necessary.

		FILECHAR_t szPath[_MAX_PATH];
#if defined(_WIN32) && ! defined(UNDER_CE)
		// from Shell32.dll
		// for Win XP e.g. C:\Documents and Settings\Dennis\Application Data\X = CSIDL_APPDATA or CSIDL_LOCAL_APPDATA
		// for Win Vista e.g. c:\Users\Dennis\Application Data\X
		// hRes = _FNF(::SHGetFolderPathAndSubDir)( HANDLE_NULL, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, pszSubFolder, szPath);
		HRESULT hRes = _FNF(::SHGetFolderPath)(HANDLE_NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath);	// ASSUME _MAX_PATH
		if (FAILED(hRes))
			return "";
		StrLen_t iLen = StrT::Len(szPath);
#elif defined(__linux__)
		// e.g. "/home/Dennis/X"
		StrLen_t iLen = CFilePath::CombineFilePath(szPath, STRMAX(szPath), _FN(FILESTR_DirSep) _FN("home"), CAppState::GetCurrentUserName());
		HRESULT hRes = S_OK;
#endif
		if (iLen <= 0)
			return "";
		if (!StrT::IsNullOrEmpty(pszSubFolder))
		{
			iLen = CFilePath::CombineFilePathA(szPath, STRMAX(szPath), iLen, pszSubFolder);
			if (bCreate)
			{
				hRes = CFileDir::CreateDirectoryX(szPath);
				if (FAILED(hRes))
					return "";
			}
		}
		return szPath;
	}

	HMODULE GRAYCALL CAppState::get_HModule() // static
	{
		//! Same as HINSTANCE Passed to app at start in _WIN32 WinMain(HINSTANCE hInstance)
		//! @return the HMODULE to the current running EXE module. for resources.
#ifdef _WIN32
		if (sm_hInstance == HMODULE_NULL)
		{
			// _IMAGE_DOS_HEADER __ImageBase
			_FNF(::GetModuleHandleEx)(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, nullptr, (HMODULE*)&sm_hInstance);
		}
#endif
		return sm_hInstance;	// Assume this is set correctly at init.
	}

#if 0
	bool CAppState::GetStatTimes(FILETIME* pKernelTime, FILETIME* pUserTime) const
	{
		//! How much time usage does this process have ? how long have i run ?
#ifdef _WIN32
		// GetProcessTimes

#elif defined(__linux__)
		// How much time has this process used ? times(struct tms *buf) would also work.
		// (1) Get the usage data structure at this moment (man getrusage)
		getrusage(0, &t);	// RUSAGE_SELF = 0
		// (2) What is the elapsed time ? - CPU time = User time + System time
		// (2a) Get the seconds
		procTime = t.ru_utime.tv_sec + t.ru_stime.tv_sec;
		// (2b) More precisely! Get the microseconds part !
		return (procTime + (t.ru_utime.tv_usec + t.ru_stime.tv_usec) * 1e-6);
#endif
	}
#endif

	//*******************************************************************

	CAppExitCatcher::CAppExitCatcher() : CSingletonStatic<CAppExitCatcher>(this)
	{
		::atexit(ExitCatchProc);
		// Register for SIGABRT ?? for abort() ?
	}

	CAppExitCatcher::~CAppExitCatcher()
	{
	}

	void CAppExitCatcher::ExitCatch() // virtual
	{
		//! Someone (library) called "exit()" that should not have? Does not catch "abort()".
		//! The SQL driver calls "exit()" sometimes. bastards.
		//! but this is also called legit at the application termination.

		APPSTATE_TYPE_ eAppState = CAppState::GetAppState();
		if (eAppState >= APPSTATE_Exit)
		{
			// Legit exit.
			DEBUG_MSG(("CAppExitCatcher::ExitCatch() OK", eAppState));
			// Just pass through as we are exiting anyhow.
		}
		else
		{
			// We should not be here !
			DEBUG_ERR(("CAppExitCatcher::ExitCatch() in CAppState %d redirect.", eAppState));
			cExceptionAssert::Throw("CAppExitCatcher::ExitCatch", CDebugSourceLine("unknown", "", 1));
		}
	}

	void __cdecl CAppExitCatcher::ExitCatchProc() // static
	{
		if (CAppExitCatcher::isSingleCreated())
		{
			CAppExitCatcher::I().ExitCatch();
		}
	}
}

//*******************************************************************

#if defined(_WIN32)
CAppStateMain::CAppStateMain(HINSTANCE hInstance, const FILECHAR_t* pszCommandArgs)
	: m_AppState(CAppState::I())
{
	//! WinMain()
	//! Current state should be APPSTATE_Init
	m_AppState.InitAppState();	// set to APPSTATE_Run
	m_AppState.InitArgsW(pszCommandArgs);
	ASSERT(hInstance == CAppState::get_HModule());
	CAppState::sm_hInstance = hInstance;
}
#endif
CAppStateMain::CAppStateMain(int argc, APP_ARGS_t argv)
	: m_AppState(CAppState::I())
{
	//! main() or _tmain()
	//! Current state should be APPSTATE_Init
	m_AppState.InitAppState();	// set to APPSTATE_Run
	m_AppState.InitArgs2(argc, argv);
}

//*******************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"
#include "CString.h"

UNITTEST_CLASS(CAppState)
{
	UNITTEST_METHOD(CAppState)
	{
		CUnitTestAppState inmain;
		CAppState& app = CAppState::I();

		CAppArgs args2;
		args2.InitArgsW(_FN(""));

		args2.InitArgsW(_FN("a b c"));

		args2.InitArgsW(_FN("a=1 b=2 c=3"));

		args2.InitArgsW(_FN("a='sdf sdf' b='123123' c='sdf sdf sdf sdf '"));

#ifdef _WIN32
		_IMAGE_DOS_HEADER* pHeader = (_IMAGE_DOS_HEADER*)app.get_HModule();			//!< the current applications instance handle/base address. _IMAGE_DOS_HEADER
		UNITTEST_TRUE(pHeader != nullptr);
#endif

		cString sUserName = app.GetCurrentUserName();
		UNITTEST_TRUE(!sUserName.IsEmpty());

		UNITTEST_TRUE(app.isAppRunning());	// CAppStateMain was called!
		DEBUG_MSG(("Arg Qty = %d", app.m_Args.get_ArgsQty()));

		for (int i = 0; i < app.m_Args.get_ArgsQty(); i++)
		{
			CStringF sArg = app.m_Args.GetArgsEnum(i);
		}

		CArrayString<FILECHAR_t> aEnv;
		app.GetEnvironArray(aEnv);
		UNITTEST_TRUE(aEnv.GetSize());
		DEBUG_MSG(("Env Qty = %d", aEnv.GetSize()));

		CStringF sCurrentDir = app.get_CurrentDir();	// just testing.
		DEBUG_MSG(("Current Dir = '%s'", LOGSTR(sCurrentDir)));

		CStringF sDirTmp = app.get_TempDir();
		UNITTEST_TRUE(sDirTmp.GetLength() > 0);
		DEBUG_MSG(("Temp Dir = '%s'", LOGSTR(sDirTmp)));
	}
};
UNITTEST_REGISTER(CAppState, UNITTEST_LEVEL_Core);
#endif
