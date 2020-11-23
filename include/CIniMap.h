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
		//! value/property stored as string. AKA CXmlAttribute.
		//! like cPair, cPairT,
		//! similar to "std::tuple<>" or "System.Collections.Generic.KeyValuePair<>", CVarTuple

	public:
		cAtomRef m_aKey;	//!< property key name.
		cStringI m_sVal;	//!< property value as a string.
	public:
		cIniKeyValue()
		{
		}
		cIniKeyValue(cAtomRef aKey, cStringI sVal)
		: m_aKey(aKey), m_sVal(sVal)
		{
		}
		ATOMCODE_t get_HashCode() const
		{
			return m_aKey.get_HashCode();
		}
		bool operator == (const cIniKeyValue& e) const
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
	//, public IIniObjectWriteN
	{
		//! @class Gray::cIniMap
		//! A bag of tuples. A collection of key/value pairs stored as strings.
		//! Can be used for HTTP cookie sets, CXmlAttributeSet, JSON objects.
		//! ASSUME NO duplicated keys.
		//! Similar to cIniObject but props are NOT known/predefined.

	public:
		cIniMap()
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
		virtual HRESULT PropEnum(IPROPIDX_t ePropIdx, OUT cStringI& rsValue, cStringI* psKey = nullptr) const override;

		void SetCopy(const cIniMap& rAttribs);

		// Type conversion get/set
		HRESULT GetValInt(const IniChar_t* pszPropTag, int* piValue) const;
		HRESULT GetValDouble(const IniChar_t* pszPropTag, double* pdValue) const;
		HRESULT SetValInt(const IniChar_t* pszPropTag, int iVal);
	};
};

#endif
