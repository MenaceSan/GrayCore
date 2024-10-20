//! @file cAppState.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cAppState_H
#define _INC_cAppState_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cAppArgs.h"
#include "cBits.h"
#include "cOSProcess.h"
#include "cSingleton.h"
#include "cThreadLocalSys.h"
#include "cThreadLock.h"

namespace Gray {
class cOSModule;

/// <summary>
/// What state is the app in at the moment? I may be a DLL/SO.
/// </summary>
enum class APPSTATE_t : BYTE {
    _Init,     /// static class init time. constructors called for static stuff. maybe set for single thread loading DLL dynamically. main() not called yet.
    _RunInit,  /// not static init but still init. In main() but not main loop yet. InitInstance()
    _Run,      /// we are in main() main loop. Run() and OnTickApp(). called by cAppStateMain or via InitInstance()
    _RunExit,  /// classes are being cleaned up. destructor called. Trying to exit. ExitInstance().
    _Exit,     /// static classes are being cleaned up. destructor called. isInCExit()
};

typedef FILECHAR_t** APP_ARGW_t;  /// _WIN32 really defined this as LPWSTR*

/// <summary>
/// Singleton to track the state of the current running app/process.
/// Don't combine this with cWinApp/cAppImpl since we may be a (dll/Shared) library or using this in static init.
/// Track when static init is complete and when static destructor are called.
/// @todo What desktop/session is this app and user attached to?
/// @note Use _WIN32 CSIDL_WINDOWS etc to find special app folders.
/// @note see cWinApp/cAppImpl for my app specialization stuff.
/// </summary>
class GRAYCORE_LINK cAppState final : public cSingleton<cAppState> {
    typedef cSingleton<cAppState> SUPER_t;
    friend class cAppImpl;
    friend struct cAppStateMain;
    friend class cUnitTestAppState;

public:
    DECLARE_cSingleton(cAppState);

 public:
    const cObjectSignature<> _Sig;                /// Used to check for compatible build/compile config and alignments. (_INC_GrayCore_H, sizeof(cAppState))
    cThreadLocalSysT<cOSModule> _ModuleLoading;  /// any thread is currently loading a DLL/SO? isInCInit(). use cAppStateModuleLoad
    cAppArgs _Args;                              /// Application Command line arguments. [0] = app name.

    static ::HMODULE sm_hInstance;  /// the current applications HINSTANCE handle/base address. _IMAGE_DOS_HEADER, HMODULE=HINSTANCE 
    static bool sm_IsInAppExit;     /// APPSTATE_t::_Exit state is global and permanent.

 protected:
    APPSTATE_t _eAppState = APPSTATE_t::_Init;  /// The main state of the application. use isInCInit() for loading DLL's.

    cString _sUserName;              /// Applications assigned login/user name. Cache/Read this just once.
    cFilePath _sTempDir;             /// Cache my temporary files directory path.
    bool _isTempDirWritable = false;  /// I have test written to the temp directory. its good.

 protected:
    cAppState();

    HRESULT CheckValidSignatureI(UINT32 nGrayCoreVer, size_t nSizeofThis) const noexcept;

 public:
    ~cAppState() override;

    /// <summary>
    /// Get The full path+name of the current EXE/PE. like _pgmptr in POSIX
    /// </summary>
    static cFilePath GRAYCALL get_AppFilePath();

    /// <summary>
    /// Get the title of the app EXE file. File name with No extension.
    /// </summary>
    static cFilePath GRAYCALL get_AppFileTitle();

    /// <summary>
    /// Get the directory the current application EXE is in.
    /// </summary>
    static cFilePath GRAYCALL get_AppFileDir();

