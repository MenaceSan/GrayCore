//! @file cArraySort.h
//! c++ sorted collections.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cArraySort_H
#define _INC_cArraySort_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrT.h"
#include "cArray.h"

namespace Gray {
/// <summary>
/// An array of some sorted TYPE. added duplicates may be destroyed.
/// default is that it is just sorted by its bytes.
/// Similar to .NET HashSet
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="TYPE_ARG"></typeparam>
/// <typeparam name="TYPE_KEY"></typeparam>
template <class TYPE, class TYPE_ARG, typename TYPE_KEY = TYPE_ARG>
class cArraySorted : public cArrayImpl<cSpanSorted<TYPE, TYPE_ARG>> {
    typedef cArrayImpl<cSpanSorted<TYPE, TYPE_ARG>> SUPER_t;

 protected:
    typedef TYPE ELEM_t;
    typedef TYPE_ARG ARG_t;
    typedef TYPE_KEY KEY_t;  // make a typedef for this type.

    virtual ~cArraySorted() {}

    ITERATE_t Add(TYPE_ARG newElement) = delete;  // dont use this directly. Use AddSort().

    /// <summary>
    /// Compare the element by some arbitrary key. @note: We have no idea how to access its internal key at this point.
    /// </summary>
    /// <param name="key1"></param>
    /// <param name="Data2"></param>
    /// <returns></returns>
    virtual COMPARE_t CompareKey(KEY_t key1, TYPE_ARG data2) const noexcept = 0;

    /// <summary>
    /// Remove element from array. find it by its key. @note Can't use this for arrays that allow dupes ! (e.g. get_Value()) Use FindIForAK() instead.
    /// </summary>
    /// <param name="pObj"></param>
    /// <param name="key"></param>
    /// <returns></returns>
    bool RemoveArgKey(TYPE_ARG pObj, KEY_t key) {
        const ITERATE_t index = FindIForKey(key);
        if (index <= k_ITERATE_BAD) return false;
        TYPE_ARG pObjOld = this->GetAt(index);
        if (!(pObjOld == pObj)) {  // not the current one !! weird! we don't allow dupes !
            ASSERT(0);
            return false;  // this should never happen!
        }
        this->RemoveAt(index);
        return true;
    }

 public:
    /// <summary>
    /// Do a binary search for the key. For use with AddPresorted()
    /// </summary>
    /// <param name="key"></param>
    /// <param name="iCompareRes">OUT 0 = key match with element at the index.
    ///		-1 = key is less than element at the index. COMPARE_Less
    ///		+1 = key is greater than element at the index</param>
    /// <returns>index</returns>
    ITERATE_t FindINearKey(KEY_t key, OUT COMPARE_t& iCompareRes) const noexcept;

    /// <summary>
    /// Find index for exact key match. Similar to FindIFor()
    /// </summary>
    /// <param name="key"></param>
    /// <returns>index into array. 0 based of course. -1 = failed</returns>
    ITERATE_t FindIForKey(KEY_t key) const noexcept {
        COMPARE_t iCompareRes;
        const ITERATE_t index = FindINearKey(key, OUT iCompareRes);
        if (iCompareRes != COMPARE_Equal) return k_ITERATE_BAD;
        return index;
    }

    /// <summary>
    /// Find first the occurrence of this nKey. Since values are allowed to duplicate.
    /// </summary>
    /// <param name="nKey"></param>
    /// <returns>index, -1 = k_ITERATE_BAD = none.</returns>
    ITERATE_t FindIFirstForKey(TYPE_KEY nKey) const {
        ITERATE_t i = this->FindIForKey(nKey);
        if (i < 0) return k_ITERATE_BAD;
        // Walk Back to get First.
        for (;;) {
            if (--i < 0) break;
            if (CompareKey(nKey, this->GetAt(i)) != COMPARE_Equal) break;
        }
        return i + 1;
    }
    /// <summary>
    /// Find the last occurrence of this nKey. Since values are allowed to duplicate.
    /// </summary>
    /// <param name="nKey"></param>
    /// <returns>index, -1 = k_ITERATE_BAD = none.</returns>
    ITERATE_t FindILastForKey(TYPE_KEY nKey) const {
        ITERATE_t i = this->FindIForKey(nKey);
        if (i < 0) return k_ITERATE_BAD;
        for (;;) {  // Walk Forward to get Last.
            if (++i >= this->GetSize()) break;
            if (CompareKey(nKey, this->GetAt(i)) != COMPARE_Equal) break;
        }
        return i - 1;  // last
    }

