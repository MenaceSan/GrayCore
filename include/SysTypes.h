//! @file SysTypes.h
//! Supply all common system base types that are usually available.
//! System include files should be stable and not give any warnings. (but this is not true of course)
//! Pull in System definitions from system header files. Stuff that can and should be put into a precompiled header for most all programs.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//! Can be included from an .RC file. RC_INVOKED
//! Add #defines and typedefs to arbitrate platform differences.
//! @note DO NOT add new functionality here !
 
#ifndef _INC_SysTypes_H
#define _INC_SysTypes_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

//! Always compiled for __cplusplus
//! try to compile in several different environments:
//!
//! 1. Windows Static Library = _WIN32 (NOT WIN32, NOT _WINDLL) if GRAY_STATICLIB
//! 2. Windows DLL = _WIN32, _WINDLL, GRAY_DLL (opposite of GRAY_STATICLIB)
//! 3. Windows MFC DLL with Gray* Static Library= _WIN32, _AFXDLL, _MFC_VER > 0x0600
//! 4. WindowsCE / PocketPC= UNDER_CE, _WIN32
//! 5. LINUX 32/64 bit static lib = __linux__ (NOT _BSD?) ( USE_64BIT = __ia64__, __s390x__, __x86_64__, __powerpc64__ )
//! 6. LINUX 32/64 bit SO module = __linux__ 
//! 7. __APPLE__ ??

// M$ compiler versions:
// _MSC_VER=1000 = VS5 = can do "#pragma once",
// _MSC_VER=1200 = VS6
// _MSC_VER=1202 = eVC++4
// _MSC_VER=1300 = VC++ 7.0, NET 2002, can do "__super"
// _MSC_VER=1310 = VC++ 7.1, NET 2003
// _MSC_VER=1400 = VC++ 8 = VS 2005,
// _MSC_VER=1500 = VC++ 9 = VS 2008, (PlatformToolset = 90)
// _MSC_VER=1600 = VC++ 10 = VS 2010, (PlatformToolset = 100)
// _MSC_VER=1700 = VC++ 12 = VS 2012, compiler is 17.00.51106.1
// _MSC_VER=1800 = VC++ 13 = VS 2013,
// _MSC_VER=1900 = VS v14 = VS 2015, (PlatformToolset = 140)
// _MSC_VER=1910 = VS v15 = VS 2017, (PlatformToolset = 141)
// _MSC_VER=19xx = VS v16 = VS 2019, (PlatformToolset = 142)

// what platform/OS target version of windows minimum/assumed?
// Windows 95 			_WIN32_WINDOWS>=0x0400, WINVER>=0x0400
// Windows NT 4.0 		_WIN32_WINNT>=0x0400,	WINVER>=0x0400
// Windows 98 			_WIN32_WINDOWS>=0x0410, WINVER>=0x0410
// Windows Me 			_WIN32_WINDOWS=0x0500,	WINVER>=0x0500
// Windows 2000 		_WIN32_WINNT>=0x0500,	WINVER>=0x0500
// Windows XP 			_WIN32_WINNT>=0x0501,	WINVER>=0x0501
// Windows Server 2003 	_WIN32_WINNT>=0x0502,	WINVER>=0x0502
// Windows Vista 		_WIN32_WINNT>=0x0600,	WINVER>=0x0600  (lowest we support 2017)
// Windows 7 			_WIN32_WINNT>=0x0601,	WINVER>=0x0601
// Windows Server 2012											(NT 6.2)
// Windows 8 			_WIN32_WINNT>=0x0602,	WINVER>=0x0602	(NT 6.2)
// Windows 8.1
// Windows 10 			_WIN32_WINNT>=0x0A00,	WINVER>=0x0A00

