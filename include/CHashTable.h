//! @file cHashTable.h
//! A Hash Table with a HASHCODE_t as the key.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cHashTable_H
#define _INC_cHashTable_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"
#include "cArraySortRef.h"
#include "cBits.h"

namespace Gray {
/// <summary>
/// used to enumerate/iterate position in cHashTableT. TODO Base on cIterator
/// </summary>
struct cHashIterator {  // inline
    ITERATE_t m_b;      /// Bucket number in the hash.
    ITERATE_t m_j;      /// element inside Bucket m_b.

    cHashIterator(ITERATE_t nBucketNum = 0, ITERATE_t jj = 0) noexcept : m_b(nBucketNum), m_j(jj) {}
    /// <summary>
    /// We are iterating the hash, and we deleted something, go back.
    /// </summary>
    void SkipRemoved() noexcept {
        m_j--;
    }
    ITERATE_t get_BucketNum() const noexcept {
        //! use with GetBucketSize()
        return m_b;
    }
    bool isValid() const noexcept {
        return m_j >= 0;
    }
};

/// <summary>
/// An array of buckets. _TYPE_BUCKET
/// </summary>
template <class _TYPE_BUCKET, BIT_ENUM_t TYPE_HASHBITS = 5>
class cHashStorageT {
 public:
    static const BIT_ENUM_t k_HASHBITS = TYPE_HASHBITS;
    static const ITERATE_t k_HASH_BUCKET_QTY = cBits::Mask1<ITERATE_t>(TYPE_HASHBITS);
    typedef typename _TYPE_BUCKET::ELEM_t ELEM_t;
    typedef cHashIterator iterator;        // like STL.
    typedef cHashIterator const_iterator;  // like STL

 protected:
    _TYPE_BUCKET m_aBucket[k_HASH_BUCKET_QTY];  // array of buckets

 public:
    inline const bool IsValidBucketNum(ITERATE_t nBucketNum) const {
        return IS_INDEX_GOOD(nBucketNum, k_HASH_BUCKET_QTY);
    }
    inline const _TYPE_BUCKET& GetBucket(ITERATE_t nBucketNum) const {
        DEBUG_CHECK(IsValidBucketNum(nBucketNum));
        return m_aBucket[nBucketNum];
    }
    inline _TYPE_BUCKET& RefBucket(ITERATE_t nBucketNum) {
        DEBUG_CHECK(IsValidBucketNum(nBucketNum));
        return m_aBucket[nBucketNum];
    }
    inline ITERATE_t GetBucketSize(ITERATE_t nBucketNum) const noexcept {
        // Get the current fill level of a particular bucket.
        DEBUG_CHECK(IsValidBucketNum(nBucketNum));
        return m_aBucket[nBucketNum].GetSize();
    }

    bool IsEmpty() const noexcept {
        for (ITERATE_t nBucketNum = 0; nBucketNum < k_HASH_BUCKET_QTY; nBucketNum++) {
            if (!m_aBucket[nBucketNum].isEmpty()) return false;
        }
        return true;
    }

    ITERATE_t get_TotalCount() const noexcept {
        ITERATE_t iTotalCount = 0;
        for (ITERATE_t nBucketNum = 0; nBucketNum < k_HASH_BUCKET_QTY; nBucketNum++) {
            iTotalCount += m_aBucket[nBucketNum].GetSize();
        }
        return iTotalCount;
    }

    void RemoveAll() {
        // AKA Empty()
        for (ITERATE_t nBucketNum = 0; nBucketNum < k_HASH_BUCKET_QTY; nBucketNum++) {
            this->m_aBucket[nBucketNum].RemoveAll();
        }
    }
    void RemoveAt(cHashIterator& i) {
        ASSERT(IsValidBucketNum(i.m_b));
        m_aBucket[i.m_b].RemoveAt(i.m_j);
        i.SkipRemoved();
    }

