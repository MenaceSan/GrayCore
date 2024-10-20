//! @file cAppConsole.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cAppConsole_H
#define _INC_cAppConsole_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "StrArg.h"
#include "cSingleton.h"
#include "cStream.h"
#include "cCmdInput.h"

#if !defined(UNDER_CE)
namespace Gray {
/// <summary>
/// Standard streams/handles. True for both __linux__ and _WIN32. (though __linux__ implementation is hidden)
/// </summary>
enum class AppStd_t : BYTE {
    _stdin = 0,   /// stdin  (&__iob_func()[0]) -> GetStdHandle(STD_INPUT_HANDLE) = STDIN_FILENO
    _stdout = 1,  /// stdout (&__iob_func()[1]) -> GetStdHandle(STD_OUTPUT_HANDLE) = STDOUT_FILENO
    _stderr = 2,  /// stderr (&__iob_func()[2]) -> GetStdHandle(STD_ERROR_HANDLE) = STDERR_FILENO
    _QTY = 3,
};

/// <summary>
/// What type of console is connected?
/// </summary>
enum class AppCon_t : signed char {
    _Unknown = -1,
    _None = 0,    /// NOT a console mode app.
    _Proc = 1,    /// Native mode. Process was build as _CONSOLE mode. stdin,stdout already setup.
    _Attach = 2,  /// Attached to parent console. must call FreeConsole()
    _Create = 3,  /// Created my own console. must call FreeConsole()
};

/// <summary>
/// Singleton to Manage console output/input for this app. use of printf() etc
/// This allows apps not compiled in _CONSOLE mode to attach to a console if they are started in one (or create one if not).
/// </summary>
class GRAYCORE_LINK cAppConsole final : public cSingleton<cAppConsole>, public ITextWriter {
#if defined(_WIN32)
    ::HANDLE _hStd[static_cast<int>(AppStd_t::_QTY)];  /// stdin,stdout,stderr as cOSHandle. But i don't need to close these ? ::GetStdHandle()
#elif defined(__linux__)
    // __iob_func
#endif

    bool _isKeyEchoMode = true;  /// default true = echo the keys to the display.
    bool _isKeyEnterMode = true;  /// default true = wait for enter before return. false = get each key char as it comes in (raw).

    AppCon_t _eConsoleType = AppCon_t::_Unknown;  /// 2 = I called AttachConsole(), 3 = I called AllocConsole() and must call FreeConsole()
    bool _isConsoleParent = false;                 /// My parent process is a console. I may attach to it.
    int _iAllocConsoleCount = 0;  /// I had to create my own console. must call FreeConsole() this many times.

    mutable cThreadLockableX _Lock;  /// serialize multiple threads to console.

 public:
    DECLARE_cSingleton(cAppConsole);
    static const COUNT_t k_MAX_CONSOLE_LINES = 500;  /// arbitrary max lines shown at once.
    cCmdInput _CmdInput;    // For ReadStringLine

 protected:
    cAppConsole();

    /// <summary>
    /// Is the process already running from a console window? _CONSOLE
    /// Was process started by a console ?
    /// e.g. Linux applications started from GNOME desktop have no console window.
    /// @note printf() might not work until i call RedirectIOToConsole()
    /// @note "GetConsoleWindow();" returns null if Windows 10 and i was a windows app started in console.
    /// </summary>
    void CheckConsoleMode() noexcept;
    bool AttachConsoleSync();
#if defined(_WIN32)
    HRESULT WriteStrH(::HANDLE h, const char* pszText);
#else
    HRESULT WriteStrH(::FILE* pFile, const char* pszText);
#endif
    int ReadKeyRaw() const;

 public:
    ~cAppConsole() override;

    /// <summary>
    /// started from command line ? Call AllocConsole to start using console.
    /// </summary>
    /// <returns></returns>
    bool HasConsoleParent() noexcept {
        CheckConsoleMode();
        return _isConsoleParent;
    }
    AppCon_t get_ConsoleMode() noexcept {
        CheckConsoleMode();
        return _eConsoleType;
    }

#if defined(_WIN32)
    ::HANDLE GetStd(AppStd_t i) const {
        return _hStd[(int)i];
    }
#endif

