//! @file cAppConsole.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "HResult.h"
#include "cAppConsole.h"
#include "cAppState.h"
#include "cFileText.h"
#include "cLogMgr.h"
#include "cOSHandleSet.h"

#if !defined(UNDER_CE)

#if defined(_WIN32) && USE_CRT
#include <conio.h>
#include <io.h>
// #include <ios>	// std::ios::sync_with_stdio
#elif defined(__linux__)
#include <sys/ioctl.h>
#include <termios.h>
#endif
#include <fcntl.h>  // _O_RDONLY

namespace Gray {
cAppConsole::cAppConsole() : cSingleton<cAppConsole>(this, typeid(cAppConsole)), m_bKeyEchoMode(true), m_bKeyEnterMode(true), m_eConsoleType(AppCon_t::_Unknown), m_bConsoleParent(false), m_iAllocConsoleCount(0) {
#ifdef _WIN32
    for (int i = 0; i < static_cast<int>(AppStd_t::_QTY); i++) {
        m_hStd[i] = INVALID_HANDLE_VALUE;
    }
#endif
}

cAppConsole::~cAppConsole() {
    // Is m_iAllocConsoleCount = 0 ?
}

void cAppConsole::CheckConsoleMode() noexcept {
    if (m_eConsoleType != AppCon_t::_Unknown) return;

    cStringF sPrompt = cAppState::GetEnvironStr(_FN("PROMPT"));  // Good for Linux and Windows.
    m_bConsoleParent = !sPrompt.IsEmpty();

#if defined(_WIN32)
    HWND hWnd = ::GetConsoleWindow();
    if (hWnd != WINHANDLE_NULL)
#else  // __linux__
    if (stdout != nullptr)  // TODO: detect if Linux app is in a console!?
#endif
    {
        m_eConsoleType = AppCon_t::_Proc;  // My parent is build using _CONSOLE
        AttachConsoleSync();
    } else {
        m_eConsoleType = AppCon_t::_None;  // i have no console. Assume I'm a GUI app or headless service.
    }
}

bool cAppConsole::AttachConsoleSync() {
    //! Synchronize the C std* buffers (needs USE_CRT) with _WIN32 Console.
    //! NOTE: Not sure why this isn't just handled by AllocConsole()
    //! Similar to std::ios::sync_with_stdio()

    ASSERT(isConsoleMode());

#if defined(_WIN32) && USE_CRT
    for (int i = 0; i < static_cast<int>(AppStd_t::_QTY); i++) {
        // redirect un-buffered STDOUT to the console
        DWORD nStdHandle;
        ::FILE* pFileDest;
        OF_FLAGS_t nFileFlags = OF_WRITE | OF_TEXT;
        switch ((AppStd_t)i) {
            case AppStd_t::_stdin:
                nStdHandle = STD_INPUT_HANDLE;  // CONIN$
                pFileDest = stdin;
                nFileFlags = OF_READ | OF_TEXT;
                break;
            case AppStd_t::_stdout:
                nStdHandle = STD_OUTPUT_HANDLE;  // CONOUT$
                pFileDest = stdout;
                break;
            case AppStd_t::_stderr:
                nStdHandle = STD_ERROR_HANDLE;
                pFileDest = stderr;
                break;
            default:
                ASSERT(0);
                return false;
        }

        m_hStd[i] = ::GetStdHandle(nStdHandle);

        if (m_eConsoleType != AppCon_t::_Proc) {
            // Now attach it to the appropriate std FILE*,
            cFileText fileStd;
            HRESULT hRes = fileStd.OpenFileHandle(m_hStd[i], nFileFlags);
            if (FAILED(hRes)) return false;
            *pFileDest = *fileStd.DetachFileStream();  // copy FILE struct contents! NOT Just pointer.
        }
    }

    if (m_eConsoleType != AppCon_t::_Proc) {
        // set the screen buffer to be big enough to let us scroll text
        ::CONSOLE_SCREEN_BUFFER_INFO coninfo;
        if (!::GetConsoleScreenBufferInfo(GetStd(AppStd_t::_stdout), &coninfo)) {
            return false;
        }

        coninfo.dwSize.Y = k_MAX_CONSOLE_LINES;
        if (!::SetConsoleScreenBufferSize(GetStd(AppStd_t::_stdout), coninfo.dwSize)) {
            return false;
        }
#if 0
			// Synchronize STL iostreams. if anyone cares.
			std::ios::sync_with_stdio();
#endif
    }
#endif

    return true;
}

bool cAppConsole::AttachOrAllocConsole(bool bAttachElseAlloc) {
    //! 1. Do i already have a console. use it. if _CONSOLE app.
    //! 2. Attach to my parents console if there is one.
    //! 3. allocate a new console for this app.
    //! http://stackoverflow.com/questions/493536/can-one-executable-be-both-a-console-and-gui-application/494000#494000
    //! https://www.tillett.info/2013/05/13/how-to-create-a-windows-program-that-works-as-both-as-a-gui-and-console-application/

    if (isConsoleMode()) {       // I'm already in a console.
        m_iAllocConsoleCount++;  // Must have same number of closes with ReleaseConsole().
        return true;
    }

    ASSERT(m_iAllocConsoleCount == 0);
    ASSERT(m_eConsoleType == AppCon_t::_Unknown || m_eConsoleType == AppCon_t::_None);

#if defined(_WIN32)

    // We must specifically attach to the console.
    // A process can be associated with only one console,
    // so the AllocConsole function fails if the calling process already has a console

    // HasConsoleParent()
    if (::AttachConsole(ATTACH_PARENT_PROCESS)) {  // try to use my parents console.
        m_eConsoleType = AppCon_t::_Attach;
    } else {
        if (!bAttachElseAlloc) return false;
        if (!::AllocConsole()) {  // Make my own private console.
            // Failed to get or create a console. i probably already have one?
            return false;
        }
        m_eConsoleType = AppCon_t::_Create;
    }

#ifdef _DEBUG
    DWORD adwProcessList[32];
    const DWORD dwRet = ::GetConsoleProcessList(adwProcessList, _countof(adwProcessList));
    ASSERT(dwRet >= 1);
    const HWND hWnd = ::GetConsoleWindow();
    ASSERT(hWnd != WINHANDLE_NULL);
#endif

    m_iAllocConsoleCount = 1;

#if USE_CRT
    if (!AttachConsoleSync()) {
        m_iAllocConsoleCount = 0;
        m_eConsoleType = AppCon_t::_None;
        return false;
    }
#endif

    return true;
#else
    // AppCon_t::_Proc
    ASSERT(0);
    return false;  // this should NEVER be called.
#endif
}

void cAppConsole::ReleaseConsole() {
    m_iAllocConsoleCount--;
#if defined(_WIN32)
    if (m_iAllocConsoleCount <= 0 && m_eConsoleType > AppCon_t::_Proc) {
        // I called AllocConsole
        ::FreeConsole();
        m_eConsoleType = AppCon_t::_None;
    }
#endif
}

HRESULT cAppConsole::WriteStrErr(const char* pszText) {
    //! Does not support UNICODE ?
    //! @return EOF = (-1) error
    //! >=0 = success
    if (!isConsoleMode()) return true;

    const auto guard(m_Lock.Lock());

#if defined(_WIN32)
    // @note we must do this to get the dual windows/console stuff to work.
    DWORD dwLengthWritten;
    DWORD dwDataSize = StrT::Len(pszText);
    bool bRet = ::WriteFile(GetStd(AppStd_t::_stderr), pszText, dwDataSize, &dwLengthWritten, nullptr);
    if (!bRet) {
        // GetLastError code ERROR_IO_PENDING is not a failure. Async complete.
        HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
        return hRes;
    }
#else  // POSIX
    FILE* pFile = stderr;
    int iRet = ::fputs(pszText, pFile);
    if (iRet == EOF) {
        // failed.
        HRESULT hRes = HResult::GetPOSIXLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
        return hRes;
    }
#endif
    return S_OK;  // we are good.
}

HRESULT cAppConsole::WriteStrOut(const char* pszText) {
    //! Write to console. Does not support UNICODE ?
    //! @return HRESULT_WIN32_C(ERROR_HANDLE_DISK_FULL)
    //! >=0 = success
    //! @note _WIN32 could probably use the m_h directly.

    if (!isConsoleMode()) return S_OK;
    const auto guard(m_Lock.Lock());
#if defined(_WIN32)
    // @note we must do this to get the dual windows/console stuff to work.
    DWORD dwLengthWritten = 0;
    DWORD dwDataSize = StrT::Len(pszText);
    bool bRet = ::WriteFile(GetStd(AppStd_t::_stdout), pszText, dwDataSize, &dwLengthWritten, nullptr);
    if (!bRet) return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
#else  // POSIX
    FILE* pFile = stdout;
    int iRet = ::fputs(pszText, pFile);
    if (iRet == EOF) return HResult::GetPOSIXLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));  // failed. EBADF=9
