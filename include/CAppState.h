//
//! @file cAppState.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cAppState_H
#define _INC_cAppState_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cSingleton.h"
#include "cString.h"
#include "cObject.h"
#include "cArrayString.h"
#include "cFilePath.h"
#include "cThreadLocalSys.h"
#include "cThreadLock.h"
#include "cBits.h"
#include "cOSProcess.h"
#include "cUnitTestDecl.h"

namespace Gray
{
	UNITTEST2_PREDEF(cAppState);

	enum APPSTATE_TYPE_
	{
		//! @enum Gray::APPSTATE_TYPE_
		//! What state is the app in at the moment?
		APPSTATE_Init,		//!< static class init time. constructors called for static stuff. maybe set for single thread loading DLL dynamically.
		APPSTATE_RunInit,	//!< not static init but still init. In main() but not main loop yet. InitInstance()
		APPSTATE_Run,		//!< we are in main() main loop. Run() and OnTickApp()
		APPSTATE_RunExit,	//!< classes are being cleaned up. destructors called. Trying to exit. ExitInstance().
		APPSTATE_Exit,		//!< static classes are being cleaned up. destructors called.
		APPSTATE_QTY,
	};

	typedef const FILECHAR_t* const* APP_ARGS_t;	//!< the args passed to main() nullptr terminated array.

	class GRAYCORE_LINK cAppArgs
	{
		//! @class Gray::cAppArgs
		//! Parse and store command line args used to start an app. Handle Windows and POSIX/DOS formats.
		//! Use FILECHAR_t
		//! Like MFC CCommandLineInfo

	private:
		cStringF m_sArguments;					//!< The unparsed command line arguments. NOT including 'appname.exe'. Maybe generated as needed in get_ArgsStr(). if main() style entry.
	public:
		cArrayString<FILECHAR_t> m_asArgs;		//!< Array of parsed m_sArguments. [0]=appname.exe, [1]=first arg. NOT nullptr terminated like APP_ARGS_t. Honors quoted text.

	private:
		void InitArgsInt(ITERATE_t argc, APP_ARGS_t ppszArgs);

	public:
		static inline bool IsArgSwitch(wchar_t ch) noexcept
		{
			//! Is FILECHAR_t char 'ch' a command line switch char?
			return ((ch) == '-' || (ch) == '/');
		}

		cStringF get_ArgsStr() const noexcept;
		ITERATE_t get_ArgsQty() const noexcept;
		cStringF GetArgsEnum(ITERATE_t i) const;	//!< command line arg.

		void InitArgsF(const FILECHAR_t* pszCommandArgs, const FILECHAR_t* pszSep = nullptr);
		void InitArgs2(int argc, APP_ARGS_t argv);

		ITERATE_t FindCommandArg(const FILECHAR_t* pszCommandArg, bool bRegex = true, bool bIgnoreCase = true) const;
		ITERATE_t _cdecl FindCommandArgs(bool bIgnoreCase, const FILECHAR_t* pszCommandArgFind, ...) const;

		bool HasCommandArg(const FILECHAR_t* pszCommandArg, bool bRegex = true, bool bIgnoreCase = true) const
		{
			//! Do we have a particular argument? pszCommandArg
			ITERATE_t iRet = FindCommandArg(pszCommandArg, bRegex, bIgnoreCase);
			return (iRet >= 0);
		}
	};

	class GRAYCORE_LINK cAppState : public cSingleton<cAppState>
	{
		//! @class Gray::cAppState
		//! Singleton to track the state of the current running app/process.
		//! Don't combine this with CWinApp/cAppImpl since we may be a (dll/Shared) library or using this in static init.
		//! Track when static init is complete and when static destructors are called.
		//! @todo What desktop/session is this app and user attached to?
		//! @note Use _WIN32 CSIDL_WINDOWS etc to find special app folders.
		//! @note see CWinApp/cAppImpl for my app specialization stuff.

		friend class cSingleton < cAppState >;
		friend class cAppImpl;
		friend class cAppStateMain;
		friend class cUnitTestAppState;

