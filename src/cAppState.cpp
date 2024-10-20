//! @file cAppState.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrT.h"
#include "cAppConsole.h"
#include "cAppState.h"
#include "cExceptionAssert.h"
#include "cFileDir.h"
#include "cHandlePtr.h"
#include "cLogMgr.h"
#include "cOSModule.h"
#include "cRandom.h"
#include "cUnitTest.h"

#if defined(_WIN32) && !defined(UNDER_CE)
#include <shlobj.h>                  // CSIDL_WINDOWS, M$ documentation says this nowhere, but this is the header file for SHGetPathFromIDList shfolder.h
#pragma comment(lib, "shell32.lib")  // SHGetFolderPath
#elif defined(__linux__)
#include <unistd.h>  // char *getlogin();
#endif

namespace Gray {
cSingletonStatic_IMPL(cAppExitCatcher);
cSingleton_IMPL(cAppState);

#ifdef _WIN32
inline void CloseHandleType_Sid(::PSID h) noexcept {
    ::FreeSid(h);
}
#endif

::HMODULE cAppState::sm_hInstance = HMODULE_NULL;
bool cAppState::sm_IsInAppExit = false;

cAppArgs::cAppArgs(const FILECHAR_t* p) {
    InitArgsLine(p, " ");
}

ITERATE_t cAppArgs::get_ArgsQty() const noexcept {
    return _aArgs.GetSize();
}
cStringF cAppArgs::GetArgEnum(ITERATE_t i) const {  // command line arg.
    return _aArgs.GetAtCheck(i);
}
ITERATE_t cAppArgs::AppendArg(const FILECHAR_t* pszCmd, bool sepEquals) {
    if (sepEquals) {
        // use "Cmd=Val" syntax? split the string into command and 1 argument ?
        // args can come from parse for = sign else get from the next arg in sequence (that isn't starting with IsSwitch(-/))
        const FILECHAR_t* pszArgEq = StrT::FindChar<FILECHAR_t>(pszCmd, '=');
        if (pszArgEq != nullptr) {
            const cStringF sCmd2(ToSpan(pszCmd, cValSpan::Diff(pszArgEq, pszCmd)));
            _aArgs.Add(sCmd2);
            pszCmd = pszArgEq + 1;
        }
    }
    return _aArgs.Add(pszCmd);
}

void cAppArgs::InitArgsArray(ITERATE_t argc, APP_ARGS_t ppszArgs, bool sepEquals) {
    //! set pre-parsed arguments _aArgs like normal 'c' main()
    //! _aArgs[0] = app name.
    _aArgs.RemoveAll();
    for (ITERATE_t i = 0; i < argc; i++) {
        AppendArg(ppszArgs[i], sepEquals);
    }
}

void cAppArgs::InitArgsPosix(int argc, APP_ARGS_t ppszArgs) {
    // build raw _sArguments from APP_ARGS_t ppszArgs.
    ASSERT_NN(ppszArgs);
    _sArguments.Empty();
    for (int i = 1; i < argc; i++) {
        if (i > 1) _sArguments += _FN(" ");
        _sArguments += ppszArgs[i];
    }
    InitArgsArray(argc, ppszArgs, true);
}

void cAppArgs::InitArgsLine(const FILECHAR_t* pszCommandArgs, const FILECHAR_t* pszSep) {
    if (pszCommandArgs == nullptr) return;

    _sArguments = pszCommandArgs;  // Raw unparsed.

    const FILECHAR_t* apszArgs[k_ARG_ARRAY_MAX];  // arbitrary max.
    FILECHAR_t szNull[1];
    int iSkip = 0;

    if (pszSep == nullptr) {  // do default WIN32 WinMain action.
        pszSep = _FN("\t ");
        iSkip = 1;
        szNull[0] = '\0';
        apszArgs[0] = szNull;  // app name to be filled in later. from cAppState::InitArgsWin, cAppImpl etc.
    }

    // skip first argument as it is typically the name of my app EXE.
    FILECHAR_t szTmp[StrT::k_LEN_Default];
    const ITERATE_t iArgsQty = StrT::ParseArrayTmp<FILECHAR_t>(TOSPAN(szTmp), pszCommandArgs, TOSPAN_LIT(apszArgs).GetSkip(iSkip), pszSep, STRP_DEF);

    InitArgsArray(iArgsQty + iSkip, apszArgs, StrChar::IsSpace(*pszSep));  // skip first?
}

ITERATE_t cAppArgs::FindCommandArg(const FILECHAR_t* pszCommandArgFind, bool bRegex, bool bIgnoreCase) const {
    const ITERATE_t iArgsQty = get_ArgsQty();
    for (ITERATE_t i = 0; i < iArgsQty; i++) {
        const cStringF sArg = GetArgEnum(i);
        const FILECHAR_t* pszArg = sArg;
        while (IsArgSwitch(*pszArg)) pszArg++;

        // Match?
        if (bRegex) {
            if (StrT::MatchRegEx(pszArg, pszCommandArgFind, bIgnoreCase) > 0) return i;
        } else if (bIgnoreCase) {
            if (StrT::CmpI(pszArg, pszCommandArgFind) == COMPARE_Equal) return i;  // match all.
        } else {
            if (StrT::Cmp(pszArg, pszCommandArgFind) == COMPARE_Equal) return i;  // match all.
        }
    }
    return k_ITERATE_BAD;
}

ITERATE_t cAppArgs::FindCommandArgs(bool bIgnoreCase, const FILECHAR_t* pszCommandArgFind, ...) const {
    const ITERATE_t iArgsQty = get_ArgsQty();
    for (ITERATE_t i = 0; i < iArgsQty; i++) {
        const cStringF sArg = GetArgEnum(i);
        const FILECHAR_t* pszArg = sArg.get_CPtr();
        while (cAppArgs::IsArgSwitch(pszArg[0])) pszArg++;

        // Match? nullptr terminated.
        va_list vargs;
        va_start(vargs, pszCommandArgFind);
        const FILECHAR_t* pszFind = pszCommandArgFind;
        for (;;) {
            if (StrT::IsNullOrEmpty(pszFind)) break;
            if (bIgnoreCase) {
                if (StrT::CmpI(pszArg, pszFind) == COMPARE_Equal) return i;  // match all.
            } else {
                if (StrT::Cmp(pszArg, pszFind) == COMPARE_Equal) return i;  // match all.
            }
            pszFind = va_arg(vargs, const FILECHAR_t*);  // next
        }
        va_end(vargs);
    }
    return k_ITERATE_BAD;
}

//*********************************************************

const FILECHAR_t k_EnvironName[] = _FN(GRAY_NAMES) _FN("Core");

cAppState::cAppState() : cSingleton<cAppState>(this), _Sig(_INC_GrayCore_H, sizeof(cAppState)) {
    //! Cache the OS params for this process/app ?
    ASSERT(_ModuleLoading.isInit());

    if (GetEnvironStr(k_EnvironName).IsEmpty()) {
        // MUST not already exist in this process space! Checks for DLL hell.
        FILECHAR_t szValue[StrNum::k_LEN_MAX_DIGITS_INT + 2];
        StrT::ULtoA(CastPtrToNum(this), TOSPAN(szValue), 16);
        SetEnvironStr(k_EnvironName, szValue);  // record this globally to any consumer in the process space.
    } else {
        // This is BAD !!! DLL HELL!
        ASSERT(0);
    }
}

cAppState::~cAppState() noexcept {}

UINT GRAYCALL cAppState::get_LibVersion() noexcept {  // static
    return _INC_GrayCore_H;
}

bool GRAYCALL cAppState::isDebuggerPresent() noexcept {  // static
#ifdef _WIN32
    return ::IsDebuggerPresent() ? true : false;
#elif defined(__linux__)
    //! @todo __linux__ is there a debugger attached?
    return false;
#endif
}

bool GRAYCALL cAppState::isRemoteSession() noexcept {  // static
#ifdef _WIN32
    // Equiv to .NET System.Windows.Forms.SystemInformation.TerminalServerSession;
    return ::GetSystemMetrics(SM_REMOTESESSION);
#elif defined(__linux__)
    return false;
#endif
}

cFilePath GRAYCALL cAppState::get_AppFilePath() {  // static
#ifdef _WIN32
    FILECHAR_t szPath[cFilePath::k_MaxLen];
    const DWORD dwRetLen = _FNF(::GetModuleFileName)(HMODULE_NULL, szPath, STRMAX(szPath));
    if (dwRetLen <= 0) return {};
    return cFilePath(ToSpan(szPath, dwRetLen));
#elif defined(__linux__)
    return I().GetArgEnum(0);  // The name of the current app.
#else
#error NOOS
#endif
}

cFilePath GRAYCALL cAppState::get_AppFileTitle() {  // static
    return cFilePath::GetFileNameNE(cAppState::get_AppFilePath().get_SpanStr());
}
cFilePath GRAYCALL cAppState::get_AppFileDir() {  // static
    return cFilePath::GetFileDir(cAppState::get_AppFilePath());
}

HRESULT cAppState::CheckValidSignatureI(UINT32 nGrayCoreVer, size_t nSizeofThis) const noexcept {  // protected
    // Is the pAppEx what we think it is ? NOT inline compiled.
    // Assume cAppState is relatively stable annd wont just crash. CheckValidSignatureX was called.

    if (nGrayCoreVer != _INC_GrayCore_H) {  // check this again in the compiled version.
        // My *Core DLL is not the correct version
        DEBUG_ERR(("cAppState nGrayCoreVer"));
        return HRESULT_WIN32_C(ERROR_PRODUCT_VERSION);
    }

    if (!_Sig.IsValidSignature(nGrayCoreVer, nSizeofThis)) {
        // Something is wrong. No idea.
        DEBUG_ERR(("cAppState ! IsValidSignature"));
        return HRESULT_WIN32_C(ERROR_INTERNAL_ERROR);
    }

    FILECHAR_t szValue[StrNum::k_LEN_MAX_DIGITS_INT + 2];
    const StrLen_t len = GetEnvironStr(k_EnvironName, TOSPAN(szValue));  // record this globally to any consumer in the process space.
    UNREFERENCED_PARAMETER(len);

    const UINT_PTR uVal = StrT::toUP<FILECHAR_t>(szValue, nullptr, 16);
    if (uVal != CastPtrToNum(this)) {
        // Mix of GRAY_STATICLIB and DLL linkage is not allowed.
        DEBUG_ERR(("cAppState Mix of GRAY_STATICLIB and DLL linkage is not allowed"));
        return HRESULT_WIN32_C(ERROR_INTERNAL_ERROR);
    }

    return S_OK;  // good.
}

APPSTATE_t GRAYCALL cAppState::GetAppState() noexcept {  // static
    if (SUPER_t::isSingleCreated()) {
        return I().get_AppState();
    } else {
        return APPSTATE_t::_Exit;  // isInCExit()
    }
}

void cAppState::put_AppState(APPSTATE_t eAppState) noexcept {
    _eAppState = eAppState;
    sm_IsInAppExit |= eAppState == APPSTATE_t::_Exit;   // isInCExit()
}

GRAYCORE_LINK bool GRAYCALL cAppState::isInCInit() noexcept {  // static
    const cAppState& app = I();
    const APPSTATE_t eAppState = app._eAppState;
    if (eAppState == APPSTATE_t::_Init) return true;
    if (app._ModuleLoading.GetData() != nullptr) return true;  // this thread is in init loading a DLL/SO.
    return false;
}

GRAYCORE_LINK bool GRAYCALL cAppState::isAppRunning() noexcept {  // static
    const APPSTATE_t eAppState = I()._eAppState;
    return eAppState == APPSTATE_t::_RunInit || eAppState == APPSTATE_t::_Run || eAppState == APPSTATE_t::_RunExit;
}

/// <summary>
/// the process/app is in APPSTATE_t::_Run? Use cAppStateMain inmain;
/// </summary>
GRAYCORE_LINK bool GRAYCALL cAppState::isAppStateRun() noexcept {  // static
    const APPSTATE_t eAppState = I()._eAppState;
    return eAppState == APPSTATE_t::_Run;
}

StrLen_t GRAYCALL cAppState::GetEnvironStr(const FILECHAR_t* pszVarName, cSpanX<FILECHAR_t> ret) noexcept {  // static
#ifdef UNDER_CE
    ASSERT(0);
    return 0;  // no such thing.
#elif defined(_WIN32)
    const DWORD nReturnSize = _FNF(::GetEnvironmentVariable)(pszVarName, ret.get_PtrWork(), ret.get_MaxLen());
    if (nReturnSize == 0) return 0;  // HResult::GetLast() to get real error.
    return nReturnSize;
#elif defined(__linux__)
    return StrT::Copy(ret, ::getenv(pszVarName));
#endif
}

cStringF GRAYCALL cAppState::GetEnvironStr(const FILECHAR_t* pszVarName) noexcept {  // static
#ifdef _WIN32
    FILECHAR_t szValue[cFilePath::k_MaxLen];
    if (GetEnvironStr(pszVarName, TOSPAN(szValue)) <= 0) return _FN("");
    return szValue;
#elif defined(__linux__)
    return ::getenv(pszVarName);
#endif
}

#if 0
cStringF cAppState::ExpandEnvironmentString() {
	//! Expand things like %PATH% in Environment strings or REG_EXPAND_SZ
	return ::ExpandEnvironmentStrings();
}
#endif

ITERATE_t GRAYCALL cAppState::GetEnvironArray(cArrayString<FILECHAR_t>& a) {  // static
    ITERATE_t i = 0;

#ifdef UNDER_CE
    ASSERT(0);  // no such thing.
#elif defined(_WIN32)
    FILECHAR_t* pszEnv0 = _FNFW(::GetEnvironmentStrings)();
    if (pszEnv0 == nullptr) return 0;

    FILECHAR_t* pszEnv = pszEnv0;
    for (;; i++) {
        if (pszEnv[0] == '\0') {
            _FNF(::FreeEnvironmentStrings)(pszEnv0);
            break;
        }
        a.Add(pszEnv);
        pszEnv += StrT::Len(pszEnv) + 1;
    }
#elif defined(__linux__)
    for (;; i++) {
        const FILECHAR_t* pszEnv = ::environ[i];
        if (pszEnv == nullptr) break;
        a.Add(pszEnv);
    }
#endif
    return i;
}

bool cAppState::SetEnvironStr(const FILECHAR_t* pszVarName, const FILECHAR_t* pszVal) noexcept {  // static
#ifdef UNDER_CE
    ASSERT(0);
    return false;  // no such thing
#elif defined(_WIN32)
    return _FNF(::SetEnvironmentVariable)(pszVarName, pszVal) ? true : false;
#elif defined(__linux__)
    int iErrNo;
    if (pszVal == nullptr) {
        iErrNo = ::unsetenv(pszVarName);  // NOT ::clearenv()
    } else {
        iErrNo = ::setenv(pszVarName, pszVal, true);
    }
    if (iErrNo == 0) return true;
    return false;
#endif
}

StrLen_t GRAYCALL cAppState::GetCurrentDir(cSpanX<FILECHAR_t> ret) {  // static
    if (ret.isEmpty()) return 0;
#if defined(UNDER_CE)
    ret.get_PtrWork()[0] = '\0';
    return 0;  // no concept of current directory in UNDER_CE. just use the root. (or get_AppFileDir()??)
#elif defined(_WIN32)
    const DWORD dwRetLen = _FNF(::GetCurrentDirectory)(CastN(DWORD, ret.get_Count() - 1), ret.get_PtrWork());
    return CastN(StrLen_t, dwRetLen);
#elif defined(__linux__)
    if (::getcwd(ret.get_PtrWork(), ret.get_Count() - 1) == nullptr) return 0;
    return StrT::Len<FILECHAR_t>(ret);
#else
#error NOOS
    return 0;
#endif
}

cFilePath GRAYCALL cAppState::get_CurrentDir() {  // static
    FILECHAR_t szPath[cFilePath::k_MaxLen];
    const StrLen_t iLen = GetCurrentDir(TOSPAN(szPath));
    if (iLen <= 0) return "";
    return szPath;
}

bool GRAYCALL cAppState::SetCurrentDir(const FILECHAR_t* pszDir) {  // static
    //! set the current directory path for the current app.
    //! like chdir() or _chdir()
    //! @return true = OK
#ifdef UNDER_CE
    return false;  // Cant do this UNDER_CE.
#elif defined(_WIN32)
    return _FNF(::SetCurrentDirectory)(pszDir) ? true : false;
#elif defined(__linux__)
    const int iRet = ::chdir(pszDir);
    return iRet == 0;
#endif
}

cFilePath cAppState::get_TempDir() {
    //! Get a directory i can write temporary files. ends with '\'
    //! This is decided based on the OS,User,App,
    //! Similar to _FNF(::SHGetFolderPath)(CSIDL_INTERNET_CACHE)
    //! Assume cInstallDir::IsInstallDirRestricted()

    if (!_sTempDir.IsEmpty()) return _sTempDir;  // cached value.

    FILECHAR_t szTmp[cFilePath::k_MaxLen];
#ifdef UNDER_CE
    DWORD nLenRet1 = _FNF(::GetTempPath)(STRMAX(szTmp), szTmp);
    UNREFERENCED_PARAMETER(nLenRet1);

#elif defined(_WIN32)
    // GetTempPath return is PathGetShortPath() with ~1. fix that.
    FILECHAR_t szTmp1[cFilePath::k_MaxLen];
    DWORD nLenRet1 = _FNF(::GetTempPath)(STRMAX(szTmp1), szTmp1);
    UNREFERENCED_PARAMETER(nLenRet1);

    DWORD nLenRet2 = _FNF(::GetLongPathName)(szTmp1, szTmp, STRMAX(szTmp));
    UNREFERENCED_PARAMETER(nLenRet2);

#elif defined(__linux__)
    StrLen_t iLen = GetEnvironStr(_FN("TMP"), TOSPAN(szTmp));
    if (iLen <= 0) {
        iLen = GetEnvironStr(_FN("TEMP"), TOSPAN(szTmp));
        if (iLen <= 0) {
            iLen = GetEnvironStr(_FN("USERPROFILE"), TOSPAN(szTmp));
            if (iLen <= 0) {
                iLen = StrT::Copy(TOSPAN(szTmp), _FN("/tmp"));  // append tmp
            }
        }
    }
    cFilePath::AddFileDirSep(TOSPAN(szTmp), iLen);
#endif

    _sTempDir = szTmp;  // cache this.
    return _sTempDir;
}

cFilePath cAppState::GetTempFile(const FILECHAR_t* pszFileTitle) {
    //! Create a temporary file to store stuff. Make sure its not colliding.
    //! @note if pszFileTitle == nullptr then just make a new random named file.

    cStringF sTmp;
    if (pszFileTitle == nullptr) {
        // make up a random unused file name. ALA _WIN32 GetTempFileName()
        // TODO: Test if the file already exists? use base64 name ?
        BYTE noise[8];
        g_Rand.GetNoise(TOSPAN(noise));  // assume InitSeed() called.
        constexpr StrLen_t lenHex = cMemSpan::GetHexDigestSize(sizeof(noise));
        FILECHAR_t* pTmp = sTmp.GetBuffer(lenHex + 1);
        pTmp[0] = 'T';
        TOSPAN(noise).GetHexDigest(cMemSpan(pTmp + 1, lenHex + 1));
        sTmp.ReleaseBuffer(lenHex + 1);
        pszFileTitle = sTmp;
    }

    // TODO: _isTempDirWritable = Test if we can really write to get_TempDir?
    return cFilePath::CombineFilePathX(get_TempDir(), pszFileTitle);
}

cFilePath cAppState::GetTempDir(const FILECHAR_t* pszFileDir, bool bCreate) {
    //! Get/Create a temporary folder in temporary folder space.
    //! @arg bCreate = create the sub directory if it doesn't already exist.

    cStringF sTempDir = GetTempFile(pszFileDir);
    if (bCreate) {
        HRESULT hRes = cFileDir::CreateDirectoryX(sTempDir);
        if (FAILED(hRes)) return "";
    }
    return sTempDir;
}

void cAppState::InitArgsWin(const FILECHAR_t* pszCommandArgs) {
    cStringF sAppPath = get_AppFilePath();

    if (pszCommandArgs == nullptr) {
        // Get command line from the OS ?
#ifdef _WIN32
        _Args.InitArgsLine(_FNF(::GetCommandLine)());
#elif defined(__linux__)
        // We should not get here. InitArgsPosix
        // Use "/proc/self/cmdline"?
        return;
#endif
    } else {
        _Args.InitArgsLine(pszCommandArgs);
    }

    _Args._aArgs.SetAt(0, sAppPath);
}

void cAppState::InitArgsPosix(int argc, APP_ARGS_t argv) {
    //! POSIX main() style init.
    //! If called by ServiceMain this might be redundant.
    _Args.InitArgsPosix(argc, argv);
}

void GRAYCALL cAppState::AbortApp(APP_EXITCODE_t uExitCode) {  // static
    if (SUPER_t::isSingleCreated()) {
        I().put_AppState(APPSTATE_t::_Exit);  // cAppExitCatcher should not block this now.
    }
#ifdef _WIN32
    ::ExitProcess((UINT)uExitCode);
#elif defined(__linux__)
    ::exit((UINT)uExitCode);
#endif
}

HRESULT GRAYCALL cAppState::ShowMessageBox(cString sMsg, UINT uFlags) {  // static
    //! Display a message that needs user feedback. This is something very important that the user should see.
    //! Use the console if it exists else put up a dialog if i can.
    //! @arg uFlags = 1 = MB_OKCANCEL;
    //! TODO Show message in GUI with MessageBox if no console available.
    //! like AfxMessageBox, MessageBox, etc
    //! http://unix.stackexchange.com/questions/144924/creating-a-messagebox-using-commandline
    //! @return 1 = IDOK, 2=IDCANCEL

    cAppConsole& ac = cAppConsole::I();
    if (ac.isConsoleMode()) {
        // Show prompt message.
        ac.WriteStrOut(StrArg<char>(sMsg));  // WriteStrOut WriteStrErr

        // Wait for user response.
        int iKey = ac.ReadKeyWait();  //
        UNREFERENCED_PARAMETER(iKey);
        return 1;
    }

    // NOT in console. So we must use a pop up.

#ifdef _WIN32
    int iRet = _FNF(MessageBox)(HWND_DESKTOP, StrArg<FILECHAR_t>(sMsg), cAppState::get_AppFileTitle(), uFlags);  // wait to attach debug.

#elif defined(__linux__)
    cOSProcess proc;
    int iRet = 1;
    // TODO launch sub cOSProcess to create message box.
    // notify-send "My name is bash and I rock da house"
    // notify-send -t 0 'hi there!' // does not expire.
    //

#endif
    return iRet;  // IDOK = 1
}

void GRAYCALL cAppState::SetExecutionState(bool bActiveCPU, bool bActiveGUI) {  // static
    //! Tell the system it should not sleep/hibernate if it is active. I have a big task to complete.
    //! try this for Vista, it will fail on XP

    UNREFERENCED_PARAMETER(bActiveGUI);
#if defined(_WIN32)
    if (bActiveCPU) {
        if (::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED) == 0) {
            // try XP variant as well just to make sure
            ::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
        }
    } else {
        ::SetThreadExecutionState(ES_CONTINUOUS);
    }
#elif defined(__linux__)
    // __linux__ ?

#endif
}

