//
//! @file cAppImpl.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cAppImpl.h"
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
    if (m_pszAbbrev != nullptr) {                        // optional
        if (StrT::Cmp(m_pszAbbrev, p) == COMPARE_Equal)  // case Sensitive.
            return true;
    }
    return StrT::CmpI(m_pszName, p) == COMPARE_Equal;
}

cString cAppCommand::get_HelpText() const {
    cString sHelp;
    if (m_pszAbbrev != nullptr) {
        sHelp += _GT("-");
        sHelp += StrArg<GChar_t>(m_pszAbbrev);
        sHelp += _GT(", ");
    }

    sHelp += _GT("-");
    sHelp += StrArg<GChar_t>(m_pszName);
    sHelp += _GT(", ");

    if (m_pszHelpArgs != nullptr) {
        sHelp += StrArg<GChar_t>(m_pszHelpArgs);
        sHelp += _GT(", ");
    }

    sHelp += StrArg<GChar_t>(m_pszHelp);
    return sHelp;
}

cAppCommand* cAppCommands::RegisterCommand(cAppCommand& cmd) {
    for (auto pCmd2 : m_a) {  // collision?
        if (StrT::CmpI(pCmd2->m_pszName, cmd.m_pszName) == COMPARE_Equal) {
            // collide, replace ?
            DEBUG_ERR(("RegisterCommand collision '%s'", LOGSTR(cmd.m_pszName)));
            return pCmd2;
        }
        if (StrT::Cmp(pCmd2->m_pszAbbrev, cmd.m_pszAbbrev) == COMPARE_Equal) {
            // allow collide ?
        }
    }

    m_a.Add(&cmd);
    return &cmd;
}

cAppCommand* cAppCommands::FindCommand(CommandId_t id) const {  // override;
    // IAppCommands
    return m_a.GetAt(id);
}

