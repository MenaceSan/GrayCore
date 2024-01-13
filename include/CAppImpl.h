//! @file cAppImpl.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cAppImpl_H
#define _INC_cAppImpl_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cAppCommand.h"
#include "cAppState.h"
#include "cOSModule.h"
#include "cObject.h"

namespace Gray {
#ifndef _MFC_VER
/// <summary>
/// Entry point for my implemented application. I am not a _WINDLL.
/// like CWinApp for MFC (maybe windowed or console)
/// I am NOT a library/DLL. I am an application implementation. NOT the same as (or to be merged with) cAppState.
/// Basic framework for my application I implement. Windows or Console.
/// Assume a static like cAppImpl theApp is defined some place.
/// </summary>
class GRAYCORE_LINK cAppImpl : public cSingletonStatic<cAppImpl>, public IAppCommands {
 protected:
    THREADID_t m_nMainThreadId;  /// The thread the app started with. main().

 public:
    const FILECHAR_t* m_pszAppName;  /// Specifies the name of my application. (display friendly)
    TIMESYSD_t m_nMinTickTime;       /// Minimum amount of time to spend in the OnTickApp() (mSec). cThreadId::SleepCurrent() if there is extra time.
    cAppState& m_State;              /// Quick reference to cAppState singleton.
    bool m_bCloseSignal;             /// Polite request to close the application. checked in Run() and OnTickApp() >= APPSTATE_t::_RunExit APPSTATE_t::_Exit

    cAppCommands _Commands;  /// built a list of possible commands. Dynamically add new command handlers to the app. to process cAppArgs.

 public:
    cAppImpl(const FILECHAR_t* pszAppName);
    ~cAppImpl() override;

    /// <summary>
    /// The thread we started with.
    /// </summary>
    /// <returns></returns>
    THREADID_t inline get_MainThreadId() const noexcept {
        return m_nMainThreadId;
    }
    cAppCommand* FindCommand(CommandId_t id) const override {
        return _Commands.FindCommand(id);
    }
    cAppCommand* FindCommand(const ATOMCHAR_t* pszName) const override {
        return _Commands.FindCommand(pszName);
    }

    HRESULT RunCommand(ITERATE_t i);
    HRESULT RunCommands();

    static inline HINSTANCE get_HInstance() {
        // Similar to MFC?
        return cAppState::get_HModule();
    }

    virtual cString get_HelpText() const;

    virtual BOOL InitInstance();
    virtual bool OnTickApp();

    /// <summary>
    /// APPSTATE_t::_Run
    /// Override this to make the application do something. Main loop of main thread.
    /// Like CWinApp for MFC
    /// @note In _WIN32, If my parent is a console, the console will return immediately! Not on first message loop like old docs say.
    /// </summary>
    /// <returns>APP_EXITCODE_t return app exit code. APP_EXITCODE_OK (NOT THREAD_EXITCODE_t?)</returns>
    virtual int Run();

    /// <summary>
    /// APPSTATE_t::_RunExit
    /// Override this to make the application do something to clean up.
    /// This should be called if Run() fails. NOT called if InitInstance fails.
    /// Like CWinApp for MFC
    /// </summary>
    /// <returns>APP_EXITCODE_t return app exit code. APP_EXITCODE_OK</returns>
    virtual int ExitInstance();

    /// <summary>
    /// The main application entry point and process loop. Like MFC AfxWinMain()
    /// Assume cAppStateMain was used.
    /// </summary>
    /// <param name="hInstance"></param>
    /// <returns>APP_EXITCODE_t</returns>
    APP_EXITCODE_t Main(HMODULE hInstance = HMODULE_NULL);
};
#endif
}  // namespace Gray
#endif