// Other external #define(s) observed:
// __linux__ = Build for Linux OS.
// _DEBUG = debug code. generated by compiler 7 on /MTd or /Mdd option. These options specify debug versions of the C run-time library
// _WIN32 = Build for Windows OS. Is defined for both 64 and 32 bit versions. generated by compiler 7 on.
// WIN32 = use _WIN32 instead although some code seems to look at this as well. Deprecated?
// _WIN64 = 64 bit windows. will also set USE_64BIT and _WIN32
// _M_IX86, _M_X64, _M_AMD64 = what processor type assumed? _M_IX86 only for use of _asm
// _MT = multi-threaded vs. single-threaded ? ASSUME __linux__ IS ALWAYS _MT. non _MT may be deprecated?
// __GNUC__ = using the GCC GNU C compiler (uses __asm__) May work for _WIN32 or __linux__
//
// other compilers:
// __MINGW32__, __BORLANDC__, __WATCOMC__

#if defined(_MSC_VER) && (_MSC_VER >= 1000)
// Remove globally annoying warnings.
#pragma warning(disable:4800)	// "forcing value to bool 'true' or 'false' (performance warning)" (convert BOOL/int to bool)
#pragma warning(disable:4251)	// 'CC' needs to have DLL - interface to be used by clients of class 'Gray::HResult' (FOR DLLs ONLY and inline type children)

// Useless MSVC2010 warnings: at /W4 level
#pragma warning(disable:4510)	// default constructor could not be generated
#pragma warning(disable:4512)	// assignment operator could not be generated
#pragma warning(disable:4610)	// class 'Gray::cTypeInfo' can never be instantiated - user defined constructor required

#endif // _MSC_VER >= 1000

#ifndef USE_CRT
#define USE_CRT 1	// 1 = use all normal CRT functions. 0 = attempt to use minimal/no CRT.
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

#ifndef RC_INVOKED

// see cMemT::NtoH() and cMemT::HtoN() for auto conversion to big endian (network)
// e.g. on little endian (Intel). 0x123456 = 56 34 12 00
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(_M_IA64) || defined(_AMD64_) || defined(__ia64__) || defined(__x86_64__)
#define USE_INTEL 1				//!< X86 type instructions and _asm.
#define USE_LITTLE_ENDIAN 1		//!< Intel = little endian = high values in high memory. increasing numeric significance with increasing memory addresses
#endif

//!< use 64 bit native code/pointers. __linux__ or _WIN32&_WIN64.
//!< NOT _M_IX86
#if defined(_WIN64)
#define USE_64BIT
#elif defined(_MSC_VER) && ( defined(_M_AMD64) || defined(_M_X64) || defined(_M_IA64))
#define USE_64BIT
#elif defined(__GNUC__) && (                          \
	defined(__amd64__) || defined(__x86_64__)    || \
	defined(__ppc64__) || defined(__powerpc64__) || \
	defined(__ia64__)  || defined(__alpha__)     || \
	(defined(__sparc__) && defined(__arch64__))  || \
	defined(__s390x__) || defined(__mips64) )
#define USE_64BIT
#endif

#define USE_FLOAT	// Assume we can do float and double types. Some embedded systems don't support float types. probably ieee .

// Assume we do native int64 types though we may not be true 64 bit code.
// What cases don't we do native 64 bit int types ?
#define USE_INT64	//!< has INT64(__int64 (_MSC_VER) or int64_t (C99/__GNUC__ standard)) as a base/native type. may not be true 64 bit code. USE_64BIT

#if defined(__GNUC__) && defined(USE_64BIT)
#define USE_LONG_AS_INT64	//!< avoid ambiguous/duplicated types. __GNUC__ defines __int64_t as 'signed long int' but usually (_MSC_VER) long is just 32 bits
#endif

//********************************

#if defined(_WIN32) && ! defined(WIN32)
#define WIN32	// Some external stuff needs this alternate define. like JS (JavaScript)
#endif
#if defined(DEBUG) && ! defined(_DEBUG)
#define _DEBUG	// Make sure everyone uses this define.
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define __DECL_EXPORT  __declspec(dllexport)		// exported from in a DLL
#define __DECL_IMPORT  __declspec(dllimport)		// in a DLL
#define __DECL_ALIGN(x) __declspec(align(x))
#elif defined (__GNUC__)
// #define __declspec(x)
#define __DECL_EXPORT  //__declspec(dllexport)
#define __DECL_IMPORT  //__declspec(dllimport)
#define __DECL_ALIGN(x)
#elif defined(__WATCOMC__)
#define __declspec(x)
#define __DECL_EXPORT  //__export
#define __DECL_IMPORT  //__import
#define __DECL_ALIGN(x)
#else
#error UNKNOWN COMPILER
#endif

