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
cSingleton_IMPL(cLogMgr);

//************************************************************************

cLogNexus::cLogNexus(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel) : _LogFilter(uAttrMask, eLogLevel) {}

HRESULT cLogNexus::addEvent(cLogEvent& rEvent) noexcept {  // virtual
    CODEPROFILEFUNC();
    if (!cLogMgr::isSingleCreated()) {
        DEBUG_CHECK(cLogMgr::isSingleCreated());
        return HRESULT_WIN32_C(ERROR_EMPTY);
    }
    if (!IsLogged(rEvent.get_LogAttrMask(), rEvent.get_LogLevel())) {  // I don't care about these ?
        return HRESULT_WIN32_C(ERROR_EMPTY);                           // no sinks care about this.
    }
    if (rEvent._sMsg.IsEmpty()) return E_INVALIDARG;

    _LogThrottle._nQtyLogLast++;

    int iUsed = 0;
    cLogEventPtr pEventHolder(&rEvent);  // one more reference on this.
    HRESULT hResAdd = S_OK;

    for (int i = 0;; i++) {
        cLogSink* pSink;
        {
            const auto guard(_LockLog.Lock());  // sync multiple threads.
            pSink = EnumSinks(i);
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
    const auto guard(_LockLog.Lock());
    for (cLogSink* pSink : _aSinks) {
        if (pSink == nullptr) break;
        pSink->FlushLogs();
    }
    return S_OK;
}

bool cLogNexus::HasSink(cLogSink* pSinkFind, bool bDescend) const {
    if (pSinkFind == nullptr) return false;
    const auto guard(_LockLog.Lock());
    for (const cLogSink* pSink : _aSinks) {
        if (pSink == nullptr) break;
        if (pSinkFind == pSink) return true;
        if (bDescend) {
            const cLogNexus* pLogNexus = pSink->get_ThisLogNexus();
            if (pLogNexus != nullptr && pLogNexus->HasSink(pSinkFind, true)) return true;
        }
    }
    return false;
}

HRESULT cLogNexus::AddSink(cLogSink* pSinkAdd) {
    if (pSinkAdd == nullptr) return E_POINTER;
    const auto guard(_LockLog.Lock());
    if (HasSink(pSinkAdd, true)) return S_FALSE;  // no dupes.
    _aSinks.AddHead(pSinkAdd);
    return S_OK;
}

bool cLogNexus::RemoveSink(cLogSink* pSinkRemove, bool bDescend) {
    if (pSinkRemove == nullptr) return false;
    bool bRemoved = false;
    const auto guard(_LockLog.Lock());
    for (ITERATE_t i = 0;; i++) {
        cLogSink* pSink = EnumSinks(i);
        if (pSink == nullptr) return bRemoved;
        if (pSinkRemove == pSink) {
            _aSinks.RemoveAt(i);
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
    const auto guard(_LockLog.Lock());
    for (const cLogSink* pSink : _aSinks) {
        if (pSink == nullptr) break;                                       // never happens!
        if (typeid(*pSink) == rType) return const_cast<cLogSink*>(pSink);  // already here.
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

cLogMgr::cLogMgr() noexcept
    : cSingleton<cLogMgr>(this),
#ifdef _DEBUG
      cLogNexus((LOG_ATTR_MASK_t)LOG_ATTR_ALL_MASK, LOGLVL_t::_TRACE)
#else
      cLogNexus((LOG_ATTR_MASK_t)LOG_ATTR_ALL_MASK, LOGLVL_t::_INFO)
#endif
{
#ifdef _DEBUG
    if (cAppState::isDebuggerPresent()) {
        cLogSinkDebug::AddSinkCheck(this);  // send logs to the debugger.
    }
#endif
}

cLogMgr::~cLogMgr() {
    ASSERT(get_ThisLogNexus());
}

void cLogMgr::ReleaseModuleChildren(::HMODULE hMod) {
    // TODO  cLogNexus _aSinks
    UNREFERENCED_PARAMETER(hMod);
    const auto guard(_LockLog.Lock());
    for (ITERATE_t i = _aSinks.GetSize(); i;) {
        const cLogSink* p = EnumSinks(--i);
        // if (p == nullptr || p->get_HModule() != hMod) continue; // TODO ?
        UNREFERENCED_PARAMETER(p);
        // _aSinks.RemoveAt(i);
    }
}

#ifdef _CPPUNWIND
void cLogMgr::LogExceptionV(cExceptionHolder* pEx, const LOGCHAR_t* pszCatchContext, va_list vargs) noexcept {
    //! if ( !cMem::IsValidApp(this)) may be OK?
    CODEPROFILEFUNC();

    const TIMESECD_t tNowSec = static_cast<TIMESECD_t>(cTimeSys::GetTimeNow() / cTimeSys::k_FREQ);
    if (sm_TimePrevException == tNowSec) return;  // prevent message floods. throttle 1 per sec.
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

        const LOGLVL_t eSeverity = (pEx == nullptr) ? LOGLVL_t::_CRIT : (pEx->get_Severity());
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

cLogSubject::cLogSubject(const char* pszSubject) : _pszSubject(pszSubject) {}

bool cLogSubject::IsLogged(LOG_ATTR_MASK_t uAttrMask, LOGLVL_t eLogLevel) const noexcept {
    return cLogMgr::I().IsLogged(uAttrMask, eLogLevel);
}
HRESULT cLogSubject::addEvent(cLogEvent& rEvent) noexcept {  // override
    rEvent._pszSubject = _pszSubject;                      // categorize with this subject.
    return cLogMgr::I().addEvent(rEvent);
}
}  // namespace Gray
