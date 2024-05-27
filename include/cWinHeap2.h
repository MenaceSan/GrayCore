//! @file cWinHeap2.h
//! define inline headers for Global vs Local Heap variations. This doesnt matter anymore ?
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cWinHeap2_H
#define _INC_cWinHeap2_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cLogMgr.h"
#include "cPtrFacade.h"
#include "cWinHeap.h"

#ifdef _WIN32
// cWinGlobalHandle or cWinGlobalV will create cWinGlobalT<X> use GMEM_*
#define WINHEAPN(n) cWinGlobal##n
#define WINHEAPM(n) GMEM_##n
#define WINHEAPH HGLOBAL
#ifdef UNDER_CE
#define WINHEAPF(x) Local##x
#else
#define WINHEAPF(x) ::Global##x
#endif
#include "cWinHeap.inl"
#undef WINHEAPN
#undef WINHEAPM
#undef WINHEAPH
#undef WINHEAPF

// cWinLocalHandle, cWinLocalV, cWinLocalT<> use LMEM_*
#define WINHEAPN(n) cWinLocal##n
#define WINHEAPM(n) LMEM_##n
#define WINHEAPH HLOCAL
#define WINHEAPF(x) ::Local##x
#include "cWinHeap.inl"
#undef WINHEAPN
#undef WINHEAPM
#undef WINHEAPH
#undef WINHEAPF

namespace Gray {
template <class _TYPE>
struct cWinGlobalLocker : public cPtrFacade<_TYPE> {
    ::HGLOBAL _h;
    cWinGlobalLocker(::HGLOBAL h) : _h(h), cPtrFacade((_TYPE*)::GlobalLock(_h)) {}
    ~cWinGlobalLocker() {
        ::GlobalUnlock(_h);
    }
};
}  // namespace Gray
#endif
#endif  // _INC_cWinHeap2_H