    /// <summary>
    /// Add entry into array if i already know its sorted. For use with FindINearKey()
    /// </summary>
    /// <param name="index"></param>
    /// <param name="iCompareRes"></param>
    /// <param name="pNew"></param>
    /// <returns>index in the array. (temporary if sorted)</returns>
    ITERATE_t AddPresorted(ITERATE_t index, COMPARE_t iCompareRes, TYPE_ARG pNew) {
        if (iCompareRes > 0) index++;  // key is greater than existing element at index. so put it after.
        this->InsertAt(index, pNew);
        return index;
    }

    /// <summary>
    /// Might be dangerous for arrays that allow dupes?
    /// </summary>
    bool RemoveKey(TYPE_KEY key) {
        const ITERATE_t index = FindIForKey(key);
        if (index <= k_ITERATE_BAD) return false;
        this->RemoveAt(index);
        return true;
    }

    /// <summary>
    //! Insertion sort. duplicates may be destroyed. do nothing if IsEqual3()
    /// </summary>
    /// <param name="pNew"></param>
    /// <param name="collideAction">-1 = ignore new, keep old, 1 = destroy old, replace with new. else keep old. 2=ASSERT</param>
    /// <returns>index in the array. (temporary if sorted)</returns>
    ITERATE_t AddSort(TYPE_ARG pNew, int collideAction = 0);  // add in sorted order.

    /// <summary>
    /// Add all the entries in array a to this array. sorted add. like InsertArray() sort of.
    /// </summary>
    void AddArray(const SUPER_t& src) {
        for (auto elem : src) AddSort(elem, 1);
    }
};

template <class TYPE, class TYPE_ARG, typename TYPE_KEY>
ITERATE_t cArraySorted<TYPE, TYPE_ARG, TYPE_KEY>::AddSort(TYPE_ARG pNew, int collideAction) {
    COMPARE_t iCompareRes;
    const ITERATE_t index = this->FindINearS(pNew, OUT iCompareRes);
    if (iCompareRes == COMPARE_Equal) {
        if (IsEqual3<TYPE_ARG>(pNew, this->GetAt(index))) return index;  // identical.
        switch (collideAction) {
            case -1:
                return k_ITERATE_BAD;  // keep old. intentional. ignore new.
            case 1:
                this->SetAt(index, pNew);  // replace the old one. keep new. // DestructElements is called automatically for previous.
                return index;
            case 2:
                DEBUG_CHECK(0);  // should NEVER happen! DEBUG this!
                break;           // we should fail if dupe!?
        }
        return k_ITERATE_BAD;  // failed to add ! Dupe. keep old. No idea what intent there is.
    }
    return AddPresorted(index, iCompareRes, pNew);
}

template <class TYPE, class TYPE_ARG, typename TYPE_KEY>
ITERATE_t cArraySorted<TYPE, TYPE_ARG, TYPE_KEY>::FindINearKey(KEY_t key, OUT COMPARE_t& riCompareRes) const noexcept {
    if (this->isEmpty()) {
        riCompareRes = COMPARE_Less;
        return 0;
    }

    ITERATE_t iHigh = this->GetSize() - 1;
    ITERATE_t iLow = 0;
    ITERATE_t i = 0;
    COMPARE_t iCompareRes = COMPARE_Less;
    while (iLow <= iHigh) {
        i = (iHigh + iLow) / 2;
        iCompareRes = CompareKey(key, this->GetAt(i));
        if (iCompareRes == COMPARE_Equal) break;
        if (iCompareRes > 0) {
            iLow = i + 1;
        } else {
            iHigh = i - 1;
        }
    }

    riCompareRes = iCompareRes;
    return i;
}

//********************************************************************

/// <summary>
/// A sorted array of some native/simple TYPE of values (NOT Pointers)
/// No duplicates allowed.
/// DEFAULT = sort is low to high
/// </summary>
/// <typeparam name="TYPE">ASSUME supports cValT::Compare()</typeparam>
template <class TYPE>
class cArraySortVal : public cArraySorted<TYPE, TYPE, TYPE> {
    typedef cArraySorted<TYPE, TYPE, TYPE> SUPER_t;

