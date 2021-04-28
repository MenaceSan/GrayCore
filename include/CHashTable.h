//
//! @file cHashTable.h
//! A Hash Table with a HASHCODE_t as the key.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cHashTable_H
#define _INC_cHashTable_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArraySortRef.h"
#include "cBits.h"

namespace Gray
{
	class cHashIterator	// inline
	{
		//! @class Gray::cHashIterator
		//! used to enumerate/iterate position in cHashTableT
	public:
		ITERATE_t m_i;	//!< array/Bucket number in the hash.
		ITERATE_t m_j;	//!< element inside a array/Bucket.
	public:
		cHashIterator(ITERATE_t nBucket = 0, ITERATE_t jj = 0) noexcept
			: m_i(nBucket)
			, m_j(jj)
		{
		}
		void SkipRemoved() noexcept
		{
			//! We are iterating the hash, and we deleted something.
			m_j--;
		}
		ITERATE_t get_ArrayNum() const noexcept
		{
			//! use with GetArraySize()
			return m_i;
		}
		bool isValid() const noexcept
		{
			return m_j >= 0;
		}
	};

	template<class _TYPEARRAY, typename TYPE_HASHCODE = HASHCODE_t, int TYPE_HASHBITS = 5 >
	class cHashTableT
	{
		//! @class Gray::cHashTableT
		//! base class for a full hash table. similar to CMap in MFC
		//! @note beware TYPE_HASHBITS can make this object huge! TYPE_HASHBITS=5 = 32 buckets.

	public:
		static const int k_HASHBITS = TYPE_HASHBITS;	// BIT_ENUM_t
		static const ITERATE_t k_HASH_ARRAY_QTY = _1BITMASK(TYPE_HASHBITS);
		typedef cHashIterator iterator;	// like STL.

	protected:
		_TYPEARRAY m_aTable[k_HASH_ARRAY_QTY];

	public:
		inline ITERATE_t GetHashArray(TYPE_HASHCODE rid) const noexcept
		{
			//! @return the hash table array/bucket number for TYPE_HASHCODE rid.
			return (ITERATE_t)(rid & (k_HASH_ARRAY_QTY - 1));
		}

		inline const _TYPEARRAY& GetArray(ITERATE_t iArray) const
		{
			DEBUG_CHECK(IS_INDEX_GOOD(iArray, k_HASH_ARRAY_QTY));
			return m_aTable[iArray];
		}
		inline _TYPEARRAY& RefArray(ITERATE_t iArray)
		{
			DEBUG_CHECK(IS_INDEX_GOOD(iArray, k_HASH_ARRAY_QTY));
			return m_aTable[iArray];
		}
		inline ITERATE_t GetArraySize(ITERATE_t iArray) const noexcept
		{
			DEBUG_CHECK(IS_INDEX_GOOD(iArray, k_HASH_ARRAY_QTY));
			return m_aTable[iArray].GetSize();
		}

		bool IsEmpty() const noexcept
		{
			for (ITERATE_t i = 0; i < k_HASH_ARRAY_QTY; i++)
			{
				if (!m_aTable[i].IsEmpty())
					return false;
			}
			return true;
		}
		ITERATE_t get_TotalCount() const noexcept
		{
			ITERATE_t iTotalCount = 0;
			for (ITERATE_t i = 0; i < k_HASH_ARRAY_QTY; i++)
			{
				iTotalCount += m_aTable[i].GetSize();
			}
			return iTotalCount;
		}

		void RemoveAll()
		{
			// AKA Empty()
			for (ITERATE_t i = 0; i < k_HASH_ARRAY_QTY; i++)
			{
				this->m_aTable[i].RemoveAll();
			}
		}

		iterator FindIForKey(TYPE_HASHCODE rid) const
		{
			ITERATE_t iBucket = GetHashArray(rid);
			return cHashIterator(iBucket, m_aTable[iBucket].FindIForKey(rid));
		}
		TYPE_HASHCODE FindKeyFree(TYPE_HASHCODE rid) const
		{
			//! Find the next free/unused TYPE_HASHCODE key after rid.
			while (FindIForKey(rid).isValid())	// found it ?
			{
				rid++;
			}
			return rid;
		}
		bool DeleteKey(TYPE_HASHCODE rid)
		{
			//! delete it
			return m_aTable[GetHashArray(rid)].RemoveKey(rid);
		}

		void RemoveAt(iterator& i)
		{
			ASSERT(IS_INDEX_GOOD(i.m_i, k_HASH_ARRAY_QTY));
			m_aTable[i.m_i].RemoveAt(i.m_j);
			i.SkipRemoved();
		}
	};

