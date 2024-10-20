//! @file cAppImpl.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrBuilder.h"
#include "cAppImpl.h"
#include "cAtom.h"
#include "cLogMgr.h"
#include "cLogSinkConsole.h"
#include "cOSModule.h"

#ifdef _MFC_VER
extern int AFXAPI AfxWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
#endif

namespace Gray {
bool cAppCommand::IsMatch(const ATOMCHAR_t* p) const {
    // assume switch prefix (-/) has already been stripped.
    ASSERT_NN(p);
    if (_pszAbbrev != nullptr) {                                     // optional
        if (StrT::Cmp(_pszAbbrev, p) == COMPARE_Equal) return true;  // case Sensitive.
    }
    return StrT::CmpI(_pszName, p) == COMPARE_Equal;
}

void cAppCommand::GetHelpText(StrBuilder<GChar_t>& sb) const {
    if (_pszAbbrev != nullptr) {
        sb.AddSpan(TOSPAN_LIT(_GT("-")));
        sb.WriteString(_pszAbbrev);
        sb.AddSpan(TOSPAN_LIT(_GT(", ")));
    }

    sb.AddSpan(TOSPAN_LIT(_GT("-")));
    sb.WriteString(_pszName);
    sb.AddSpan(TOSPAN_LIT(_GT(", ")));

    if (_pszHelpArgs != nullptr) {
        sb.WriteString(_pszHelpArgs);
        sb.AddSpan(TOSPAN_LIT(_GT(", ")));
    }

    sb.WriteString(_pszHelp);
}

cAppCommand* cAppCommands::RegisterCommand(cAppCommand& cmd) {
    for (auto* pCmd2 : _a) {  // collision?
        if (StrT::CmpI(pCmd2->_pszName, cmd._pszName) == COMPARE_Equal) {
            // collide => replace ?
            DEBUG_WARN(("RegisterCommand name collision '%s'", LOGSTR(cmd._pszName)));
            return pCmd2;
        }
        // allow collide _pszAbbrev ?
    }
    _a.Add(&cmd);
    return &cmd;
}

cAppCommand* cAppCommands::GetCommand(CommandId_t id) const {  // override;
    // IAppCommands
    return _a.GetAt(id);
}

cAppCommand* cAppCommands::FindCommand(const ATOMCHAR_t* pszCmd) const {  // override;
    // IAppCommands
    for (auto* pCmd2 : _a) {  // find handler for this type of command.
        if (pCmd2->IsMatch(pszCmd)) return pCmd2;
    }
    return nullptr;
}

#ifndef _MFC_VER
/// <summary>
/// request help text via the command line
/// All apps should support "-help" "-?" requests for assistance.
/// Is this arg present on the command line ? like FindCommandArg()
/// </summary>
struct cAppCmdHelp final : public cAppCommand {
    cAppCmdHelp() : cAppCommand("?", CATOM_N(help), nullptr, "Get a general description of this program.") {}

    /// <summary>
    /// Show my help text via console or dialog .
    /// </summary>
    /// <param name="iArgN"></param>
    /// <param name="pszArg">nullptr or extra arg for specific help.</param>
    /// <returns></returns>
    HRESULT DoCommand(const cAppArgs& args, int iArgN = 0) override {
        UNREFERENCED_PARAMETER(args);
        UNREFERENCED_PARAMETER(iArgN);

        cString sText = cAppState::get_AppFileTitle() + STR_NL;
        sText += cAppImpl::I().get_HelpText();

        // is console mode or have a parent in console mode ?
        cLogProcessor& log = cLogMgr::I();

        // ASSUME logger will Break into multi lines.
        log.addEventS(LOG_ATTR_PRINT, LOGLVL_t::_MAJOR, sText);
        return 0;  // consume no more arguments.
    }
};

/// <summary>
/// I want to debug something in startup code.
/// </summary>
struct cAppCmdWaitForDebug final : public cAppCommand {
    cAppCmdWaitForDebug() : cAppCommand("wfd", "waitfordebugger", nullptr, "Wait for the debugger to attach.") {}