 public:
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;

 protected:
    COMPARE_t CompareElems(ARG_t data1, ARG_t data2) const noexcept override {
        return cValT::Compare(data1, data2);
    }
    COMPARE_t CompareKey(KEY_t key1, ARG_t data2) const noexcept override {
        return cValT::Compare(key1, data2);
    }

 public:
    bool RemoveArgKey(TYPE data1) {
        return SUPER_t::RemoveArgKey(data1, data1);
    }
};

/// <summary>
/// Just sort the struct by its memory. similar to cArraySortVal
/// </summary>
template <class TYPE>
class cArraySortStruct : public cArraySorted<TYPE, const TYPE&, const TYPE&> {
    typedef cArraySorted<TYPE, const TYPE&, const TYPE&> SUPER_t;

 public:
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;

 protected:
    COMPARE_t CompareElems(ARG_t data1, ARG_t data2) const noexcept override {
        return cValT::Compare(data1, data2);
    }

    /// <summary>
    /// Assume key is just a pointer to struct ?? default.
    /// </summary>
    COMPARE_t CompareKey(KEY_t key1, ARG_t data2) const noexcept override {
        return cMem::Compare(&key1, &data2, sizeof(TYPE));
    }
};

/// <summary>
/// A get_Name() sorted array of some type of structure (Non dynamic structure,NOT Pointer)
/// DEFAULT = Alphabetic Non cased sort is low to high. A-Z
/// </summary>
/// <typeparam name="TYPE">ASSUME supports get_Name()</typeparam>
/// <typeparam name="_TYPECH"></typeparam>
template <class TYPE, typename _TYPECH = char>
class cArraySortStructName : public cArraySorted<TYPE, const TYPE&, const _TYPECH*> {
    typedef cArraySorted<TYPE, const TYPE&, const _TYPECH*> SUPER_t;

 public:
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;

 protected:
    COMPARE_t CompareElems(ARG_t data1, ARG_t data2) const noexcept override {
        return StrT::CmpI<_TYPECH>(data1.get_Name(), data2.get_Name());
    }
    COMPARE_t CompareKey(KEY_t key1, ARG_t data2) const noexcept override {
        return StrT::CmpI<_TYPECH>(key1, data2.get_Name());
    }

 public:
    const TYPE* FindArgForKey(KEY_t key1) const noexcept {
        //! put the result in TYPE derived pointer.
        const ITERATE_t index = FindIForKey(key1);
        if (index < 0) return nullptr;
        return &(this->GetAt(index));
    }
};

/// <summary>
/// A get_SortValue() sorted array of some type of structure (Non dynamic structure,NOT Pointer)
/// Similar to HashCode but different in the it can be any type. (float,etc)
/// @note allow duplicate get_SortValue() but NOT duplicate objects!
/// </summary>
/// <typeparam name="TYPE">ASSUME supports get_SortValue()</typeparam>
/// <typeparam name="TYPE_KEY"></typeparam>
template <class TYPE, typename TYPE_KEY = int>
class cArraySortStructValue : public cArraySorted<TYPE, const TYPE&, TYPE_KEY> {
    typedef cArraySorted<TYPE, const TYPE&, TYPE_KEY> SUPER_t;

 public:
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;