cString GRAYCALL cAppState::GetCurrentUserName(bool bForceRead) {  // static
    cAppState* pThis = cAppState::get_Single();
    ASSERT_NN(pThis);
    if (!bForceRead && !pThis->_sUserName.IsEmpty())  // cached name,.
        return pThis->_sUserName;

#if defined(_WIN32)
    GChar_t szUserName[256];
    DWORD dwLength = STRMAX(szUserName);  // ::GetUserName
#if defined(UNDER_CE)
    if (!_GTN(::GetUserNameEx)(NameUnknown, szUserName, &dwLength))
#else
    if (!_GTN(::GetUserName)(szUserName, &dwLength))
#endif
    {
        return _GT("");
    }
    pThis->_sUserName = szUserName;
#elif defined(__linux__)
    // getlogin() = the session/desktop user.
    // cuserid() in <stdio.h> for the current process user.
    // GetEnvironStr("LOGNAME"); similar to GetEnvironStr("uid")
    pThis->_sUserName = ::getlogin();
#endif
    return pThis->_sUserName;
}

bool GRAYCALL cAppState::isCurrentUserAdmin() {  // static
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
    cHandlePtr< ::PSID, CloseHandleType_Sid> AdministratorsGroup;
    ::SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    BOOL b = ::AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup.ref_Handle());
    if (!b) {
        return false;
    }
    if (!::CheckTokenMembership(cOSHandle::kNULL, AdministratorsGroup, &b)) {
        b = false;
    }
    return b;
