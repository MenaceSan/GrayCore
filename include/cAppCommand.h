//! @file cAppCommand.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cAppCommand_H
#define _INC_cAppCommand_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cAppArgs.h"

namespace Gray {
/// <summary>
/// An integer that represents some static command that we might execute. cAppCommand like: DLGID_t
/// Some user initialed command/change/action.
/// </summary>
typedef WORD CommandId_t;
typedef HRESULT(GRAYCALL* AppCommandF_t)(const cAppArgs& args, int iArgN);  // FARPROC optional.

/// <summary>
/// define a named command line switch that does something. similar to MFC CCmdTarget, like CCmdUI ?
/// Abstract Base class for a command handler (plugin).
/// typically static allocated. Correlates to a CommandId_t.
/// </summary>
struct GRAYCORE_LINK cAppCommand {
    const ATOMCHAR_t* m_pszName;    /// symbolic name for -command or /command (case insensitive). MUST be unique. MUST be first so we can sort.
    const ATOMCHAR_t* m_pszAbbrev;  /// optional abbreviated -command or /command (case sensitive), nullptr allowed
    const GChar_t* m_pszHelpArgs;   /// describe any extra args this function might take. "[optional arg]. nullptr = takes none.
    const GChar_t* m_pszHelp;       /// verbose help description. (tool tip)
    AppCommandF_t m_pCommand;       /// optional function pointer to implement else we can override DoCommand().

 public:
    cAppCommand(const ATOMCHAR_t* pszAbbrev, const ATOMCHAR_t* pszName, const GChar_t* pszHelpArgs, const GChar_t* pszHelp,
                AppCommandF_t pCommand = nullptr) noexcept
        : m_pszAbbrev(pszAbbrev), m_pszName(pszName), m_pszHelpArgs(pszHelpArgs), m_pszHelp(pszHelp), m_pCommand(pCommand) {}
    virtual ~cAppCommand() {}

    bool IsMatch(const ATOMCHAR_t* p) const;
    cString get_HelpText() const;

    virtual const ATOMCHAR_t* get_Title() const { // allow override	for more friendly short title for the command
        return m_pszName;
    }

    /// <summary>
    /// Execute a command via function pointer or override this to exec in some other way.
    /// Default (non override) behavior is exec the command via m_pCommand function pointer.
    /// @note check return. it can consume more arguments (or not).
    /// </summary>
    /// <param name="iArgN"></param>
    /// <param name="pszArgs"></param>
    /// <returns># of EXTRA args consumed. or -lt- 0 = error.</returns>
    virtual HRESULT DoCommand(const cAppArgs& args, int iArgN = 0) {
        if (m_pCommand == nullptr) return E_NOTIMPL;
        return m_pCommand(args, iArgN);
    }
};

/// <summary>
/// Interface to manage a list of cAppCommand(s)
/// </summary>
struct IAppCommands {
    virtual cAppCommand* FindCommand(CommandId_t id) const = 0;
    virtual cAppCommand* FindCommand(const ATOMCHAR_t* pszName) const = 0;
};

/// <summary>
/// manage a list of named cAppCommand
/// </summary>
struct GRAYCORE_LINK cAppCommands : public IAppCommands {
    /// <summary>
    /// built a list of possible commands. Dynamically add new command handlers to the app. to process cAppArgs.
    /// </summary>
    cArrayPtr<cAppCommand> m_a;

    /// <summary>
    /// Add or override existing cAppCommand. ASSUME static allocation of cAppCommand.
    /// </summary>
    cAppCommand* RegisterCommand(cAppCommand& cmd);
    cAppCommand* FindCommand(CommandId_t id) const override;
    cAppCommand* FindCommand(const ATOMCHAR_t* pszName) const override;
};
}  // namespace Gray
#endif