 protected:
    /// <summary>
    /// Compare a data record to another data record.
    /// </summary>
    COMPARE_t CompareElems(ARG_t data1, ARG_t data2) const noexcept override {
        const TYPE_KEY key1 = data1.get_SortValue();
        const TYPE_KEY key2 = data2.get_SortValue();
        const COMPARE_t iDiff = cValT::Compare(key1, key2);
        if (iDiff == COMPARE_Equal) {                                           // allow duplicate get_SortValue() but NOT duplicate objects!
            return cValT::Compare(CastPtrToNum(&data1), CastPtrToNum(&data2));  // like IsEqual3 ?
        }
        return iDiff;
    }
    COMPARE_t CompareKey(KEY_t key1, ARG_t data2) const noexcept override {
        const TYPE_KEY key2 = data2.get_SortValue();
        return cValT::Compare(key1, key2);
    }

 public:
    /// <summary>
    /// Find key and return result in TYPE derived pointer.
    /// </summary>
    /// <param name="key1"></param>
    /// <returns></returns>
    const TYPE* FindArgForKey(KEY_t key1) const {
        const ITERATE_t index = FindIForKey(key1);
        if (index < 0) return nullptr;
        return &(this->GetAt(index));
    }
};

/// <summary>
/// A get_HashCode() sorted array of some TYPE of structure (Non dynamic structure,NOT Pointer).
/// Does NOT allow dupe hash codes!
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="_TYPE_HASH"></typeparam>
template <class TYPE, typename _TYPE_HASH = HASHCODE_t>
class cArraySortStructHash : public cArraySorted<TYPE, const TYPE&, _TYPE_HASH> {
    typedef cArraySorted<TYPE, const TYPE&, _TYPE_HASH> SUPER_t;

 public:
    typedef typename SUPER_t::ELEM_t ELEM_t;
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;

 protected:
    COMPARE_t CompareElems(ARG_t data1, ARG_t data2) const noexcept override {
        const _TYPE_HASH key1 = data1.get_HashCode();
        const _TYPE_HASH key2 = data2.get_HashCode();
        return cValT::Compare(key1, key2);
    }
    COMPARE_t CompareKey(KEY_t key1, ARG_t data2) const noexcept override {
        const _TYPE_HASH key2 = data2.get_HashCode();
        return cValT::Compare(key1, key2);  // @note x-y will not work for extreme values! e.g. INT_MAX - INT_MIN must be positive!
    }

 public:
    const TYPE* FindArgForKey(KEY_t key1) const {
        //! put the result in TYPE derived pointer.
        const ITERATE_t index = FindIForKey(key1);
        if (index < 0) return nullptr;
        return &(this->GetAt(index));
    }
};

//********************************************************************

/// <summary>
/// A sorted array of some TYPE_PTR pointers. overload this. Abstract base.
/// Sorted by TYPE_KEY get_SortVal
/// Default Sort by cMem::Compare()
/// </summary>
/// <typeparam name="TYPE">the pointer or facade we are storing.</typeparam>
/// <typeparam name="TYPE_PTR"></typeparam>
/// <typeparam name="TYPE_KEY"></typeparam>
template <class TYPE, class TYPE_PTR = TYPE*, typename TYPE_KEY = TYPE*>
class cArraySortFacade : public cArraySorted<TYPE, TYPE_PTR, TYPE_KEY> {
    typedef cArraySorted<TYPE, TYPE_PTR, TYPE_KEY> SUPER_t;

 public:
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;

 protected:
    /// <summary>
    /// default implementation = Binary compare the whole thing. @note: Big Endian numbers wont actually be comparing correctly but maybe we dont care.
    /// </summary>
    COMPARE_t CompareElems(ARG_t data1, ARG_t data2) const noexcept override {
        return cValT::Compare(data1, data2);
    }

 public:
    /// <summary>
    /// Get TYPE wrapped pointer at index. NOT just the raw TYPE_PTR.
    /// </summary>
    /// <param name="nIndex"></param>
    /// <returns></returns>
    TYPE GetAtCheck(ITERATE_t nIndex) const {
        if (!SUPER_t::IsValidIndex(nIndex)) return nullptr;
        return this->GetAt(nIndex);
    }
    /// <summary>
    /// NOT the same as IsValidIndex()
    /// </summary>
    bool IsValidAt(ITERATE_t i) const noexcept {
         return GetAtCheck(i) != nullptr;
    }