#elif defined(__linux__)
    if (!StrT::CmpI<GChar_t>(GetCurrentUserName(), _GT("root"))) return true;
    if (::geteuid() == 0) return true;
    // TODO __linux__ user is admin group ??
    return false;
#endif
}

#if defined(_WIN32)
StrLen_t GRAYCALL cAppState::GetFolderPath(int csidl, FILECHAR_t* pszPath) {  // static
    // hRes = _FNF(::SHGetFolderPathAndSubDir)( WINHANDLE_NULL, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, pszSubFolder, szPath);
    pszPath[0] = '\0';
    const HRESULT hRes = _FNF(::SHGetFolderPath)(WINHANDLE_NULL, csidl, cOSHandle::kNULL, SHGFP_TYPE_CURRENT, pszPath);  // ASSUME _MAX_PATH, cFilePath::k_MaxLen
    if (FAILED(hRes)) return 0;
    return StrT::Len(pszPath);
}
#endif

cFilePath GRAYCALL cAppState::GetCurrentUserDir(const FILECHAR_t* pszSubFolder, bool bCreate) {  // static
    FILECHAR_t szPath[cFilePath::k_MaxLen];
#if defined(_WIN32) && !defined(UNDER_CE)
    StrLen_t iLen = GetFolderPath(CSIDL_LOCAL_APPDATA, szPath);
#elif defined(__linux__)
    // e.g. "/home/Dennis/X"
    StrLen_t iLen = cFilePath::CombineFilePath(TOSPAN(szPath), _FN(FILESTR_DirSep) _FN("home"), cAppState::GetCurrentUserName());
#endif
    if (iLen <= 0) return _FN("");
    if (!StrT::IsNullOrEmpty(pszSubFolder)) {
        iLen = cFilePath::CombineFilePathA(TOSPAN(szPath), iLen, pszSubFolder);
        if (bCreate) {
            HRESULT hRes = cFileDir::CreateDirectoryX(szPath);
            if (FAILED(hRes)) return _FN("");
        }
    }
    return szPath;
}

