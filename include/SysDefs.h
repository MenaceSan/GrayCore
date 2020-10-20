//
//! @file SysDefs.h
//! Pull in System definitions from system header files. Stuff that can and should be put into a precompiled header for most all programs.
//! System include files should be stable and not give any warnings. (but this is not true of course)
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_SysDefs_H
#define _INC_SysDefs_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#ifndef USE_CRT
#define USE_CRT 1	// 1 = use all normal CRT functions. 0 = attempt to use minimal CRT.
#endif

// Define your compiler here.
#ifdef _MSC_VER
// MSVC 1900
#define GRAY_COMPILER_NAME "Msc"
#define GRAY_COMPILER_VER _MSC_VER

#elif defined(__GNUC__)
// GNU 4.5 = 40500
#define GRAY_COMPILER_NAME "gnuc"
#define GRAY_COMPILER_VER (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#undef __STRICT_ANSI__

#elif defined(__WATCOMC__) || defined(__BORLANDC__) || defined(__MINGW32__)
#define GRAY_COMPILER_NAME "cc"	// other ?
#define GRAY_COMPILER_VER 0
#error UNSUPPORTED COMPILER

#else
#error UNKNOWN COMPILER
#endif

// http://en.wikipedia.org/wiki/Pragma_once
#if defined(_MSC_VER) && (_MSC_VER < 1000)
#define NO_PRAGMA_ONCE		// This MUST be put on the command line.
#endif
#if defined(__GNUC__) && (GRAY_COMPILER_VER >= 30400)	// >= 3.4
// #define NO_PRAGMA_ONCE
#define _NATIVE_WCHAR_T_DEFINED		// This seems to be true in newer versions. wchar_t
#endif

#ifdef _WIN32

#ifdef UNDER_CE

#define WINCE	// Some code depends on this ?
#pragma comment(linker, "/nodefaultlib:libc.lib")
#pragma comment(linker, "/nodefaultlib:libcd.lib")
// NOTE - this value is not strongly correlated to the Windows CE OS version being targeted
#define _WIN32_WCE 0x0500
#define WINVER _WIN32_WCE
#include <ceconfig.h>	// __CECONFIG_H__

#else // UNDER_CE

#if 0
// Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#include <SDKDDKVer.h>
#else

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0600		// Change this to the appropriate value to target other versions of Windows. (0x0600 = Vista or higher)
#endif
#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0600	// Change this to the appropriate value to target other versions of Windows. 
#endif
#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows XP or later.
#define _WIN32_WINDOWS 0x0600 // Change this to the appropriate value to target Windows Me or later.
#endif
#ifndef _WIN32_IE			// Allow use of features specific to Windows IE version.
#define _WIN32_IE 0x0700	// Change this to the appropriate value to target IE 7 or later. (default for Vista)
#endif

#endif	// SDKDDKVer.h

#endif // ! UNDER_CE

#define _WINSOCKAPI_		// prevent _WINSOCKAPI_ from loading because i want _WINSOCK2API_
#define WINBASE_DECLARE_GET_MODULE_HANDLE_EX	// allow GetModuleHandleEx
#ifndef STRICT
#define STRICT	1	// Make sure DECLARE_HANDLE has type info.
#endif

#ifdef _DEBUG
#define DEBUG_VALIDATE_ALLOC	// slows us down but checks memory often
#endif

#ifdef _AFXDLL	// will define _MFC_VER
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// Windows with MFC (NON GUI)
#ifndef __AFX_H__	// not already included
#include <afx.h>         // MFC core and standard components.  MFC Strings. CString. defines _MFC_VER
#endif
#ifndef _MFC_VER // should be defined in <afxver_.h> in <afx.h>
#error NO _MFC_VER?
#endif
#include <afxtempl.h>		// MFC Templates. CArray.
#include <Winsock2.h>	// must get loaded before <afxwin.h> to prevent problems with _WINSOCKAPI_ vs _WINSOCK2API_ in VC2013 with _AFXDLL, _MFC_VER

#else
#include <windows.h>
#endif	// ! _AFXDLL

#include <unknwn.h>	// IUnknown __IUnknown_INTERFACE_DEFINED__ IClassFactory

#if defined(_WIN32) && ! defined(_WINDOWS_)
#ifdef _WINDOWS_H			// __GNUC__ version of "windows.h"
#define _WINDOWS_
#elif defined(__WINDOWS__)	// UNDER_CE version
#define _WINDOWS_
#elif ! defined(_WINDOWS_) // should be defined in <afx.h> !
#error NO _WINDOWS_ defined?
#endif
#endif

#endif // _WIN32

//*******************************************
// ASSUME these ANSI C standard types are universal. rely on CRT.

#include <stddef.h> // offsetof(), size_t
#include <stdlib.h> // _MAX_PATH, __max(), __min() if !defined(__OpenBSD__) ?
#include <string.h>	// memcpy()
#include <stdarg.h>	// va_list ...
#include <limits.h> // INT_MAX, (PATH_MAX in Linux)  is std::numeric_limits<T>::max() in <limits> better ?

#if USE_CRT
#include <stdio.h>	// define FILE _vsnprintf_s printf() function
#endif

#ifdef UNDER_CE
#include <new>
#else
#include <assert.h>	// assert() (no idea why UNDER_CE doesn't like this)
#endif

#ifdef __GNUC__
#include <stdint.h> // int64_t in C99 standard.
#define _CPPUNWIND	// exceptions are supported
#define _CPPRTTI	// RTTI is supported
#define _MT			// __linux__ and __GNUC__ is always considered to be multi-threaded.
#endif	// __GNUC__

#ifdef __linux__	// emulate windows type stuff for Linux.
// __linux__, Linux or _BSD
#include <signal.h>
#include <sched.h>
#include <fcntl.h>	// open()
#include <unistd.h> // gethostname()
#include <linux/unistd.h>
#endif

#if defined(_WIN32) && !defined(UNDER_CE) && ! defined(__GNUC__) && USE_CRT
#include <crtdbg.h>
#include <direct.h>	// _chdir
#endif

#endif	// _INC_SysDefs_H
