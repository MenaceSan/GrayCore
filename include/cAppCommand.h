//! @file cAppCommand.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cAppCommand_H
#define _INC_cAppCommand_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cAppArgs.h"
#include "cObject.h"

namespace Gray {
/// <summary>
/// An integer that represents some static command that we might execute. cAppCommand like: DLGID_t
/// Some user initialed command/change/action.
/// </summary>
typedef WORD CommandId_t;
typedef HRESULT(GRAYCALL* AppCommandF_t)(const cAppArgs& args, int iArgN);

/// <summary>
/// define a named command line switch that does something. similar to MFC CCmdTarget, like CCmdUI ? IIniBaseSetter?
/// Abstract Base class for a command handler (plugin). "Command Pattern".
/// typically static allocated. Correlates to a CommandId_t.
/// </summary>
struct GRAYCORE_LINK cAppCommand : public cObject {
    const ATOMCHAR_t* _pszName;    /// symbolic name for -command or /command (case insensitive). MUST be unique. MUST be first so we can sort.
    const ATOMCHAR_t* _pszAbbrev;  /// optional abbreviated -command or /command (case sensitive), nullptr allowed
    const GChar_t* _pszHelpArgs;   /// describe any extra args this function might take. "[optional arg]. nullptr = takes none.
    const GChar_t* _pszHelp;       /// verbose help description. (tool tip)
    AppCommandF_t _pCommand;       /// optional function pointer to implement else we can override DoCommand().

 public:
    cAppCommand(const ATOMCHAR_t* pszAbbrev, const ATOMCHAR_t* pszName, const GChar_t* pszHelpArgs = nullptr, const GChar_t* pszHelp = nullptr, AppCommandF_t pCommand = nullptr) noexcept
        : _pszName(pszName), _pszAbbrev(pszAbbrev), _pszHelpArgs(pszHelpArgs), _pszHelp(pszHelp), _pCommand(pCommand) {
        ASSERT_NN(_pszName);
    }
    virtual ~cAppCommand() {}

    bool IsMatch(const ATOMCHAR_t* p) const;
    void GetHelpText(StrBuilder<GChar_t>& sb) const;

    virtual const ATOMCHAR_t* get_Title() const {  // allow override	for more friendly short title for the command
        return _pszName;
    }

    /// <summary>
    /// Execute a command via function pointer or override this to exec in some other way.
    /// Default (non override) behavior is exec the command via _pCommand function pointer.
    /// @note check return. it can consume more arguments (or not).
    /// </summary>
    /// <param name="iArgN">index in args for next.</param>
    /// <returns># of EXTRA args consumed. or -lt- 0 = error.</returns>
    virtual HRESULT DoCommand(const cAppArgs& args, int iArgN = 0) {
        if (_pCommand == nullptr) return E_NOTIMPL;
        return _pCommand(args, iArgN);
    }
};

/// <summary>
/// Interface to manage a list of possible cAppCommand(s)
/// </summary>
struct IAppCommands {
    virtual cAppCommand* GetCommand(CommandId_t id) const = 0;
    virtual cAppCommand* FindCommand(const ATOMCHAR_t* pszName) const = 0;
};

/// <summary>
/// manage a list of named cAppCommand
/// </summary>
struct GRAYCORE_LINK cAppCommands : public IAppCommands {
    /// <summary>
    /// built a list of possible commands. Dynamically add new command handlers to the app. to process cAppArgs.
    /// </summary>
    cArrayPtr<cAppCommand> _a;

    /// <summary>
    /// Add or override existing cAppCommand. ASSUME static allocation of cAppCommand.
    /// </summary>
    cAppCommand* RegisterCommand(cAppCommand& cmd);
    cAppCommand* GetCommand(CommandId_t id) const override;
    cAppCommand* FindCommand(const ATOMCHAR_t* pszName) const override;
};
}  // namespace Gray
#endif
