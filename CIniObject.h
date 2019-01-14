//
//! @file CIniObject.h
//! very simplistic string scriptable object.
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_CIniObject_H
#define _INC_CIniObject_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CIniBase.h"
#include "CUnitTestDecl.h"

UNITTEST_PREDEF(CIniObject)

namespace Gray
{
	class CStreamOutput;

	DECLARE_INTERFACE(IIniObjectDef)
	{
		//! @interface Gray::IIniObjectDef
		//! get basic name metadata about the props supported.
		IGNORE_WARN_INTERFACE(IIniObjectDef);

		virtual IPROPIDX_t get_PropQty() const = 0;
		virtual const IniChar_t* get_PropName(IPROPIDX_t ePropIdx) const = 0;
		virtual IPROPIDX_t FindProp(const IniChar_t* pszPropTag) const = 0;
	};
	DECLARE_INTERFACE(IIniObjectWriteN)
	{
		//! @interface Gray::IIniObjectWriteN
		//! Set enumerated properties by index. (value is a string)
		IGNORE_WARN_INTERFACE(IIniObjectWriteN);
		virtual HRESULT PropSetN(IPROPIDX_t ePropIdx, const IniChar_t* pszValue) = 0;
	};

	class GRAYCORE_LINK CIniObject
	: public IIniObjectDef
	, public IIniObjectWriteN
	, public IIniBaseSetter
	, public IIniBaseGetter
	, public IIniBaseEnumerator
	{
		//! @class Gray::CIniObject
		//! Base class for generic object with predefined/known props (Unlike CIniSection) read/written via interfaces.
		//! can be stored as CIniSectionData. Also like CIniMap
		//! Much more simplistic form of IScriptableObj.

		typedef UINT64 PROPMASK_t;	//!< bitmask of IPROPIDX_t. Max 64 props.

	public:
		mutable PROPMASK_t m_nDirtyMask;	//!< bitmask of IPROPIDX_t to be written/persisted.

	public:
		CIniObject()
		: m_nDirtyMask(0)
		{
		}
		virtual ~CIniObject()
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

		virtual HRESULT PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) override;
		virtual HRESULT PropGet(const IniChar_t* pszPropTag, OUT CStringI& rsValue) const override;

		HRESULT FileWriteN(CStreamOutput& sOut, IPROPIDX_t ePropIdx) const;
		HRESULT FileWrite(CStreamOutput& sOut, const IniChar_t* pszProp);
		HRESULT FileWriteAll(CStreamOutput& sOut);

		UNITTEST_FRIEND(CIniObject);
	};
};
#endif // _INC_CIniObject_H
