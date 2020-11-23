//
//! @file cIniBase.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_cIniBase_H
#define _INC_cIniBase_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "cString.h"

namespace Gray
{
	typedef int IPROPIDX_t;		//!< enumerate known properties for some interface/object/class. similar to SCRIPTPROPID_t
	typedef char IniChar_t;		//!< char format even on UNICODE system! Screw M$, INI files should ALWAYS have UTF8 contents
	typedef cStringT<IniChar_t> cStringI;	//!< A .INI file string.

	DECLARE_INTERFACE(IIniBaseSetter)	
	{
		//! @interface Gray::IIniBaseSetter
		//! Writer Sets the properties as a string to a named tag (by its string tag name). Assume pszPropTag is unique ?
		IGNORE_WARN_INTERFACE(IIniBaseSetter);
		virtual HRESULT PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) = 0;
	};
	DECLARE_INTERFACE(IIniBaseGetter)	
	{
		//! @interface Gray::IIniBaseGetter
		//! Reader Get the properties as a set of named props with string values. Assume pszPropTag is unique ?
		IGNORE_WARN_INTERFACE(IIniBaseGetter);
		virtual HRESULT PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const = 0;
	};
	DECLARE_INTERFACE(IIniBaseEnumerator) 
	{
		//! @interface Gray::IIniBaseEnumerator
		//! Read/Enumerate the object properties. 0 to max defined tags.
		IGNORE_WARN_INTERFACE(IIniBaseEnumerator);
		virtual HRESULT PropEnum(IPROPIDX_t ePropIdx, OUT cStringI& rsValue, cStringI* psPropTag = nullptr) const = 0;
	};
} 

#endif
