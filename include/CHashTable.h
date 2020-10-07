//
//! @file CHashTable.h
//! A Hash Table with a HASHCODE_t as the key.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CHashTable_H
#define _INC_CHashTable_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CArraySort.h"
#include "CBits.h"
#include "CUnitTestDecl.h"

UNITTEST_PREDEF(CHashTableT)

namespace Gray
{
	class CHashIterator	// inline
	{
		//! @class Gray::CHashIterator
		//! used to enumerate/iterate position in CHashTableT
	public:
		ITERATE_t m_i;	//!< array/Bucket number in the hash.
		ITERATE_t m_j;	//!< element inside a array/Bucket.
	public:
		CHashIterator(ITERATE_t nBucket = 0, ITERATE_t jj = 0)
		: m_i(nBucket)
		, m_j(jj)
		{
		}
		void SkipRemoved()
		{
			//! We are iterating the hash, and we deleted something.
			m_j--;
		}
		ITERATE_t get_ArrayNum() const
		{
			//! use with GetArraySize()
			return m_i;
		}
		bool isValid() const
		{
			return(m_j >= 0);
		}
	};

	template<class _TYPEARRAY, class TYPE, typename TYPE_HASHCODE = HASHCODE_t, int TYPE_HASHBITS = 5 >
	class CHashTableT
	{
		//! @class Gray::CHashTableT
		//! base class for a full hash table. similar to CMap in MFC
		//! @note beware TYPE_HASHBITS can make this object huge! TYPE_HASHBITS=5 = 32 buckets.

	public:
		static const ITERATE_t k_HASH_ARRAY_QTY = _1BITMASK(TYPE_HASHBITS);
		typedef CHashIterator iterator;	// like STL.
		_TYPEARRAY m_aTable[k_HASH_ARRAY_QTY];

	public:
		int get_HashBits() const
		{
			return TYPE_HASHBITS;
		}
		ITERATE_t get_HashArrayQty() const
		{
			return k_HASH_ARRAY_QTY;
		}
		ITERATE_t GetHashArray(TYPE_HASHCODE rid) const
		{
			//! @return the hash table array/bucket number for TYPE_HASHCODE rid.
			return((ITERATE_t)(rid & (k_HASH_ARRAY_QTY - 1)));
		}
		ITERATE_t GetArraySize(ITERATE_t iArray) const
		{
			ASSERT(IS_INDEX_GOOD(iArray, k_HASH_ARRAY_QTY));
			return(m_aTable[iArray].GetSize());
		}
		iterator FindIForKey(TYPE_HASHCODE rid) const
		{
			ITERATE_t iBucket = GetHashArray(rid);
			return(CHashIterator(iBucket, m_aTable[iBucket].FindIForKey(rid)));
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
		bool IsEmpty() const
		{
			for (ITERATE_t i = 0; i < k_HASH_ARRAY_QTY; i++)
			{
				if (!m_aTable[i].IsEmpty())
					return false;
			}
			return true;
		}
		ITERATE_t get_TotalCount() const
		{
			ITERATE_t iTotalCount = 0;
			for (ITERATE_t i = 0; i < k_HASH_ARRAY_QTY; i++)
			{
				iTotalCount += m_aTable[i].GetSize();
			}
			return iTotalCount;
		}
		void RemoveAt(iterator& i)
		{
			ASSERT(IS_INDEX_GOOD(i.m_i, k_HASH_ARRAY_QTY));
			m_aTable[i.m_i].RemoveAt(i.m_j);
			i.SkipRemoved();
		}

		void RemoveAll()
		{
			for (ITERATE_t i = 0; i < k_HASH_ARRAY_QTY; i++)
			{
				this->m_aTable[i].RemoveAll();
			}
		}

		void Empty()
		{
			RemoveAll();
		}

		UNITTEST_FRIEND(CHashTableT);
	};

