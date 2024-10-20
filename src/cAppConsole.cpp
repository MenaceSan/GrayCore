//! @file cAppConsole.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#include "pch.h"
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
cSingleton_IMPL(cAppConsole);

cAppConsole::cAppConsole() : cSingleton<cAppConsole>(this) {
#ifdef _WIN32
    for (int i = 0; i < static_cast<int>(AppStd_t::_QTY); i++) {
        _hStd[i] = INVALID_HANDLE_VALUE;
    }
#endif
}

cAppConsole::~cAppConsole() {
    // Is _iAllocConsoleCount = 0 ?
}

void cAppConsole::CheckConsoleMode() noexcept {
    if (_eConsoleType != AppCon_t::_Unknown) return;

    cStringF sPrompt = cAppState::GetEnvironStr(_FN("PROMPT"));  // Good for Linux and Windows.
    _isConsoleParent = !sPrompt.IsEmpty();

#if defined(_WIN32)
    HWND hWnd = ::GetConsoleWindow();
    if (hWnd != WINHANDLE_NULL)
#else  // __linux__
    if (stdout != nullptr)  // TODO: detect if Linux app is in a console!?
#endif
    {
        _eConsoleType = AppCon_t::_Proc;  // My parent is build using _CONSOLE
        AttachConsoleSync();
    } else {
        _eConsoleType = AppCon_t::_None;  // i have no console. Assume I'm a GUI app or headless service.
    }
}

bool cAppConsole::AttachConsoleSync() {
    //! Synchronize the C std* buffers (needs USE_CRT) with _WIN32 Console.
    //! NOTE: Not sure why this isn't just handled by AllocConsole()
    //! Similar to std::ios::sync_with_stdio()

    ASSERT(isConsoleMode());

#if defined(_WIN32)
    for (int i = 0; i < _countof(_hStd); i++) {
        // redirect un-buffered STDOUT to the console
        DWORD nStdHandle;
        switch ((AppStd_t)i) {
            case AppStd_t::_stdin:
                nStdHandle = STD_INPUT_HANDLE;  // CONIN$
                break;
            case AppStd_t::_stdout:
                nStdHandle = STD_OUTPUT_HANDLE;  // CONOUT$
                break;
            case AppStd_t::_stderr:
                nStdHandle = STD_ERROR_HANDLE;
                break;
            default:
                ASSERT(0);
                return false;
        }
        _hStd[i] = ::GetStdHandle(nStdHandle);
    }

#if USE_CRT
    if (_eConsoleType != AppCon_t::_Proc) {
        // redirect un-buffered STDOUT to the console
        for (int i = 0; i < _countof(_hStd); i++) {
            ::FILE* pFileDest;
            OF_FLAGS_t nFileFlags = OF_WRITE | OF_TEXT;
            switch ((AppStd_t)i) {
                case AppStd_t::_stdin:
                    pFileDest = stdin;
                    nFileFlags = OF_READ | OF_TEXT;
                    break;
                case AppStd_t::_stdout:
                    pFileDest = stdout;
                    break;
                case AppStd_t::_stderr:
                    pFileDest = stderr;
                    break;
                default:
                    ASSERT(0);
                    return false;
            }
            // Now attach it to the appropriate std FILE*,  USE_CRT
            cFileText fileStd;
            const HRESULT hRes = fileStd.OpenFileHandle(_hStd[i], nFileFlags);
            if (FAILED(hRes)) return false;
            *pFileDest = *fileStd.DetachFileStream();  // copy ::FILE struct contents! NOT Just pointer.
        }
    }
#endif

    if (_eConsoleType != AppCon_t::_Proc) {
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
			// Synchronize STL iostreams. if anyone cares. USE_CRT
			std::ios::sync_with_stdio();
#endif
    }
#endif

    return true;
}