cAppCommand* cAppCommands::FindCommand(const ATOMCHAR_t* pszCmd) const {  // override;
    // IAppCommands
    for (auto pCmd2 : m_a) {  // find handler for this type of command.
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
struct cAppCmdHelp : public cAppCommand {
    cAppCmdHelp() : cAppCommand(_FN("?"), "help", nullptr, "Get a general description of this program.") {}

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

static cAppCmdHelp k_Help;  /// basic help command.

/// <summary>
/// I want to debug something in startup code.
/// </summary>
struct cAppCmdWaitForDebug : public cAppCommand {
    cAppCmdWaitForDebug() : cAppCommand(_FN("wfd"), "waitfordebugger", nullptr, "Wait for the debugger to attach.") {}

    HRESULT DoCommand(const cAppArgs& args, int iArgN = 0) override {
        // TODO  pop message box or use console ? to wait for user input.
        // _WIN32 ShowMessageBox()
        UNREFERENCED_PARAMETER(args);
        UNREFERENCED_PARAMETER(iArgN);
        return E_NOTIMPL;
    }
};

static cAppCmdWaitForDebug k_WaitForDebug;  /// wait for the debugger to attach.

cAppImpl::cAppImpl(const FILECHAR_t* pszAppName)
    : cSingletonStatic<cAppImpl>(this),
      m_nMainThreadId(cThreadId::k_NULL),
      m_pszAppName(pszAppName),  // from module file name ?

      m_nMinTickTime(10),  // mSec for OnTickApp

      m_State(cAppState::I()),
      m_bCloseSignal(false) {
    DEBUG_CHECK(!StrT::IsWhitespace(m_pszAppName));
    _Commands.RegisterCommand(k_Help);
    // _Commands.RegisterCommand(&k_WaitForDebug);
}

cAppImpl::~cAppImpl() {}

cString cAppImpl::get_HelpText() const {  // override
    //! Get help for all the _Commands we support.

    cString sHelp;
    for (auto pCmd : _Commands.m_a) {
        if (pCmd->m_pszHelp != nullptr) {  // not hidden
            sHelp += pCmd->get_HelpText();
            sHelp += _GT(STR_NL);
        }
    }

    return sHelp;
}

BOOL cAppImpl::InitInstance() {  // virtual
    //! APPSTATE_t::_RunInit
    //! Override this to make the application do something at start.
    //! Like CWinApp for MFC
    //! @return true = OK. false = exit now.

    // AttachToCurrentThread();
    m_nMainThreadId = cThreadId::GetCurrentId();
    return true;  // Run() will be called.
}

bool cAppImpl::OnTickApp() {  // virtual
    //! Override this to make the application do something. Main loop of main thread.
    //! @return false = exit
    return !m_bCloseSignal && m_State.isAppStateRun();  // just keep going. not APPSTATE_t::_Exit
}

HRESULT cAppImpl::RunCommand(ITERATE_t i) {
    // Run a single command and set up its arguments.
    // @arg i = index in m_Args array.
    const FILECHAR_t* pszCmd = m_State.m_Args.GetArgEnum(i);
    while (cAppArgs::IsArgSwitch(pszCmd[0])) {
        pszCmd++;
    }

    cAppCommand* pCmd = FindCommand(pszCmd);
    if (pCmd == nullptr) return i;  // no idea how to process this switch. might be an error. just skip this.

    m_State.m_ArgsValid.SetBit(i);  // found it anyhow.

    HRESULT hRes = pCmd->DoCommand(m_State.m_Args, i+1);
    if (FAILED(hRes)) {
        // Stop processing. report error.
        LOGF((LOG_ATTR_INIT, LOGLVL_t::_CRIT, "Command line '%s' failed '%s'", LOGSTR(pszCmd), LOGERR(hRes)));
        return hRes;
    }

    // How many extra args did we consume?
    const int j = i + hRes;
    for (; i < j; i++) {
        m_State.m_ArgsValid.SetBit(i + 1);  // consumed
    }

    return j;
}

HRESULT cAppImpl::RunCommands() {
    // NOTE: some commands may be run ahead of this. They effect init. m_ArgsValid
    ITERATE_t i = 1;
    const ITERATE_t nQty = m_State.m_Args.get_ArgsQty();
    for (; i < nQty; i++) {
        if (m_State.m_ArgsValid.IsSet(CastN(BIT_ENUM_t, i)))  // already processed this argument? (out of order ?). don't process it again. no double help.
            continue;
        const HRESULT hRes = RunCommand(i);
        if (FAILED(hRes)) return hRes;  // Stop processing. report error.
        i = hRes;                       // maybe skip some args ?
    }
    return i;
}

int cAppImpl::Run() { // virtual
    HRESULT hRes = RunCommands();
    if (FAILED(hRes)) {
        m_bCloseSignal = true;
        return APP_EXITCODE_FAIL;
    }

    // Log a message if there were command line arguments that did nothing. unknown.
    cStringF sInvalidArgs = m_State.get_InvalidArgs();
    if (!sInvalidArgs.IsEmpty()) {
        // Check m_ArgsValid. Show Error for any junk/unused arguments.
        cLogMgr::I().addEventF(LOG_ATTR_INIT, LOGLVL_t::_CRIT, "Unknown command line args. '%s'", LOGSTR(sInvalidArgs));
        // return APP_EXITCODE_FAIL;
    }

    for (;;) {
        TIMESYS_t tStart = cTimeSys::GetTimeNow();  // start of tick.
        if (!OnTickApp())                           // ProcessMessages()
            break;
        if (m_nMinTickTime > 0) {
            // if actual Tick time is less than minimum then wait.
            TIMESYS_t tNow = cTimeSys::GetTimeNow();
            TIMESYSD_t iDiff = tNow - tStart;
            if (iDiff < m_nMinTickTime) {
                // Sleep to keep from taking over the CPU when i have nothing to do.
                if (iDiff < 0) iDiff = 0;
                cThreadId::SleepCurrent(m_nMinTickTime - iDiff);
            }
        }
    }

    m_bCloseSignal = true;
    return APP_EXITCODE_OK;
}

int cAppImpl::ExitInstance() {  // virtual
    return APP_EXITCODE_OK;
}

APP_EXITCODE_t cAppImpl::Main(HMODULE hInstance) {
#ifdef _DEBUG
    DEBUG_MSG(("cAppImpl::Main '%s'", LOGSTR(m_pszAppName)));
    APPSTATE_t eAppState = cAppState::I().get_AppState();
    ASSERT(eAppState == APPSTATE_t::_Run);  // Assume cAppStateMain
#endif

#ifdef _WIN32
    if (hInstance != HMODULE_NULL) {  // don't clear it if already set.
        ASSERT(hInstance == cAppState::get_HModule());
        cAppState::sm_hInstance = hInstance;
    }
#endif

    m_State.put_AppState(APPSTATE_t::_RunInit);

#ifdef _MFC_VER
    // Probably calls AfxWinInit() and assume will call InitInstance()
    return (APP_EXITCODE_t)::AfxWinMain(hInstance, HMODULE_NULL, LPTSTR lpCmdLine, nCmdShow);
#else
    APP_EXITCODE_t iRet = APP_EXITCODE_FAIL;
    if (InitInstance()) {
        // Run loop until told to stop.
        m_State.put_AppState(APPSTATE_t::_Run);
        iRet = (APP_EXITCODE_t)Run();
        m_State.put_AppState(APPSTATE_t::_RunExit);
        APP_EXITCODE_t iRetExit = (APP_EXITCODE_t)ExitInstance();
        if (iRet == APP_EXITCODE_OK)  // allow exit to make this fail.
            iRet = iRetExit;
    }

    // Exit.
    m_State.put_AppState(APPSTATE_t::_Exit);
    return iRet;
#endif
}

#endif  // MFC
}  // namespace Gray