    /// <summary>
    /// Is this the correct version of cAppState?
    /// Must be agreed to by all code consumers. sizeof(cAppState) for checking alignments of structures.
    /// Ensure that some external DLL/SO caller has the same structure packing that we have.
    /// Make this inline code so it runs in the callers context.
    /// @note make sure Lib is not shared as both DLL and static. GetEnvironStr(GRAY_NAMES "Core") contains cAppState
    /// </summary>
    /// <param name="nGrayCoreVer">_INC_GrayCore_H  (from the callers perspective)</param>
    /// <param name="nSizeofThis">sizeof(cAppState) (from the callers perspective) for structure packing check.</param>
    /// <param name="pAppX"></param>
    /// <returns></returns>
    static HRESULT inline CheckValidSignatureX(UINT32 nGrayCoreVer, size_t nSizeofThis, const cAppState* pAppX) noexcept {
        if (nGrayCoreVer != _INC_GrayCore_H) return HRESULT_WIN32_C(ERROR_PRODUCT_VERSION);  // My *Core DLL is not the correct version or packing is incorrect!
        const cAppState* const pAppState = SUPER_t::get_SingleU();
        if (!cMem::IsValidApp(pAppState)) return HRESULT_WIN32_C(ERROR_INTERNAL_ERROR);            // Something is wrong. No idea.
        if (pAppX != nullptr && pAppX != pAppState) return HRESULT_WIN32_C(ERROR_INTERNAL_ERROR);  // Mix of GRAY_STATICLIB and DLL linkage is not allowed.
        return pAppState->CheckValidSignatureI(nGrayCoreVer, nSizeofThis);
    }

    /// <summary>
    /// Is this the correct version of cAppState?
    /// Force inline version.
    /// </summary>
    static HRESULT inline CheckValidSignatureX() noexcept {
        return CheckValidSignatureX(_INC_GrayCore_H, sizeof(cAppState), SUPER_t::get_SingleU());
    }

    /// <summary>
    /// What is the apps state? use isInCInit() for loading DLL's.
    /// </summary>
    APPSTATE_t inline get_AppState() const noexcept {
        return _eAppState;
    }
    static APPSTATE_t GRAYCALL GetAppState() noexcept;

    /// <summary>
    /// Indicate the process/app has changed state.
    /// use cAppStateModuleLoad for DLL/SO loading.
    /// </summary>
    /// <param name="eAppState"></param>
    void put_AppState(APPSTATE_t eAppState) noexcept;

    /// <summary>
    /// Indicate the process/app is currently initializing static variables. not yet reached main()
    /// Also set for a thread loading a DLL/SO.
    /// </summary>
    static bool GRAYCALL isInCInit() noexcept;

    /// <summary>
    /// Not in static init or destruct.
    /// Indicate the process/app is DONE initializing static variables.
    /// Thought it may be setting up or tearing down. Almost exit.
    /// Use cAppStateMain inmain; or cAppImpl
    /// </summary>
    static bool GRAYCALL isAppRunning() noexcept;

    static bool GRAYCALL isAppStateRun() noexcept;

    /// <summary>
    /// is the app exiting right now ? outside main(). APPSTATE_t::_Exit. e.g. Static exit.
    /// extern "C" int _C_Termination_Done; // undocumented C runtime variable - set to true during auto-finalization
    /// _C_Termination_Done symbol is not good in DLL.
    /// @note: do not (re) create singletons.
    /// </summary>
    static bool isInCExit() noexcept {
        return sm_IsInAppExit;
    }

    /// <summary>
    /// @note kernel debuggers like SoftIce can fool this.
    /// </summary>
    static bool GRAYCALL isDebuggerPresent() noexcept;

    /// <summary>
    /// Is the app being run in a remote terminal? Should we act different?
    /// </summary>
    static bool GRAYCALL isRemoteSession() noexcept;

    static void GRAYCALL SetExecutionState(bool bActiveCPU, bool bActiveGUI);

#if defined(_WIN32)
    /// <summary>
    /// Get OS type Folder. from Shell32.dll
    /// e.g. Win XP = C:\Documents and Settings\Dennis\Application Data\X = CSIDL_APPDATA or CSIDL_LOCAL_APPDATA
    /// e.g. Win Vista = c:\Users\Dennis\Application Data\X
    /// </summary>
    /// <param name="csidl"></param>
    /// <param name="pszPath">cFilePath::k_MaxLen</param>
    /// <returns></returns>
    static StrLen_t GRAYCALL GetFolderPath(int csidl, FILECHAR_t* pszPath);
#endif

    static bool GRAYCALL isCurrentUserAdmin();