	public:
		const cObjectSignature<> m_Sig;		//!< Used to check for compatible build/compile config and alignments. (_INC_GrayCore_H, sizeof(cAppState))
		cThreadLocalSysT<bool> m_ThreadModuleLoading;	//!< any thread is currently loading a DLL/SO? isInCInit(). use cAppStateModuleLoad
		cAppArgs m_Args;		//!< Application Command line arguments. [0] = app name.
		cBitmask<> m_ArgsValid;			//!< Track which command line args are valid/used in m_Args. assume any left over are not.
		static HMODULE sm_hInstance;	//!< the current applications HINSTANCE handle/base address. _IMAGE_DOS_HEADER, HMODULE=HINSTANCE

	protected:
		THREADID_t m_nMainThreadId;		//!< The thread we started with. main().
		APPSTATE_TYPE_ m_eAppState;		//!< The main state of the application. use isInCInit() for loading DLL's.

		cString m_sUserName;			//!< Applications assigned login/user name. Cache/Read this just once.
		cStringF m_sTempDir;			//!< Cache my temporary files directory path.
		bool m_bTempDirWritable;		//!< I have test written to the temp directory. its good.

	protected:
		cAppState();
		virtual ~cAppState();

	public:
		static cStringF GRAYCALL get_AppFilePath();			//!< The full path+name of the current EXE/PE.
		static cStringF GRAYCALL get_AppFileTitle();		//!< File name no Ext.
		static cStringF GRAYCALL get_AppFileDir();

		static HRESULT inline CheckValidSignature(UINT32 nGrayCoreVer, size_t nSizeofThis)
		{
			//! Is this the correct version of cAppState?
			//! Must be agreed to by all users. sizeof(cAppState) for checking alignments of structures.
			//! Ensure that some external DLL/SO caller has the same structure packing that we have.
			//! @arg nGrayCoreVer = _INC_GrayCore_H  (from the callers perspective)
			//! @arg nSizeofThis = sizeof(cAppState) (from the callers perspective)

			const cAppState* const pApp = get_SingleU();
			if (!cMem::IsValidApp(pApp))
			{
				// Something is wrong. No idea.
				return HRESULT_WIN32_C(ERROR_INTERNAL_ERROR);
			}
			if (nGrayCoreVer != _INC_GrayCore_H || !pApp->m_Sig.IsValidSignature(nGrayCoreVer, nSizeofThis))
			{
				// My *Core DLL is not the correct version or packing is incorrect!
				return HRESULT_WIN32_C(ERROR_PRODUCT_VERSION);
			}
			return S_OK;	// Things seem good.
		}

		APPSTATE_TYPE_ get_AppState() const noexcept
		{
			//! use isInCInit() for loading DLL's.
			return m_eAppState;
		}
		static APPSTATE_TYPE_ GRAYCALL GetAppState();
		void put_AppState(APPSTATE_TYPE_ eAppState) noexcept
		{
			//! Indicate the process/app has changed state.
			//! use cAppStateModuleLoad for DLL/SO loading.
			m_eAppState = eAppState;
		}
		void InitAppState() noexcept;

		THREADID_t get_MainThreadId() const noexcept
		{
			//! The thread we started with.
			return m_nMainThreadId;
		}

		static bool GRAYCALL isInCInit();
		static bool GRAYCALL isAppRunning();
		static bool GRAYCALL isAppStateRun();
		static bool GRAYCALL isInCExit();

		static bool GRAYCALL isDebuggerPresent();
		static bool GRAYCALL isRemoteSession();
		static void GRAYCALL SetExecutionState(bool bActiveCPU, bool bActiveGUI);

		static bool GRAYCALL isCurrentUserAdmin();
		static cString GRAYCALL GetCurrentUserName(bool bForce = false);
		static cStringF GRAYCALL GetCurrentUserDir(const FILECHAR_t* pszSubFolder = nullptr, bool bCreate = true);		//!< Get Root folder the user has write access to.

		static PROCESSID_t GRAYCALL get_CurrentProcessId()
		{
			//! similar to cOSProcess
#ifdef _WIN32
			return ::GetCurrentProcessId();
#elif defined(__linux__)
			return ::getpid();
#else
#error NOOS
			return PROCESSID_BAD;
#endif
		}

