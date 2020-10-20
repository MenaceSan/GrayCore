//
//! @file CIniObject.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CIniObject.h"
#include "CIniSection.h"
#include "CString.h"
#include "CCodeProfiler.h"
#include "CStream.h"

namespace Gray
{
	HRESULT CIniObject::PropSet(const IniChar_t* pszPropTag, const IniChar_t* pszValue) // override
	{
		//! IIniBaseSetter
		//! Set a prop by its string name.
		//! default implementation.
		//! @return E_INVALIDARG,  HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY)

		// m_nDirtyMask

		return this->PropSetN(this->FindProp(pszPropTag), pszValue);
	}

	HRESULT CIniObject::PropGet(const IniChar_t* pszPropTag, OUT CStringI& rsValue) const // override
	{
		//! IIniBaseGetter
		//! Read a prop by its string name.
		//! default implementation.
		return this->PropEnum(this->FindProp(pszPropTag), rsValue, nullptr);
	}

	HRESULT CIniObject::FileWriteN(CStreamOutput& rOut, IPROPIDX_t ePropIdx) const
	{
		//! Write the prop out to the stream.
		//! CStreamOutput
		CODEPROFILEFUNC();
		PROPMASK_t nPropMask = GetDirtyMask(ePropIdx);
		if (!(m_nDirtyMask & nPropMask))	// already written. or not changed?
			return S_FALSE;
		CStringI sValue;
		HRESULT hRes = this->PropEnum(ePropIdx, sValue);
		if (FAILED(hRes))
			return hRes;
		m_nDirtyMask &= ~nPropMask;		// not dirty anymore.
		return CIniWriter(&rOut).WriteKeyUnk(this->get_PropName(ePropIdx), sValue);
	}

	HRESULT CIniObject::FileWrite(CStreamOutput& rOut, const IniChar_t* pszProp)
	{
		//! write this prop by name.
		CODEPROFILEFUNC();
		IPROPIDX_t ePropIdx = this->FindProp(pszProp); // Str_TableFindHead(pszProp,get_Props());
		if (ePropIdx < 0)
			return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
		return FileWriteN(rOut, ePropIdx);
	}

	HRESULT CIniObject::FileWriteAll(CStreamOutput& rOut)
	{
		//! Write out all that are not already written.
		//! Assume [HEADER] already written.
		CODEPROFILEFUNC();
		IPROPIDX_t iQty = this->get_PropQty();
		for (IPROPIDX_t i = 0; i < iQty; i++)
		{
			PROPMASK_t nPropMask = GetDirtyMask(i);
			if (!(m_nDirtyMask & nPropMask))	// was already written? or not changed?
				continue;
			HRESULT hRes = FileWriteN(rOut, i);
			m_nDirtyMask &= ~nPropMask;
			if (FAILED(hRes))
			{
				return hRes;
			}
		}

		m_nDirtyMask = 0;
		return S_OK;
	}
}

//*************************************************************************
#if USE_UNITTESTS
#include "CUnitTest.h"
#include "CLogMgr.h"

UNITTEST_CLASS(CIniObject)
{
	UNITTEST_METHOD(CIniObject)
	{
		// TODO: UNITTEST CIniObject
		class CUnitTestIniObject : public CIniObject
		{
		public:
			virtual IPROPIDX_t get_PropQty(void) const
			{
				// IIniObjectDef
				return 0;
			}
			virtual const IniChar_t* get_PropName(IPROPIDX_t ePropIdx) const
			{
				// IIniObjectDef
				UNREFERENCED_PARAMETER(ePropIdx);
				return nullptr;
			}
			virtual IPROPIDX_t FindProp(const IniChar_t* pName) const
			{
				// IIniObjectDef
				UNREFERENCED_PARAMETER(pName);
				return -1;
			}
			HRESULT PropSetN(IPROPIDX_t ePropIdx, const IniChar_t* pszName)
			{
				// IIniObjectWriteN
				UNREFERENCED_PARAMETER(ePropIdx);
				UNREFERENCED_PARAMETER(pszName);
				return E_NOTIMPL;
			}
			virtual HRESULT PropEnum(IPROPIDX_t ePropIdx, OUT CStringI& rsValue, CStringI* psKey = nullptr) const
			{
				// IIniBaseEnumerator
				UNREFERENCED_PARAMETER(ePropIdx);
				UNREFERENCED_REFERENCE(rsValue);
				UNREFERENCED_PARAMETER(psKey);
				return E_NOTIMPL;
			}
		};
		CUnitTestIniObject obj;
	}
};
UNITTEST_REGISTER(CIniObject, UNITTEST_LEVEL_Core);
#endif
