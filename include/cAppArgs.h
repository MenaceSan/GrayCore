//! @file cAppArgs.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cAppArgs_H
#define _INC_cAppArgs_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cArrayString.h"
#include "cFilePath.h"

namespace Gray {
typedef const FILECHAR_t* const* APP_ARGS_t;  /// the args passed to main() nullptr terminated array. const

/// <summary>
/// Parse and store command line args/commands used to start an app. Handle Windows and POSIX/DOS formats.
/// Use FILECHAR_t. see k_ARG_ARRAY_MAX
/// Like MFC CCommandLineInfo
/// </summary>
class GRAYCORE_LINK cAppArgs {
    friend class cAppState;

    /// <summary>
    /// The unparsed raw command line arguments. NOT including 'appname.exe'. Maybe generated as needed in get_ArgsStr(). if main() style entry.
    /// </summary>
    cStringF _sArguments;

    /// <summary>
    /// Const Array of parsed _sArguments. [0]=appname.exe, [1]=first arg. NOT nullptr terminated like APP_ARGS_t. Honors quoted text.
    /// TODO cArrayPtr into cBlob? like cIniSectionData?
    /// </summary>
    cArrayString<FILECHAR_t> _aArgs;

 protected:
    void InitArgsArray(ITERATE_t argc, APP_ARGS_t ppszArgs, bool sepEquals);

 public:
    cAppArgs() noexcept {}
    cAppArgs(const FILECHAR_t* p);

    /// <summary>
    /// Is FILECHAR_t char 'ch' a command line switch char?
    /// </summary>
    /// <param name="ch"></param>
    /// <returns></returns>
    static constexpr bool IsArgSwitch(wchar_t ch) noexcept {
        return ch == '-' || ch == '/';
    }
    /// <summary>
    /// Can the next argument be considered a "sub/secondary" arg. Apply/Modify the previous arg?
    /// </summary>
    /// <param name="pszArg"></param>
    /// <returns></returns>
    static inline bool IsArgMod(const FILECHAR_t* pszArg) noexcept {
        return !StrT::IsWhitespace<FILECHAR_t>(pszArg) && !IsArgSwitch(pszArg[0]);
    }

    /// <summary>
    /// Get Unparsed command line args as a single line/string. might be used for cOSProcess.
    /// Does not contain App.exe name.
    /// </summary>
    cStringF get_ArgsStr() const noexcept {
        return _sArguments;
    }

    /// <summary>
    /// Get Qty of args including app name.
    /// </summary>
    /// <returns>1 = just app path. 2 = app has 1 argument value. etc.</returns>
    ITERATE_t get_ArgsQty() const noexcept;

    /// <summary>
    /// Get a command line argument parsed param by index.
    /// Command line arguments honor "quoted strings" as a single argument.
    /// [0]=app path.
    /// </summary>
    /// <param name="i"></param>
    /// <returns>"" = at or past end or array of args.</returns>
    cStringF GetArgEnum(ITERATE_t i) const;  /// command line arg.

    /// <summary>
    /// set (unparsed) _sArguments and parse pszCommandArgs to _aArgs. Windows WinMain() style init.
    /// Similar to _WIN32  CommandLineToArgvW()
    /// Honor quotes.
    /// </summary>
    /// <param name="pszCommandArgs">assumed to NOT contain the app path name.</param>
    /// <param name="pszSep"></param>
    void InitArgsLine(const FILECHAR_t* pszCommandArgs, const FILECHAR_t* pszSep = nullptr);

    /// <summary>
    /// Set pre-parsed arguments from console style like: Posix, _CONSOLE or DOS style arguments.
    /// main() style init.
    /// @note M$ unit tests will block arguments!
    /// ppszArgs[0] = app path
    /// </summary>
    void InitArgsPosix(int argc, APP_ARGS_t argv);

    /// <summary>
    /// For debug
    /// </summary>
    ITERATE_t AppendArg(const FILECHAR_t* pszCmd, bool sepEquals);

    /// <summary>
    /// Find a command line arg as regex or ignoring case.
    /// bRegex = Search for a wildcard prefix.
    /// </summary>
    ITERATE_t FindCommandArg(const FILECHAR_t* pszCommandArg, bool bRegex = true, bool bIgnoreCase = true) const;

    /// <summary>
    /// Find one of several possible command line args maybe ignoring case. nullptr terminated list.
    /// </summary>
    /// <param name="bIgnoreCase"></param>
    /// <param name="pszCommandArgFind"></param>
    /// <param name=""></param>
    /// <returns>index of the first one.</returns>
    ITERATE_t _cdecl FindCommandArgs(bool bIgnoreCase, const FILECHAR_t* pszCommandArgFind, ...) const;

    /// <summary>
    /// Do we have a particular argument? pszCommandArg
    /// </summary>
    bool HasCommandArg(const FILECHAR_t* pszCommandArg, bool bRegex = true, bool bIgnoreCase = true) const {
        const ITERATE_t iRet = FindCommandArg(pszCommandArg, bRegex, bIgnoreCase);
        return iRet >= 0;
    }
};
}  // namespace Gray
#endif