    /// <summary>
    /// Is the app already running in console mode? can i use printf() ?
    /// 1. I am _CONSOLE app, 2. I attached to my parent. 3. I created a console.
    /// </summary>
    /// <returns>bool</returns>
    bool isConsoleMode() noexcept {
        return get_ConsoleMode() != AppCon_t::_None;
    }

    /// <summary>
    /// make printf() go to the console. create console if needed.
    /// 1. Do i already have a console. use it. if _CONSOLE app.
    /// 2. Attach to my parents console if there is one.
    /// 3. if (bAttachElseAlloc) allocate a new console for this app.
    /// http://stackoverflow.com/questions/493536/can-one-executable-be-both-a-console-and-gui-application/494000#494000
    /// https://www.tillett.info/2013/05/13/how-to-create-a-windows-program-that-works-as-both-as-a-gui-and-console-application/
    /// </summary>
    /// <param name="bAttachElseAlloc"></param>
    /// <returns></returns>
    bool AttachOrAllocConsole(bool bAttachElseAlloc = true);

    /// <summary>
    /// Release my console. free it if i created it.
    /// </summary>
    void ReleaseConsole();

    HRESULT WriteStrErr(const char* pszText);

    /// <summary>
    /// Write to console. Does not support UNICODE.
    /// </summary>
    /// <param name="pszText"></param>
    /// <returns>HRESULT_WIN32_C(ERROR_HANDLE_DISK_FULL)</returns>
    HRESULT WriteStrOut(const char* pszText);

    HRESULT SetKeyModes(bool bEchoMode = true, bool bEnterMode = true);

    /// <summary>
    /// Are there keys to be read ?
    /// see http://www.linuxquestions.org/questions/programming-9/pausing-the-screen-44573/
    /// or http://cboard.cprogramming.com/c-programming/63166-kbhit-linux.html
    /// or http://www.control.auc.dk/~jnn/c2000/programs/mm5/keyboardhit/msg02541.html
    /// </summary>
    int get_KeyReadQty() const;

    /// <summary>
    /// Read a single key from conio stdin. block/wait for char.
    /// Arrows and escape key are sometimes special purpose here.
    /// </summary>
    /// <returns>-1 = block/wait for char failed. no char is available. else ASCII key. (like enum VK_TYPE)</returns>
    int ReadKeyWait();

    /// <summary>
    /// Get a single ASCII_t Key char produced by possibly multiple keys pressed (shift).
    /// Non blocking (Don't wait).
    /// similar to INPUTKEY_t and VK_TYPE -> VK_ESCAPE = INPUTKEY_t::_ESCAPE
    /// </summary>
    /// <returns>-1 = no char is available. else ASCII_t character produced by key or keys pressed.</returns>
    int ReadKey();

    HRESULT ReadStringLine(cSpanX<char> ret);

    /// <summary>
    /// support cStreamOutput.
    /// Do not assume line termination with \n
    /// </summary>
    /// <param name="pszStr"></param>
    /// <returns></returns>
    HRESULT WriteString(const char* pszStr) override {
        const HRESULT hRes = WriteStrOut(pszStr);
        if (FAILED(hRes)) return hRes;
        return 1;
    }

    /// <summary>
    /// support cStreamOutput.
    /// Do not assume line termination with \n
    /// </summary>
    /// <param name="pszStr"></param>
    /// <returns></returns>
    HRESULT WriteString(const wchar_t* pszStr) override {
        const HRESULT hRes = WriteStrOut(StrArg<char>(pszStr));
        if (FAILED(hRes)) return HRESULT_WIN32_C(ERROR_WRITE_FAULT);
        return 1;
    }
};
}  // namespace Gray
#endif  // UNDER_CE
#endif  // _INC_cAppConsole_H
