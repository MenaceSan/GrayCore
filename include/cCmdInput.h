//! @file cCmdInput.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cCmdInput_H
#define _INC_cCmdInput_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "ITextWriter.h"
#include "StrArg.h"
#include "cArrayString.h"

namespace Gray {
/// <summary>
/// Build a command line from keys. Keeps last X command strings.
/// allow to scroll through them using arrows.
/// </summary>
class GRAYCORE_LINK cCmdInput {
    ITERATE_t _nCurCommandIndex = 0;  /// current command.

 public:
    cString _sCmd;                       /// The current working command line. AddInputKey
    bool _isCmdComplete = false;          /// _sCmd is ready.
    ITERATE_t _iMaxCommandQty = 32;      /// arbitrary limit to store this many.
    cArrayString<GChar_t> _aCmdHistory;  /// History of commands.

#ifdef _WIN32
    static const WORD kKeyPrefix = 0xe000;  // Read as 2 bytes. 224 + Another Byte.
    static const WORD kKeyUp = 0xe048;   // 0xe0 | H (72) // like VK_UP
    static const WORD kKeyDown = 0xe050;  // 0xe0 | P (80) // like VK_DOWN
#else
    static const WORD kKeyPrefix = 0x5b00;  // Read as 3 bytes.  ESC + this.
    static const WORD kKeyUp = 0x5b41;      // 65 // like VK_UP
    static const WORD kKeyDown = 0x5b42;    // 66 // like VK_DOWN
#endif

 public:
    /// How many commands in memory queue?
    ITERATE_t get_CmdQty() const noexcept {
        return _aCmdHistory.GetSize();
    }
    cString GetCmdAt(ITERATE_t i) const {
        return _aCmdHistory.GetAtCheck(i);
    }
    void RemoveAllCmds() {
        _sCmd = "";
        _aCmdHistory.RemoveAll();
        _nCurCommandIndex = 0;
    }

    /// <summary>
    /// Add command to the end. Enforce Max qty. NO duplicates.
    /// </summary>
    void AddCmd(const GChar_t* pszCmd);

    /// <summary>
    /// scroll _nCurCommandIndex up/down.
    /// </summary>
    /// <param name="iKey">kKeyUp, kKeyDown, ASCII_t::_ESC</param>
    /// <returns></returns>
    cString ScrollCmd(int iKey); 

    /// <summary>
    /// build up a command string from key presses.
    /// Add a key to an existing command from the console.
    /// </summary>
    /// <param name="iKey">kKeyUp, kKeyDown, ASCII_t::_ESC</param>
    /// <param name="pOut">ITextWriter</param>
    /// <param name="bEcho">chars as necessary/desired to pOut.</param>
    /// <returns>0 = no change keep going.
    ///  1 = keep processing.
    ///  2 = process this command. newline.
    ///  -lt-0 = dump this connection. Error.</returns>
    HRESULT AddInputKey(int iKey, ITextWriter* pOut = nullptr, bool bEcho = false);
};
}  // namespace Gray
#endif  // _INC_cCmdInput_H