#endif
    return S_OK;  // we are good.
}

HRESULT cAppConsole::SetKeyModes(bool bEchoMode, bool bEnterMode) {
    //! @arg bEchoMode = default true = auto echo for input keys input on/off
    //! @arg bEnterMode = default true = Wait for a enter to be pressed first ? false = get all keys as they come in.
    //! @note The effects are persistent for the console in Linux. remember to undo your changes before exit.

#ifdef _WIN32
    if (bEnterMode && !bEchoMode) {
        // NON raw mode must echo ??
        // return E_INVALIDARG;
    }
    ::SetConsoleCtrlHandler(nullptr, true);  // Ignore CONTROL-C, CTRL+C
#endif

#ifdef __linux__
    char buf[256];
    cMem::Zero(buf, sizeof(buf));
    int iRet = ::readlink("/proc/self/fd/0", buf, sizeof(buf));  // Is this needed or just use stdin=0 ?
    if (iRet == -1) return HResult::GetLastDef();

    cOSHandle fdin;
    fdin.OpenHandle(buf, O_RDWR);  // NOTE: fdin should be 0 instead? stdin = iobuf[0]
    if (!fdin.isValidHandle()) return HResult::GetLastDef(E_HANDLE);

    // get the current state. fdin = 0?
    struct termios stty;
    iRet = fdin.IOCtl(TCGETS, &stty);
    if (iRet < 0) return HResult::GetLastDef();

    if (bEchoMode) {             // CLOCAL ??
        stty.c_lflag |= (ECHO);  // echo
    } else {
        stty.c_lflag &= ~(ECHO);  // suppress echo
    }
    if (bEnterMode) {
        stty.c_lflag |= (ICANON);  // must wait for enter to get keys.
    } else {
        stty.c_lflag &= ~(ICANON);  // one char @ a time
    }

    iRet = fdin.IOCtl(TCSETS, &stty);
    if (iRet < 0) return HResult::GetLastDef();

#endif
    m_bKeyEchoMode = bEchoMode;
    m_bKeyEnterMode = bEnterMode;
    return S_OK;
}