    /// <summary>
    /// Get the current system user name for the process/app. (i have this users accounts privs)
    /// @note Can't name this "GetUserName" because _WIN32 has a "#define" on that.
    /// </summary>
    /// <param name="bForceRead">Read the UserName from the OS, It may change by impersonation.</param>
    /// <returns></returns>
    static cString GRAYCALL GetCurrentUserName(bool bForceRead = false);

    /// <summary>
    /// get a folder the user has write access to. for placing log files and such.
    /// </summary>
    /// <param name="pszSubFolder"></param>
    /// <param name="bCreate">create the sub folder if necessary.</param>
    /// <returns></returns>
    static cFilePath GRAYCALL GetCurrentUserDir(const FILECHAR_t* pszSubFolder = nullptr, bool bCreate = true);  /// Get Root folder the user has write access to.

    /// <summary>
    /// Get current process id. similar to cOSProcess
    /// </summary>
    /// <returns></returns>
    static PROCESSID_t inline get_CurrentProcessId() noexcept {
#ifdef _WIN32
        return ::GetCurrentProcessId();
#elif defined(__linux__)
        return ::getpid();
#else
#error NOOS
        return kPROCESSID_BAD;
#endif
    }

    /// <summary>
    /// get HINSTANCE Passed to app at start in _WIN32 WinMain(HINSTANCE hInstance)
    /// </summary>
    /// <returns>the ::HMODULE to the current running EXE module. for resources.</returns>
    static ::HMODULE GRAYCALL get_HModule() noexcept;

    static UINT GRAYCALL get_LibVersion() noexcept;  /// _INC_GrayCore_H

    /// <summary>
    /// Get a environment variable by name. e.g. %SystemRoot%
    /// </summary>
    /// <param name="pszVarName"></param>
    /// <param name="ret">the output value for the pszVarName. nullptr = i just wanted the length.</param>
    /// <returns>the length i would have needed. 0 = none.</returns>
    static StrLen_t GRAYCALL GetEnvironStr(const FILECHAR_t* pszVarName, cSpanX<FILECHAR_t> ret) noexcept;

    /// <summary>
    /// Get a named environment variable.
    /// @note environment variables can be cascaded for System:User:Process. no way to tell which level by name.
    /// @note environment variables are very similar to a default block AppProfile.ini or registry entry.
    /// </summary>
    /// <param name="pszVarName">nullptr = get a list of all variable names for the process?</param>
    /// <returns></returns>
    static cStringF GRAYCALL GetEnvironStr(const FILECHAR_t* pszVarName) noexcept;  /// environment variable. from (system,user,app)

    /// <summary>
    /// Get the full block of environ strings for this process. similar to cVarMap or cIniSectionData.
    /// Each entry is in the form "Var1=Value1".
    /// http://linux.die.net/man/7/environ
    /// </summary>
    /// <param name="a"></param>
    /// <returns></returns>
    static ITERATE_t GRAYCALL GetEnvironArray(cArrayString<FILECHAR_t>& a);

    /// <summary>
    /// Set a single env var for this process.
    /// </summary>
    /// <param name="pszVarName">ASSUME pszVarName is valid format.</param>
    /// <param name="pszVal">nullptr = (or "") to erase it.</param>
    /// <returns></returns>
    static bool GRAYCALL SetEnvironStr(const FILECHAR_t* pszVarName, const FILECHAR_t* pszVal) noexcept;

    /// <summary>
    /// Get Current directory for the app. @note UNDER_CE has no such thing. just use the root.
    /// In __linux__ and _WIN32 the Process has a current/default directory. UNDER_CE does not.
    /// @note Windows services start with current directory = windows system directory.
    /// </summary>
    /// <param name="ret"></param>
    /// <returns>Length of the directory string.</returns>
    static StrLen_t GRAYCALL GetCurrentDir(cSpanX<FILECHAR_t> ret);

    /// <summary>
    /// Get the current directory path for the process. Not applicable to WINCE/UNDER_CE
    /// </summary>
    static cFilePath GRAYCALL get_CurrentDir();

    static bool GRAYCALL SetCurrentDir(const FILECHAR_t* pszDir);

    // cFilePath GetUserHomeDir();
    // cFilePath GetUserHomeAppDir();

