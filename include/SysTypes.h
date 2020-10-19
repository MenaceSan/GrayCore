//! @file SysTypes.h
//! Supply all common system base types that are usually available.
//! System include files should be stable and not give any warnings.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//! Can be included from an .RC file. RC_INVOKED
//

#ifndef _INC_SysTypes_H
#define _INC_SysTypes_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "SysDefs.h"

#ifndef RC_INVOKED

// see CMemT::NtoH() and CMemT::HtoN() for auto conversion to big endian (network)
// e.g. on little endian (Intel). 0x123456 = 56 34 12 00
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(_M_IA64) || defined(_AMD64_) || defined(__ia64__) || defined(__x86_64__)
#define USE_INTEL 1		//!< X86 type instructions and _asm.
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

#define USE_FLOAT	// Assume we can do float and double types. Some embedded systems can't. probably ieee .

// Assume we do native int64 types though we may not be true 64 bit code.
// What cases don't we do native 64 bit int types ?
#define USE_INT64	//!< has INT64(__int64 (_MSC_VER) or int64_t (C99/__GNUC__ standard)) as a base/native type. may not be true 64 bit code. USE_64BIT

#if defined(__GNUC__) && defined(USE_64BIT)
#define USE_LONG_IS_INT64	//!< avoid ambiguous/duplicated types. __GNUC__ defines __int64_t as 'signed long int' but usually (_MSC_VER) long is just 32 bits
#endif

#if defined(_WIN32) && ! defined(WIN32)
#define WIN32	// Some external stuff needs this alternate define. like js (JavaScript)
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

// __GNUC__ non keywords.
#define _cdecl
#define __cdecl
#define _stdcall
#define __stdcall
#define IN		// document direction/usage of params. (const) AKA _In_
#define OUT		// aka _Out_
#define FAR		// Not used in modern architectures anymore but M$ headers use this.

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
typedef unsigned long 	ULONG;		// ULONG may be equiv to UINT32 or UINT64. check USE_LONG_IS_INT64

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

#define CALLBACK			GRAYCALL // ?
#define STDMETHODCALLTYPE	GRAYCALL // ?

#define _MAX_PATH 			PATH_MAX	// #include <limits.h> or MAX_PATH in windef.h. (Put this in Filepath.h ?)

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

// These must be made to work for both longs and ints. impliment as macro not code.
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

namespace Gray {};		// We should always know about this namespace.

#endif	// RC_INVOKED
#endif	// _INC_SysTypes_H