    const ELEM_t& GetAtHash(const cHashIterator& i) const {
        //! get from hash table. i must exist.
        ASSERT(IS_INDEX_GOOD_ARRAY(i.m_b, this->m_aBucket));
        return this->m_aBucket[i.m_b].GetAt(i.m_j);
    }
};

/// <summary>
/// base/internal class for a full hash table. similar to CMap in MFC
/// @note beware TYPE_HASHBITS can make this object huge! TYPE_HASHBITS=5 = 32 buckets.
/// </summary>
/// <typeparam name="_TYPE_BUCKET"></typeparam>
/// <typeparam name="TYPE_HASHCODE"></typeparam>
template <class _TYPE_BUCKET, typename TYPE_HASHCODE = HASHCODE_t, BIT_ENUM_t TYPE_HASHBITS = 5>
struct cHashTableT : public cHashStorageT<_TYPE_BUCKET, TYPE_HASHBITS> {
    /// <summary>
    /// get the hash table bucket number for TYPE_HASHCODE rid.
    /// </summary>
    /// <param name="rid"></param>
    /// <returns></returns>
    static constexpr ITERATE_t GetBucketNum(TYPE_HASHCODE rid) noexcept {
        return CastN(ITERATE_t, rid & (k_HASH_BUCKET_QTY - 1));
    }

    cHashIterator FindIForKey(TYPE_HASHCODE rid) const {
        const ITERATE_t nBucketNum = GetBucketNum(rid);
        return cHashIterator(nBucketNum, m_aBucket[nBucketNum].FindIForKey(rid));
    }

    bool DeleteKey(TYPE_HASHCODE rid) {
        //! delete it
        return m_aBucket[GetBucketNum(rid)].RemoveKey(rid);
    }
    const ELEM_t& GetAt2(TYPE_HASHCODE rid, ITERATE_t index) const {
        return this->m_aBucket[this->GetBucketNum(rid)].GetAt(index);
    }
};

/// <summary>
/// Hash table that holds structs not references/pointers.
/// ASSUME TYPE is just a class that has a get_HashCode() method.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="TYPE_HASHCODE"></typeparam>
template <class TYPE, typename TYPE_HASHCODE = HASHCODE_t, BIT_ENUM_t TYPE_HASHBITS = 5>
class cHashTableStruct : public cHashTableT<cArraySortStructHash<TYPE, TYPE_HASHCODE>, TYPE_HASHCODE, TYPE_HASHBITS> {
 public:
    typedef const TYPE& ARG_t;  // How to refer to this? value or ref or pointer?

 public:
    const TYPE* FindArgForKey(TYPE_HASHCODE rid) const {
        const ITERATE_t nBucketNum = this->GetBucketNum(rid);
        return this->m_aBucket[nBucketNum].FindArgForKey(rid);
    }
    const TYPE& Add(ARG_t rNew) {
        const ITERATE_t nBucketNum = this->GetBucketNum(rNew.get_HashCode());
        const ITERATE_t index = this->m_aBucket[nBucketNum].Add(rNew);
        return this->m_aBucket[nBucketNum].GetAt(index);
    }
    /// <summary>
    /// Add only new hash node.
    /// </summary>
    /// <param name="rNew"></param>
    /// <returns>pointer ONLY if existing hash node. nullptr = it was new.</returns>
    TYPE* AddSpecial(ARG_t rNew) {
        const ITERATE_t nBucketNum = this->GetBucketNum(rNew.get_HashCode());
        COMPARE_t iCompareRes;
        ITERATE_t index = this->m_aBucket[nBucketNum].FindINear(rNew, OUT iCompareRes);
        if (iCompareRes == COMPARE_Equal) {
            // duplicated.
            return &this->m_aBucket[nBucketNum].ElementAt(index);  // special return that says it already was here.
        }
        // not duplicate. must add
        index = this->m_aBucket[nBucketNum].AddPresorted(index, iCompareRes, rNew);
        return nullptr;  // special return that says i added it.
    }
};

/// <summary>
/// Hash table that holds refs. ASSUME TYPE is cRefBase and implements get_HashCode().
/// Must have external lock to make thread safe.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="TYPE_HASHCODE"></typeparam>
template <class TYPE, typename TYPE_HASHCODE = HASHCODE_t, BIT_ENUM_t TYPE_HASHBITS = 5>
class cHashTableRef : public cHashTableT<cArraySortHash<TYPE, TYPE_HASHCODE>, TYPE_HASHCODE, TYPE_HASHBITS> {
 protected:
    typedef cRefPtr<TYPE> REF_t;