    TYPE_PTR FindArgForKey(TYPE_KEY key1) const noexcept {
        const ITERATE_t index = this->FindIForKey(key1);
        if (index < 0) return nullptr;  // k_ITERATE_BAD
        return this->GetAt(index);
    }

    /// <summary>
    /// Find the index of a specified entry. same as FindIFor() but arg is const TYPE*.
    /// Don't use this brute force version . Use the FindIForAK instead !
    /// </summary>
    /// <param name="pData"></param>
    /// <returns>index, -1 = k_ITERATE_BAD = none.</returns>
    ITERATE_t FindIForAC_BRUTEFORCE(const TYPE_PTR pData) const {
        if (pData == nullptr) return k_ITERATE_BAD;
        const TYPE* p = this->get_PtrConst();
        for (ITERATE_t nIndex = 0; nIndex < this->GetSize(); nIndex++) {
            if (p[nIndex] == pData) return nIndex;
        }
        return k_ITERATE_BAD;
    }

    TYPE PopHead() {
        if (this->isEmpty()) return nullptr;
        return SUPER_t::PopHead();
    }
    TYPE PopTail() {
        if (this->isEmpty()) return nullptr;
        return SUPER_t::PopTail();
    }
};

/// <summary>
/// An array of pointers, sorted arbitrary. Does NOT allow dupes.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cArraySortPtr : public cArraySortFacade<TYPE*, const TYPE*, const TYPE*> {
    typedef cArraySortFacade<TYPE*, const TYPE*, const TYPE*> SUPER_t;

 protected:
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;  // TYPE_KEY

    COMPARE_t CompareKey(KEY_t key1, ARG_t pBase) const noexcept override {
        if (pBase == nullptr) return COMPARE_Greater;
        return cMem::Compare(key1, pBase, sizeof(TYPE));
    }
};

/// <summary>
/// A array of some TYPE* pointers. get_SortValue() sorted
/// @note allow duplicate get_SortValue() but NOT duplicate objects!
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="TYPE_PTR"></typeparam>
/// <typeparam name="TYPE_KEY"></typeparam>
template <class TYPE, class TYPE_PTR, typename TYPE_KEY>
class cArraySortFacadeValue : public cArraySortFacade<TYPE, TYPE_PTR, TYPE_KEY> {
    typedef cArraySortFacade<TYPE, TYPE_PTR, TYPE_KEY> SUPER_t;

 public:
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;  // TYPE_KEY

 protected:
    COMPARE_t CompareElems(ARG_t pData1, ARG_t pData2) const noexcept override {
        ASSERT_NN(pData1);
        ASSERT_NN(pData2);
        const KEY_t key1 = pData1->get_SortValue();
        const KEY_t key2 = pData2->get_SortValue();
        const COMPARE_t iDiff = cValT::Compare(key1, key2);
        if (iDiff == COMPARE_Equal) return cValT::Compare(CastPtrToNum(pData1), CastPtrToNum(pData2));  // allow duplicate get_SortValue() but NOT duplicate objects!
        return iDiff;
    }
    COMPARE_t CompareKey(KEY_t key1, ARG_t pBase) const noexcept override {
        if (pBase == nullptr) return COMPARE_Greater;
        const KEY_t key2 = pBase->get_SortValue();
        return cValT::Compare(key1, key2);
    }

 public:
    /// <summary>
    /// Equivalent of FindIFor() but uses the key for faster access. must check dupes.
    /// </summary>
    /// <param name="pBase"></param>
    /// <returns>index, -1 = k_ITERATE_BAD = none.</returns>
    ITERATE_t FindIForAK(const TYPE_PTR pBase) const {
        if (pBase == nullptr) return k_ITERATE_BAD;
        const TYPE_KEY nKey = pBase->get_SortValue();
        ITERATE_t i = this->FindIFirstForKey(nKey);
        if (i < 0) return k_ITERATE_BAD;
        for (;;) {
            if (this->GetAt(i) == pBase) return i;  // since sorted values are allowed to duplicate.
            if (++i >= this->GetSize()) break;
        }
        // This probably shouldn't happen? pBase is not in the array!
        return k_ITERATE_BAD;  // FindIForAC(pBase);	// just do a brute force search.
    }

