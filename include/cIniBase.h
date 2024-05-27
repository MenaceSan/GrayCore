//! @file cIniBase.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cIniBase_H
#define _INC_cIniBase_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "cString.h"

namespace Gray {
typedef char IniChar_t;                /// char format even on UNICODE system! Screw M$, ASSUME INI files should ALWAYS have UTF8 contents
typedef cStringT<IniChar_t> cStringI;  /// A .INI file string.

/// <summary>
/// define interface where Writer Sets the properties as a string to a named tag (by its string tag name).
/// Assume pszPropTag is unique ?
/// </summary>
DECLARE_INTERFACE(IIniBaseSetter) {
    IGNORE_WARN_INTERFACE(IIniBaseSetter);
    virtual HRESULT PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) = 0;
};

/// <summary>
/// define interface where Reader Get the properties as a set of named props with string values.
/// Assume pszPropTag is unique ?
/// </summary>
DECLARE_INTERFACE(IIniBaseGetter) {
    IGNORE_WARN_INTERFACE(IIniBaseGetter);
    virtual HRESULT PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const = 0;
    // Helper
    GRAYCORE_LINK static cStringI GRAYCALL Get2(IIniBaseGetter * p, const IniChar_t* pszPropTag);
};

/// <summary>
/// define interface to Read/Enumerate the object properties. 0 to max defined tags. assume also supports IIniBaseGetter
/// </summary>
DECLARE_INTERFACE(IIniBaseEnumerator) {
    IGNORE_WARN_INTERFACE(IIniBaseEnumerator);
    virtual HRESULT PropGetEnum(PROPIDX_t ePropIdx, OUT cStringI & rsValue, OUT cStringI* psPropTag = nullptr) const = 0;
    // virtual HRESULT PropSetEnum(PROPIDX_t ePropIdx, const IniChar_t* pszValue) = 0;
};
}  // namespace Gray
#endif
