//! @file cIniObject.h
//! very simplistic string scriptable object.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cIniObject_H
#define _INC_cIniObject_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cBits.h"
#include "cIniBase.h"
#include "StrArg.h"
#include "ITextWriter.h"

namespace Gray {

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

    /// <summary>
    /// Read a prop by its string name. default IIniBaseGetter implementation.
    /// </summary>
    /// <param name="pszPropTag"></param>
    /// <param name="rsValue"></param>
    /// <returns></returns>
    HRESULT PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const override;

    /// <summary>
    /// Write the prop out to the stream if _DirtyMask.
    /// </summary>
    /// <param name="sOut">cStreamOutput</param>
    /// <param name="ePropIdx"></param>
    /// <returns></returns>
    HRESULT FileWriteN(ITextWriter& sOut, PROPIDX_t ePropIdx) const;

    HRESULT FileWrite(ITextWriter& sOut, const IniChar_t* pszProp);
    HRESULT FileWriteAll(ITextWriter& sOut);
};
}  // namespace Gray
#endif  // _INC_cIniObject_H
