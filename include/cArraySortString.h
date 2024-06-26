//! @file cArraySortString.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cArraySortString_H
#define _INC_cArraySortString_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cArraySortRef.h"
#include "cArrayString.h"

namespace Gray {
/// <summary>
/// Alpha Sorted array of strings. Case Ignored. duplicates are lost.
/// </summary>
/// <typeparam name="_TYPE_CH"></typeparam>
template <typename _TYPE_CH = TCHAR>
struct GRAYCORE_LINK cArraySortString : public cArraySorted<cStringT<_TYPE_CH>, const cStringT<_TYPE_CH>&, const _TYPE_CH*> {
    typedef cArraySorted<cStringT<_TYPE_CH>, const cStringT<_TYPE_CH>&, const _TYPE_CH*> SUPER_t;

 public:
    typedef cStringT<_TYPE_CH> STR_t;       // alias for container
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;

 protected:
    COMPARE_t CompareElems(ARG_t data1, ARG_t data2) const noexcept override {
        return StrT::CmpI<_TYPE_CH>(data1, data2);
    }
    COMPARE_t CompareKey(KEY_t key1, ARG_t data2) const noexcept override {
        ASSERT_NN(key1);
        return StrT::CmpI<_TYPE_CH>(key1, data2);
    }

 public:
    ITERATE_t AddStr(const _TYPE_CH* pszStr) {
        ASSERT_NN(pszStr);
        return this->AddSort(STR_t(pszStr), 0);
    }

    /// <summary>
    /// Is pszRoot a root of one of the listed paths ? opposite of FindKeyDerived
    /// e.g. pszRoot = a, element[x] = abc
    /// like cFilePath::IsRelativeRoot()
    /// </summary>
    /// <param name="pszRoot"></param>
    /// <returns>-1 = found nothing that would be derived from pszRoot.</returns>
    ITERATE_t FindKeyRoot(const cSpan<_TYPE_CH>& root) const noexcept {
        if (this->isEmpty()) return k_ITERATE_BAD;
        ITERATE_t iHigh = this->GetSize() - 1;
        ITERATE_t iLow = 0;
        while (iLow <= iHigh) {
            const ITERATE_t i = (iHigh + iLow) / 2;
            const STR_t& sCur = this->GetAt(i);
            const COMPARE_t iCompare = StrT::CmpIN<_TYPE_CH>(root, sCur, root.get_MaxLen());
            if (iCompare == COMPARE_Equal) return i;  // pszRoot is a parent of
            if (iCompare > 0) {
                iLow = i + 1;
            } else {
                iHigh = i - 1;
            }
        }
        return k_ITERATE_BAD;
    }

    /// <summary>
    /// Is one of the listed paths a root of pszDerived ? pszDerived is a child. opposite of FindKeyRoot
    /// e.g. pszDerived = abc, element[x] = a
    /// like cFilePath::IsRelativeRoot
    /// </summary>
    /// <param name="pszDerived"></param>
    /// <returns>-1 = found nothing that would be root of pszDerived.</returns>
    ITERATE_t FindKeyDerived(const _TYPE_CH* pszDerived) const noexcept {
        if (this->isEmpty()) return k_ITERATE_BAD;
        ITERATE_t iHigh = this->GetSize() - 1;
        ITERATE_t iLow = 0;
        while (iLow <= iHigh) {
            const ITERATE_t i = (iHigh + iLow) / 2;
            const STR_t sCur = this->GetAt(i);
            const COMPARE_t iCompare = StrT::CmpIN<_TYPE_CH>(pszDerived, sCur, sCur.GetLength());
            if (iCompare == COMPARE_Equal) return i;  // pszDerived is a child of sCur (derived from root sCur)
            if (iCompare > 0) {
                iLow = i + 1;
            } else {
                iHigh = i - 1;
            }
        }
        return k_ITERATE_BAD;
    }
};

typedef cArraySortString<char> cArraySortStringA;
typedef cArraySortString<wchar_t> cArraySortStringW;
}  // namespace Gray
#endif
