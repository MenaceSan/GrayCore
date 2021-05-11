//
//! @file cAppState.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cAppState.h"
#include "StrT.h"
#include "cFileDir.h"
#include "cLogMgr.h"
#include "cOSModule.h"
#include "cExceptionAssert.h"
#include "cUnitTest.h"
#include "cRandom.h"

#if defined(_WIN32) && ! defined(UNDER_CE)
#include <shlobj.h>		// M$ documentation says this nowhere, but this is the header file for SHGetPathFromIDList shfolder.h
#elif defined(__linux__)
#include <unistd.h>		// char *getlogin(void);
#endif

namespace Gray
{
	HMODULE cAppState::sm_hInstance = HMODULE_NULL; //!< the current applications HINSTANCE handle/base address. _IMAGE_DOS_HEADER

	cStringF cAppArgs::get_ArgsStr() const noexcept
	{
		//! Unparsed Command line args as a single line/string. might be used for cOSProcess.
		//! Does not contain App.exe name.
		return m_sArguments;
	}

	ITERATE_t cAppArgs::get_ArgsQty() const noexcept
	{
		//! @return 1 = just app path. 2 = app has 1 argument value. etc.
		return m_asArgs.GetSize();
	}

	cStringF cAppArgs::GetArgEnum(ITERATE_t i) const					// command line arg.
	{
		//! Get a command line argument parsed param by index.
		//! Command line arguments honor "quoted strings" as a single argument.
		//! @arg i [0]=app path.
		//! @return
		//!  "" = end or array of args.
		return m_asArgs.GetAtCheck(i);
	}

	void cAppArgs::InitArgsArray(ITERATE_t argc, APP_ARGS_t ppszArgs)
	{
		//! set pre-parsed arguments. like normal 'c' main()
		//! m_asArgs[0] = app name.
		m_asArgs.SetSize(argc);
		for (int i = 0; i < argc; i++)
		{
			m_asArgs.SetAt(i, ppszArgs[i]);
		}
	}
	void cAppArgs::InitArgsLine(ITERATE_t argc, APP_ARGS_t ppszArgs)
	{
		// build m_sArguments
		ASSERT_N(ppszArgs != nullptr);

		// build m_sArguments from APP_ARGS_t
		m_sArguments.Empty();
		for (int i = 1; i < argc; i++)
		{
			if (i > 1)
				m_sArguments += _FN(" ");
			m_sArguments += ppszArgs[i];
		}
	}

	void cAppArgs::InitArgs2(int argc, APP_ARGS_t ppszArgs)
	{
		//! Posix, _CONSOLE or DOS style arguments. main() style init.
		//! set pre-parsed arguments from console style start. ppszArgs[0] = app path
		//! @note M$ unit tests will block arguments!

		InitArgsLine(argc, ppszArgs);
		InitArgsArray(argc, ppszArgs);
	}

	void cAppArgs::InitArgsF(const FILECHAR_t* pszCommandArgs, const FILECHAR_t* pszSep)
	{
		//! set (unparsed) m_sArguments and parse pszCommandArgs to cArrayString. Windows WinMain() style init.
		//! @arg pszCommandArgs = assumed to NOT contain the app path name.
		//! Similar to _WIN32  CommandLineToArgvW()
		//! Honor quotes.

		if (pszCommandArgs == nullptr)
			return;

		m_sArguments = pszCommandArgs;	// Raw unparsed.

		FILECHAR_t* apszArgs[k_ARG_ARRAY_MAX];	// arbitrary max.
		FILECHAR_t szNull[1];
		int iSkip = 0;

		if (pszSep == nullptr)	// do default action.
		{
			pszSep = _FN("\t ");
			iSkip = 1;
			szNull[0] = '\0';
			apszArgs[0] = szNull;		// app name to be filled in later. from cAppState, cAppImpl etc.
		}

		// skip first argument as it is typically the name of my app EXE.
		FILECHAR_t szTmp[StrT::k_LEN_MAX];
		ITERATE_t iArgsQty = StrT::ParseCmdsTmp<FILECHAR_t>(szTmp, STRMAX(szTmp), pszCommandArgs, apszArgs + iSkip, _countof(apszArgs) - iSkip, pszSep, STRP_DEF);

		InitArgsArray(iArgsQty + iSkip, apszArgs);	// skip first.
	}