    HRESULT DoCommand(const cAppArgs& args, int iArgN = 0) override {
        // TODO  pop message box or use console ? to wait for user input.
        // _WIN32 ShowMessageBox()
        UNREFERENCED_PARAMETER(args);
        UNREFERENCED_PARAMETER(iArgN);
        return E_NOTIMPL;
    }
};

cAppImpl::cAppImpl(const FILECHAR_t* pszAppName) : cSingletonType<cAppImpl>(this), cDependRegister(typeid(cAppImpl)), _pszAppName(pszAppName) {
    DEBUG_CHECK(!StrT::IsWhitespace(_pszAppName));
    static cAppCmdHelp k_Help;  /// basic help command.
    _Commands.RegisterCommand(k_Help);
    static cAppCmdWaitForDebug k_WaitForDebug;  /// wait for the debugger to attach.
    _Commands.RegisterCommand(k_WaitForDebug);
}

cString cAppImpl::get_HelpText() const {  // override
    StrBuilder<GChar_t> sb;
    for (const auto* pCmd : _Commands._a) {
        if (pCmd->_pszHelp != nullptr) {  // not hidden
            pCmd->GetHelpText(sb);
            sb.AddSpan(TOSPAN_LIT(_GT(STR_NL)));
        }
    }
    return sb.get_SpanStr();
}

void cAppImpl::SetArgValid(ITERATE_t i) {
    _ArgsValid.SetBit((BIT_ENUM_t)i);
}

cStringF cAppImpl::get_InvalidArgs() const {
    //! Get a list of args NOT marked as valid. Not IN _ArgsValid
    cStringF sInvalidArgs;
    const auto& rState = cAppState::I();
    const ITERATE_t iArgsQty = rState._Args.get_ArgsQty();
    for (ITERATE_t i = 1; i < iArgsQty; i++) {
        if (_ArgsValid.IsSet(CastN(BIT_ENUM_t, i))) continue;
        if (!sInvalidArgs.IsEmpty()) sInvalidArgs += _FN(",");
        sInvalidArgs += rState.GetArgEnum(i);
    }
    return sInvalidArgs;
}

BOOL cAppImpl::InitInstance() {  // virtual
    //! APPSTATE_t::_RunInit
    //! Override this to make the application do something at start.
    //! Like CWinApp for MFC
    //! @return true = OK. false = exit now.

    // AttachToCurrentThread();
    _nMainThreadId = cThreadId::GetCurrentId();
    return true;  // Run() will be called.
}

bool cAppImpl::OnTickApp() {  // virtual
    //! Override this to make the application do something. Main loop of main thread.
    //! @return false = exit
    const auto& rState = cAppState::I();
    return !_isCloseSignaled && rState.isAppStateRun();  // just keep going. not APPSTATE_t::_Exit
}

void cAppImpl::ReleaseModuleChildren(::HMODULE hMod) {  // override;
    for (ITERATE_t i = _Commands._a.GetSize(); i;) {
        const cAppCommand* p = _Commands._a.GetAtCheck(--i);
        if (p == nullptr || p->get_HModule() != hMod) continue;
        _Commands._a.RemoveAt(i);
    }
}

HRESULT cAppImpl::RunCommand(const cAppArgs& args, int i) {
    const FILECHAR_t* pszCmd = args.GetArgEnum(i);
    while (cAppArgs::IsArgSwitch(pszCmd[0])) pszCmd++;

    cAppCommand* pCmd = FindCommand(pszCmd);
    if (pCmd == nullptr) return E_INVALIDARG;  // no idea how to process this switch. might be an error. just skip this.

    return pCmd->DoCommand(args, i + 1);
}

HRESULT cAppImpl::RunCommandN(ITERATE_t i) {
    if (_ArgsValid.IsSet(CastN(BIT_ENUM_t, i))) return i;  // already processed this argument? (out of order ?). don't process it again. no double help.

    const auto& rState = cAppState::I();
    const FILECHAR_t* pszCmd = rState._Args.GetArgEnum(i);
    while (cAppArgs::IsArgSwitch(pszCmd[0])) pszCmd++;

    cAppCommand* pCmd = FindCommand(pszCmd);
    if (pCmd == nullptr) return i;  // no idea how to process this switch. might be an error. just skip this.

    _ArgsValid.SetBit(i);  // found it anyhow. block this from re-entrancy.

    const HRESULT hRes = pCmd->DoCommand(rState._Args, i + 1);
    if (FAILED(hRes)) {
        // Stop processing. report error.
        if (hRes == HRESULT_WIN32_C(ERROR_INVALID_STATE)) {
            _ArgsValid.ClearBit(i);  // try again later ?
            return i;
        }
        LOGF((LOG_ATTR_INIT, LOGLVL_t::_CRIT, "Command line '%s' failed '%s'", LOGSTR(pszCmd), LOGERR(hRes)));
        return hRes;
    }

    // How many extra args did we consume?
    const int j = i + hRes;
    for (; i < j; i++) {
        _ArgsValid.SetBit(i + 1);  // consumed
    }

    return j;
}

HRESULT cAppImpl::RunCommands() {
    // NOTE: some commands may be run ahead of this. They effect init. checks _ArgsValid
    const auto& rState = cAppState::I();
    const ITERATE_t nQty = rState._Args.get_ArgsQty();
    ITERATE_t i = 1;
    for (; i < nQty; i++) {
        const HRESULT hRes = RunCommandN(i);
        if (FAILED(hRes)) return hRes;  // Stop processing. report error.
        i = hRes;                       // maybe skip some args ?
    }
    return i;
}

APP_EXITCODE_t cAppImpl::Run() {  // virtual
    const HRESULT hRes = RunCommands();
    if (FAILED(hRes)) {
        _isCloseSignaled = true;
        return APP_EXITCODE_t::_FAIL;
    }

    // Log a message if there were command line arguments that did nothing. unknown.
    cStringF sInvalidArgs = get_InvalidArgs();
    if (!sInvalidArgs.IsEmpty()) {
        // Check _ArgsValid. Show Error for any junk/unused arguments.
        cLogMgr::I().addEventF(LOG_ATTR_INIT, LOGLVL_t::_CRIT, "Unknown command line args. '%s'", LOGSTR(sInvalidArgs));
        // return APP_EXITCODE_t::_FAIL;
    }

    for (;;) {
        const TIMESYS_t tStart = cTimeSys::GetTimeNow();  // start of tick.
        if (!OnTickApp()) break;                          // ProcessMessages()

        if (_nMinTickTime > 0) {
            // if actual Tick time is less than minimum then wait.
            const TIMESYS_t tNow = cTimeSys::GetTimeNow();
            TIMESYSD_t iDiff = tNow - tStart;
            if (iDiff < _nMinTickTime) {
                // Sleep to keep from taking over the CPU when i have nothing to do.
                if (iDiff < 0) iDiff = 0;
                cThreadId::SleepCurrent(_nMinTickTime - iDiff);
            }
        }
    }

    _isCloseSignaled = true;
    return APP_EXITCODE_t::_OK;
}

APP_EXITCODE_t cAppImpl::ExitInstance() {  // virtual
    return APP_EXITCODE_t::_OK;
}

APP_EXITCODE_t cAppImpl::Main(::HMODULE hInstance) {
    auto& rState = cAppState::I();

#ifdef _DEBUG
    DEBUG_MSG(("cAppImpl::Main '%s'", LOGSTR(_pszAppName)));
    const APPSTATE_t eAppState = rState.get_AppState();
    ASSERT(eAppState == APPSTATE_t::_Run);  // Assume cAppStateMain
#endif

#ifdef _WIN32
    if (hInstance != HMODULE_NULL) {  // don't clear it if already set.
        ASSERT(hInstance == cAppState::get_HModule());
        cAppState::sm_hInstance = hInstance;
    }
#endif

    rState.put_AppState(APPSTATE_t::_RunInit);

#ifdef _MFC_VER
    // Probably calls AfxWinInit() and assume will call InitInstance()
    return (APP_EXITCODE_t)::AfxWinMain(hInstance, HMODULE_NULL, LPTSTR lpCmdLine, nCmdShow);
#else
    APP_EXITCODE_t iRet = APP_EXITCODE_t::_FAIL;
    if (InitInstance()) {
        // Run loop until told to stop.
        rState.put_AppState(APPSTATE_t::_Run);
        iRet = (APP_EXITCODE_t)Run();
        rState.put_AppState(APPSTATE_t::_RunExit);
        const APP_EXITCODE_t iRetExit = ExitInstance();
        if (iRet == APP_EXITCODE_t::_OK) iRet = iRetExit;  // allow exit to make this fail.
    }

    // Exit.
    rState.put_AppState(APPSTATE_t::_Exit);
    return iRet;
#endif
}

#endif  // MFC
}  // namespace Gray
