//
//! @file cArrayString.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cArrayString_H
#define _INC_cArrayString_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArrayRef.h"
#include "cString.h"

namespace Gray
{
	template< typename _TYPE_CH = TCHAR >
	class GRAYCORE_LINK cArrayString : public cArrayTyped < cStringT<_TYPE_CH>, const _TYPE_CH* >
	{
		//! @class Gray::cArrayString
		//! Non-sorted array of strings.
		typedef cStringT<_TYPE_CH> STR_t;
		typedef cArrayTyped< cStringT<_TYPE_CH>, const _TYPE_CH* > SUPER_t;
		typedef cArrayString<_TYPE_CH> THIS_t;
	public:
		static const ITERATE_t k_MaxDefault = 32;	//!< default max for AddUniqueMax.
		static const ITERATE_t k_MaxElements = 64 * 1024;	//!< Max elements. reasonable arbitrary limit.
	public:
		cArrayString() noexcept
		{
		}
		cArrayString(const _TYPE_CH** ppStr, ITERATE_t iCount)
		{
			SetStrings(ppStr, iCount);
		}
		explicit cArrayString(const cArrayString& a)
		{
			this->SetCopy(a);
		}
		cArrayString(THIS_t&& ref) noexcept
			: SUPER_t(ref)
		{
			//! move constructor.
		}

		inline ~cArrayString()
		{
		}
		void SetStrings(const _TYPE_CH** ppStr, ITERATE_t iCount)
		{
			//! Set array of strings.
			this->RemoveAll();
			ASSERT(iCount < k_MaxElements);	// reasonable max.
			for (ITERATE_t i = 0; i < iCount; i++)
			{
				this->Add(ppStr[i]);
			}
		}
		void SetStrings(cStringT<_TYPE_CH>* ppStr, ITERATE_t iCount)
		{
			//! Set array of strings.
			this->RemoveAll();
			ASSERT(iCount < k_MaxElements);	// reasonable max.
			for (ITERATE_t i = 0; i < iCount; i++)
			{
				this->Add(ppStr[i]);
			}
		}
		ITERATE_t _cdecl AddFormat(const _TYPE_CH* pszFormat, ...)
		{
			cStringT<_TYPE_CH> sTmp;
			va_list vargs;
			va_start(vargs, pszFormat);
			sTmp.FormatV(pszFormat, vargs);
			va_end(vargs);
			return this->Add(sTmp);
		}
		ITERATE_t AddTable(const _TYPE_CH* const* ppszTable, size_t iElemSize = sizeof(_TYPE_CH*))
		{
			ITERATE_t i = 0;
			for (; *ppszTable != nullptr; i++)
			{
				ASSERT(i < k_MaxElements);	// reasonable max.
				this->Add(*ppszTable);
				ppszTable = (const _TYPE_CH* const*)(((const BYTE*)ppszTable) + iElemSize);
			}
			return i;
		}
		ITERATE_t AddUniqueMax(const _TYPE_CH* pszStr, ITERATE_t iMax = k_MaxDefault)
		{
			//! Add a non dupe to the list end. if dupe then return index of match.
			//! Enforce iMax qty by delete head.

			if (StrT::IsNullOrEmpty(pszStr) || iMax < 1)
				return k_ITERATE_BAD;
			ITERATE_t iQty = this->GetSize();
			for (ITERATE_t i = 0; i < iQty; i++)
			{
				if (!StrT::CmpI(this->GetAt(i).get_CPtr(), pszStr))	// dupe
				{
					return i;
				}
			}
			while (iQty >= iMax)
			{
				this->RemoveAt(0);	// roll off extras from head.
				iQty--;
			}
			this->Add(pszStr);
			return iQty;
		}
		ITERATE_t FindCmpI(const _TYPE_CH* pszFind) const
		{
			//! find the (whole) string in the unsorted array of strings. Case Ignored.
			const ITERATE_t iQty = this->GetSize();
			for (ITERATE_t i = 0; i < iQty; i++)
			{
				if (!StrT::CmpI(this->GetAt(i).get_CPtr(), pszFind))
					return i;
			}
			return k_ITERATE_BAD;
		}
		ITERATE_t FindStrIR(const _TYPE_CH* pszSearch) const
		{
			//! find inside pszSearch a partial string match of any of the unsorted array of strings. Case Ignored.
			ITERATE_t iQty = this->GetSize();
			for (ITERATE_t i = 0; i < iQty; i++)
			{
				if (StrT::FindStrI(pszSearch, this->GetAt(i).get_CPtr()) != nullptr)
					return i;
			}
			return k_ITERATE_BAD;
		}

		_TYPE_CH** get_PPStr() const
		{
			return (_TYPE_CH**)this->GetData();
		}
		STR_t GetAtCheck(ITERATE_t i) const
		{
			//! @return "" = if index is out of range.
			if (!SUPER_t::IsValidIndex(i))
				return cStrConst::k_Empty.Get<_TYPE_CH>();	// STR_t("")
			return SUPER_t::GetAt(i);
		}

		ITERATE_t SetStrSep(const _TYPE_CH* pszStr, _TYPE_CH chSep = ',')
		{
			//! Set list from parsing a comma (or other) separated list of strings.
			_TYPE_CH szSep[2];
			szSep[0] = chSep;
			szSep[1] = '\0';
			_TYPE_CH szTmp[StrT::k_LEN_MAX];
			_TYPE_CH* aCmds[k_ARG_ARRAY_MAX];
			ITERATE_t iStrings = StrT::ParseArrayTmp(szTmp, STRMAX(szTmp), pszStr, aCmds, _countof(aCmds), szSep);
			SetStrings((const _TYPE_CH**)aCmds, iStrings);
			return iStrings;
		}

		STR_t GetStrSep(_TYPE_CH chSep = ',', ITERATE_t iMax = 0x7FFF) const
		{
			//! @return the whole array as a single comma separated string.
			if (iMax > this->GetSize())
				iMax = this->GetSize();
			STR_t sRet;
			for (ITERATE_t i = 0; i < iMax; i++)
			{
				sRet += SUPER_t::GetAt(i);
				sRet += chSep;
			}
			return sRet;
		}
	};

	typedef cArrayString<char> cArrayStringA;
	typedef cArrayString<wchar_t> cArrayStringW;

}

#endif	// _INC_cArrayString_H