	ITERATE_t cAppArgs::FindCommandArg(const FILECHAR_t* pszCommandArgFind, bool bRegex, bool bIgnoreCase) const
	{
		//! Find a command line arg as regex or ignoring case.
		//! @arg bRegex = Search for a wildcard prefix.

		const ITERATE_t iArgsQty = get_ArgsQty();
		for (ITERATE_t i = 0; i < iArgsQty; i++)
		{
			cStringF sArg = GetArgEnum(i);
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

	ITERATE_t cAppArgs::FindCommandArgs(bool bIgnoreCase, const FILECHAR_t* pszCommandArgFind, ...) const
	{
		//! Find one of several possible command line args maybe ignoring case. nullptr terminated list.
		//! @return index of the first one.

		const ITERATE_t iArgsQty = get_ArgsQty();
		for (ITERATE_t i = 0; i < iArgsQty; i++)
		{
			cStringF sArg = GetArgEnum(i);
			const FILECHAR_t* pszArg = sArg.get_CPtr();
			while (cAppArgs::IsArgSwitch(pszArg[0]))
				pszArg++;

			// Match? nullptr terminated.
			va_list vargs;
			va_start(vargs, pszCommandArgFind);
			const FILECHAR_t* pszFind = pszCommandArgFind;
			for (;;)
			{
				if (StrT::IsNullOrEmpty(pszFind))
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
				pszFind = va_arg(vargs, const FILECHAR_t*);	// next
			}
			va_end(vargs);
		}
		return k_ITERATE_BAD;
	}


	//*********************************************************

	const FILECHAR_t k_EnvironName[] = _FN(GRAY_NAMES) _FN("Core");

	cAppState::cAppState()
		: cSingleton<cAppState>(this, typeid(cAppState))
		, m_Sig(_INC_GrayCore_H, sizeof(cAppState)) // help with debug versioning and DLL usage.
		, m_nMainThreadId(cThreadId::k_NULL)
		, m_eAppState(APPSTATE_Init)
		, m_bTempDirWritable(false)
	{
		//! Cache the OS params for this process/app ?
		ASSERT(m_ThreadModuleLoading.isInit());

		if (GetEnvironStr(k_EnvironName, nullptr, 0) == 0)
		{
			// MUST not already exist !
			FILECHAR_t szValue[StrNum::k_LEN_MAX_DIGITS_INT + 2];
			StrT::ULtoA((UINT_PTR)this, szValue, STRMAX(szValue), 16);
			SetEnvironStr(k_EnvironName, szValue);		// record this globally to any consumer in the process space.
		}
	}

	cAppState::~cAppState() noexcept
	{
	}

	UINT GRAYCALL cAppState::get_LibVersion() // static
	{
		return _INC_GrayCore_H;
	}

	bool GRAYCALL cAppState::isDebuggerPresent() // static
	{
		//! @note kernel debuggers like SoftIce can fool this.
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0400) && ! defined(UNDER_CE)
		return ::IsDebuggerPresent() ? true : false;
#else
		return false;
#endif
#elif defined(__linux__)
		//! @todo __linux__ is there a debugger attached?
		return false;
#endif
	}

	bool GRAYCALL cAppState::isRemoteSession() // static
	{
		//! Should we act different if this is a remote terminal?
#ifdef _WIN32
		// Equiv to .NET System.Windows.Forms.SystemInformation.TerminalServerSession;
		return ::GetSystemMetrics(SM_REMOTESESSION);
#elif defined(__linux__)
		return false;
#endif
	}

	cStringF GRAYCALL cAppState::get_AppFilePath() // static
	{
		//! like _pgmptr in POSIX
		//! @return The full path of the app EXE now
#ifdef _WIN32
		FILECHAR_t szPath[_MAX_PATH];
		DWORD dwRetLen = _FNF(::GetModuleFileName)(HMODULE_NULL, szPath, STRMAX(szPath));
		if (dwRetLen <= 0)
		{
			return cStrConst::k_Empty.Get<FILECHAR_t>();
		}
		return cStringF(szPath, dwRetLen);
#elif defined(__linux__)
		return I().GetArgEnum(0);	// The name of the current app.
#else
#error NOOS
#endif
	}

	cStringF GRAYCALL cAppState::get_AppFileTitle() // static
	{
		//! Get the title of the app EXE file. No extension.
		return cFilePath::GetFileNameNE(cAppState::get_AppFilePath());
	}
	cStringF GRAYCALL cAppState::get_AppFileDir() // static
	{
		//! Get the directory the app EXE is in.
		return cFilePath::GetFileDir(cAppState::get_AppFilePath());
	}

	HRESULT cAppState::CheckValidSignatureI(UINT32 nGrayCoreVer, size_t nSizeofThis) const noexcept // protected
	{
		// Is the pAppEx what we think it is ? NOT inline compiled.
		// Assume cAppState is relatively stable annd wont just crash. CheckValidSignatureX was called.

		if (nGrayCoreVer != _INC_GrayCore_H)	// check this again in the compiled version.
		{
			// My *Core DLL is not the correct version 
			DEBUG_ERR(("cAppState nGrayCoreVer"));
			return HRESULT_WIN32_C(ERROR_PRODUCT_VERSION);
		}

		if (!m_Sig.IsValidSignature(nGrayCoreVer, nSizeofThis))
		{
			// Something is wrong. No idea.
			DEBUG_ERR(("cAppState ! IsValidSignature"));
			return HRESULT_WIN32_C(ERROR_INTERNAL_ERROR);
		}

		FILECHAR_t szValue[StrNum::k_LEN_MAX_DIGITS_INT + 2];
		const StrLen_t len = GetEnvironStr(k_EnvironName, szValue, STRMAX(szValue));		// record this globally to any consumer in the process space.
		UNREFERENCED_PARAMETER(len);

		const UINT_PTR uVal = StrT::toUP<FILECHAR_t>(szValue, nullptr, 16);
		if (uVal != (UINT_PTR)this)
		{
			// Mix of GRAY_STATICLIB and DLL linkage is not allowed.
			DEBUG_ERR(("cAppState Mix of GRAY_STATICLIB and DLL linkage is not allowed"));
			return HRESULT_WIN32_C(ERROR_INTERNAL_ERROR);
		}

		return S_OK;	// good.
	}

	APPSTATE_TYPE_ GRAYCALL cAppState::GetAppState() // static
	{
		if (isSingleCreated())
		{
			return I().get_AppState();
		}
		else
		{
			return APPSTATE_Exit;
		}
	}
	void cAppState::InitAppState() noexcept
	{
		//! The main app thread has started. often called by cAppStateMain or via InitInstance() in _MFC_VER.
		this->m_nMainThreadId = cThreadId::GetCurrentId();
		put_AppState(APPSTATE_Run);
	}

	GRAYCORE_LINK bool GRAYCALL cAppState::isInCInit() // static
	{
		//! Indicate the process/app is currently initializing static variables. not yet reached main()
		//! Also set for a thread loading a DLL/SO.
		cAppState& app = I();
		APPSTATE_TYPE_ eAppState = app.m_eAppState;
		if (eAppState == APPSTATE_Init)
			return true;
		if (app.m_ThreadModuleLoading.GetData())
		{
			return true;	// this thread is in init loading a DLL/SO.
		}
		return false;
	}
	GRAYCORE_LINK bool GRAYCALL cAppState::isAppRunning() // static
	{
		//! Not in static init or destruct.
		//! Indicate the process/app is DONE initializing static variables. 
		//! Thought it may be setting up or tearing down. Almost exit.
		//! Use cAppStateMain inmain;
		APPSTATE_TYPE_ eAppState = I().m_eAppState;
		return eAppState == APPSTATE_RunInit || eAppState == APPSTATE_Run || eAppState == APPSTATE_RunExit;
	}
	GRAYCORE_LINK bool GRAYCALL cAppState::isAppStateRun() // static
	{
		//! the process/app is in APPSTATE_Run?
		//! Use cAppStateMain inmain;
		APPSTATE_TYPE_ eAppState = I().m_eAppState;
		return eAppState == APPSTATE_Run;
	}
	GRAYCORE_LINK bool GRAYCALL cAppState::isInCExit() // static
	{
		//! is the app exiting right now ? outside main()
		//! extern "C" int _C_Termination_Done; // undocumented C runtime variable - set to true during auto-finalization
		//! return _C_Termination_Done;	// undocumented symbol is not good in DLL.
		//! @note _C_Termination_Done wouldn't work properly in a DLL.
		if (!isSingleCreated())
			return true;
		APPSTATE_TYPE_ eAppState = I().m_eAppState;
		return eAppState == APPSTATE_Exit;
	}

	StrLen_t GRAYCALL cAppState::GetEnvironStr(const FILECHAR_t* pszVarName, FILECHAR_t* pszValue, StrLen_t iLenMax) noexcept	// static
	{
		//! Get a named environment variable by name.
		//! @arg iLenMax = 0 just return the length i would have needed.
		//! @return
		//!  pszValue = the output value for the pszVarName. nullptr = i just wanted the length.
		//!  Length of the pszValue string. 0 = none. 
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

	cStringF GRAYCALL cAppState::GetEnvironStr(const FILECHAR_t* pszVarName) noexcept	// static
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
			return _FN("");
		}
		return szValue;
#elif defined(__linux__)
		return ::getenv(pszVarName);
#endif
	}

#if 0
	cStringF cAppState::ExpandEnvironmentString()
	{
		//! Expand things like %PATH% in Environment strings or REG_EXPAND_SZ
		//!

		return ::ExpandEnvironmentStrings();
	}
#endif

