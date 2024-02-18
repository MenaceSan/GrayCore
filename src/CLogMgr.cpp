//! @file cLogMgr.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cAppState.h"
#include "cCodeProfiler.h"
#include "cLogEvent.h"
#include "cLogMgr.h"
#include "cStream.h"
#include "cString.h"

namespace Gray {
TIMESECD_t cLogMgr::sm_TimePrevException;  /// doesn't actually matter what this value is at init time.

//************************************************************************

cLogNexus::cLogNexus(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel) : m_LogFilter(uAttrMask, eLogLevel) {}

HRESULT cLogNexus::addEvent(cLogEvent& rEvent) noexcept {  // virtual
    CODEPROFILEFUNC();
    if (!cLogMgr::isSingleCreated()) {
        DEBUG_CHECK(cLogMgr::isSingleCreated());
        return HRESULT_WIN32_C(ERROR_EMPTY);
    }
    if (!IsLogged(rEvent.get_LogAttrMask(), rEvent.get_LogLevel())) {  // I don't care about these ?
        return HRESULT_WIN32_C(ERROR_EMPTY);                           // no sinks care about this.
    }
    if (rEvent.m_sMsg.IsEmpty()) return E_INVALIDARG;

    m_LogThrottle.m_nQtyLogLast++;

    int iUsed = 0;
    cLogEventPtr pEventHolder(&rEvent);  // one more reference on this.
    HRESULT hResAdd = S_OK;

    for (int i = 0;; i++) {
        cLogSink* pSink;
        {
            const auto guard(m_LockLog.Lock());  // sync multiple threads.
            pSink = m_aSinks.GetAtCheck(i);
            if (pSink == nullptr) break;
        }
        hResAdd = pSink->addEvent(rEvent);
        if (hResAdd > 0) {
            iUsed++;  // handled.
            continue;
        }
        if (hResAdd < 0) continue;  // another form of filtering. don't process this sink.
        // FAILED ? or filtered write.
    }

#ifdef _DEBUG
    if (cAppState::isDebuggerPresent()) FlushLogs();
#endif
    return iUsed > 0 ? iUsed : hResAdd;  // did we do any work?
}

HRESULT cLogNexus::FlushLogs() {  // virtual
    const auto guard(m_LockLog.Lock());
    for (ITERATE_t i = 0;; i++) {
        cLogSink* pSink = EnumSinks(i);
        if (pSink == nullptr) break;
        pSink->FlushLogs();
    }
    return S_OK;
}

bool cLogNexus::HasSink(cLogSink* pSinkFind, bool bDescend) const {
    if (pSinkFind == nullptr) return false;
    const auto guard(m_LockLog.Lock());
    for (ITERATE_t i = 0;; i++) {
        const cLogSink* pSink = EnumSinks(i);
        if (pSink == nullptr) return false;  // end marker ?
        if (pSinkFind == pSink) return true;
        if (bDescend) {
            const cLogNexus* pLogNexus = pSink->get_ThisLogNexus();
            if (pLogNexus != nullptr && pLogNexus->HasSink(pSinkFind, true)) return true;
        }
    }
}

HRESULT cLogNexus::AddSink(cLogSink* pSinkAdd) {
    if (pSinkAdd == nullptr) return E_POINTER;
    if (HasSink(pSinkAdd, true)) return S_FALSE;  // no dupes.
    const auto guard(m_LockLog.Lock());
    m_aSinks.AddHead(pSinkAdd);
    return S_OK;
}

bool cLogNexus::RemoveSink(cLogSink* pSinkRemove, bool bDescend) {
    if (pSinkRemove == nullptr) return false;
    bool bRemoved = false;
    const auto guard(m_LockLog.Lock());
    for (ITERATE_t i = 0;; i++) {
        cLogSink* pSink = EnumSinks(i);
        if (pSink == nullptr) return bRemoved;
        if (pSinkRemove == pSink) {
            m_aSinks.RemoveAt(i);
            i--;
            bRemoved = true;
        } else if (bDescend) {
            const cLogNexus* pLogNexus = pSink->get_ThisLogNexus();
            if (pLogNexus != nullptr) {
                bRemoved |= const_cast<cLogNexus*>(pLogNexus)->RemoveSink(pSinkRemove, true);
            }
        }
    }
}

cLogSink* cLogNexus::FindSinkType(const TYPEINFO_t& rType, bool bDescend) const {
    const auto guard(m_LockLog.Lock());
    for (ITERATE_t i = 0;; i++) {
        const cLogSink* pSink = EnumSinks(i);
        if (pSink == nullptr) break;
        if (typeid(*pSink) == rType)  // already here.
            return const_cast<cLogSink*>(pSink);
        if (bDescend) {
            const cLogNexus* pLogNexus = pSink->get_ThisLogNexus();
            if (pLogNexus != nullptr) {
                pSink = pLogNexus->FindSinkType(rType, true);
                if (pSink != nullptr) return const_cast<cLogSink*>(pSink);
            }
        }
    }
    return nullptr;
}

//************************************************************************

cLogMgr::cLogMgr()
    : cSingleton<cLogMgr>(this, typeid(cLogMgr)),
#ifdef _DEBUG
      cLogNexus((LOG_ATTR_MASK_t)LOG_ATTR_ALL_MASK, LOGLVL_t::_TRACE)
#else
      cLogNexus((LOG_ATTR_MASK_t)LOG_ATTR_ALL_MASK, LOGLVL_t::_INFO)
#endif
{
    //! ideally this is in the very first static initialize.
#ifdef _DEBUG
    if (cAppState::isDebuggerPresent()) {
        cLogSinkDebug::AddSinkCheck(this);  // send logs to the debugger.
    }
#endif
}

HRESULT cLogMgr::WriteString(const char* pszStr)  { // override
    // Someone wants to dump raw text at the logger. Not sure why.
    return addEventS(LOG_ATTR_PRINT | LOG_ATTR_SCRIPT, LOGLVL_t::_INFO, pszStr);
}

#ifdef _CPPUNWIND
void cLogMgr::LogExceptionV(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, va_list vargs) noexcept {
    //! if ( this == nullptr ) may be OK?
    CODEPROFILEFUNC();

    TIMESECD_t tNowSec = static_cast<TIMESECD_t>(cTimeSys::GetTimeNow() / cTimeSys::k_FREQ);
    if (sm_TimePrevException == tNowSec)  // prevent message floods. 1 per sec.
        return;
    sm_TimePrevException = tNowSec;

    if (!cMem::IsValidApp(this)) {
        // Nothing we can do about this?!
        ASSERT(0);
        return;
    }

    // Keep a record of what we catch.
    try {
        LOGCHAR_t szMsg[cExceptionHolder::k_MSG_MAX_SIZE];
        StrBuilder<LOGCHAR_t> sb(TOSPAN(szMsg));
        if (pEx == nullptr) {
            sb.AddStr("Unknown exception");
        } else {
            pEx->GetErrorMessage(sb);
        }

        sb.AddStr(", in ");
        if (vargs == nullptr || vargs == k_va_list_empty) {
            sb.AddStr(pszCatchContext);
        } else {
            sb.AddFormatV(pszCatchContext, vargs);
        }

        LOGLVL_t eSeverity = (pEx == nullptr) ? LOGLVL_t::_CRIT : (pEx->get_Severity());
        addEventS(LOG_ATTR_DEBUG, eSeverity, szMsg);
    } catch (...) {
        // Not much we can do about this.
    }
}

void _cdecl cLogMgr::LogExceptionF(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, ...) noexcept {
    va_list vargs;
    va_start(vargs, pszCatchContext);
    LogExceptionV(pEx, pszCatchContext, vargs);
    va_end(vargs);
}
#endif

//************************************************************************

cLogSubject::cLogSubject(const char* pszSubject) : m_pszSubject(pszSubject) {}

bool cLogSubject::IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel) const noexcept {
    return cLogMgr::I().IsLogged(uAttrMask, eLogLevel);
}
HRESULT cLogSubject::addEvent(cLogEvent& rEvent) noexcept {  // override
    rEvent.m_pszSubject = m_pszSubject;                      // categorize with this subject.
    return cLogMgr::I().addEvent(rEvent);
}
}  // namespace Gray
