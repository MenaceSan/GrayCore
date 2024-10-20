//! @file cArraySortRef.h
//! c++ sorted collections.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cArraySortRef_H
#define _INC_cArraySortRef_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cArrayRef.h"
#include "cArraySort.h"

namespace Gray {
/// <summary>
/// A sorted array of "cRefPtr<TYPE>" objects. TYPE_KEY = get_SortVal()
/// the array has a reference to the element. similar to cArrayRef but sorted
/// It will get deleted when the reference count is 0.
/// default sort by cMem::Compare() pointers.
/// </summary>
/// <typeparam name="TYPE">must be cRefBase based</typeparam>
/// <typeparam name="TYPE_KEY"></typeparam>
template <class TYPE, typename TYPE_KEY>
class cArraySortRef : public cArraySortFacade<cRefPtr<TYPE>, TYPE*, TYPE_KEY> {
    typedef cArraySortFacade<cRefPtr<TYPE>, TYPE*, TYPE_KEY> SUPER_t;

 public:
    /// <summary>
    /// Similar to RemoveAll() except it calls DisposeThis() to try to dereference all the entries.
    /// ASSUME TYPE supports DisposeThis(); like cXObject
    /// @note often DisposeThis() has the effect of removing itself from the list. We protect against this.
    /// </summary>
    void DisposeAll() {
        ITERATE_t iSize = this->GetSize();
        for (ITERATE_t i = iSize - 1; i >= 0; i--) {  // reverse order they got added. might be faster?
            cRefPtr<TYPE> pObj = this->GetAt(i);
            if (pObj != nullptr) pObj->DisposeThis();

            // DisposeThis might remove itself (and children) from the list? So the list might get shorter.
            const ITERATE_t iSize2 = this->GetSize();
            if (iSize2 != iSize) i = iSize = iSize2;  // start over.
        }
        this->RemoveAll();
    }
};

//*************************************************

/// <summary>
/// A _TYPE_HASH get_HashCode() sorted array of cRefPtr<TYPE>
/// does NOT allow dupe hash codes !
/// </summary>
/// <typeparam name="TYPE">typically based on IScriptableObj</typeparam>
/// <typeparam name="_TYPE_HASH">get_HashCode()</typeparam>
template <class TYPE, typename _TYPE_HASH = HASHCODE_t>
class cArraySortHash : public cArraySortRef<TYPE, _TYPE_HASH> {
    typedef cArraySortRef<TYPE, _TYPE_HASH> SUPER_t;

 public:
    typedef typename SUPER_t::ELEM_t ELEM_t;
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;

 protected:
    COMPARE_t CompareElems(ARG_t pData1, ARG_t pData2) const noexcept override {
        //! Compare a data record to another data record.
        ASSERT_NN(pData1);
        ASSERT_NN(pData2);
        const KEY_t key1 = pData1->get_HashCode();
        const KEY_t key2 = pData2->get_HashCode();
        return cValT::Compare(key1, key2);
    }
    COMPARE_t CompareKey(KEY_t key1, ARG_t pBase) const noexcept override {
        //! INT_MAX - INT_MIN must be positive !
        //! @note x-y will not work for extreme values so we use cValT::Compare
        ASSERT_NN(pBase);
        const KEY_t key2 = pBase->get_HashCode();
        return cValT::Compare(key1, key2);
    }

 public:
    ITERATE_t FindIForAK(const TYPE* pBase) const {
        //! Like of FindIFor() but uses the key.
        //! @return index, -1 = k_ITERATE_BAD = none.
        if (pBase == nullptr) return k_ITERATE_BAD;
        return this->FindIForKey(pBase->get_HashCode());
    }
    bool RemoveArgKey(TYPE* pBase) {
        if (pBase == nullptr) return false;
        return SUPER_t::RemoveArgKey(pBase, pBase->get_HashCode());
    }
};

//*************************************************

/// <summary>
/// A TYPE_KEY get_SortValue() sorted array of cRefPtr<TYPE>. sort low to high
/// @note allow duplicate get_SortValue() but NOT duplicate objects!
/// Similar to HashCode but different in the it can be any type. (float,etc)
/// </summary>
/// <typeparam name="TYPE">based on IScriptableObj and cRefBase typically</typeparam>
/// <typeparam name="TYPE_KEY">get_SortValue()</typeparam>
template <class TYPE, typename TYPE_KEY = int>
class cArraySortValue : public cArraySortRef<TYPE, TYPE_KEY> {
    typedef cArraySortRef<TYPE, TYPE_KEY> SUPER_t;

