//
//! @file cIniMap.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cIniMap.h"

namespace Gray {
HRESULT cIniKeyValue::GetValInt(int* piValue) const {
    //! get value type converted to int.
    //! error as if using scanf("%d")
    ASSERT_NN(piValue);
    const char* pszVal = m_sVal.c_str();
    const char* pszEnd = nullptr;
    *piValue = StrT::toI(pszVal, &pszEnd);
    if (pszEnd == nullptr || pszEnd <= pszVal) return HRESULT_WIN32_C(ERROR_DATATYPE_MISMATCH);
    return S_OK;
}
HRESULT cIniKeyValue::GetValDouble(double* pdValue) const {
    //! get value type converted to double.
    //! error as if using scanf("%lf")
    ASSERT_NN(pdValue);
    const char* pszVal = m_sVal.c_str();
    const char* pszEnd = nullptr;
    *pdValue = StrT::toDouble(pszVal, &pszEnd);
    if (pszEnd == nullptr || pszEnd <= pszVal) return HRESULT_WIN32_C(ERROR_DATATYPE_MISMATCH);
    return S_OK;
}

//***************************************************************

ITERATE_t cIniMap::Find(const IniChar_t* pszPropTag) const {
    ATOMCODE_t ac = cAtomRef::FindAtomStr(pszPropTag).get_HashCode();
    if (ac == 0) return k_ITERATE_BAD;
    return this->FindIForKey(ac);
}

const IniChar_t* cIniMap::GetVal(const IniChar_t* pszPropTag) const {
    //! get the value of a named attribute.
    ITERATE_t i = Find(pszPropTag);
    if (i < 0) return nullptr;
    return GetAt(i).m_sVal;
}

HRESULT cIniMap::SetVal(const IniChar_t* pszPropTag, cStringI sValue) {
    //! will replace if existing key.
    //! @return E_INVALIDARG,  HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY)
    ITERATE_t i = this->Add(cIniKeyValue(pszPropTag, sValue));
    return (HRESULT)i;
}

HRESULT cIniMap::PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const {  // override
    //! IIniBaseGetter
    ITERATE_t i = Find(pszPropTag);
    if (i < 0) return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
    rsValue = GetAt(i).m_sVal;
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
        *psKey = val.m_aKey;
    }
    rsValue = val.m_sVal;
    return (HRESULT)ePropIdx;
}

void cIniMap::SetCopy(const cIniMap& rAttribs) {
    //! Copy the attributes from rAttribs to this.
    ASSERT(&rAttribs != this);
    for (auto e : rAttribs) {
        this->Add(cIniKeyValue(e.m_aKey, e.m_sVal));
    }
}

HRESULT cIniMap::GetValInt(const IniChar_t* pszPropTag, int* piValue) const {
    //! error as if using scanf("%d")
    ITERATE_t i = this->Find(pszPropTag);
    if (i < 0) return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
    return GetAt(i).GetValInt(piValue);
}

HRESULT cIniMap::GetValDouble(const IniChar_t* pszPropTag, double* pdValue) const {
    //! error as if using scanf("%lf")
    ITERATE_t i = this->Find(pszPropTag);
    if (i < 0) return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
    return GetAt(i).GetValDouble(pdValue);
}

HRESULT cIniMap::SetValInt(const IniChar_t* pszPropTag, int iVal) {
    char szBuffer[k_LEN_MAX_CSYM];
    StrT::ItoA(iVal, szBuffer, STRMAX(szBuffer));
    return SetVal(pszPropTag, szBuffer);
}

}  // namespace Gray
