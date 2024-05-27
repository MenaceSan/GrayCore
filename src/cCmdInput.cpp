//! @file cCmdInput.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#include "pch.h"
#include "StrCharAscii.h"
#include "cArray.h"
#include "cCmdInput.h"
#include "cLogMgr.h"

namespace Gray {

void cCmdInput::AddCmd(const GChar_t* pszCmd) {
    const ITERATE_t iRet = m_aCmdHistory.AddUniqueMax(pszCmd, m_iMaxCommandQty);
    if (iRet < 0) return;
    m_nCurCommandIndex = iRet;
}

cString cCmdInput::ScrollCmd(int iKey) {
    const ITERATE_t iQty = m_aCmdHistory.GetSize();
    if (iQty <= 0 || iKey == (char)ASCII_t::_ESC) {
        m_nCurCommandIndex = iQty;
        return "";
    }

    if (iKey == kKeyUp) {
        m_nCurCommandIndex--;
    } else {
        m_nCurCommandIndex++;  // kKeyDown
    }

    if (m_nCurCommandIndex >= iQty) m_nCurCommandIndex = iQty - 1;
    if (m_nCurCommandIndex < 0) m_nCurCommandIndex = 0;

    return m_aCmdHistory[m_nCurCommandIndex];
}

HRESULT cCmdInput::AddInputKey(int iKey, ITextWriter* pOut, bool bEcho) {
    if (iKey == 0) return 0;  // just ignore this.

    if (iKey == '\r' || iKey == '\n') {
        if (bEcho && pOut != nullptr) {
            pOut->WriteString(FILE_EOL);
        }
        this->AddCmd(m_sCmd);
        m_bCmdComplete = true;
        return 2;  // process line.
    }

    if (m_bCmdComplete) {
        m_sCmd = "";             // clear previous command.
        m_bCmdComplete = false;  // New command.
    }

    const StrLen_t iLen = m_sCmd.GetLength();

    // History.
    switch (iKey) {
        case (char)ASCII_t::_ESC:  // INPUTKEY_ESCAPE = ASCII_t::_ESC = 27
            if (iLen == 0) return HRESULT_WIN32_C(ERROR_CANCELLED);
            [[fallthrough]];

        case kKeyUp:               // 0x26 = 38. like VK_UP
        case kKeyDown:             // 0x28 = 40. like VK_DOWN
            if (pOut != nullptr) {
                pOut->WriteCharRepeat('\b', iLen);  // clear whats on the line now.
            }
            m_sCmd = this->ScrollCmd(iKey);
            if (pOut != nullptr) {
                pOut->WriteString(m_sCmd);
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
        m_sCmd = cString(m_sCmd.Left(iLen - 1));
        return 1;
    }

    if (iLen >= k_LEN_MAX_CSYM) {
        if (pOut != nullptr) {
            pOut->WriteString("Command text is too long!");
        }
        return HRESULT_WIN32_C(RPC_S_STRING_TOO_LONG);
    }

    m_sCmd += CastN(char, iKey);
    return 1;   // key added.
}
}  // namespace Gray
