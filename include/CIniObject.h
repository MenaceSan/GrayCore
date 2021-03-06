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

#include "cIniBase.h"

namespace Gray
{
	class cStreamOutput;

	DECLARE_INTERFACE(IIniObjectDef)
	{
		//! @interface Gray::IIniObjectDef
		//! get basic name metadata about the props supported.
		IGNORE_WARN_INTERFACE(IIniObjectDef);

		virtual IPROPIDX_t get_PropQty() const = 0;
		virtual const IniChar_t* get_PropName(IPROPIDX_t ePropIdx) const = 0;
		virtual IPROPIDX_t FindProp(const IniChar_t* pszPropTag) const = 0;
	};
	 

	class GRAYCORE_LINK cIniObject
	: public IIniObjectDef
 	, public IIniBaseSetter
	, public IIniBaseGetter
	, public IIniBaseEnumerator
	{
		//! @class Gray::cIniObject
		//! Base class for generic object with predefined/known props (Unlike cIniSection) read/written via interfaces.
		//! can be stored as cIniSectionData. Also like cIniMap
		//! Much more simplistic form of IScriptableObj.

		typedef UINT64 PROPMASK_t;	//!< bitmask of IPROPIDX_t. Max 64 props.

	public:
		mutable PROPMASK_t m_nDirtyMask;	//!< bitmask of IPROPIDX_t to be written/persisted.

	public:
		cIniObject() noexcept
		: m_nDirtyMask(0)
		{
		}
		virtual ~cIniObject()
		{
		}

		inline static PROPMASK_t GetDirtyMask(IPROPIDX_t ePropIdx)
		{
			//! ASSUME get_PropQty() <= bits in PROPMASK_t, like _1BITMASK()
			return(((PROPMASK_t)1) << ePropIdx);
		}
		void SetAllDirty()
		{
			//! ASSUME get_PropQty() <= bits in PROPMASK_t
			ASSERT(get_PropQty() <= (IPROPIDX_t) sizeof(PROPMASK_t) * 8);
			m_nDirtyMask = GetDirtyMask(get_PropQty()) - 1;
		}

		// virtual HRESULT PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) override;
		virtual HRESULT PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const override;

		HRESULT FileWriteN(cStreamOutput& sOut, IPROPIDX_t ePropIdx) const;
		HRESULT FileWrite(cStreamOutput& sOut, const IniChar_t* pszProp);
		HRESULT FileWriteAll(cStreamOutput& sOut);
	};
}
#endif // _INC_cIniObject_H
