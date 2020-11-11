//! @file cArraySortRef.h
//! c++ sorted collections.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cArraySortRef_H
#define _INC_cArraySortRef_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArraySort.h"
#include "cArrayRef.h"
 
namespace Gray
{
	template<class TYPE, typename TYPE_KEY>
	class cArraySortRef : public cArraySortFacade < cRefPtr<TYPE>, TYPE*, TYPE_KEY >
	{
		//! @class Gray::cArraySortRef
		//! A sorted array of cRefPtr<TYPE> objects.
		//! the array has a reference to the element. similar to cArrayRef but sorted
		//! It will get deleted when the reference count is 0.
		//! default sort by memcmp() pointers.

		typedef cArraySortFacade< cRefPtr<TYPE>, TYPE*, TYPE_KEY > SUPER_t;

	public:

		void DisposeAll()
		{
			//! Similar to RemoveAll() except it calls DisposeThis() to try to dereference all the entries.
			//! @note often DisposeThis() has the effect of removing itself from the list. We protect against this.
			//! ASSUME TYPE supports DisposeThis(); like CXObject

			ITERATE_t iSize = this->GetSize();
			if (iSize > 0)
			{
				{	// save original list, call DisposeThis on everything from original list
					cArrayRef<TYPE> orig;
					orig.SetCopy(*this);

					ASSERT(orig.GetSize() == iSize);
					for (ITERATE_t i = 0; i < iSize; i++)
					{
						TYPE* pObj = orig.GetAt(i);
						if (pObj != nullptr)
							pObj->DisposeThis();
					}
				}
				this->RemoveAll();
			}
		}
	};

	//*************************************************

	template <class TYPE, typename _TYPE_HASH = HASHCODE_t>
	class cArraySortHash : public cArraySortRef < TYPE, _TYPE_HASH >
	{
		//! @class Gray::cArraySortHash
		//! A _TYPE_HASH get_HashCode() sorted array of cRefPtr<TYPE>
		//! TYPE based on IScriptableObj typically
		//! does NOT allow dupe hash codes !

	public:
		typedef cArraySortRef<TYPE, _TYPE_HASH> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::KEY_t KEY_t;

	protected:
		virtual COMPARE_t CompareData(REF_t pData1, REF_t pData2) const override
		{
			//! Compare a data record to another data record.
			ASSERT_N(pData1 != nullptr);
			ASSERT_N(pData2 != nullptr);
			KEY_t key1 = pData1->get_HashCode();
			KEY_t key2 = pData2->get_HashCode();
			return cValT::Compare(key1, key2);
		}
		virtual COMPARE_t CompareKey(KEY_t key1, REF_t pBase) const override
		{
			//! INT_MAX - INT_MIN must be positive !
			//! @note x-y will not work for extreme values so we use cValT::Compare
			ASSERT_N(pBase != nullptr);
			KEY_t key2 = pBase->get_HashCode();
			return cValT::Compare(key1, key2);
		}
	public:
		virtual ~cArraySortHash()
		{}

		ITERATE_t FindIForAK(const TYPE* pBase) const
		{
			//! Like of FindIFor() but uses the key.
			//! @return index, -1 = k_ITERATE_BAD = none.
			if (pBase == nullptr)
				return k_ITERATE_BAD;
			return this->FindIForKey(pBase->get_HashCode());
		}
		bool RemoveArgKey(TYPE* pBase)
		{
			if (pBase == nullptr)
				return false;
			return SUPER_t::RemoveArgKey(pBase, pBase->get_HashCode());
		}
	};

	//*************************************************

	template <class TYPE, typename TYPE_KEY = int>
	class cArraySortValue : public cArraySortRef < TYPE, TYPE_KEY >
	{
		//! @class Gray::cArraySortValue
		//! A TYPE_KEY get_SortValue() sorted array of cRefPtr<TYPE>. sort low to high
		//! TYPE based on cRefBase
		//! TYPE based on IScriptableObj typically
		//! @note allow duplicate get_SortValue() but NOT duplicate objects!
		//! Similar to HashCode but different in the it can be any type. (float,etc)

	public:
		typedef cArraySortRef<TYPE, TYPE_KEY> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::KEY_t KEY_t;

