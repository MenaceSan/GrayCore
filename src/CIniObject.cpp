//
//! @file cIniObject.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cIniObject.h"
#include "cIniSection.h"
#include "cString.h"
#include "cCodeProfiler.h"
#include "cStream.h"

namespace Gray
{
	HRESULT cIniObject::PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const // override
	{
		//! IIniBaseGetter
		//! Read a prop by its string name.
		//! default implementation.
		return this->PropGetEnum(this->FindProp(pszPropTag), rsValue, nullptr);
	}

	HRESULT cIniObject::FileWriteN(cStreamOutput& rOut, IPROPIDX_t ePropIdx) const
	{
		//! Write the prop out to the stream.
		//! cStreamOutput
		CODEPROFILEFUNC();
		PROPMASK_t nPropMask = GetDirtyMask(ePropIdx);
		if (!(m_nDirtyMask & nPropMask))	// already written. or not changed?
			return S_FALSE;
		cStringI sValue;
		HRESULT hRes = this->PropGetEnum(ePropIdx, sValue);
		if (FAILED(hRes))
			return hRes;
		m_nDirtyMask &= ~nPropMask;		// not dirty anymore.
		return cIniWriter(&rOut).WriteKeyUnk(this->get_PropName(ePropIdx), sValue);
	}

	HRESULT cIniObject::FileWrite(cStreamOutput& rOut, const IniChar_t* pszProp)
	{
		//! write this prop by name.
		CODEPROFILEFUNC();
		IPROPIDX_t ePropIdx = this->FindProp(pszProp); // Str_TableFindHead(pszProp,get_Props());
		if (ePropIdx < 0)
			return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
		return FileWriteN(rOut, ePropIdx);
	}

	HRESULT cIniObject::FileWriteAll(cStreamOutput& rOut)
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
 