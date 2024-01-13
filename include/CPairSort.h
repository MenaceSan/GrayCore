//
//! @file cPairSort.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cPairSort_H
#define _INC_cPairSort_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArraySort.h"
#include "cPair.h"

namespace Gray {
/// <summary>
/// array sorted by _TYPE_KEY numeric or string.
/// ITERATE_t FindIForKey("Text") will just find the index to the pointer
/// </summary>
/// <typeparam name="_TYPE_PAIR"></typeparam>
/// <typeparam name="_TYPE_KEY"></typeparam>
template <class _TYPE_PAIR, class _TYPE_KEY>
class cPairSortBase : public cArraySorted<_TYPE_PAIR*, _TYPE_PAIR*, _TYPE_KEY> {
 public:
    virtual ~cPairSortBase() {}

    ITERATE_t InitAssocElements(const _TYPE_PAIR* pInit, size_t iSizeElement = sizeof(_TYPE_PAIR)) {
        //! Init the sorted array with a static array of values.
        ITERATE_t i = 0;
        for (;; i++) {
            BYTE* pData = ((BYTE*)pInit) + (i * iSizeElement);
            if (!*((_TYPE_KEY*)pData)) break;
            SetAtGrow(i, (_TYPE_PAIR*)pData);
        }
        this->QSort();
        return i;
    }
    virtual COMPARE_t CompareKey(KEY_t Key, ARG_t Data2) const noexcept override {
        //! Compare by a key that may not be part of a data record yet.
        //! @note If we reach here assume the key is the whole record !
        return cValT::Compare(Key, *((_TYPE_KEY*)Data2));
    }
    virtual COMPARE_t CompareData(ARG_t Data1, ARG_t Data2) const noexcept override {
        //! Compare a data record to another data record.
        return cValT::Compare(*((_TYPE_KEY*)Data1), *((_TYPE_KEY*)Data2));
    }
    _TYPE_PAIR* FindArgForKey(_TYPE_KEY Key) const {
        //! @note we should put the result in cRefPtr derived pointer.
        ITERATE_t index = FindIForKey(Key);
        if (index < 0) return nullptr;
        return this->GetAt(index);
    }
};

/// <summary>
/// we are sorted by _TYPE_A value (not a string)
/// </summary>
/// <typeparam name="_TYPE_A"></typeparam>
/// <typeparam name="_TYPE_B"></typeparam>
template <class _TYPE_A, class _TYPE_B>
class cPairSortVal : public cPairSortBase<cPair<_TYPE_A, _TYPE_B>, _TYPE_A> {
    typedef cPair<_TYPE_A, _TYPE_B> PAIR_t;
};

/// <summary>
/// we are sorted by _TYPE_A string
/// </summary>
/// <typeparam name="_TYPE_A"></typeparam>
/// <typeparam name="_TYPE_B"></typeparam>
template <class _TYPE_A, class _TYPE_B>
class cPairSortStr : public cPairSortBase<cPair<_TYPE_A, _TYPE_B>, const ATOMCHAR_t*> {
    typedef cPair<_TYPE_A, _TYPE_B> PAIR_t;

 public:
    virtual ~cPairSortStr() {}

    virtual COMPARE_t CompareKey(KEY_t pszKey, ARG_t Data2) const noexcept override {
        //! Compare by a key that may not be part of a data record yet.
        //! @note If we reach here assume the key is the whole record !
        return StrT::CmpI((const ATOMCHAR_t*)pszKey, *(const ATOMCHAR_t**)Data2);
    }
    virtual COMPARE_t CompareData(ARG_t Data1, ARG_t Data2) const noexcept override {
        //! Compare a data record to another data record.
        return StrT::CmpI(*(const ATOMCHAR_t**)Data1, *(const ATOMCHAR_t**)Data2);
    }
    _TYPE_B FindKeyRetB(const ATOMCHAR_t* pszKey) const {
        PAIR_t* pEntry = this->FindArgForKey(pszKey);
        if (pEntry == nullptr) {
            return (_TYPE_B)k_ITERATE_BAD;
        }
        return pEntry->get_B();
    }
};
}  // namespace Gray

#endif