    bool RemoveArgKey(TYPE_PTR pBase) {
        const ITERATE_t i = FindIForAK(pBase);
        if (i < 0) return false;
        SUPER_t::RemoveAt(i);
        return true;
    }
};

/// An array of pointers sorted by get_SortValue()
template <class TYPE, typename TYPE_KEY = TYPE*>
class cArraySortPtrValue : public cArraySortFacadeValue<TYPE*, TYPE*, TYPE_KEY> {};

/// <summary>
/// a _TYPE_HASH get_HashCode() sorted array of TYPE* pointers
/// does NOT allow dupe hash codes !
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="TYPE_PTR"></typeparam>
/// <typeparam name="_TYPE_HASH"></typeparam>
template <class TYPE, class TYPE_PTR, typename _TYPE_HASH = HASHCODE_t>
class cArraySortFacadeHash : public cArraySortFacade<TYPE, TYPE_PTR, _TYPE_HASH> {
    typedef cArraySortFacade<TYPE, TYPE_PTR, _TYPE_HASH> SUPER_t;

 public:
    typedef typename SUPER_t::ARG_t ARG_t;  // template weirdness.
    typedef typename SUPER_t::KEY_t KEY_t;

 protected:
    COMPARE_t CompareElems(ARG_t pData1, TYPE_PTR pData2) const noexcept override {
        const _TYPE_HASH key1 = pData1->get_HashCode();
        const _TYPE_HASH key2 = pData2->get_HashCode();
        return cValT::Compare(key1, key2);
    }
    COMPARE_t CompareKey(KEY_t key1, ARG_t pData2) const noexcept override {
        //! @note x-y will not work for extreme values so we use cValT::Compare. e.g. INT_MAX - INT_MIN must be positive !
        const _TYPE_HASH key2 = pData2->get_HashCode();
        return cValT::Compare(key1, key2);
    }
};

/// Array of pointers sorted by get_HashCode
template <class TYPE, typename TYPE_KEY = HASHCODE_t>
class cArraySortPtrHash : public cArraySortFacadeHash<TYPE*, TYPE*, TYPE_KEY> {};

/// <summary>
/// A get_Name() sorted array of some TYPE* pointers. overload this.
/// ASSUME supports get_Name()
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="_TYPECH"></typeparam>
template <class TYPE, typename _TYPECH = GChar_t>
class cArraySortPtrName : public cArraySortFacade<TYPE*, TYPE*, const _TYPECH*> {
    typedef cArraySortFacade<TYPE*, TYPE*, _TYPECH*> SUPER_t;

 public:
    typedef TYPE* ARG_t;
    typedef const _TYPECH* KEY_t;

 protected:
    COMPARE_t CompareElems(ARG_t pData1, ARG_t pData2) const noexcept override {
        ASSERT_NN(pData1);
        ASSERT_NN(pData2);
        if (pData1 == pData2) return COMPARE_Equal;  // IsEqual3 ?
        return StrT::CmpI<_TYPECH>(pData1->get_Name(), pData2->get_Name());
    }
    COMPARE_t CompareKey(KEY_t key1, ARG_t pObj) const noexcept override {
        ASSERT_NN(key1);
        ASSERT_NN(pObj);
        return StrT::CmpI<_TYPECH>(key1, pObj->get_Name());
    }

 public:
    ITERATE_t FindIForAK(ARG_t pBase) const {
        if (pBase == nullptr) return k_ITERATE_BAD;
        return SUPER_t::FindIForKey(pBase->get_Name());
    }
    bool RemoveArgKey(KEY_t pBase) {
        if (pBase == nullptr) return false;
        return SUPER_t::RemoveArgKey(pBase, pBase->get_Name());
    }
};
}  // namespace Gray
#endif  // _INC_cArraySort_H
