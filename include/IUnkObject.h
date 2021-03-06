//
//! @file IUnkObject.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_IUnkObject_H
#define _INC_IUnkObject_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "IUnknown.h"
#include "cString.h"

namespace Gray
{
	MIDL_INTERFACE("b01dface-0000-0000-C000-100000000046") IUnkObject : public IUnknown
	{
		//! @interface Gray::IUnkObject
		//! IUnknown base for object like cXObject, cScriptableObj
		//! e.g. Base interface to client for cProtocolStream. used by cNetServerConnection
		IGNORE_WARN_INTERFACE(IUnkObject);

		//! UID/HashCode for the instance.
		STDMETHOD_(HASHCODE_t, get_HashCodeX)() const noexcept = 0;

		//! friendly/viewable name for object.
		STDMETHOD_(cString, get_Name)() const = 0;

		//! Resource object instance can have its own symbolic name (AKA moniker). Default to UID if it has no SymName assigned.
		STDMETHOD_(cStringA, get_SymName)() const noexcept = 0;
	};
}

#endif