#ifndef UNREFERENCED_PARAMETER	//!< _WIN32 type thing. get rid of stupid warning.
#define UNREFERENCED_PARAMETER(P)       ((void)(P))
#endif

// This should NEVER execute. Suppress warning if i put code here because the compiler knows its not reachable!? Or if i don't.
#if defined(_MSC_VER)
#define UNREACHABLE_CODE(x)			__assume(0)		// M$ emulate the GCC __builtin_unreachable. 
#elif defined(__GNUC__) && (GRAY_COMPILER_VER >= 40500)
#define UNREACHABLE_CODE(x)			__builtin_unreachable()		// __GNUC__ Defines __builtin_unreachable() natively. use it.
#else
// no_return
#define UNREACHABLE_CODE(x)			ASSERT(0); x	// has nothing like this. But 
#endif

#ifdef __GNUC__
#define ES_AWAYMODE_REQUIRED ((DWORD)0x00000040)
#define UNREFERENCED_REFERENCE(x)	((void)(x)) // references don't work with the normal UNREFERENCED_PARAMETER() macro !??
#elif ( _MSC_VER > 1600 )
#define UNREFERENCED_REFERENCE UNREFERENCED_PARAMETER // This doesn't work for VS2010 if type is not fully defined! ( _MSC_VER > 1600 )
#else
#define UNREFERENCED_REFERENCE(x)  ((void)(&x)) // x doesn't work for VS2010 if type is not fully defined! ( _MSC_VER > 1600 )
#endif

#ifndef DECLSPEC_NOVTABLE	// __GNUC__ has no such concept. Maybe use pragma ?
#define DECLSPEC_NOVTABLE		//__declspec(novtable)	// This is a abstract class or an interface the may not be instantiated directly.
#endif

// NOTE: __interface seems to imply base on IUnknown in M$. DONT use it! Use MIDL_INTERFACE(a) instead.
// _MSC_VER has a bug __declspec(dllexport) a class based on a __interface. can't create = operator ?

#ifndef DECLARE_INTERFACE // for __GNUC__
#define interface struct DECLSPEC_NOVTABLE		// a plain interface that may not support IUnknown. For use with DirectX defs.
#define DECLARE_INTERFACE(iface)	interface iface 
#define DECLARE_INTERFACE_(iface, baseiface)	interface iface : public baseiface
#endif

#if defined(_MSC_VER) && ( _MSC_VER < 1100 )	// MSC Version 5.0 defines these from the ANSI spec. MSC 4.1 does not.
#define bool	int
#define false	0
#define true	1
#endif	// _MSC_VER

#if defined(_MSC_VER) && (_MSC_VER < 1300)
typedef LONG LONG_PTR;
#endif

// fixed size on all platforms.
// http://msdn2.microsoft.com/en-us/library/aa384264.aspx

#ifdef __linux__	// emulate windows type names

// dummy out __GNUC__ non keywords.
#define _cdecl
#define __cdecl
#define _stdcall
#define __stdcall
#define IN		// document direction/usage of params. (const) AKA _In_
#define OUT		// aka _Out_
#define FAR		// Not used in modern architectures anymore but M$ headers use this. ignore it.

// Missing Windows header file stuff.
typedef int BOOL;	// match with sqltypes.h and _WIN32

typedef unsigned char	BYTE;			// always 8 bits
typedef unsigned short	WORD;			// always 16 bits
#ifdef USE_64BIT
typedef unsigned int	DWORD;	// not sure if 32 or 64 bits ? always 32 bits in _WIN32 but defined in Linux sqltypes.h as (long unsigned int)
#else
typedef unsigned long	DWORD;	// In 32 bit code this is 32 bits. Match sqltypes.h.
#endif

typedef signed short	INT16;
typedef unsigned short	UINT16;

typedef __int32_t		INT32;			// always 32 bits.
typedef __uint32_t		UINT32;			// always 32 bits.

