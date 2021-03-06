//
//! @file cIniMap.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_cIniMap_H
#define _INC_cIniMap_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cIniBase.h"
#include "cAtom.h"
#include "cArraySort.h"

namespace Gray
{
	class GRAYCORE_LINK cIniKeyValue
	{
		//! @class Gray::cIniKeyValue
		//! A single tuple. a key/named string and value pair. 
		//! value/property stored as string. AKA cXmlAttribute.
		//! like cPair, cPairT but more complex (dynamic types)
		//! similar to "std::tuple<>" or "System.Collections.Generic.KeyValuePair<>", cVarTuple

	public:
		cAtomRef m_aKey;	//!< property key name.
		cStringI m_sVal;	//!< property value as a string.
	public:
		cIniKeyValue() noexcept
		{
		}
		cIniKeyValue(cAtomRef aKey, cStringI sVal) noexcept
		: m_aKey(aKey), m_sVal(sVal)
		{
		}
		ATOMCODE_t get_HashCode() const noexcept
		{
			return m_aKey.get_HashCode();
		}
		bool operator == (const cIniKeyValue& e) const noexcept
		{
			return e.get_HashCode() == get_HashCode();
		}

		// Type conversion
		HRESULT GetValInt(int* piValue) const;
		HRESULT GetValDouble(double* pdValue) const;
	};

	class GRAYCORE_LINK cIniMap
	: public IIniBaseSetter
	, public IIniBaseGetter
	, public IIniBaseEnumerator
	, public cArraySortStructHash<cIniKeyValue>
 	{
		//! @class Gray::cIniMap
		//! A bag of tuples. A collection of key/value pairs stored as strings.
		//! Can be used for HTTP cookie sets, cXmlAttributeSet, JSON objects.
		//! ASSUME NO duplicated keys.
		//! Similar to cIniObject but props are NOT known/predefined.

	public:
		cIniMap() noexcept
		{
		}
		virtual ~cIniMap()
		{
		}

		ITERATE_t Find(const IniChar_t* pszPropTag) const;
		const IniChar_t* GetVal(const IniChar_t* pszPropTag) const;
		HRESULT SetVal(const IniChar_t* pszPropTag, cStringI sValue);

		virtual HRESULT PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) override;
		virtual HRESULT PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const override;
		virtual HRESULT PropGetEnum(IPROPIDX_t ePropIdx, OUT cStringI& rsValue, OUT cStringI* psKey = nullptr) const override;

		void SetCopy(const cIniMap& rAttribs);

		// Type conversion get/set
		HRESULT GetValInt(const IniChar_t* pszPropTag, int* piValue) const;
		HRESULT GetValDouble(const IniChar_t* pszPropTag, double* pdValue) const;
		HRESULT SetValInt(const IniChar_t* pszPropTag, int iVal);
	};
};

#endif