 public:
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;  // TYPE_KEY

 protected:
    COMPARE_t CompareElems(ARG_t pData1, ARG_t pData2) const noexcept override {
        //! Compare a data record to another data record.
        ASSERT_NN(pData1);
        ASSERT_NN(pData2);
        const TYPE_KEY key1 = pData1->get_SortValue();
        const TYPE_KEY key2 = pData2->get_SortValue();
        const COMPARE_t iDiff = cValT::Compare(key1, key2);
        if (iDiff == COMPARE_Equal)  // allow duplicate get_SortValue() but NOT duplicate objects!
            return cValT::Compare(CastPtrToNum(pData1), CastPtrToNum(pData2));
        return iDiff;
    }
    COMPARE_t CompareKey(KEY_t key1, ARG_t pBase) const noexcept override {
        if (pBase == nullptr) return COMPARE_Greater;
        const TYPE_KEY key2 = pBase->get_SortValue();
        return cValT::Compare(key1, key2);
    }

 public:
    /// <summary>
    /// Equivalent of FindIFor() but uses the key for faster access. must ignore key dupes.
    /// </summary>
    /// <param name="pBase"></param>
    /// <returns>index, -1 = k_ITERATE_BAD = none.</returns>
    ITERATE_t FindIForAK(const TYPE* pBase) const {
        if (pBase == nullptr) return k_ITERATE_BAD;
        TYPE_KEY nKey = pBase->get_SortValue();
        ITERATE_t i = this->FindIFirstForKey(nKey);
        if (i < 0) return k_ITERATE_BAD;
        for (;; i++) {
            cRefPtr<TYPE> pBase2 = this->GetAtCheck(i);
            if (pBase2 == nullptr) break;   // pBase is not in the array!
            if (pBase2 == pBase) return i;  // since sorted values are allowed to duplicate.
        }
        // This probably shouldn't happen? pBase is not in the array!
        return k_ITERATE_BAD;  // FindIForAC(pBase);	// just do a brute force search.
    }
    bool RemoveArgKey(TYPE* pBase) {
        const ITERATE_t index = this->FindIForAK(pBase);
        if (index < 0) return false;
        SUPER_t::RemoveAt(index);
        return true;
    }
    /// <summary>
    /// Add this last after any duplicate keys.
    /// </summary>
    ITERATE_t AddAfter(TYPE* pBase) {
        ASSERT_NN(pBase);
        const TYPE_KEY nKey = pBase->get_SortValue();
        ITERATE_t i = this->FindILastForKey(nKey);
        if (i < 0) return this->AddSort(pBase);  // one of the same type is here? add new sorted by nKey.
        this->InsertAt(++i, pBase);              // add to the end of the get_SortValue series.
        return i;
    }
};

//*************************************************

/// <summary>
/// get_Name() sorted array of cRefPtr TYPE.
/// does NOT allow dupe names !
/// </summary>
/// <typeparam name="TYPE">must support get_Name() and be cRefBase</typeparam>
/// <typeparam name="_TYPECH"></typeparam>
template <class TYPE, typename _TYPECH = GChar_t>
class cArraySortName : public cArraySortRef<TYPE, const _TYPECH*> {
    typedef cArraySortRef<TYPE, const _TYPECH*> SUPER_t;

 public:
    typedef typename SUPER_t::ELEM_t ELEM_t;
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;

 protected:
    COMPARE_t CompareElems(ARG_t pData1, ARG_t pData2) const noexcept override {
        //! Compare a data record to another data record.
        ASSERT_NN(pData1);
        ASSERT_NN(pData2);
        return StrT::CmpI<_TYPECH>(pData1->get_Name(), pData2->get_Name());
    }
    COMPARE_t CompareKey(KEY_t key1, ARG_t pObj) const noexcept override {
        ASSERT_NN(key1);
        ASSERT_NN(pObj);
        return StrT::CmpI<_TYPECH>(key1, pObj->get_Name());
    }

 public:
    ITERATE_t FindIForAK(const TYPE* pBase) const {
        //! Equivalent of FindIFor() but uses the key.
        //! FindIForKey using the key.
        //! @return index, -1 = k_ITERATE_BAD = none.
        if (pBase == nullptr) return k_ITERATE_BAD;
        return this->FindIForKey(pBase->get_Name());
    }
    bool RemoveArgKey(TYPE* pBase) {
        if (pBase == nullptr) return false;
        return SUPER_t::RemoveArgKey(pBase, pBase->get_Name());
    }
};
}  // namespace Gray
#endif  // _INC_cArraySortRef_H
