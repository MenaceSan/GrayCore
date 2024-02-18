//! @file cIniMap.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cIniMap_H
#define _INC_cIniMap_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArraySort.h"
#include "cAtom.h"
#include "cIniBase.h"

namespace Gray {
/// <summary>
/// A single tuple. a key/named string and value pair.
/// value/property stored as string. AKA cXmlAttribute.
/// like cPair, cPairT but more complex (dynamic types)
/// similar to "std::tuple" or "System.Collections.Generic.KeyValuePair", cVarTuple
/// </summary>
struct GRAYCORE_LINK cIniKeyValue {
    cAtomRef m_aKey;  /// property key name.
    cStringI m_sVal;  /// property value as a string.

    cIniKeyValue() noexcept {}
    cIniKeyValue(cAtomRef aKey, cStringI sVal) noexcept : m_aKey(aKey), m_sVal(sVal) {}
    ATOMCODE_t get_HashCode() const noexcept {
        return m_aKey.get_HashCode();
    }
    bool operator==(const cIniKeyValue& e) const noexcept {
        return e.get_HashCode() == get_HashCode();
    }

    // Type conversion
    HRESULT GetValInt(int* piValue) const;
    HRESULT GetValDouble(double* pdValue) const;
};

/// <summary>
/// A bag of tuples. A collection of key/value pairs stored as strings.
/// Can be used for HTTP cookie sets, cXmlAttributeSet, JSON objects.
/// ASSUME NO duplicated keys.
/// Similar to cIniObject but props are NOT known/predefined.
/// </summary>
struct GRAYCORE_LINK cIniMap : public IIniBaseSetter, public IIniBaseGetter, public IIniBaseEnumerator, public cArraySortStructHash<cIniKeyValue> {
    cIniMap() noexcept {}
 
    ITERATE_t Find(const IniChar_t* pszPropTag) const;
    const IniChar_t* GetVal(const IniChar_t* pszPropTag) const;
    HRESULT SetVal(const IniChar_t* pszPropTag, cStringI sValue);

    HRESULT PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) override;
    HRESULT PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const override;
    HRESULT PropGetEnum(PROPIDX_t ePropIdx, OUT cStringI& rsValue, OUT cStringI* psKey = nullptr) const override;

    void SetCopy(const cIniMap& rAttribs);

    // Type conversion get/set helpers
    HRESULT GetValInt(const IniChar_t* pszPropTag, int* piValue) const;
    HRESULT GetValDouble(const IniChar_t* pszPropTag, double* pdValue) const;
    HRESULT SetValInt(const IniChar_t* pszPropTag, int iVal);
};
}  // namespace Gray
#endif
