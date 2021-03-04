//
//! @file GrayCore.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//! Can be included from an .RC file. RC_INVOKED

#ifndef _INC_GrayCore_H
#define _INC_GrayCore_H	0x003	//!< 0.0.3 Version stamp the API. Especially important to the Variant and Archive types.
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

// Pull in my system/compiler CLR includes and arbitrate them with common names.
#include "SysTypes.h"

// override the system #define UNICODE and _UNICODE. I use my own USE_UNICODE system as default char type. We don't have to use UNICODE just because the system does.
#ifndef USE_UNICODE
#if defined(UNICODE) // || defined(_UNICODE)
#define USE_UNICODE 1				//!< This allows the including of core headers that use conflicting #define UNICODE to still work.
#else
#define USE_UNICODE 0				// same as _MBCS
#endif
#endif
#ifndef USE_UNICODE_FN
#define USE_UNICODE_FN USE_UNICODE			//!< make file names UNICODE as well?
#endif

namespace Gray		//!< The main namespace for all Core functions.
{
	//! @namespace Gray
	//! The main namespace for all Core functions.
	//! The main/default namespace for Gray library
#ifndef GRAY_NAME
#define GRAY_NAME	Gray		//!< Root name.
#define GRAY_NAMES	"Gray"		//!< Use GRAYNAME for string.
#endif
#define GRAYCALL	__stdcall	//!< declare calling convention for static functions so everyone knows the arg passing scheme. don't assume compiler default. _cdecl.

	// use _LIB && _WINDLL && _MFC_VER to identify the type of LIB build. or it may just be who is including us.
#ifndef GRAYCORE_LINK
#if defined(_MFC_VER) || defined(GRAY_STATICLIB)	// GRAY_STATICLIB or _MFC_VER can be defined to make Gray* all static lib
#define GRAYCORE_LINK
#else
	// We are building Gray DLL/SO instead of static lib. use __DECL_IMPORT or __DECL_EXPORT. opposite of GRAY_STATICLIB
#define GRAYCORE_LINK __DECL_IMPORT	// default is to include from a DLL/SO 
#endif
#endif

#if defined(_DEBUG) || ! defined(_MSC_VER)
#define _LOCCALL        // static (local) calls might have better calling conventions? But turn off during _DEBUG
#else
#define _LOCCALL        __fastcall // Local procedure name modifier. will not stack dump properly!
#endif

#if defined(__GNUC__) || (! defined(_MSC_VER)) || (_MSC_VER < 1600)
#define __noop		((void)0)		// A macro that does nothing. Compiles out some code. do { } while( 0 )
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1600 // No C++11 features.
	// Get rid of C++11 features. e.g. "= delete" and override
#define noexcept
#define override	// tell the compiler this is an intentional override
#define IS_DELETE
#else
#define IS_DELETE = delete		// to get rid of this normally default supplied method.
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1916 // VS2017 has internal errors when noexcept is used.
#define NOEXCEPT  
#else
#define NOEXCEPT noexcept 
#endif

#ifdef __GNUC__
#define IGNORE_WARN_INTERFACE(c)		virtual ~c() {}		// quiet this warning for interfaces. should we do this ?
#else
#define IGNORE_WARN_INTERFACE(c)	 
#endif	// __GNUC__
#define IGNORE_WARN_ABSTRACT(c)			virtual ~c() {}		// quiet this warning for abstract base classes

	// a structure should be byte packed and not aligned ? use #pragma pack(push,1) as well
#if defined(__MINGW32__) 
#define CATTR_PACKED __attribute__((packed))	// MING compiler uses this to indicate structure packing required.
#else
#define CATTR_PACKED	// _MSC_VER and __GNUC__ use #pragma pack(1) to indicate packing required.
#endif

#ifdef _MSC_VER
#define CATTR_NORETURN __declspec(noreturn)
#else // __GNUC__
#define CATTR_NORETURN __attribute__((noreturn))
#endif

#ifdef __GNUC__
#define CATTR_CONSTRUCTOR	__attribute__((constructor)) 
#define CATTR_DESTRUCTOR	__attribute__((destructor)) 
#else
#define CATTR_CONSTRUCTOR
#define CATTR_DESTRUCTOR
#endif

// Allow some method to be deprecated. warn the user to change to some new version.
#ifdef __GNUC__
#define CATTR_DEPRECATEDAT(versionNumber, alternative)	__attribute__((deprecated))
#define CATTR_DEPRECATED								__attribute__((deprecated))
#elif _MSC_VER >= 1400
#define CATTR_DEPRECATEDAT(versionNumber, alternative) __declspec(deprecated("[" #versionNumber "] This function is now deprecated. Please use '" #alternative "' instead."))
#define CATTR_DEPRECATED
#else
#define CATTR_DEPRECATEDAT(versionNumber, alternative)
#define CATTR_DEPRECATED
#endif // _WIN32 && MSVS2005

	typedef UINT_PTR	HASHCODE_t;				//!< could hold a pointer converted to a number? maybe 64 or 32 bit ? same as size_t.
	typedef UINT32		HASHCODE32_t;			//!< always 32 bits.
	const HASHCODE_t	k_HASHCODE_CLEAR = 0;		//!< not a valid index.

};

#endif	// _INC_GRAYCORE