bool cAppConsole::AttachOrAllocConsole(bool bAttachElseAlloc) {
    if (isConsoleMode()) {       // I'm already in a console.
        _iAllocConsoleCount++;  // Must have same number of closes with ReleaseConsole().
        return true;
    }

    ASSERT(_iAllocConsoleCount == 0);
    ASSERT(_eConsoleType == AppCon_t::_Unknown || _eConsoleType == AppCon_t::_None);

#if defined(_WIN32)

    // We must specifically attach to the console.
    // A process can be associated with only one console,
    // so the AllocConsole function fails if the calling process already has a console

    // HasConsoleParent()
    if (::AttachConsole(ATTACH_PARENT_PROCESS)) {  // try to use my parents console.
        _eConsoleType = AppCon_t::_Attach;
    } else {
        if (!bAttachElseAlloc) return false;
        if (!::AllocConsole()) {  // Make my own private console.
            // Failed to get or create a console. i probably already have one?
            return false;
        }
        _eConsoleType = AppCon_t::_Create;
    }

#ifdef _DEBUG
    DWORD adwProcessList[32];
    const DWORD dwRet = ::GetConsoleProcessList(adwProcessList, _countof(adwProcessList));
    ASSERT(dwRet >= 1);
    const HWND hWnd = ::GetConsoleWindow();
    ASSERT(hWnd != WINHANDLE_NULL);
#endif

    _iAllocConsoleCount = 1;

#if USE_CRT
    if (!AttachConsoleSync()) {
        _iAllocConsoleCount = 0;
        _eConsoleType = AppCon_t::_None;
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
    _iAllocConsoleCount--;
#if defined(_WIN32)
    if (_iAllocConsoleCount <= 0 && _eConsoleType > AppCon_t::_Proc) {
        // I called AllocConsole
        ::FreeConsole();
        _eConsoleType = AppCon_t::_None;
    }
#endif
}

#if defined(_WIN32)
HRESULT cAppConsole::WriteStrH(::HANDLE h, const char* pszText) {
    // @note we must do this to get the dual windows/console stuff to work.
    // @note we can halt forever here if the console is blocking this!!!
    DWORD dwLengthWritten;
    const DWORD dwDataSize = StrT::Len(pszText);
    const bool bRet = ::WriteFile(h, pszText, dwDataSize, &dwLengthWritten, nullptr);
    if (!bRet) {
        // GetLastError code ERROR_IO_PENDING is not a failure. Async complete.
        const HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
        return hRes;
    }
    return S_OK;  // we are good.
}
#else  // POSIX
HRESULT cAppConsole::WriteStrH(::FILE* pFile, const char* pszText) {
    int iRet = ::fputs(pszText, pFile);
    if (iRet == EOF) {
        // failed. // EBADF=9
        HRESULT hRes = HResult::GetPOSIXLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
        return hRes;
    }
    return S_OK;  // we are good.
}
#endif

HRESULT cAppConsole::WriteStrErr(const char* pszText) {
    if (!isConsoleMode()) return S_OK;
    const auto guard(_Lock.Lock());
#if defined(_WIN32)
    return WriteStrH(GetStd(AppStd_t::_stderr), pszText);
#else  // POSIX
    return WriteStrH(stderr, pszText);
#endif
}

HRESULT cAppConsole::WriteStrOut(const char* pszText) {
    if (!isConsoleMode()) return S_OK;
    const auto guard(_Lock.Lock());
#if defined(_WIN32)
    return WriteStrH(GetStd(AppStd_t::_stdout), pszText);
#else  // POSIX
    return WriteStrH(stdout, pszText);
#endif
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
    _isKeyEchoMode = bEchoMode;
    _isKeyEnterMode = bEnterMode;
    return S_OK;
}

int cAppConsole::get_KeyReadQty() const {
#ifdef _WIN32
#if USE_CRT
    return ::_kbhit() ? 1 : 0;
#else
    DWORD dwEvents = 0;
    ::GetNumberOfConsoleInputEvents(GetStd(AppStd_t::_stdin), &dwEvents);
    return dwEvents;
#endif
#else
    // NOTE: can i use FIONREAD ?
    cOSHandleSet hs(0);
    if (hs.WaitForObjects(0) == S_OK) return 1;
    return 0;
#endif
}

int cAppConsole::ReadKeyRaw() const {
#ifdef _WIN32
    // NOTE: _WIN32 fgetc(stdin) will block until the ENTER key is pressed ! then feed chars until it runs out.
#if USE_CRT
    if (!_isKeyEnterMode) {
        if (_isKeyEchoMode) return ::_getche();
        return ::_getch();  // don't wait for ENTER. return as we get them.
    }
    return ::getchar();  // buffer chars and returns when ENTER is pressed for a whole line.
#else
    // https://stackoverflow.com/questions/24708700/c-detect-when-user-presses-arrow-key
    auto h = GetStd(AppStd_t::_stdin);
    ::INPUT_RECORD eventBuffer[1];
    DWORD dwEventsRead = 0;
    if (!_GTN(::ReadConsoleInput)(h, eventBuffer, 1, &dwEventsRead)) {
        return 0;
    }
    // eventBuffer[i].EventType == KEY_EVENT && eventBuffer[i].Event.KeyEvent.bKeyDown
    return eventBuffer[0].Event.KeyEvent.wVirtualKeyCode;
#endif
#elif defined(__linux__)
    // @note NON raw mode always echoes.
    return ::getchar();  // buffer chars and returns when ENTER is pressed for a whole line.
#endif
}

int cAppConsole::ReadKeyWait() {
    if (!isConsoleMode()) return -1;
    int iKey = ReadKeyRaw();
    if (iKey == HIBYTE(cCmdInput::kKeyPrefix)) {  // Multi byte keys.
        iKey = cCmdInput::kKeyPrefix | ReadKeyRaw();
    }
    return iKey;
}

int cAppConsole::ReadKey() {
    if (get_KeyReadQty() <= 0) return -1;  // none ready.
    return ReadKeyWait();
}

HRESULT cAppConsole::ReadStringLine(cSpanX<char> ret) {
    SetKeyModes(false, false);
    for (;;) {
        const int iKey = ReadKeyWait();  // 0xe0 = first of 2 char seq. 0xe04b = down arrow
        if (iKey <= 0) return 0;         // Cancel.
        HRESULT hRes = _CmdInput.AddInputKey(iKey, this, true);
        if (FAILED(hRes)) return hRes;
        if (hRes == 2) {  // done
            StrT::CopyStr<char>(ret, _CmdInput._sCmd.get_SpanStr());
            return 2;
        }
    }
}

}  // namespace Gray
#endif  // UNDER_CE
