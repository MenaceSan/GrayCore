//! @file cLogSink.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrBuilder.h"
#include "cBits.h"
#include "cCodeProfiler.h"
#include "cLogEvent.h"
#include "cLogMgr.h"
#include "cLogSink.h"
#include "cSpan.h"

#ifdef UNDER_CE
#include <dbgapi.h>  // OutputDebugStringA
#endif

namespace Gray {
void cLogEvent::GetFormattedDefault(StrBuilder<LOGCHAR_t>& s) const {
    if (get_LogLevel() >= LOGLVL_t::_WARN) {
        s.AddStr(cLogLevel::GetPrefixStr(get_LogLevel()));
    }
#ifdef _DEBUG
    if (get_LogLevel() >= LOGLVL_t::_ERROR) {
        s.AddStr("!");
    }
#endif
    if (_pszSubject != nullptr) {
        // Show subject as Prefix?
        s.AddStr(_pszSubject);
        s.AddChar(':');
    }

    s.AddStr(_sMsg);
    const StrLen_t iLen = _sMsg.GetLength();
    ASSERT(iLen > 0);

    const LOGCHAR_t chLast = _sMsg[iLen - 1];
    const bool bHasCRLF = (chLast == '\r' || chLast == '\n');  // FILE_EOL ?
    if (!bHasCRLF && !IsLogAttrMask(LOG_ATTR_NOCRLF)) {
        s.AddStr(FILE_EOL);
    }
}

cStringL cLogEvent::get_FormattedDefault() const {
    LOGCHAR_t szTemp[StrT::k_LEN_Default];  // assume this magic number is big enough. Logging is weird and special so dont use dynamic memory.
    StrBuilder<LOGCHAR_t> s(TOSPAN(szTemp));
    GetFormattedDefault(s);
    return s.get_CPtr();
}

//**************************************************************

cLogThrottle::cLogThrottle() noexcept : _fLogThrottle(2000.0f), _TimeLogLast(cTimeSys::k_CLEAR), _nQtyLogLast(0) {}

//************************************************************************

HRESULT cLogProcessor::addEventS(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel, cStringL sMsg) noexcept {
    // Pre-Check if anyone cares before creating cLogEvent
    if (!IsLogged(uAttrMask, eLogLevel)) return HRESULT_WIN32_C(ERROR_EMPTY);  // no log Sinks care about this. toss it.
    cLogEventPtr pEvent(new cLogEvent(uAttrMask, eLogLevel, sMsg));
    return addEvent(*pEvent);
}

HRESULT cLogProcessor::addEventV(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel, const LOGCHAR_t* pszFormat, va_list vargs) noexcept {  // , cStringL sContext /* = "" */
    //! Add a log message (line) to the system in the sprintf() format (with arguments)
    //! ASSUME new line.
    //! @return <0 = failed, 0=not processed by anyone, # = number of processors.
    CODEPROFILEFUNC();
    if (StrT::IsNullOrEmpty(pszFormat)) return E_INVALIDARG;

    LOGCHAR_t szTemp[StrT::k_LEN_Default];  // assume this magic number is big enough.
    const StrLen_t iLen = StrT::vsprintfN(TOSPAN(szTemp), pszFormat, vargs);
    if (iLen <= 0) return E_INVALIDARG;
    return addEventS(uAttrMask, eLogLevel, szTemp);
}

HRESULT cLogProcessor::WriteString(const char* pszStr) {  // override
    return addEventS(LOG_ATTR_PRINT | LOG_ATTR_SCRIPT, LOGLVL_t::_INFO, pszStr);
}

//************************************************************************

bool cLogSink::RemoveSinkThis() {
    if (!cLogMgr::isSingleCreated()) return false;
    return cLogMgr::I().RemoveSink(this, true);
}

//************************************************************************

HRESULT cLogSinkDebug::WriteString(const LOGCHAR_t* pszText) {  // override
    //! Do NOT assume new line.
#ifdef _WIN32
    const auto guard(_Lock.Lock());
#ifdef UNDER_CE
    ::OutputDebugStringW(StrArg<wchar_t>(pszText));
#else
    ::OutputDebugStringA(pszText);
#endif
#elif defined(__linux__)
    // ::syslog()
#endif
    return S_OK;
}

HRESULT cLogSinkDebug::addEvent(cLogEvent& rEvent) noexcept {  // override;
    return WriteString(rEvent.get_FormattedDefault());
}

HRESULT GRAYCALL cLogSinkDebug::AddSinkCheck(cLogNexus* pLogger) {  // static
    //! Apps should call this in main() or in some static init.
    //! default logger = cLogMgr
    if (pLogger == nullptr) pLogger = cLogMgr::get_Single();
    if (pLogger->FindSinkType(typeid(cLogSinkDebug)) != nullptr) return S_FALSE;
    pLogger->AddSink(new cLogSinkDebug);
    return S_OK;
}
}  // namespace Gray
