//! @file cLogSinkConsole.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cAppConsole.h"
#include "cAppState.h"
#include "cLogEvent.h"
#include "cLogMgr.h"
#include "cLogSinkConsole.h"
#include "cOSProcess.h"

#if !defined(UNDER_CE)
namespace Gray { 

HRESULT cLogSinkConsole::WriteString(const LOGCHAR_t* pszText) {  // virtual
    // LOG_ATTR_PRINT? // WriteStrErr or WriteStrOut?
    HRESULT hRes = cAppConsole::I().WriteStrOut(pszText);
    if (FAILED(hRes)) return hRes;
    return 1;  // something was written.
}
HRESULT cLogSinkConsole::addEvent(cLogEvent& rEvent) noexcept {  // override;
    return WriteString(rEvent.get_FormattedDefault());
}

cLogSinkConsole* GRAYCALL cLogSinkConsole::AddSinkCheck(cLogNexus* pLogger, bool bAttachElseAlloc) {  // static
    //! Make sure cLogSinkConsole singleton is attached to cLogMgr
    //! Push log messages to the console (or my parent console) if there is one.
    //! Add this console sink to cLogMgr/pLogger if not already added.
    //! @arg bAttachOrCreate = create my own console if there is no parent to attach to.

    if (pLogger == nullptr) {
        pLogger = cLogMgr::get_Single();
    }

    cLogSink* pSink0 = pLogger->FindSinkType(typeid(cLogSinkConsole));
    if (pSink0 != nullptr) return static_cast<cLogSinkConsole*>(pSink0);  // already has this sink.

    cAppConsole& ac = cAppConsole::I();
    if (!ac.isConsoleMode()) { // no console ?
        // Attach to parent console or create my own.
        if (!ac.AttachOrAllocConsole(bAttachElseAlloc)) return nullptr;
    }

    // attach new console sink to logging system.
    cLogSinkConsole* pSink = new cLogSinkConsole;
    pLogger->AddSink(pSink);
    return pSink;
}

bool GRAYCALL cLogSinkConsole::RemoveSinkCheck(cLogNexus* pLogger, bool bOnlyIfParent) {  // static
    //! remove this sink if there is a parent console. leave it if i created the console.
    //! We only created it for start up status and errors.
    //! @arg bOnlyIfParent = only remove the console sink if its my parent process console. NOT if I'm _CONSOLE or I created the console.
    if (pLogger == nullptr) {
        pLogger = cLogMgr::get_Single();
    }

    cLogSink* pSink = pLogger->FindSinkType(typeid(cLogSinkConsole));
    if (pSink == nullptr) return false;
    if (bOnlyIfParent) {
        // Detach from parent console.
        cAppConsole& ac = cAppConsole::I();
        if (ac.get_ConsoleMode() != AppCon_t::_Attach) {
            return false;
        }
        ac.ReleaseConsole();
    }
    return pLogger->RemoveSink(pSink, true);
}

HRESULT GRAYCALL cLogSinkConsole::WaitForDebugger() {  // static
    // Wait for the debugger to attach. -debugger command line arg.
    // cAppState::isDebuggerPresent()
    cAppState::ShowMessageBox("Waiting for debugger", 0);
    return S_OK;
}
}  // namespace Gray
#endif
