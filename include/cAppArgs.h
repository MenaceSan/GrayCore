//
//! @file cAppArgs.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

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
/// Parse and store command line args used to start an app. Handle Windows and POSIX/DOS formats.
/// Use FILECHAR_t. see k_ARG_ARRAY_MAX
/// Like MFC CCommandLineInfo
/// </summary>
class GRAYCORE_LINK cAppArgs {
    /// <summary>
    /// The unparsed raw command line arguments. NOT including 'appname.exe'. Maybe generated as needed in get_ArgsStr(). if main() style entry.
    /// </summary>
    cStringF m_sArguments;

 public:
    /// <summary>
    /// Array of parsed m_sArguments. [0]=appname.exe, [1]=first arg. NOT nullptr terminated like APP_ARGS_t. Honors quoted text.
    /// TODO Pointers into StrBuilder ??
    /// </summary>
    cArrayString<FILECHAR_t> m_aArgs;

 protected:
     void InitArgsArray(ITERATE_t argc, APP_ARGS_t ppszArgs, bool sepEquals);

 public:
    /// <summary>
    /// Is FILECHAR_t char 'ch' a command line switch char?
    /// </summary>
    /// <param name="ch"></param>
    /// <returns></returns>
    static constexpr bool IsArgSwitch(wchar_t ch) noexcept {
        return ch == '-' || ch == '/';
    }
    static inline bool IsArg(const FILECHAR_t* pszArg) noexcept {
        if (StrT::IsWhitespace<FILECHAR_t>(pszArg)) return false;
        if (IsArgSwitch(pszArg[0])) return false;
        return true;
    }

    cStringF get_ArgsStr() const noexcept;
    ITERATE_t get_ArgsQty() const noexcept;

    /// <summary>
    /// Get a command line argument parsed param by index.
    /// Command line arguments honor "quoted strings" as a single argument.
    /// [0]=app path.
    /// </summary>
    /// <param name="i"></param>
    /// <returns>"" = at or past end or array of args.</returns>
    cStringF GetArgEnum(ITERATE_t i) const;  /// command line arg.

    void InitArgsWin(const FILECHAR_t* pszCommandArgs, const FILECHAR_t* pszSep = nullptr);
    void InitArgsPosix(int argc, APP_ARGS_t argv);

    ITERATE_t FindCommandArg(const FILECHAR_t* pszCommandArg, bool bRegex = true, bool bIgnoreCase = true) const;
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