    cFilePath get_TempDir();
    cFilePath GetTempFile(const FILECHAR_t* pszFileTitle = nullptr);
    cFilePath GetTempDir(const FILECHAR_t* pszFileDir, bool bCreate = true);

    inline cStringF GetArgEnum(ITERATE_t i) const {
        return _Args.GetArgEnum(i);
    }

    /// <summary>
    /// Init via Windows/WinMain() style (unparsed) arguments.
    /// Command line arguments honor "quoted strings" as a single argument.
    /// can get similar results from the win32 GetCommandLine() (which includes the app path as arg 0)
    /// similar to _WIN32 shell32 CommandLineToArgvW( pszCommandArgs, dwArgc );
    /// </summary>
    /// <param name="pszCommandArgs"></param>
    void InitArgsWin(const FILECHAR_t* pszCommandArgs);

    void InitArgsPosix(int argc, APP_ARGS_t argv);

    /// <summary>
    /// Abort the application from some place other than the main() or WinMain() fall through.
    /// Call this instead of abort() or exit() directly to preclude naughty libraries from exiting badly.
    /// @note: throw from noexcept code or from destructor will still abort even though we block that.
    /// </summary>
    /// <param name="uExitCode">APP_EXITCODE_t like return from "int main()". APP_EXITCODE_t::_ABORT = 3 = like c()</param>
    /// <returns></returns>
    static void GRAYCALL AbortApp(APP_EXITCODE_t uExitCode = APP_EXITCODE_t::_ABORT);
    static HRESULT GRAYCALL ShowMessageBox(cString sMsg, UINT uFlags = 1);  // 1= MB_OKCANCEL
};

/// <summary>
/// Define an instance of this at the top of WinMain(), _tmain() or main() to indicate we are in the main body of the application.
/// For use with cAppImpl and by extension cAppState. This is technically a singleton but its instantiated in main()
/// e.g. cAppStateMain inmain();
/// </summary>
struct GRAYCORE_LINK cAppStateMain {
    cAppState& _AppState;

#if defined(_WIN32)
    cAppStateMain(HINSTANCE hInstance, const FILECHAR_t* pszCommandArgs);
#endif
    /// <summary>
    /// in main() or _tmain()
    /// Current state should be APPSTATE_t::_Init
    /// </summary>
    cAppStateMain(int argc, APP_ARGS_t argv);
    ~cAppStateMain() {
        _AppState.put_AppState(APPSTATE_t::_Exit);  // destructor should be called next.
    }
};

/// <summary>
/// Define an instance of this when loading a DLL on a given thread.
/// @note a dynamic .DLL/.SO module can load after the app is fully loaded and in any thread.
/// isInCInit() will now return the correct value for DLL static init.
/// </summary>
struct cAppStateModuleLoad {
    cOSModule* _Recursive = nullptr;
    cAppStateModuleLoad(cOSModule& mod) noexcept {
        cAppState& I = cAppState::I();
        _Recursive = I._ModuleLoading.GetData();  // skip recursive loads.
        if (_Recursive) return;
        I._ModuleLoading.PutData(&mod);
    }
    ~cAppStateModuleLoad() {
        cAppState& I = cAppState::I();
        DEBUG_CHECK(I._ModuleLoading.GetData());
        if (_Recursive) return;
        I._ModuleLoading.PutData(nullptr);
    }
};

#if USE_CRT
/// <summary>
/// misbehaving libraries can call exit(). This does NOT work with abort() calls.
/// Try to catch and block this or at least log it.
/// @NOTE uncaught exceptions and throws in destructor can exit the application .
/// </summary>
class GRAYCORE_LINK cAppExitCatcher final : public cSingletonStatic<cAppExitCatcher> {
    typedef cSingletonStatic<cAppExitCatcher> SUPER_t;
    DECLARE_cSingletonStatic(cAppExitCatcher);
    static void __cdecl ExitCatchProc();

 protected:
    /// <summary>
    /// Someone (library) called "exit()" that should not have? Does not catch "abort()".
    /// The SQL driver calls "exit()" sometimes on errors. bastards.
    /// but is also legit called at the application termination.
    /// </summary>
    virtual void ExitCatch();

 public:
    cAppExitCatcher();
};
#endif
}  // namespace Gray
#endif  // _INC_cAppState_H
