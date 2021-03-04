//! @file SysRes.h
//! define system resource id's. Will be included in *.rc files.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//! Can be included from an .RC file. RC_INVOKED, APSTUDIO_READONLY_SYMBOLS

#ifndef _INC_SysRes_H
#define _INC_SysRes_H
#pragma once

#if defined(_MFC_VER) || defined(_AFXDLL)	// rc file will define _AFXDLL
#include <afxres.h>         // must be included before <afxwin.h> or <afxext.h>

#if defined(RC_INVOKED) &&  !defined(APSTUDIO_INVOKED)
#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_ENU)
#include "afxprint.rc"       // printing/print preview resources
#include "afxolesv.rc"       // OLE server resources
#endif
#endif	// RC_INVOKED // not APSTUDIO_INVOKED

#else // !_MFC_VER

// Emulate what is in AFX if AFX is not available.  APSTUDIO_READONLY_SYMBOLS
#ifdef _WIN32

#define APSTUDIO_HIDDEN_SYMBOLS
#define IDC_STATIC -1
#include "windows.h"
#undef APSTUDIO_HIDDEN_SYMBOLS

#else

// #define DS_MODALFRAME       0x80L   /* Winuser.h Can be combined with WS_CAPTION  */
#define LANG_ENGLISH  0x09		// winnt.h
#define SUBLANG_ENGLISH_US 0x01

#endif
#endif

#include "GrayVersion.h"

#endif
