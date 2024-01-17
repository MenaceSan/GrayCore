//
//! @file cIniObject.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cCodeProfiler.h"
#include "cIniObject.h"
#include "cIniSection.h"
#include "cStream.h"
#include "cString.h"

namespace Gray {
HRESULT cIniObject::PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const {  // override
    //! IIniBaseGetter
    //! Read a prop by its string name.
    //! default implementation.
    return this->PropGetEnum(this->FindProp(pszPropTag), rsValue, nullptr);
}

HRESULT cIniObject::FileWriteN(cStreamOutput& rOut, PROPIDX_t ePropIdx) const {
    //! Write the prop out to the stream if _DirtyMask.
    //! cStreamOutput
    CODEPROFILEFUNC();
    if (!_DirtyMask.IsSet(ePropIdx))  // already written. or not changed?
        return S_FALSE;
    cStringI sValue;
    HRESULT hRes = this->PropGetEnum(ePropIdx, sValue);
    if (FAILED(hRes)) return hRes;
    _DirtyMask.ClearBit(ePropIdx);  // not dirty anymore.
    return cIniWriter(&rOut).WriteKeyUnk(this->get_PropName(ePropIdx), sValue);
}

HRESULT cIniObject::FileWrite(cStreamOutput& rOut, const IniChar_t* pszProp) {
    //! write this prop by name.
    CODEPROFILEFUNC();
    const PROPIDX_t ePropIdx = this->FindProp(pszProp);  // Str_TableFindHead(pszProp,get_Props());
    if (ePropIdx < 0) return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
    return FileWriteN(rOut, ePropIdx);
}

HRESULT cIniObject::FileWriteAll(cStreamOutput& rOut) {
    //! Write out all that are not already written.
    //! Assume [HEADER] already written.
    CODEPROFILEFUNC();
    const PROPIDX_t iQty = this->get_PropQty();
    for (PROPIDX_t i = 0; i < iQty; i++) {
        if (!_DirtyMask.IsSet(i))  // was already written? or not changed?
            continue;
        HRESULT hRes = FileWriteN(rOut, i);
        _DirtyMask.ClearBit(i);
        if (FAILED(hRes)) return hRes;
    }
    _DirtyMask.ClearMask();
    return S_OK;
}
}  // namespace Gray