	template<class TYPE, typename TYPE_HASHCODE = HASHCODE_t, int TYPE_HASHBITS = 5 >
	class CHashTableStruct : public CHashTableT < CArraySortStructHash<TYPE, TYPE_HASHCODE>, TYPE, TYPE_HASHCODE, TYPE_HASHBITS >
	{
		//! @class Gray::CHashTableStruct
		//! ASSUME TYPE is just a class that has a get_HashCode() method.
	public:
		typedef CHashTableT< CArraySortStructHash<TYPE, TYPE_HASHCODE>, TYPE, TYPE_HASHCODE, TYPE_HASHBITS > SUPER_t;
		typedef const TYPE& REF_t;		// How to refer to this? value or ref or pointer?

	public:
		const TYPE& FindArgForKey(TYPE_HASHCODE rid) const
		{
			ITERATE_t iBucket = this->GetHashArray(rid);
			return this->m_aTable[iBucket].FindArgForKey(rid);
		}
		const TYPE& GetAtHash(const CHashIterator& i) const
		{
			//! Walk hash table.
			ASSERT(IS_INDEX_GOOD_ARRAY(i.m_i, this->m_aTable));
			return(this->m_aTable[i.m_i].ConstElementAt(i.m_j));
		}
		ITERATE_t Add(REF_t rNew)
		{
			return(this->m_aTable[this->GetHashArray(rNew.get_HashCode())].Add(rNew));
		}
		CHashIterator AddNew(REF_t rNew)
		{
			// Add only new hash node. return index if existing hash node.
			ITERATE_t iBucket = this->GetHashArray(rNew.get_HashCode());
			COMPARE_t iCompareRes;
			ITERATE_t index = this->m_aTable[iBucket].FindINear(rNew, iCompareRes);
			if (iCompareRes == COMPARE_Equal)
			{
				// duplicates
				return(CHashIterator(iBucket, index));
			}
			this->m_aTable[iBucket].AddPresorted(index, iCompareRes, rNew);
			return CHashIterator(iBucket, k_ITERATE_BAD);
		}
	};

	template<class TYPE, typename TYPE_HASHCODE = HASHCODE_t, int TYPE_HASHBITS = 5 >
	class CHashTableSmart : public CHashTableT < CArraySortHash<TYPE, TYPE_HASHCODE>, TYPE, TYPE_HASHCODE, TYPE_HASHBITS >
	{
		//! @class Gray::CHashTableSmart
		//! ASSUME TYPE is CSmartBase and implements get_HashCode()
	protected:
		typedef CSmartPtr<TYPE> PTR_t;
	public:
		PTR_t FindArgForKey(TYPE_HASHCODE rid) const
		{
			ITERATE_t i = this->GetHashArray(rid);
			return this->m_aTable[i].FindArgForKey(rid);
		}
		ITERATE_t Add(TYPE* pNew)
		{
			ASSERT(pNew != nullptr);
			return(this->m_aTable[this->GetHashArray(pNew->get_HashCode())].Add(pNew));
		}
		bool DeleteArg(TYPE* pObj)
		{
			if (pObj == nullptr)
				return false;
			return this->m_aTable[this->GetHashArray(pObj->get_HashCode())].RemoveArgKey(pObj);
		}
		PTR_t GetAtHash(const CHashIterator& i) const
		{
			//! Walk hash table.
			ASSERT(IS_INDEX_GOOD_ARRAY(i.m_i, this->m_aTable));
			return(this->m_aTable[i.m_i].ConstElementAt(i.m_j));
		}

		PTR_t GetAt(TYPE_HASHCODE rid, ITERATE_t index) const
		{
			return(this->m_aTable[this->GetHashArray(rid)].ConstElementAt(index));
		}
		void DisposeAll()
		{
			//! ASSUME TYPE supports DisposeThis(); like CXObject
			for (ITERATE_t i = 0; i < this->k_HASH_ARRAY_QTY; i++)
			{
				this->m_aTable[i].DisposeAll();
			}
		}
	};

	// Iterate through all members. iterator i;
#define FOR_HASH_TABLE(h,i)	for ( ; i.m_i<h.k_HASH_ARRAY_QTY; i.m_i++ ) for ( i.m_j=0; i.m_j<h.m_aTable[i.m_i].GetSize(); i.m_j++ )
};

#endif // _INC_CHASH_H