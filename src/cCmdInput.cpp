//! @file cCmdInput.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#include "pch.h"
#include "StrCharAscii.h"
#include "cArray.h"
#include "cCmdInput.h"
#include "cLogMgr.h"

namespace Gray {

void cCmdInput::AddCmd(const GChar_t* pszCmd) {
    const ITERATE_t iRet = _aCmdHistory.AddUniqueMax(pszCmd, _iMaxCommandQty);
    if (iRet < 0) return;
    _nCurCommandIndex = iRet;
}

cString cCmdInput::ScrollCmd(int iKey) {
    const ITERATE_t iQty = _aCmdHistory.GetSize();
    if (iQty <= 0 || iKey == (char)ASCII_t::_ESC) {
        _nCurCommandIndex = iQty;
        return "";
    }

    if (iKey == kKeyUp) {
        _nCurCommandIndex--;
    } else {
        _nCurCommandIndex++;  // kKeyDown
    }

    if (_nCurCommandIndex >= iQty) _nCurCommandIndex = iQty - 1;
    if (_nCurCommandIndex < 0) _nCurCommandIndex = 0;

    return _aCmdHistory[_nCurCommandIndex];
}

HRESULT cCmdInput::AddInputKey(int iKey, ITextWriter* pOut, bool bEcho) {
    if (iKey == 0) return 0;  // just ignore this.

    if (iKey == '\r' || iKey == '\n') {
        if (bEcho && pOut != nullptr) {
            pOut->WriteString(FILE_EOL);
        }
        this->AddCmd(_sCmd);
        _isCmdComplete = true;
        return 2;  // process line.
    }

    if (_isCmdComplete) {
        _sCmd = "";             // clear previous command.
        _isCmdComplete = false;  // New command.
    }

    const StrLen_t iLen = _sCmd.GetLength();

    // History.
    switch (iKey) {
        case (char)ASCII_t::_ESC:  // INPUTKEY_t::_ESCAPE = ASCII_t::_ESC = 27
            if (iLen == 0) return HRESULT_WIN32_C(ERROR_CANCELLED);
            [[fallthrough]];

        case kKeyUp:               // 0x26 = 38. like VK_UP
        case kKeyDown:             // 0x28 = 40. like VK_DOWN
            if (pOut != nullptr) {
                pOut->WriteCharRepeat('\b', iLen);  // clear whats on the line now.
            }
            _sCmd = this->ScrollCmd(iKey);
            if (pOut != nullptr) {
                pOut->WriteString(_sCmd);
            }
            return 0;
        default:
            break;
    }

   if (iKey == '\b') {  // back space key
        if (iLen <= 0) return 0;
    }

    if (bEcho && pOut != nullptr) {
        pOut->WriteCharRepeat(CastN(char, iKey));  // Echo
    }

    if (iKey == '\b') {  // back space key
        _sCmd = cString(_sCmd.Left(iLen - 1));
        return 1;
    }

    if (iLen >= k_LEN_MAX_CSYM) {
        if (pOut != nullptr) {
            pOut->WriteString("Command text is too long!");
        }
        return HRESULT_WIN32_C(RPC_S_STRING_TOO_LONG);
    }

    _sCmd += CastN(char, iKey);
    return 1;   // key added.
}
}  // namespace Gray