 public:
    const TYPE* FindArgForKey(TYPE_HASHCODE rid) const {
        const ITERATE_t nBucketNum = this->GetBucketNum(rid);
        return this->m_aBucket[nBucketNum].FindArgForKey(rid);
    }
    ITERATE_t Add(TYPE* pNew) {
        ASSERT(pNew != nullptr);
        return this->m_aBucket[this->GetBucketNum(pNew->get_HashCode())].Add(pNew);
    }
    bool DeleteArg(TYPE* pObj) {
        if (pObj == nullptr) return false;
        return this->m_aBucket[this->GetBucketNum(pObj->get_HashCode())].RemoveArgKey(pObj);
    }

    /// <summary>
    /// Like RemoveAll() but Dispose. Called at destructor time?
    /// ASSUME TYPE supports DisposeThis(); like cXObject
    /// </summary>
    void DisposeAll() {
        for (ITERATE_t nBucketNum = 0; nBucketNum < this->k_HASH_BUCKET_QTY; nBucketNum++) {
            this->m_aBucket[nBucketNum].DisposeAll();
        }
    }
};

/// <summary>
/// Hash table that holds refs ordered by alpha. Typical Map. ASSUME TYPE is cRefBase and implements get_Name()
/// </summary>
/// <typeparam name="TYPE">cRefBase that supports get_Name()</typeparam>
/// <typeparam name="TYPE_HASHBITS"></typeparam>
template <class TYPE, BIT_ENUM_t TYPE_HASHBITS = 4>
class cHashTableName : public cHashStorageT<cArraySortName<TYPE, char>, TYPE_HASHBITS> {
 protected:
    typedef cRefPtr<TYPE> REF_t;
    typedef const char* KEY_t;

 public:
    inline ITERATE_t GetBucketNum(KEY_t pszName) const noexcept {
        return CastN(ITERATE_t, pszName[0] & (k_HASH_BUCKET_QTY - 1));
    }

    REF_t FindArgForKey(KEY_t pszName) const {
        const ITERATE_t nBucketNum = this->GetBucketNum(pszName);
        return this->m_aBucket[nBucketNum].FindArgForKey(pszName);
    }

    cHashIterator FindINearKey(KEY_t pszName, OUT COMPARE_t& iCompareRes) const {
        const ITERATE_t nBucketNum = GetBucketNum(pszName);
        return cHashIterator(nBucketNum, m_aBucket[nBucketNum].FindINearKey(pszName, iCompareRes));
    }

    ITERATE_t InsertAt(const cHashIterator& index, COMPARE_t iCompareRes, TYPE* pNew) {
        ASSERT(pNew != nullptr);
        const ITERATE_t nBucketNum = GetBucketNum(pNew->get_Name());
        return m_aBucket[nBucketNum].AddPresorted(index.m_j, iCompareRes, pNew);
    }
    ITERATE_t Add(TYPE* pNew) {
        ASSERT(pNew != nullptr);
        return this->m_aBucket[this->GetBucketNum(pNew->get_Name())].Add(pNew);
    }

    bool DeleteArg(TYPE* pObj) {
        if (pObj == nullptr) return false;
        return this->m_aBucket[this->GetBucketNum(pObj->get_Name())].RemoveArgKey(pObj);
    }
    bool isHashSorted() const {
        // No Dupes.
        for (ITERATE_t nBucketNum = 0; nBucketNum < k_HASH_BUCKET_QTY; nBucketNum++) {
            if (!this->m_aBucket[nBucketNum].isSpanSortedND()) return false;
        }
        return true;
    }
};

// Iterate through all members. iterator i; 	// similar to BOOST_FOREACH()
#define FOREACH_HASH_TABLE(h, i)                                \
    for (cHashIterator i; i.m_b < h.k_HASH_BUCKET_QTY; i.m_b++) \
        for (i.m_j = 0; i.m_j < h.GetBucketSize(i.m_b); i.m_j++)
}  // namespace Gray

#endif  // _INC_CHASH_H