		static HMODULE GRAYCALL get_HModule();

		static UINT GRAYCALL get_LibVersion();			//!< _INC_GrayCore_H

		static StrLen_t GRAYCALL GetEnvironStr(const FILECHAR_t* pszVarName, FILECHAR_t* pszValue, StrLen_t iLenMax) noexcept;
		static cStringF GRAYCALL GetEnvironStr(const FILECHAR_t* pszVarName) noexcept;	//!< environment variable. from (system,user,app)
		static ITERATE_t GRAYCALL GetEnvironArray(cArrayString<FILECHAR_t>& a);
		static bool GRAYCALL SetEnvironStr(const FILECHAR_t* pszVarName, const FILECHAR_t* pszVal);

		//! Current default directory for the app. @note UNDER_CE has no such thing. just use the root.
		static StrLen_t GRAYCALL GetCurrentDir(FILECHAR_t* pszDir, StrLen_t iSizeMax);
		static cStringF GRAYCALL get_CurrentDir();	//!< For the process. Not applicable to WINCE
		static bool GRAYCALL SetCurrentDir(const FILECHAR_t* pszDir);

		// cStringF GetUserHomeDir();
		// cStringF GetUserHomeAppDir();

		cStringF get_TempDir();
		cStringF GetTempFile(const FILECHAR_t* pszFileTitle);
		cStringF GetTempDir(const FILECHAR_t* pszFileDir, bool bCreate = true);

		void SetArgValid(ITERATE_t i);
		cStringF get_InvalidArgs() const;

		void InitArgsF(const FILECHAR_t* pszCommandArgs);
		void InitArgs2(int argc, APP_ARGS_t argv);

		static void GRAYCALL AbortApp(APP_EXITCODE_t uExitCode = APP_EXITCODE_ABORT);

		CHEAPOBJECT_IMPL;
		UNITTEST_FRIEND(cAppState);
	};

	class GRAYCORE_LINK cAppStateMain
	{
		//! @class Gray::cAppStateMain
		//! Define an instance of this at the top of WinMain(), _tmain() or main() to indicate we are in the main body of the application.
		//! For use with cAppState and cAppImpl. this is technically a singleton but its instantiated in main()
		//! e.g. cAppStateMain inmain();
	public:
		cAppState& m_AppState;

	public:
#if defined(_WIN32)
		cAppStateMain(HINSTANCE hInstance, const FILECHAR_t* pszCommandArgs);
#endif
		cAppStateMain(int argc, APP_ARGS_t argv);
		~cAppStateMain() noexcept
		{
			m_AppState.put_AppState(APPSTATE_Exit);	// destructors should be called next.
		}
	};

	class cAppStateModuleLoad
	{
		//! @class Gray::cAppStateModuleLoad
		//! Define an instance of this when loading a DLL on a given thread.
		//! @note a dynamic .DLL/.SO module can load after the app is fully loaded and in any thread.
		//! isInCInit() will now return the correct value for DLL static init.
	public:
		cAppStateModuleLoad()
		{
			cAppState& I = cAppState::I();
			ASSERT(!I.m_ThreadModuleLoading.GetData());
			I.m_ThreadModuleLoading.PutData(true);
		}
		~cAppStateModuleLoad()
		{
			cAppState& I = cAppState::I();
			ASSERT(I.m_ThreadModuleLoading.GetData());
			I.m_ThreadModuleLoading.PutData(false);
		}
	};

	class GRAYCORE_LINK cAppExitCatcher : public cSingletonStatic < cAppExitCatcher >
	{
		//! @class Gray::cAppExitCatcher
		//! misbehaving libraries can call exit(). This does NOT work with abort() calls.
		//! Try to catch and block this or at least log it.
	private:
		static void __cdecl ExitCatchProc();
	protected:
		virtual void ExitCatch();
	public:
		cAppExitCatcher();
		~cAppExitCatcher();
	};
}
#endif // _INC_cAppState_H