	template<class TYPE, typename TYPE_HASHCODE = HASHCODE_t, int TYPE_HASHBITS = 5 >
	class cHashTableStruct : public cHashTableT < cArraySortStructHash<TYPE, TYPE_HASHCODE>, TYPE_HASHCODE, TYPE_HASHBITS >
	{
		//! @class Gray::cHashTableStruct
		//! ASSUME TYPE is just a class that has a get_HashCode() method.
	public:
		typedef cHashTableT< cArraySortStructHash<TYPE, TYPE_HASHCODE>, TYPE_HASHCODE, TYPE_HASHBITS > SUPER_t;
		typedef const TYPE& ARG_t;		// How to refer to this? value or ref or pointer?

	public:
		const TYPE* FindArgForKey(TYPE_HASHCODE rid) const
		{
			ITERATE_t iBucket = this->GetHashArray(rid);
			return this->m_aTable[iBucket].FindArgForKey(rid);
		}
		const TYPE& GetAtHash(const cHashIterator& i) const
		{
			//! get from hash table. i must exist.
			ASSERT(IS_INDEX_GOOD_ARRAY(i.m_i, this->m_aTable));
			return this->m_aTable[i.m_i].GetAt(i.m_j);
		}
		cHashIterator FindHash(TYPE_HASHCODE rid) const
		{
			ITERATE_t iBucket = this->GetHashArray(rid);
			ITERATE_t index = this->m_aTable[iBucket].FindArgForKey(rid);
			return cHashIterator(iBucket, index);
		}
		const TYPE& Add(ARG_t rNew)
		{
			ITERATE_t iBucket = this->GetHashArray(rNew.get_HashCode());
			ITERATE_t index = this->m_aTable[iBucket].Add(rNew);
			return this->m_aTable[iBucket].GetAt(index);
		}
		TYPE* AddSpecial(ARG_t rNew)
		{
			// Add only new hash node. return index ONLY if existing hash node.
			ITERATE_t iBucket = this->GetHashArray(rNew.get_HashCode());
			COMPARE_t iCompareRes;
			ITERATE_t index = this->m_aTable[iBucket].FindINear(rNew, iCompareRes);
			if (iCompareRes == COMPARE_Equal)
			{
				return &this->m_aTable[iBucket].ElementAt(index);	// special return that says it already was here.
			}

			// not duplicate. must add
			index = this->m_aTable[iBucket].AddPresorted(index, iCompareRes, rNew);
			return nullptr;	// special return that says i added it.
		}
	};

	template<class TYPE, typename TYPE_HASHCODE = HASHCODE_t, int TYPE_HASHBITS = 5 >
	class cHashTableRef : public cHashTableT < cArraySortHash<TYPE, TYPE_HASHCODE>, TYPE_HASHCODE, TYPE_HASHBITS >
	{
		//! @class Gray::cHashTableRef
		//! ASSUME TYPE is cRefBase and implements get_HashCode()
	protected:
		typedef cRefPtr<TYPE> PTR_t;
	public:
		PTR_t FindArgForKey(TYPE_HASHCODE rid) const
		{
			ITERATE_t i = this->GetHashArray(rid);
			return this->m_aTable[i].FindArgForKey(rid);
		}
		ITERATE_t Add(TYPE* pNew)
		{
			ASSERT(pNew != nullptr);
			return this->m_aTable[this->GetHashArray(pNew->get_HashCode())].Add(pNew);
		}
		bool DeleteArg(TYPE* pObj)
		{
			if (pObj == nullptr)
				return false;
			return this->m_aTable[this->GetHashArray(pObj->get_HashCode())].RemoveArgKey(pObj);
		}
		PTR_t GetAtHash(const cHashIterator& i) const
		{
			//! Walk hash table.
			ASSERT(IS_INDEX_GOOD_ARRAY(i.m_i, this->m_aTable));
			return this->m_aTable[i.m_i].GetAt(i.m_j);
		}

		PTR_t GetAt(TYPE_HASHCODE rid, ITERATE_t index) const
		{
			return this->m_aTable[this->GetHashArray(rid)].GetAt(index);
		}
		void DisposeAll()
		{
			//! ASSUME TYPE supports DisposeThis(); like cXObject
			for (ITERATE_t i = 0; i < this->k_HASH_ARRAY_QTY; i++)
			{
				this->m_aTable[i].DisposeAll();
			}
		}
	};

	// Iterate through all members. iterator i;
#define FOR_HASH_TABLE(h,i)	for ( ; i.m_i<h.k_HASH_ARRAY_QTY; i.m_i++ ) for ( i.m_j=0; i.m_j<h.GetArraySize(i.m_i); i.m_j++ )
}

#endif // _INC_CHASH_H
