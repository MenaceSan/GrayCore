//
//! @file IUnknown.h
//! support for COM style interfaces. might even be used in __linux__.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_IUnknown_H
#define _INC_IUnknown_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"

#ifdef _WIN32
#include <Unknwn.h>	// IUnknown __IUnknown_INTERFACE_DEFINED__ IClassFactory
#endif

#ifndef DECLSPEC_UUID
#define DECLSPEC_UUID(x)
#endif

#ifdef __GNUC__
#define __uuidof(x) IID_##x	// IID_IUnknown == __uuidof(IUnknown) . This doesn't work in templates of course.
#ifndef MIDL_INTERFACE
#define MIDL_INTERFACE(a)	interface // assume we get struct DECLSPEC_UUID(x) DECLSPEC_NOVTABLE and will support IUnknown. Similar to DECLARE_INTERFACE
#endif
#endif

// #define GRAY_GUID_POSTFIX_S "-0000-0000-C000-100000000046"		// Use this GUID postfix for all but first 4 bytes. (8 hex digits)

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
	//! @struct _GUID
	//! GUID = 16 bytes long = 128 bits, 32 hex digits.
	//! same as size as SQLGUID and IPv6 address
	//! string encoded like "b01dface-0000-0000-C000-100000000046"
public:
	inline bool operator == ( const _GUID& other ) const
	{
		return ! ::memcmp( this, &other, sizeof(other));
	}
public:
	UINT32 Data1;
	WORD Data2;
	WORD Data3;
	BYTE Data4[8];
} GUID;
#endif

#ifdef __linux__
#define STDMETHOD(method) 		virtual HRESULT _stdcall method
#define STDMETHOD_(type,method) virtual type _stdcall method
#define STDMETHODIMP            HRESULT _stdcall
#define STDMETHODIMP_(type)     type _stdcall
#define __RPC_FAR		// only used for _MSC_VER ?

#define DECLSPEC_UUID(x)		//__declspec(uuid(x))
#endif // __linux__

#if ! defined(__IUnknown_INTERFACE_DEFINED__)
#define __IUnknown_INTERFACE_DEFINED__

typedef GUID IID;
extern GRAYCORE_LINK GUID IID_IUnknown;

MIDL_INTERFACE("00000000-0000-0000-C000-000000000046") IUnknown
{
	//! @interface IUnknown
	STDMETHOD(QueryInterface)( /* [in] */ const IID& riid, /* [iid_is][out] */ void**ppvObject ) = 0;
	STDMETHOD_(ULONG,AddRef)(void) = 0;
	STDMETHOD_(ULONG, Release)(void) = 0;
};

#endif // __IUnknown_INTERFACE_DEFINED__

#if 1

//! COM IUnknown interface support.
//! identify the true base class for IUnknown. in the case of multiple inheritance of IUnknown based classes.
//! @note Do not have multiple inheritance in interface definitions. MultiInherit is for classes not for interfaces.
//!  or if you do, don't make IUnknown base ambiguous to resolve in the interface! Leave that for classes.

#define IUNKNOWN_DISAMBIG_R(TYPE) \
	STDMETHOD_(ULONG,AddRef)(void) override /* disambiguate.*/ \
	{ return TYPE::AddRef(); } \
	STDMETHOD_(ULONG, Release)(void) override /* disambiguate.*/\
	{ return TYPE::Release(); }

// IUNKNOWN_DISAMBIG_R and the QueryInterface() call. use this TYPE.
#define IUNKNOWN_DISAMBIG(TYPE) STDMETHOD(QueryInterface)( /* [in] */ const IID& riid, /* [iid_is][out] */ void __RPC_FAR* __RPC_FAR* ppvObject ) override \
	{ return TYPE::QueryInterface(riid,ppvObject); } \
	IUNKNOWN_DISAMBIG_R(TYPE);

#else

#define IUNKNOWN_DISAMBIG_R(TYPE)
#define IUNKNOWN_DISAMBIG(TYPE) 
#endif

#endif // _INC_IUnknown_H