#ifdef USE_INT64
typedef __int64_t		INT64;			// always 64 bits if USE_INT64. AKA "long long int"
typedef __uint64_t		UINT64;			// always 64 bits. AKA "unsigned long long int"
#endif

typedef char TCHAR;

// Basic common types. variable/vaguely sized. (e.g. 64 or 32 bits)
typedef unsigned int	UINT;		// probably 32 bits.
typedef long			LONG;
typedef unsigned long 	ULONG;		// ULONG may be equiv to UINT32 or UINT64. check USE_LONG_AS_INT64

// portable types that are safe if 32 or 64 bits. large enough to hold a pointer.
typedef size_t			UINT_PTR;	// USE_64BIT ?
typedef ptrdiff_t		INT_PTR;	// USE_64BIT ?
typedef long			LONG_PTR;	// USE_64BIT ?
typedef unsigned long	ULONG_PTR;	// Same as DWORD_PTR
typedef unsigned long	DWORD_PTR;	// Same as ULONG_PTR

// normally in <windef.h>
#define MAKEWORD(l,h)		((WORD)(((BYTE)(l)) | ((WORD)((BYTE)(h))) << 8))
#define MAKELONG(low, high) ((LONG)(((WORD)(low)) | (((LONG)((WORD)(high))) << 16)))

#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((UINT32)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)			((BYTE)(((WORD)(w))&0xFF))
#define HIBYTE(w)			((BYTE)(((WORD)(w))>>8))

#define CALLBACK			GRAYCALL // just make this the same as GRAYCALL
#define STDMETHODCALLTYPE	GRAYCALL // just make this the same as GRAYCALL

#define _MAX_PATH 			PATH_MAX	// #include <limits.h> or MAX_PATH in windef.h. (Put this in Filepath.h ?) FILENAME_MAX ?

#endif

#if defined(__GNUC__) || defined(UNDER_CE)
#define _Inout_			// just stub this out.
#define _countof(a)		((size_t)(sizeof(a)/sizeof((a)[0])))	//!< count of elements of an array (MSC_VER in stdlib.h) AKA ARRAYSIZE() or dimensionof() ?
#endif

#ifdef __GNUC__
#define nullptr		__null		// This is a void* not an int value.
#elif defined(_MSC_VER) && ( _MSC_VER < 1700 ) && ! defined(__cplusplus_cli)
//!< Don't define if M$ '/clr' switch is used. __cplusplus_cli
#define nullptr		NULL
#endif

#ifndef _MAX_PATH
#define _MAX_PATH 260	// __GNUC__ can leave this out if __STRICT_ANSI__
#endif

#ifdef UNDER_CE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

// Largest integral sized type. NOT always the fastest. use USE_64BIT
#ifdef USE_INT64
typedef INT64	INTMAX_t;			// 64 bits if USE_INT64 not just USE_64BIT
typedef UINT64	UINTMAX_t;			// 64 bits
#elif defined(_WIN16)
typedef INT16	INTMAX_t;			// 16 bits 
typedef UINT16	UINTMAX_t;			// 16 bits
#else	// else assume we can do 32 bits.
typedef INT32	INTMAX_t;			// 32 bits if NOT USE_INT64
typedef UINT32	UINTMAX_t;			// 32 bits
#endif

#ifndef _UINTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED
#ifdef USE_64BIT
typedef UINT64	uintptr_t;			// 64 bits. like size_t or intptr_t. maybe needs to be aligned to be truly fast? _SIZEOF_PTR should be the same as sizeof(size_t)
#else
typedef UINT32	uintptr_t;			// 32 bits. like size_t or intptr_t. _SIZEOF_PTR should be the same as sizeof(size_t).
#endif
#endif

// These must be made to work for both longs and ints. implement as macro not code.
// use "#include <xutility>" or "#include <minmax.h>" ?
#ifndef MAX
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif
#ifndef ABS
#define ABS(n)		(((n) < 0) ? (-(n)) : (n))	// no conflict with <math.h> abs()
#endif	// sign

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef INT32 HRESULT;		//!< _WIN32 style error codes. INT32
#endif	// _HRESULT_DEFINED

#endif	// RC_INVOKED
#endif	// _INC_SysTypes_H
