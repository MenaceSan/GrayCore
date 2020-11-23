//
//! @file cWinHeap2.h
//! define inline headers.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cWinHeap2_H
#define _INC_cWinHeap2_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cWinHeap.h"
#include "cLogMgr.h"

#ifdef _WIN32
	// CWinGlobalHandle or CWinGlobalV will create CWinGlobalT<X> use GMEM_*
#define WINHEAPN(n)	CWinGlobal##n
#define WINHEAPM(n)	GMEM_##n
#define WINHEAPH	HGLOBAL
#ifdef UNDER_CE
#define WINHEAPF(x)	Local##x
#else
#define WINHEAPF(x)	::Global##x
#endif
#include "cWinHeap.inl"
#undef WINHEAPN
#undef WINHEAPM
#undef WINHEAPH
#undef WINHEAPF

	// CWinLocalHandle, CWinLocalV, CWinLocalT<> use LMEM_*
#define WINHEAPN(n)	CWinLocal##n
#define WINHEAPM(n)	LMEM_##n
#define WINHEAPH	HLOCAL
#define WINHEAPF(x)	::Local##x
#include "cWinHeap.inl"
#undef WINHEAPN
#undef WINHEAPM
#undef WINHEAPH
#undef WINHEAPF
 
#endif
#endif // _INC_cWinHeap2_H
