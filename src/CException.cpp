//! @file cException.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrBuilder.h"
#include "StrU.h"
#include "cCodeProfiler.h"
#include "cException.h"
#include "cLogMgr.h"
#include "cString.h"
#include "cTimeSys.h"

namespace Gray {
cException* cExceptionHolder::get_Ex() const {
    return this->get_PtrDyn<cException>();
}

BOOL cExceptionHolder::GetErrorMessage(StrBuilder<LOGCHAR_t>& sb) const {
    if (!this->isValidPtr()) {
        sb.AddChar('?');
        return false;
    }

#ifdef _MFC_VER  // using _MFC_VER.
#if USE_UNICODE
    GChar_t szTmp[cExceptionHolder::k_MSG_MAX_SIZE];
    get_Ptr()->GetErrorMessage(szTmp, (UINT)_countof(szTmp), nullptr);
    StrU::UNICODEtoUTF8(lpszError, nLenMaxError, TOSPAN(szTmp));
    return true;
#else
    return get_Ptr()->GetErrorMessage(lpszError, (UINT)nLenMaxError, nullptr);
#endif
#else
    cException* pEx = get_Ex();
    if (pEx == nullptr) {  // NOT cException based. STL exception.
        sb.AddStr(get_Ptr()->what());
        return true;
    }
    return pEx->GetErrorMessage(sb, nullptr);
#endif
}
cStringL cExceptionHolder::get_ErrorStr() const {
    if (!this->isValidPtr()) return "?";

#ifdef _MFC_VER  // using _MFC_VER.
    LOGCHAR_t szTmp[cExceptionHolder::k_MSG_MAX_SIZE];
    GetErrorMessage(StrBuilder<LOGCHAR_t>(TOSPAN(szTmp)));
    return szTmp;
#else
    cException* pEx = get_Ex();
    if (pEx == nullptr) {  // raw STL exception.
        return get_Ptr()->what();
    }
    return pEx->get_ErrorStr();
#endif
}
LOGLVL_t cExceptionHolder::get_Severity() const {
    cException* pEx = get_Ex();
    if (pEx == nullptr) {  // NOT cException based. raw STL/MFC  exception.
        return LOGLVL_t::_CRIT;
    }
    return pEx->get_Severity();
}

//*********************************************************************
const LOGCHAR_t* cException::k_szDescriptionDefault = "Exception";

IMPLEMENT_DYNAMIC(cException, cExceptionBase);

BOOL cException::GetErrorMessage(StrBuilder<GChar_t>& sb, UINT* pnHelpContext) {  // virtual
    //! Compatible with MFC CException and CFileException. Assume this is typically overridden at a higher level.
    //! @note stupid BOOL return is for MFC compatibility.
    //! allow UNICODE version of this.
    CODEPROFILEFUNC();
    UNREFERENCED_PARAMETER(pnHelpContext);
    sb.AddFormat(_GT("%s'%s'"), StrArg<GChar_t>(cLogLevel::GetPrefixStr(m_eSeverity)), StrArg<GChar_t>(m_pszDescription));
    return true;
}

GRAYCORE_LINK cStringL cException::get_ErrorStr() {  // similar to STL what()
    // Get error in UTF8.
    GChar_t szTmp[cExceptionHolder::k_MSG_MAX_SIZE];
    StrBuilder<GChar_t> sb(TOSPAN(szTmp));
    GetErrorMessage(sb, nullptr);
    return szTmp;
}

//***************************************************************************

BOOL cExceptionHResult::GetErrorMessage(StrBuilder<GChar_t>& sb, UINT* pnHelpContext) {  // virtual
    //! Compatible with MFC CException and CFileException
    //! @note pointless BOOL return is for MFC compatibility.
    CODEPROFILEFUNC();
    UNREFERENCED_PARAMETER(pnHelpContext);

    if (m_hResultCode != S_OK) {
        // return the message defined by the system for the error code
        sb.AddStr(_GT("Error Pri="));
        sb.AddUInt((UINT)m_eSeverity);
        sb.AddStr(_GT(", Code="));
        sb.AddUInt((UINT)m_hResultCode, 0x10);
        sb.AddStr(_GT("("));
        HResult::GetTextV(m_hResultCode, sb, k_va_list_empty);  // pnHelpContext
        sb.AddStr(_GT("), Desc="));
        sb.AddStrQ(m_pszDescription);
        return true;
    }
    return SUPER_t::GetErrorMessage(sb, pnHelpContext);
}
}  // namespace Gray