int cAppConsole::get_KeyReadQty() const {
#ifdef _WIN32
#if USE_CRT
    return ::_kbhit() ? 1 : 0;
#else
    return false;
#endif
#else
    // NOTE: can i use FIONREAD ?
    cOSHandleSet hs(0);
    if (hs.WaitForObjects(0) == S_OK) return 1;
    return 0;
#endif
}

int cAppConsole::ReadKeyWait() {
    if (!isConsoleMode()) return -1;

#ifdef _WIN32
        // NOTE: _WIN32 fgetc(stdin) will block until the ENTER key is pressed ! then feed chars until it runs out.
#if USE_CRT
    if (!m_bKeyEnterMode) {
        if (m_bKeyEchoMode) return ::_getche();
        return ::_getch();  // don't wait for ENTER return as we get them.
    }
    return ::getchar();  // buffer chars and returns when ENTER is pressed for a whole line.
#else
    return -1;
#endif
#elif defined(__linux__)
    // @note NON raw mode always echoes.
    return ::getchar();  // buffer chars and returns when ENTER is pressed for a whole line.
#endif
}

int cAppConsole::ReadKey() {
    if (get_KeyReadQty() <= 0) return -1;  // none ready.
    return ReadKeyWait();
}
}  // namespace Gray
#endif  // UNDER_CE
