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
    _Exit,     /// static classes are being cleaned up. destructor called.
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
    SINGLETON_IMPL(cAppState);
    friend class cAppImpl;
    friend struct cAppStateMain;
    friend class cUnitTestAppState;

 public:
    const cObjectSignature<> _Sig;                /// Used to check for compatible build/compile config and alignments. (_INC_GrayCore_H, sizeof(cAppState))
    cThreadLocalSysT<cOSModule> m_ModuleLoading;  /// any thread is currently loading a DLL/SO? isInCInit(). use cAppStateModuleLoad
    cAppArgs m_Args;                              /// Application Command line arguments. [0] = app name.
    static ::HMODULE sm_hInstance;                /// the current applications HINSTANCE handle/base address. _IMAGE_DOS_HEADER, HMODULE=HINSTANCE

 protected:
    APPSTATE_t m_eAppState = APPSTATE_t::_Init;  /// The main state of the application. use isInCInit() for loading DLL's.

    cString m_sUserName;              /// Applications assigned login/user name. Cache/Read this just once.
    cFilePath m_sTempDir;             /// Cache my temporary files directory path.
    bool m_bTempDirWritable = false;  /// I have test written to the temp directory. its good.

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
        if (nGrayCoreVer != _INC_GrayCore_H) {
            return HRESULT_WIN32_C(ERROR_PRODUCT_VERSION);  // My *Core DLL is not the correct version or packing is incorrect!
        }
        const cAppState* const pAppState = SUPER_t::get_SingleU();
        if (!cMem::IsValidApp(pAppState)) {
            return HRESULT_WIN32_C(ERROR_INTERNAL_ERROR);  // Something is wrong. No idea.
        }
        if (pAppX != nullptr && pAppX != pAppState) {
            return HRESULT_WIN32_C(ERROR_INTERNAL_ERROR);  // Mix of GRAY_STATICLIB and DLL linkage is not allowed.
        }
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
        return m_eAppState;
    }
    static APPSTATE_t GRAYCALL GetAppState() noexcept;

    /// <summary>
    /// Indicate the process/app has changed state.
    /// use cAppStateModuleLoad for DLL/SO loading.
    /// </summary>
    /// <param name="eAppState"></param>
    void put_AppState(APPSTATE_t eAppState) noexcept {
        m_eAppState = eAppState;
    }

    static bool GRAYCALL isInCInit() noexcept;

    /// <summary>
    /// Not in static init or destruct.
    /// Indicate the process/app is DONE initializing static variables.
    /// Thought it may be setting up or tearing down. Almost exit.
    /// Use cAppStateMain inmain; or cAppImpl
    /// </summary>
    static bool GRAYCALL isAppRunning() noexcept;

    static bool GRAYCALL isAppStateRun() noexcept;
    static bool GRAYCALL isInCExit() noexcept;

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
    static cString GRAYCALL GetCurrentUserName(bool bForce = false);

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
    static ITERATE_t GRAYCALL GetEnvironArray(cArrayString<FILECHAR_t>& a);
    static bool GRAYCALL SetEnvironStr(const FILECHAR_t* pszVarName, const FILECHAR_t* pszVal) noexcept;

    /// Current default directory for the app. @note UNDER_CE has no such thing. just use the root.
    static StrLen_t GRAYCALL GetCurrentDir(cSpanX<FILECHAR_t> ret);
    /// For the process. Not applicable to WINCE
    static cFilePath GRAYCALL get_CurrentDir();
    static bool GRAYCALL SetCurrentDir(const FILECHAR_t* pszDir);

    // cFilePath GetUserHomeDir();
    // cFilePath GetUserHomeAppDir();

    cFilePath get_TempDir();
    cFilePath GetTempFile(const FILECHAR_t* pszFileTitle = nullptr);
    cFilePath GetTempDir(const FILECHAR_t* pszFileDir, bool bCreate = true);

    inline cStringF GetArgEnum(ITERATE_t i) const {
        return m_Args.GetArgEnum(i);
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

    static void GRAYCALL AbortApp(APP_EXITCODE_t uExitCode = APP_EXITCODE_t::_ABORT);
    static HRESULT GRAYCALL ShowMessageBox(cString sMsg, UINT uFlags = 1);  // 1= MB_OKCANCEL
};

/// <summary>
/// Define an instance of this at the top of WinMain(), _tmain() or main() to indicate we are in the main body of the application.
/// For use with cAppImpl and by extension cAppState. This is technically a singleton but its instantiated in main()
/// e.g. cAppStateMain inmain();
/// </summary>
struct GRAYCORE_LINK cAppStateMain {
    cAppState& m_AppState;

#if defined(_WIN32)
    cAppStateMain(HINSTANCE hInstance, const FILECHAR_t* pszCommandArgs);
#endif
    /// <summary>
    /// in main() or _tmain()
    /// Current state should be APPSTATE_t::_Init
    /// </summary>
    cAppStateMain(int argc, APP_ARGS_t argv);
    ~cAppStateMain() {
        m_AppState.put_AppState(APPSTATE_t::_Exit);  // destructor should be called next.
    }
};

/// <summary>
/// Define an instance of this when loading a DLL on a given thread.
/// @note a dynamic .DLL/.SO module can load after the app is fully loaded and in any thread.
/// isInCInit() will now return the correct value for DLL static init.
/// </summary>
struct cAppStateModuleLoad {
    cOSModule* _Recursive = nullptr;
    cAppStateModuleLoad(cOSModule& mod) {
        cAppState& I = cAppState::I();
        _Recursive = I.m_ModuleLoading.GetData();  // skip recursive loads.
        if (_Recursive) return;
        I.m_ModuleLoading.PutData(&mod);
    }
    ~cAppStateModuleLoad() {
        cAppState& I = cAppState::I();
        ASSERT(I.m_ModuleLoading.GetData());
        if (_Recursive) return;
        I.m_ModuleLoading.PutData(nullptr);
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
    static void __cdecl ExitCatchProc();

 protected:
    virtual void ExitCatch();

 public:
    cAppExitCatcher();
};
#endif
}  // namespace Gray
#endif  // _INC_cAppState_H
