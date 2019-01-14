//
//! @file CArraySortString.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CArraySortString_H
#define _INC_CArraySortString_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CArraySort.h"
#include "CArrayString.h"
#include "CUnitTestDecl.h"

UNITTEST_PREDEF(CArraySortString)

namespace Gray
{
	template< typename _TYPE_CH = TCHAR >
	class GRAYCORE_LINK CArraySortString : public CArraySorted < cStringT<_TYPE_CH>, cStringT<_TYPE_CH>, const _TYPE_CH* >
	{
		//! @class Gray::CArraySortString
		//! Alpha Sorted array of strings. Case Ignored. duplicates are lost.

	public:
		typedef CArraySorted< cStringT<_TYPE_CH>, cStringT<_TYPE_CH>, const _TYPE_CH*> SUPER_t;
		typedef cStringT<_TYPE_CH> STR_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::KEY_t KEY_t;

	public:
		virtual ~CArraySortString()
		{
		}
		virtual COMPARE_t CompareKey(KEY_t pszID1, REF_t sID2) const override
		{
			ASSERT(pszID1 != nullptr);
			return StrT::CmpI<_TYPE_CH>(pszID1, sID2);
		}
		virtual COMPARE_t CompareData(REF_t sID1, REF_t sID2) const override
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
			//! like CFilePath::IsRelativeRoot
			//! @return -1 = found nothing that would be derived from pszRoot.

			StrLen_t iStrLen = StrT::Len(pszRoot);
			ITERATE_t iHigh = this->GetSize() - 1;
			ITERATE_t iLow = 0;
			while (iLow <= iHigh)
			{
				ITERATE_t i = (iHigh + iLow) / 2;
				STR_t sTest = this->GetAt(i);
				COMPARE_t iCompare = StrT::CmpIN<_TYPE_CH>(pszRoot, sTest, iStrLen);
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
			//! like CFilePath::IsRelativeRoot
			//! @return -1 = found nothing that would be root of pszDerived.

			ITERATE_t iHigh = this->GetSize() - 1;
			ITERATE_t iLow = 0;
			while (iLow <= iHigh)
			{
				ITERATE_t i = (iHigh + iLow) / 2;
				STR_t sTest = this->GetAt(i);
				COMPARE_t iCompare = StrT::CmpIN<_TYPE_CH>(pszDerived, sTest, sTest.GetLength());
				if (iCompare == COMPARE_Equal)
					return(i);	// pszDerived is a child of sTest (derived from root sTest)
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
		UNITTEST_FRIEND(CArraySortString);
	};

	typedef CArraySortString<char> CArraySortStringA;
	typedef CArraySortString<wchar_t> CArraySortStringW;

#ifdef GRAY_DLL // force implementation/instantiate for DLL/SO.
 	template class GRAYCORE_LINK CArraySortString < char >;
	template class GRAYCORE_LINK CArraySortString < wchar_t >;
 	template class GRAYCORE_LINK CArraySorted < cStringT<char>, const char*, const char* >;
	template class GRAYCORE_LINK CArraySorted < cStringT<wchar_t>, const wchar_t*, const wchar_t* >;
#endif

};

#endif