	protected:
		virtual COMPARE_t CompareData(REF_t pData1, REF_t pData2) const override
		{
			//! Compare a data record to another data record.
			ASSERT(pData1 != nullptr);
			ASSERT(pData2 != nullptr);
			TYPE_KEY key1 = pData1->get_SortValue();
			TYPE_KEY key2 = pData2->get_SortValue();
			COMPARE_t iDiff = cValT::Compare(key1, key2);
			if (iDiff == COMPARE_Equal)	// allow duplicate get_SortValue() but NOT duplicate objects!
				return cValT::Compare((INT_PTR)pData1, (INT_PTR)pData2);
			return iDiff;
		}
		virtual COMPARE_t CompareKey(KEY_t key1, REF_t pBase) const override
		{
			if (pBase == nullptr)
				return COMPARE_Greater;
			TYPE_KEY key2 = pBase->get_SortValue();
			return cValT::Compare(key1, key2);
		}
	public:
		virtual ~cArraySortValue()
		{}

		ITERATE_t FindIForAK(const TYPE* pBase) const
		{
			//! Equivalent of FindIFor() but uses the key for faster access. must ignore key dupes.
			//! @return index, -1 = k_ITERATE_BAD = none.
			if (pBase == nullptr)
				return k_ITERATE_BAD;
			TYPE_KEY nKey = pBase->get_SortValue();
			ITERATE_t i = this->FindIFirstForKey(nKey);
			if (i < 0)
				return k_ITERATE_BAD;
			for (;; i++)
			{
				cRefPtr<TYPE> pBase2 = this->GetAtCheck(i);
				if (pBase2 == nullptr)	// pBase is not in the array!
					break;
				if (pBase2 == pBase) // since sorted values are allowed to duplicate.
					return i;
			}
			// This probably shouldn't happen? pBase is not in the array!
			return k_ITERATE_BAD;	// FindIForAC(pBase);	// just do a brute force search.
		}
		bool RemoveArgKey(TYPE* pBase)
		{
			ITERATE_t index = this->FindIForAK(pBase);
			if (index < 0)
				return false;
			SUPER_t::RemoveAt(index);
			return true;
		}
		ITERATE_t AddAfter(TYPE* pBase)
		{
			//! Add this last after any duplicate keys.
			ASSERT(pBase != nullptr);
			TYPE_KEY nKey = pBase->get_SortValue();
			ITERATE_t i = this->FindILastForKey(nKey);
			if (i < 0)	// one of the same type is here?
			{
				return this->Add(pBase);	// add new sorted by nKey.
			}
			// add to the end of the get_SortValue series.
			this->InsertAt(++i, pBase);
			return i;
		}
	};

	//*************************************************

	template <class TYPE, typename _TYPECH = GChar_t>
	class cArraySortName : public cArraySortRef < TYPE, const _TYPECH* >
	{
		//! @class Gray::cArraySortName
		//! get_Name() sorted array of cRefPtr<TYPE>.
		//! TYPE must support get_Name() and be cRefBase
		//! does  NOT allow dupe names !

	public:
		typedef cArraySortRef<TYPE, const _TYPECH*> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::KEY_t KEY_t;

	protected:
		virtual COMPARE_t CompareData(REF_t pData1, REF_t pData2) const override
		{
			//! Compare a data record to another data record.
			ASSERT_N(pData1 != nullptr);
			ASSERT_N(pData2 != nullptr);
			return StrT::CmpI<_TYPECH>(pData1->get_Name(), pData2->get_Name());
		}
		virtual COMPARE_t CompareKey(KEY_t key1, REF_t pObj) const override
		{
			ASSERT_N(key1 != nullptr);
			ASSERT_N(pObj != nullptr);
			return StrT::CmpI<_TYPECH>(key1, pObj->get_Name());
		}
	public:
		virtual ~cArraySortName()
		{}

		ITERATE_t FindIForAK(const TYPE* pBase) const
		{
			//! Equivalent of FindIFor() but uses the key.
			//! FindIForKey using the key.
			//! @return index, -1 = k_ITERATE_BAD = none.
			if (pBase == nullptr)
				return k_ITERATE_BAD;
			return this->FindIForKey(pBase->get_Name());
		}
		bool RemoveArgKey(TYPE* pBase)
		{
			if (pBase == nullptr)
				return false;
			return SUPER_t::RemoveArgKey(pBase, pBase->get_Name());
		}
	};
}
#endif // _INC_cArraySortRef_H
