//
//! @file CIniMap.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_CIniMap_H
#define _INC_CIniMap_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CIniBase.h"
#include "CAtom.h"
#include "CArraySort.h"

namespace Gray
{
	class GRAYCORE_LINK CIniKeyValue
	{
		//! @class Gray::CIniKeyValue
		//! A single tuple. a key/named string and value pair. 
		//! value/property stored as string. AKA CXmlAttribute.
		//! like CPair, CPairT,
		//! similar to "std::tuple<>" or "System.Collections.Generic.KeyValuePair<>", CVarTuple

	public:
		CAtomRef m_aKey;	//!< property key name.
		CStringI m_sVal;	//!< property value as a string.
	public:
		CIniKeyValue()
		{
		}
		CIniKeyValue(CAtomRef aKey, CStringI sVal)
		: m_aKey(aKey), m_sVal(sVal)
		{
		}
		ATOMCODE_t get_HashCode() const
		{
			return m_aKey.get_HashCode();
		}
		bool operator == (const CIniKeyValue& e) const
		{
			return e.get_HashCode() == get_HashCode();
		}

		// Type conversion
		HRESULT GetValInt(int* piValue) const;
		HRESULT GetValDouble(double* pdValue) const;
	};

	class GRAYCORE_LINK CIniMap
	: public IIniBaseSetter
	, public IIniBaseGetter
	, public IIniBaseEnumerator
	, public CArraySortStructHash<CIniKeyValue>
	//, public IIniObjectWriteN
	{
		//! @class Gray::CIniMap
		//! A bag of tuples. A collection of key/value pairs stored as strings.
		//! Can be used for HTTP cookie sets, CXmlAttributeSet, JSON objects.
		//! ASSUME NO duplicated keys.
		//! Similar to CIniObject but props are NOT known/predefined.

	public:
		CIniMap()
		{
		}
		virtual ~CIniMap()
		{
		}

		ITERATE_t Find(const IniChar_t* pszPropTag) const;
		const IniChar_t* GetVal(const IniChar_t* pszPropTag) const;
		HRESULT SetVal(const IniChar_t* pszPropTag, CStringI sValue);

		virtual HRESULT PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) override;
		virtual HRESULT PropGet(const IniChar_t* pszPropTag, OUT CStringI& rsValue) const override;
		virtual HRESULT PropEnum(IPROPIDX_t ePropIdx, OUT CStringI& rsValue, CStringI* psKey = nullptr) const override;

		void SetCopy(const CIniMap& rAttribs);

		// Type conversion get/set
		HRESULT GetValInt(const IniChar_t* pszPropTag, int* piValue) const;
		HRESULT GetValDouble(const IniChar_t* pszPropTag, double* pdValue) const;
		HRESULT SetValInt(const IniChar_t* pszPropTag, int iVal);
	};
};

#endif
