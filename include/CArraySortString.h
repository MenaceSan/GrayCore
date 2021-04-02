//
//! @file cArraySortString.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cArraySortString_H
#define _INC_cArraySortString_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArraySortRef.h"
#include "cArrayString.h"

namespace Gray
{
	template< typename _TYPE_CH = TCHAR >
	class GRAYCORE_LINK cArraySortString : public cArraySorted < cStringT<_TYPE_CH>, cStringT<_TYPE_CH>, const _TYPE_CH* >
	{
		//! @class Gray::cArraySortString
		//! Alpha Sorted array of strings. Case Ignored. duplicates are lost.

	public:
		typedef cArraySorted< cStringT<_TYPE_CH>, cStringT<_TYPE_CH>, const _TYPE_CH*> SUPER_t;
		typedef cStringT<_TYPE_CH> STR_t;	// alias for container
		typedef typename SUPER_t::ARG_t ARG_t;
		typedef typename SUPER_t::KEY_t KEY_t;

	public:
		virtual ~cArraySortString()
		{
		}
		virtual COMPARE_t CompareKey(KEY_t pszID1, ARG_t sID2) const noexcept override
		{
			ASSERT(pszID1 != nullptr);
			return StrT::CmpI<_TYPE_CH>(pszID1, sID2);
		}
		virtual COMPARE_t CompareData(ARG_t sID1, ARG_t sID2) const noexcept override
		{
			return StrT::CmpI<_TYPE_CH>(sID1, sID2);
		}

		ITERATE_t AddStr(const _TYPE_CH* pszStr)
		{
			return this->Add(STR_t(pszStr));
		}

		ITERATE_t FindKeyRoot(const _TYPE_CH* pszRoot)
		{
			//! pszRoot is a root of one of the listed paths ? opposite of FindKeyDerived
			//! e.g. pszRoot = a, element[x] = abc
			//! like cFilePath::IsRelativeRoot
			//! @return -1 = found nothing that would be derived from pszRoot.

			StrLen_t iStrLen = StrT::Len(pszRoot);
			ITERATE_t iHigh = this->GetSize() - 1;
			ITERATE_t iLow = 0;
			while (iLow <= iHigh)
			{
				ITERATE_t i = (iHigh + iLow) / 2;
				STR_t sCur = this->GetAt(i);
				COMPARE_t iCompare = StrT::CmpIN<_TYPE_CH>(pszRoot, sCur, iStrLen);
				if (iCompare == COMPARE_Equal)
					return i;	// pszRoot is a parent of
				if (iCompare > 0)
				{
					iLow = i + 1;
				}
				else
				{
					iHigh = i - 1;
				}
			}
			return k_ITERATE_BAD;
		}

		ITERATE_t FindKeyDerived(const _TYPE_CH* pszDerived)
		{
			//! one of the listed paths is a root of pszDerived ? pszDerived is a child. opposite of FindKeyRoot
			//! e.g. pszDerived = abc, element[x] = a
			//! like cFilePath::IsRelativeRoot
			//! @return -1 = found nothing that would be root of pszDerived.

			ITERATE_t iHigh = this->GetSize() - 1;
			ITERATE_t iLow = 0;
			while (iLow <= iHigh)
			{
				ITERATE_t i = (iHigh + iLow) / 2;
				STR_t sCur = this->GetAt(i);
				COMPARE_t iCompare = StrT::CmpIN<_TYPE_CH>(pszDerived, sCur, sCur.GetLength());
				if (iCompare == COMPARE_Equal)
					return(i);	// pszDerived is a child of sCur (derived from root sCur)
				if (iCompare > 0)
				{
					iLow = i + 1;
				}
				else
				{
					iHigh = i - 1;
				}
			}
			return k_ITERATE_BAD;
		}
	};

	typedef cArraySortString<char> cArraySortStringA;
	typedef cArraySortString<wchar_t> cArraySortStringW;
}

#endif
