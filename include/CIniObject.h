//
//! @file cIniObject.h
//! very simplistic string scriptable object.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_cIniObject_H
#define _INC_cIniObject_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cBits.h"
#include "cIniBase.h"

namespace Gray {
struct cStreamOutput;

/// <summary>
/// get basic name metadata about the props supported.
/// </summary>
DECLARE_INTERFACE(IIniObjectDef) {
    IGNORE_WARN_INTERFACE(IIniObjectDef);
    virtual PROPIDX_t get_PropQty() const = 0;
    virtual const IniChar_t* get_PropName(PROPIDX_t ePropIdx) const = 0;
    virtual PROPIDX_t FindProp(const IniChar_t* pszPropTag) const = 0;
};

/// <summary>
/// Base class for generic object with predefined/known props (Unlike cIniSection) read/written via interfaces.
/// can be stored as cIniSectionData. Also like cIniMap
/// Much more simplistic form of IScriptableObj.
/// </summary>
struct GRAYCORE_LINK cIniObject : public IIniObjectDef, public IIniBaseSetter, public IIniBaseGetter, public IIniBaseEnumerator {
    mutable cBitmask<UINT64> _DirtyMask;  /// bitmask of PROPIDX_t to be written/persisted. Max 64 props.
  
    // HRESULT PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) override;
    HRESULT PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const override;

    HRESULT FileWriteN(cStreamOutput& sOut, PROPIDX_t ePropIdx) const;
    HRESULT FileWrite(cStreamOutput& sOut, const IniChar_t* pszProp);
    HRESULT FileWriteAll(cStreamOutput& sOut);
};
}  // namespace Gray
#endif  // _INC_cIniObject_H