	ITERATE_t GRAYCALL cAppState::GetEnvironArray(cArrayString<FILECHAR_t>& a) // static
	{
		//! Get the full block of environ strings for this process.
		//! similar to cVarMap or cIniSectionData
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

	bool cAppState::SetEnvironStr(const FILECHAR_t* pszVarName, const FILECHAR_t* pszVal) noexcept // static
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

	StrLen_t GRAYCALL cAppState::GetCurrentDir(FILECHAR_t* pszDir, StrLen_t iSizeMax) // static
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
		const DWORD dwRetLen = _FNF(::GetCurrentDirectory)(iSizeMax - 1, pszDir);
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

	cStringF GRAYCALL cAppState::get_CurrentDir() // static
	{
		//! @return the current directory path for the process.
		FILECHAR_t szPath[_MAX_PATH];
		StrLen_t iLen = GetCurrentDir(szPath, STRMAX(szPath));
		if (iLen <= 0)
			return "";
		return szPath;
	}

	bool GRAYCALL cAppState::SetCurrentDir(const FILECHAR_t* pszDir) // static
	{
		//! set the current directory path for the current app.
		//! like chdir() or _chdir()
		//! @return true = OK
#ifdef UNDER_CE
		return false;		// Cant do this UNDER_CE.
#elif defined(_WIN32)
		return _FNF(::SetCurrentDirectory)(pszDir) ? true : false;
#elif defined(__linux__)
		const int iRet = ::chdir(pszDir);
		return iRet == 0;
#endif
	}

	cStringF cAppState::get_TempDir()
	{
		//! Get a directory i can place temporary files. ends with '\'
		//! This is decided based on the OS,User,App,
		//! Similar to _FNF(::SHGetFolderPath)(CSIDL_INTERNET_CACHE)
		//! Assume cInstallDir::IsInstallDirRestricted()

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
		cFilePath::AddFileDirSep(szTmp, iLen);
#endif

		m_sTempDir = szTmp;	// cache this.
		return m_sTempDir;
		}

	cStringF cAppState::GetTempFile(const FILECHAR_t* pszFileTitle)
	{
		//! Create a temporary file to store stuff. Make sure its not colliding.
		//! @note if pszFileTitle == nullptr then just make a new random named file.

		cStringF sTmp;
		if (pszFileTitle == nullptr)
		{
			// make up a random unused file name. ALA _WIN32 GetTempFileName()
			// TODO: Test if the file already exists? use base64 name ?
			BYTE noise[8];
			g_Rand.GetNoise(noise, sizeof(noise));	// assume InitSeed() called.

			char szNoise[(sizeof(noise) * 2) + 2];	// GetHexDigestSize
			szNoise[0] = 'T';
			StrLen_t nLen = cMem::GetHexDigest(szNoise + 1, noise, sizeof(noise));
			ASSERT(nLen == STRMAX(szNoise) - 1);

			sTmp = szNoise;
			pszFileTitle = sTmp;
		}

		// TODO: m_bTempDirWritable = Test if we can really write to get_TempDir?
		return cFilePath::CombineFilePathX(get_TempDir(), pszFileTitle);
	}

	cStringF cAppState::GetTempDir(const FILECHAR_t* pszFileDir, bool bCreate)
	{
		//! Get/Create a temporary folder in temporary folder space.
		//! @arg bCreate = create the sub directory if it doesn't already exist. 

		cStringF sTempDir = GetTempFile(pszFileDir);
		if (bCreate)
		{
			HRESULT hRes = cFileDir::CreateDirectoryX(sTempDir);
			if (FAILED(hRes))
			{
				return "";
			}
		}
		return sTempDir;
	}

	void cAppState::SetArgValid(ITERATE_t i)
	{
		m_ArgsValid.SetBit((BIT_ENUM_t)i);
	}

	cStringF cAppState::get_InvalidArgs() const
	{
		//! Get a list of args NOT marked as valid. Not IN m_ValidArgs
		cStringF sInvalidArgs;
		const ITERATE_t iArgsQty = m_Args.get_ArgsQty();
		for (ITERATE_t i = 1; i < iArgsQty; i++)
		{
			if (m_ArgsValid.IsSet((BIT_ENUM_t)i))
				continue;
			if (!sInvalidArgs.IsEmpty())
				sInvalidArgs += _FN(",");
			sInvalidArgs += GetArgEnum(i);
		}
		return sInvalidArgs;
	}

	void cAppState::InitArgsF(const FILECHAR_t* pszCommandArgs)
	{
		//! Windows style (unparsed) arguments. WinMain()
		//! Command line arguments honor "quoted strings" as a single argument.
		//! can get similar results from the win32 GetCommandLine(); (which includes the app path as arg 0)
		//! similar to _WIN32 shell32 CommandLineToArgvW( pszCommandArgs, &(dwArgc));

		cStringF sAppPath = get_AppFilePath();

		if (pszCommandArgs == nullptr)
		{
			// Get command line from the OS ? 
#ifdef _WIN32
			m_Args.InitArgsF(_FNF(::GetCommandLine)());
#elif defined(__linux__)
			// Use "/proc/self/cmdline"
			return;
#endif
		}
		else
		{
			m_Args.InitArgsF(pszCommandArgs);
		}

		m_Args.m_asArgs[0] = const_cast<FILECHAR_t*>(sAppPath.get_CPtr());
	}

	void cAppState::InitArgs2(int argc, APP_ARGS_t argv)
	{
		//! POSIX main() style init.
		//! If called by ServiceMain this might be redundant.
		m_Args.InitArgs2(argc, argv);
	}

	void GRAYCALL cAppState::AbortApp(APP_EXITCODE_t uExitCode)	// static
	{
		//! Abort the application from some place other than the main() or WinMain() fall through.
		//! Call this instead of abort() or exit() to preclude naughty libraries from exiting badly.
		//! @arg uExitCode = APP_EXITCODE_t like return from "int main()"
		//!		APP_EXITCODE_ABORT = 3 = like abort()
		if (isSingleCreated())
		{
			// cAppExitCatcher should not block this now.
			I().put_AppState(APPSTATE_Exit);
	}
#ifdef _WIN32
		::ExitProcess(uExitCode);
#elif defined(__linux__)
		::exit(uExitCode);
#endif
	}

	void GRAYCALL cAppState::SetExecutionState(bool bActiveCPU, bool bActiveGUI) // static
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

	cString GRAYCALL cAppState::GetCurrentUserName(bool bForce) // static
	{
		//! Get the current system user name for the process/app.
		//! @note Can't call this "GetUserName" because _WIN32 has a "#define" on that.
		//! @arg bForce = Read the UserName from the OS, It may change by impersonation.
		//! (i have this users accounts privs)

		cAppState* pThis = cAppState::get_Single();
		ASSERT_N(pThis != nullptr);
		if (!bForce && !pThis->m_sUserName.IsEmpty())	// cached name,.
		{
			return pThis->m_sUserName;
	}

#if defined(_WIN32)
		GChar_t szUserName[256];
		DWORD dwLength = STRMAX(szUserName);	// ::GetUserName
#if defined(UNDER_CE)
		if (!_GTN(::GetUserNameEx)(NameUnknown, szUserName, &dwLength))
#else
		if (!_GTN(::GetUserName)(szUserName, &dwLength))
#endif
		{
			return _GT("");
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

	bool GRAYCALL cAppState::isCurrentUserAdmin() // static
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

	cStringF GRAYCALL cAppState::GetCurrentUserDir(const FILECHAR_t* pszSubFolder, bool bCreate) // static
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
			return _FN("");
		StrLen_t iLen = StrT::Len(szPath);
#elif defined(__linux__)
		// e.g. "/home/Dennis/X"
		StrLen_t iLen = cFilePath::CombineFilePath(szPath, STRMAX(szPath), _FN(FILESTR_DirSep) _FN("home"), cAppState::GetCurrentUserName());
		HRESULT hRes = S_OK;
#endif
		if (iLen <= 0)
			return _FN("");
		if (!StrT::IsNullOrEmpty(pszSubFolder))
		{
			iLen = cFilePath::CombineFilePathA(szPath, STRMAX(szPath), iLen, pszSubFolder);
			if (bCreate)
			{
				hRes = cFileDir::CreateDirectoryX(szPath);
				if (FAILED(hRes))
					return _FN("");
			}
		}
		return szPath;
	}

	HMODULE GRAYCALL cAppState::get_HModule() // static
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
	bool cAppState::GetStatTimes(FILETIME* pKernelTime, FILETIME* pUserTime) const
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

#if USE_CRT
	cAppExitCatcher::cAppExitCatcher() : cSingletonStatic<cAppExitCatcher>(this)
	{
		::atexit(ExitCatchProc);
		// Register for SIGABRT ?? for abort() ?
	}

	cAppExitCatcher::~cAppExitCatcher()
	{
	}

	void cAppExitCatcher::ExitCatch() // virtual
	{
		//! Someone (library) called "exit()" that should not have? Does not catch "abort()".
		//! The SQL driver calls "exit()" sometimes. bastards.
		//! but this is also called legit at the application termination.

		APPSTATE_TYPE_ eAppState = cAppState::GetAppState();
		if (eAppState >= APPSTATE_Exit)
		{
			// Legit exit.
			DEBUG_MSG(("cAppExitCatcher::ExitCatch() OK", eAppState));
			// Just pass through as we are exiting anyhow.
		}
		else
		{
			// We should not be here !
			DEBUG_ERR(("cAppExitCatcher::ExitCatch() in cAppState %d redirect.", eAppState));
			cExceptionAssert::Throw("cAppExitCatcher::ExitCatch", cDebugSourceLine("unknown", "", 1));
		}
	}

	void __cdecl cAppExitCatcher::ExitCatchProc() // static
	{
		if (isSingleCreated())
		{
			cAppExitCatcher::I().ExitCatch();
		}
	}
#endif

	//*******************************************************************

#if defined(_WIN32)
	cAppStateMain::cAppStateMain(HINSTANCE hInstance, const FILECHAR_t* pszCommandArgs)
		: m_AppState(cAppState::I())
	{
		//! WinMain()
		//! Current state should be APPSTATE_Init
		ASSERT(m_AppState.m_eAppState == APPSTATE_Init);	//! Only call this once.
		m_AppState.InitAppState();	// set to APPSTATE_Run
		m_AppState.InitArgsF(pszCommandArgs);
		ASSERT(hInstance == cAppState::get_HModule());
		cAppState::sm_hInstance = hInstance;
	}
#endif
	cAppStateMain::cAppStateMain(int argc, APP_ARGS_t argv)
		: m_AppState(cAppState::I())
	{
		//! main() or _tmain()
		//! Current state should be APPSTATE_Init
		ASSERT(m_AppState.m_eAppState == APPSTATE_Init);	//! Only call this once.
		m_AppState.InitAppState();	// set to APPSTATE_Run
		m_AppState.InitArgs2(argc, argv);
	}
	}