::HMODULE GRAYCALL cAppState::get_HModule() noexcept {  // static
#ifdef _WIN32
    if (sm_hInstance == HMODULE_NULL) {
        // _IMAGE_DOS_HEADER __ImageBase
        _FNF(::GetModuleHandleEx)(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, nullptr, (::HMODULE*)&sm_hInstance);
    }
#endif
    return sm_hInstance;  // Assume this is set correctly at init.
}

#if 0
bool cAppState::GetStatTimes(::FILETIME* pKernelTime, ::FILETIME* pUserTime) const {
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
cAppExitCatcher::cAppExitCatcher() : SUPER_t(this) {
    ::atexit(ExitCatchProc);
    // Register for SIGABRT ?? for abort() ?
}

void cAppExitCatcher::ExitCatch() {  // virtual
    const APPSTATE_t eAppState = cAppState::GetAppState();
    if (eAppState >= APPSTATE_t::_Exit) {
        // Legit exit.
        DEBUG_MSG(("cAppExitCatcher::ExitCatch() OK", eAppState));
        // Just pass through as we are exiting anyhow.
    } else {
        // We should not be here !
        DEBUG_ERR(("cAppExitCatcher::ExitCatch() in cAppState %d redirect.", eAppState));
        cExceptionAssert::Throw("cAppExitCatcher::ExitCatch", DEBUGSOURCELINE);
    }
}

void __cdecl cAppExitCatcher::ExitCatchProc() {  // static
    if (SUPER_t::isSingleCreated()) {
        cAppExitCatcher::I().ExitCatch();
    }
}
#endif

//*******************************************************************

#if defined(_WIN32)
cAppStateMain::cAppStateMain(HINSTANCE hInstance, const FILECHAR_t* pszCommandArgs) : _AppState(cAppState::I()) {
    //! WinMain()
    //! Current state should be APPSTATE_t::_Init
    ASSERT(_AppState._eAppState == APPSTATE_t::_Init);  //! Only call this once.
    _AppState.put_AppState(APPSTATE_t::_Run);
    _AppState.InitArgsWin(pszCommandArgs);
    ASSERT(hInstance == cAppState::get_HModule());
    cAppState::sm_hInstance = hInstance;
}
#endif
cAppStateMain::cAppStateMain(int argc, APP_ARGS_t argv) : _AppState(cAppState::I()) {
    ASSERT(_AppState._eAppState == APPSTATE_t::_Init);  //! Only call this once.
    _AppState.put_AppState(APPSTATE_t::_Run);
    _AppState.InitArgsPosix(argc, argv);
}
}  // namespace Gray
