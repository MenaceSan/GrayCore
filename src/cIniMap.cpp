//! @file cIniMap.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cIniMap.h"

namespace Gray {
HRESULT cIniKeyValue::GetValInt(int* piValue) const {
    ASSERT_NN(piValue);
    const char* pszVal = _sVal.get_CPtr();
    const char* pszEnd = nullptr;
    *piValue = StrT::toI(pszVal, &pszEnd);
    if (pszEnd == nullptr || pszEnd <= pszVal) return HRESULT_WIN32_C(ERROR_DATATYPE_MISMATCH);
    return S_OK;
}
HRESULT cIniKeyValue::GetValDouble(double* pdValue) const {
    ASSERT_NN(pdValue);
    const char* pszVal = _sVal.get_CPtr();
    const char* pszEnd = nullptr;
    *pdValue = StrT::toDouble(pszVal, &pszEnd);
    if (pszEnd == nullptr || pszEnd <= pszVal) return HRESULT_WIN32_C(ERROR_DATATYPE_MISMATCH);
    return S_OK;
}

//***************************************************************

ITERATE_t cIniMap::Find(const IniChar_t* pszPropTag) const {
    const ATOMCODE_t ac = cAtomRef::FindAtomStr(pszPropTag).get_HashCode();
    if (ac == 0) return k_ITERATE_BAD;
    return this->FindIForKey(ac);
}

const IniChar_t* cIniMap::GetVal(const IniChar_t* pszPropTag) const {
    //! get the value of a named attribute.
    const ITERATE_t i = Find(pszPropTag);
    if (i < 0) return nullptr;
    return GetAt(i)._sVal;
}

HRESULT cIniMap::SetVal(const IniChar_t* pszPropTag, cStringI sValue) {
    //! will replace if existing key.
    //! @return E_INVALIDARG,  HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY)
    const ITERATE_t i = this->AddSort(cIniKeyValue(pszPropTag, sValue), 1);
    return (HRESULT)i;
}

HRESULT cIniMap::PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const {  // override
    //! IIniBaseGetter
    const ITERATE_t i = Find(pszPropTag);
    if (i < 0) return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
    rsValue = GetAt(i)._sVal;
    return (HRESULT)i;
}

HRESULT cIniMap::PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) {  // override
    //! IIniBaseSetter
    //! will replace if existing key.
    //! @return E_INVALIDARG,  HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY)
    return SetVal(pszPropTag, pszValue);
}

HRESULT cIniMap::PropGetEnum(PROPIDX_t ePropIdx, OUT cStringI& rsValue, OUT cStringI* psKey) const {  // override
    //! IIniBaseEnumerator
    //! Enum the values and keys of the map.
    //! @arg = optionally return psKey. nullptr = don't care.
    if (!this->IsValidIndex(ePropIdx)) return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
    const cIniKeyValue& val = GetAt(ePropIdx);
    if (psKey != nullptr) {
        *psKey = val._aKey;
    }
    rsValue = val._sVal;
    return CastN(HRESULT, ePropIdx);
}

void cIniMap::SetCopy(const cIniMap& rAttribs) {
    //! Copy the attributes from rAttribs to this.
    ASSERT(&rAttribs != this);
    for (const cIniKeyValue& e : rAttribs) {
        this->AddSort(cIniKeyValue(e._aKey, e._sVal), 1);
    }
}

HRESULT cIniMap::GetValInt(const IniChar_t* pszPropTag, int* piValue) const {
    //! error as if using scanf("%d")
    const ITERATE_t i = this->Find(pszPropTag);
    if (i < 0) return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
    return GetAt(i).GetValInt(piValue);
}

HRESULT cIniMap::GetValDouble(const IniChar_t* pszPropTag, double* pdValue) const {
    //! error as if using scanf("%lf")
    const ITERATE_t i = this->Find(pszPropTag);
    if (i < 0) return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
    return GetAt(i).GetValDouble(pdValue);
}

HRESULT cIniMap::SetValInt(const IniChar_t* pszPropTag, int iVal) {
    char szBuffer[k_LEN_MAX_CSYM];
    StrT::ItoA(iVal, TOSPAN(szBuffer));
    return SetVal(pszPropTag, szBuffer);
}

cStringI GRAYCALL IIniBaseGetter::Get2(IIniBaseGetter* p, const IniChar_t* pszPropTag) {  // static
    if (p == nullptr) return "";
    cStringI sVal;
    HRESULT hRes = p->PropGet(pszPropTag, sVal);
    if (FAILED(hRes)) return "";
    return sVal;
}
}  // namespace Gray
