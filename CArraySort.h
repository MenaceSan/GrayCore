//
//! @file CArraySort.h
//! c++ sorted collections.
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CArraySort_H
#define _INC_CArraySort_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CArraySmart.h"
#include "StrT.h"
#include "CValT.h"

UNITTEST_PREDEF(CArraySort)

namespace Gray
{
	template<class TYPE, class TYPE_ARG, typename TYPE_KEY>
	class CArraySorted : public CArrayTyped < TYPE, TYPE_ARG >
	{
		//! @class Gray::CArraySorted
		//! An array of some sorted TYPE. duplicates are destroyed.
		//! default is that it is just sorted by its bytes.
		//! Similar to .NET HashSet

	public:
		typedef CArrayTyped<TYPE, TYPE_ARG> SUPER_t;
		typedef TYPE_KEY KEY_t;
		typedef typename SUPER_t::REF_t REF_t;

	protected:
		virtual COMPARE_t CompareKey(KEY_t key1, REF_t Data2) const
		{
			//! Compare by a key that may not be part of a data record (yet).
			//! Default implementation. Overload this for proper implementation.
			//! @note If we reach here assume the key is &reference to the whole record !
			return CMem::Compare(&key1, &Data2, sizeof(TYPE_KEY));
		}

		bool RemoveArgKey(REF_t pObj, KEY_t key)
		{
			//! Can't use this for arrays that allow dupes ! (e.g. get_Value()) Use FindIForAK() instead.
			ITERATE_t index = FindIForKey(key);
			if (index <= k_ITERATE_BAD)
				return false;
			TYPE_ARG pObjOld = this->GetAt(index);
			if (!(pObjOld == pObj))	// not the current one !! weird!
			{
				ASSERT(0);
				return false;
			}
			this->RemoveAt(index);
			return true;
		}

	public:
		virtual ~CArraySorted()
		{
			// Make this virtual to allow derived classes to override this and make destructors work.
		}

		ITERATE_t FindINear(REF_t pNew, COMPARE_t& iCompareRes) const;
		ITERATE_t FindINearKey(KEY_t key, COMPARE_t& iCompareRes) const;
		ITERATE_t FindIForKey(KEY_t key) const
		{
			//! Find index for exact key match. Similar to FindIFor()
			//! @return index into array. 0 based of course. -1 = failed
			COMPARE_t iCompareRes;
			ITERATE_t index = FindINearKey(key, iCompareRes);
			if (iCompareRes != COMPARE_Equal)
				return k_ITERATE_BAD;
			return index;
		}

		ITERATE_t FindIFirstForKey(TYPE_KEY nKey) const
		{
			//! Find first the occurrence of this nKey. Since values are allowed to duplicate.
			//! @return index, -1 = k_ITERATE_BAD = none.
			ITERATE_t i = this->FindIForKey(nKey);
			if (i < 0)
				return k_ITERATE_BAD;
			// Walk Back to get First.
			for (;;)
			{
				if (--i < 0)
					break;
				if (CompareKey(nKey, this->GetAt(i)) != COMPARE_Equal)
					break;
			}
			return(i + 1);
		}
		ITERATE_t FindILastForKey(TYPE_KEY nKey) const
		{
			//! Find last the occurrence of this nKey. Since values are allowed to duplicate.
			//! @return index, -1 = k_ITERATE_BAD = none.
			ITERATE_t i = this->FindIForKey(nKey);
			if (i < 0)
				return k_ITERATE_BAD;
			// Walk Forward to get Last.
			for (;;)
			{
				if (++i >= this->GetSize())
					break;
				if (CompareKey(nKey, this->GetAt(i)) != COMPARE_Equal)
					break;
			}
			return(i - 1);	// last
		}

		ITERATE_t AddPresorted(ITERATE_t index, COMPARE_t iCompareRes, TYPE_ARG pNew)
		{
			//! @return index in the array. (temporary if sorted)
			if (iCompareRes > 0) // key is greater than existing element at index. so put it after.
			{
				index++;
			}
			this->InsertAt(index, pNew);
			return index;
		}
		bool RemoveKey(TYPE_KEY key)
		{
			//! Might be dangerous for arrays that allow dupes?
			ITERATE_t index = FindIForKey(key);
			if (index <= k_ITERATE_BAD)
				return false;
			this->RemoveAt(index);
			return true;
		}

		ITERATE_t Add(TYPE_ARG pNew);

		void AddArray(const SUPER_t& a)
		{
			//! Add all the entries in array a to this array. sorted add.
			for (ITERATE_t i = 0; i < a.GetSize(); i++)
			{
				Add(a[i]);
			}
		}
	};

	template<class TYPE, class TYPE_ARG, typename TYPE_KEY>
	ITERATE_t CArraySorted<TYPE, TYPE_ARG, TYPE_KEY>::FindINear(REF_t pNew, COMPARE_t& riCompareRes) const
	{
		//! Do a binary search for the elements key.
		//! @return index
		//!  riCompareRes = COMPARE_t
		//!		 0 = match with index. we may allow duplicates?
		//!		-1 = key is less than index. COMPARE_Less
		//!		+1 = key is greater than index

		ITERATE_t iHigh = this->GetSize() - 1;
		if (iHigh < 0)
		{
			riCompareRes = COMPARE_Less;
			return 0;
		}

		ITERATE_t iLow = 0;
		ITERATE_t i = 0;
		COMPARE_t iCompareRes = COMPARE_Less;
		while (iLow <= iHigh)
		{
			i = (iHigh + iLow) / 2;
			iCompareRes = this->CompareData(pNew, this->ConstElementAt(i));	// virtual call.
			if (iCompareRes == COMPARE_Equal)
				break;
			if (iCompareRes > 0)
			{
				iLow = i + 1;
			}
			else
			{
				iHigh = i - 1;
			}
		}
		riCompareRes = iCompareRes;
		return i;
	}

	template<class TYPE, class TYPE_ARG, typename TYPE_KEY>
	ITERATE_t CArraySorted<TYPE, TYPE_ARG, TYPE_KEY>::Add(TYPE_ARG pNew)
	{
		//! Insertion sort. duplicates are destroyed.
		//! @return index in the array. (temporary if sorted)
		COMPARE_t iCompareRes;
		ITERATE_t index = FindINear(pNew, iCompareRes);
		if (iCompareRes == COMPARE_Equal)
		{
			// duplicates don't normally happen, but just replace the old one just in case.
			// DestructElements is called automatically for previous.
			this->SetAt(index, pNew);
			return(index);
		}
		return AddPresorted(index, iCompareRes, pNew);
	}

	template<class TYPE, class TYPE_ARG, typename TYPE_KEY>
	ITERATE_t CArraySorted<TYPE, TYPE_ARG, TYPE_KEY>::FindINearKey(KEY_t key, COMPARE_t& riCompareRes) const
	{
		//! Do a binary search for the key.
		//! @return
		//! @arg index
		//! @arg iCompareRes =
		//!		0 = key match with element at the index.
		//!		-1 = key is less than element at the index. COMPARE_Less
		//!		+1 = key is greater than element at the index

		ITERATE_t iHigh = this->GetSize() - 1;
		if (iHigh < 0)
		{
			riCompareRes = COMPARE_Less;
			return 0; 
		}

		ITERATE_t iLow = 0;
		ITERATE_t i = 0;
		COMPARE_t iCompareRes = COMPARE_Less;
		while (iLow <= iHigh)
		{
			i = (iHigh + iLow) / 2;
			iCompareRes = CompareKey(key, this->ConstElementAt(i));
			if (iCompareRes == COMPARE_Equal)
				break;
			if (iCompareRes > 0)
			{
				iLow = i + 1;
			}
			else
			{
				iHigh = i - 1;
			}
		}
		riCompareRes = iCompareRes;
		return i;
	}

	//********************************************************************

	template<class TYPE>
	class CArraySortVal : public CArraySorted < TYPE, TYPE, TYPE >
	{
		//! @class Gray::CArraySortVal
		//! A sorted array of some native/simple TYPE of values (NOT Pointers)
		//! No duplicates allowed.
		//! ASSUME TYPE supports CValT::Compare()
		//! DEFAULT = sort is low to high

	public:
		typedef CArraySorted<TYPE, TYPE, TYPE> SUPER_t;
		typedef typename SUPER_t::KEY_t KEY_t;
		typedef typename SUPER_t::REF_t REF_t;

	protected:
		virtual COMPARE_t CompareData(REF_t Data1, REF_t Data2) const override
		{
			return CValT::Compare(Data1, Data2);
		}
		virtual COMPARE_t CompareKey(KEY_t Data1, REF_t Data2) const override
		{
			return CValT::Compare(Data1, Data2);
		}

	public:
		virtual ~CArraySortVal()
		{
		}

		bool RemoveArgKey(TYPE Data1)
		{
			return SUPER_t::RemoveArgKey(Data1, Data1);
		}
	};

	template<class TYPE, typename _TYPECH = char >
	class CArraySortStructName : public CArraySorted < TYPE, const TYPE&, const _TYPECH* >
	{
		//! @class Gray::CArraySortStructName
		//! A get_Name() sorted array of some type of structure (Non dynamic structure,NOT Pointer)
		//! ASSUME TYPE supports get_Name()
		//! DEFAULT = Alphabetic Non cased sort is low to high. A-Z

	public:
		typedef CArraySorted<TYPE, const TYPE&, const _TYPECH*> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::KEY_t KEY_t;

	protected:
		virtual COMPARE_t CompareData(REF_t Data1, REF_t Data2) const override
		{
			return StrT::CmpI<_TYPECH>(Data1.get_Name(), Data2.get_Name());
		}
		virtual COMPARE_t CompareKey(KEY_t key1, REF_t Data2) const override
		{
			return StrT::CmpI<_TYPECH>(key1, Data2.get_Name());
		}

	public:
		virtual ~CArraySortStructName()
		{}

		const TYPE* FindArgForKey(KEY_t key1) const
		{
			//! put the result in TYPE derived pointer.
			ITERATE_t index = FindIForKey(key1);
			if (index < 0)
				return nullptr;
			return &(this->ConstElementAt(index));
		}
	};

	template<class TYPE, typename TYPE_KEY = int >
	class CArraySortStructValue : public CArraySorted < TYPE, const TYPE&, TYPE_KEY >
	{
		//! @class Gray::CArraySortStructValue
		//! A get_SortValue() sorted array of some type of structure (Non dynamic structure,NOT Pointer)
		//! ASSUME TYPE supports get_SortValue()
		//! Similar to HashCode but different in the it can be any type. (float,etc)
		//! @note allow duplicate get_SortValue() but NOT duplicate objects!

	public:
		typedef CArraySorted<TYPE, const TYPE&, TYPE_KEY> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::KEY_t KEY_t;

	protected:
		virtual COMPARE_t CompareData(REF_t Data1, REF_t Data2) const override
		{
			//! Compare a data record to another data record.
			TYPE_KEY key1 = Data1.get_SortValue();
			TYPE_KEY key2 = Data2.get_SortValue();
			COMPARE_t iDiff = CValT::Compare(key1, key2);
			if (iDiff == COMPARE_Equal)	// allow duplicate get_SortValue() but NOT duplicate objects!
				return CValT::Compare((INT_PTR)&Data1, (INT_PTR)&Data2);
			return iDiff;
		}
		virtual COMPARE_t CompareKey(KEY_t key1, REF_t Base) const override
		{
			TYPE_KEY key2 = Base.get_SortValue();
			return CValT::Compare(key1, key2);
		}
	public:
		virtual ~CArraySortStructValue()
		{}

		const TYPE* FindArgForKey(KEY_t key1) const
		{
			//! put the result in TYPE derived pointer.
			ITERATE_t index = FindIForKey(key1);
			if (index < 0)
				return nullptr;
			return &(this->ConstElementAt(index));
		}
	};

	template<class TYPE, typename _TYPE_HASH = HASHCODE_t >
	class CArraySortStructHash : public CArraySorted < TYPE, const TYPE&, _TYPE_HASH >
	{
		//! @class Gray::CArraySortStructHash
		//! A get_HashCode() sorted array of some TYPE of structure (Non dynamic structure,NOT Pointer).
		//! Does NOT allow dupe hash codes!

	public:
		typedef CArraySorted<TYPE, const TYPE&, _TYPE_HASH> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::KEY_t KEY_t;

	protected:
		virtual COMPARE_t CompareData(REF_t Data1, REF_t Data2) const override
		{
			//! Compare a data record to another data record.
			_TYPE_HASH key1 = Data1.get_HashCode();
			_TYPE_HASH key2 = Data2.get_HashCode();
			return CValT::Compare(key1, key2);
		}
		virtual COMPARE_t CompareKey(KEY_t key1, REF_t Data2) const override
		{
			//! INT_MAX - INT_MIN must be positive !
			//! @note x-y will not work for extreme values! use memcmp()
			_TYPE_HASH key2 = Data2.get_HashCode();
			return CValT::Compare(key1, key2);
		}
	public:
		virtual ~CArraySortStructHash()
		{}

		const TYPE* FindArgForKey(KEY_t key1) const
		{
			//! put the result in TYPE derived pointer.
			ITERATE_t index = FindIForKey(key1);
			if (index < 0)
				return nullptr;
			return &(this->ConstElementAt(index));
		}
	};

	//********************************************************************

	template<class TYPE, class TYPE_PTR = TYPE, typename TYPE_KEY = TYPE>
	class CArraySortFacade : public CArraySorted < TYPE, TYPE_PTR, TYPE_KEY >
	{
		//! @class Gray::CArraySortFacade
		//! TYPE = the pointer or facade we are storing.
		//! A sorted array of some TYPE_PTR pointers. overload this
		//! Default Sort by memcmp()

	public:
		typedef CArraySorted<TYPE, TYPE_PTR, TYPE_KEY> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::ELEM_t ELEM_t;				//

	protected:
		virtual COMPARE_t CompareData(REF_t pData1, REF_t pData2) const override
		{
			//! default = Binary compare the whole thing.
			//! CValT::Compare(*pData1,*pData2) ??
			return ::memcmp(pData1, pData2, sizeof(*pData1));
		}

	public:
		virtual ~CArraySortFacade()
		{
			// Make sure virtuals are called correctly if storing a facade.
			this->RemoveAll();
		}

		bool IsValidIndex(ITERATE_t i) const
		{
			//! @todo RENAME THIS. Don't overload IsValidIndex. Make IsValidAt() ?
			if (!SUPER_t::IsValidIndex(i))
				return false;
			return(this->GetAt(i) != nullptr);
		}

		REF_t GetAt(ITERATE_t index) const
		{
			//! @note caller should put the result in TYPE (CSmartPtr) derived pointer.
			return this->ConstElementAt(index);
		}
		REF_t GetAtCheck(ITERATE_t nIndex) const
		{
			//! @note we should put the result in TYPE derived pointer.
			if (!this->IsValidIndex(nIndex))
			{
				return nullptr;
			}
			return this->ConstElementAt(nIndex);
		}

		TYPE_PTR FindArgForKey(TYPE_KEY key1) const
		{
			ITERATE_t index = this->FindIForKey(key1);
			if (index < 0)
				return nullptr;
			return this->GetAt(index);
		}
		ITERATE_t FindIForAC_BRUTEFORCE(const TYPE_PTR pData) const
		{
			//! Find the index of a specified entry. same as FindIFor() but arg is const TYPE*.
			//! Don't use this brute force version . Use the FindIForAK instead !
			//! @return index, -1 = k_ITERATE_BAD = none.
			if (pData == nullptr)
				return k_ITERATE_BAD;
			for (ITERATE_t nIndex = 0; nIndex < this->GetSize(); nIndex++)
			{
				if (this->m_pData[nIndex] == pData)
					return nIndex;
			}
			return k_ITERATE_BAD;
		}

		ELEM_t PopHead()
		{
			if (!this->GetSize())
			{
				return nullptr;
			}
			return SUPER_t::PopHead();
		}
		ELEM_t PopTail()
		{
			if (!this->GetSize())
			{
				return nullptr;
			}
			return SUPER_t::PopTail();
		}

		void DeleteAll()
		{
			// NOTE: this may not always be safe for the type!
			for (int i = 0; i < this->GetSize(); i++)
			{
				delete this->GetAt(i);
			}
			SUPER_t::RemoveAll();
		}
	};

	template<class TYPE, class TYPE_PTR, typename TYPE_KEY>
	class CArraySortFacadeValue : public CArraySortFacade < TYPE, TYPE_PTR, TYPE_KEY >
	{
		//! @class Gray::CArraySortFacadeValue
		//! A get_SortValue() sorted array of some TYPE* pointers. overload this
		//! @note allow duplicate get_SortValue() but NOT duplicate objects!

	public:
		typedef CArraySortFacade<TYPE, TYPE_PTR, TYPE_KEY> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::KEY_t KEY_t;

	protected:
		virtual COMPARE_t CompareData(REF_t pData1, REF_t pData2) const override
		{
			//! Compare a data record to another data record.
			ASSERT(pData1 != nullptr);
			ASSERT(pData2 != nullptr);
			KEY_t key1 = pData1->get_SortValue();
			KEY_t key2 = pData2->get_SortValue();
			COMPARE_t iDiff = CValT::Compare(key1, key2);
			if (iDiff == COMPARE_Equal)	// allow duplicate get_SortValue() but NOT duplicate objects!
				return CValT::Compare((INT_PTR)pData1, (INT_PTR)pData2);
			return iDiff;
		}
		virtual COMPARE_t CompareKey(KEY_t key1, TYPE_PTR pBase) const override
		{
			if (pBase == nullptr)
				return COMPARE_Greater;
			KEY_t key2 = pBase->get_SortValue();
			return CValT::Compare(key1, key2);
		}

	public:
		virtual ~CArraySortFacadeValue()
		{}

		ITERATE_t FindIForAK(const TYPE_PTR pBase) const
		{
			//! Equivalent of FindIFor() but uses the key for faster access. must check dupes.
			//! @return index, -1 = k_ITERATE_BAD = none.
			if (pBase == nullptr)
				return k_ITERATE_BAD;
			TYPE_KEY nKey = pBase->get_SortValue();
			ITERATE_t i = this->FindIFirstForKey(nKey);
			if (i < 0)
				return k_ITERATE_BAD;
			for (;;)
			{
				if (this->GetAt(i) == pBase) // since sorted values are allowed to duplicate.
					return i;
				if (++i >= this->GetSize())
					break;
			}
			// This probably shouldn't happen? pBase is not in the array!
			return k_ITERATE_BAD;	// FindIForAC(pBase);	// just do a brute force search.
		}

		bool RemoveArgKey(TYPE_PTR pBase)
		{
			ITERATE_t i = FindIForAK(pBase);
			if (i < 0)
				return false;
			SUPER_t::RemoveAt(i);
			return true;
		}
	};

	template<class TYPE, typename TYPE_KEY>
	class CArraySortPtrValue : public CArraySortFacadeValue < TYPE*, TYPE*, TYPE_KEY >
	{
		//! @class Gray::CArraySortPtrValue
	};

	template<class TYPE, class TYPE_PTR, typename _TYPE_HASH = HASHCODE_t >
	class CArraySortFacadeHash : public CArraySortFacade < TYPE, TYPE_PTR, _TYPE_HASH >
	{
		//! @class Gray::CArraySortFacadeHash
		//! a _TYPE_HASH get_HashCode() sorted array of TYPE* pointers
		//! does NOT allow dupe hash codes !

	public:
		typedef CArraySortFacade<TYPE, TYPE_PTR, _TYPE_HASH> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::KEY_t KEY_t;

	public:
		virtual ~CArraySortFacadeHash()
		{}

		virtual COMPARE_t CompareData(REF_t pData1, REF_t pData2) const override
		{
			//! Compare a data record to another data record.
			_TYPE_HASH key1 = pData1->get_HashCode();
			_TYPE_HASH key2 = pData2->get_HashCode();
			return CValT::Compare(key1, key2);
		}
		virtual COMPARE_t CompareKey(KEY_t key1, REF_t pData2) const override
		{
			//! @note x-y will not work for extreme values so we use CValT::Compare
			//! INT_MAX - INT_MIN must be positive !
			_TYPE_HASH key2 = pData2->get_HashCode();
			return CValT::Compare(key1, key2);
		}
	};

	template<class TYPE, typename TYPE_KEY>
	class CArraySortPtrHash : public CArraySortFacadeHash < TYPE*, TYPE*, TYPE_KEY >
	{
		//! @class Gray::CArraySortPtrHash
	};


	template<class TYPE, typename _TYPECH = GChar_t>
	class CArraySortPtrName : public CArraySortFacade < TYPE*, TYPE*, const _TYPECH* >
	{
		//! @class Gray::CArraySortPtrName
		//! A get_Name() sorted array of some TYPE* pointers. overload this
		//! ASSUME supports get_Name()

	public:
		typedef CArraySortFacade<TYPE*, TYPE*, const _TYPECH*> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;
		typedef typename SUPER_t::KEY_t KEY_t;

	protected:
		virtual COMPARE_t CompareData(REF_t pData1, REF_t pData2) const override
		{
			//! Compare a data record to another data record.
			ASSERT(pData1 != nullptr);
			ASSERT(pData2 != nullptr);
			return StrT::CmpI<_TYPECH>(pData1->get_Name(), pData2->get_Name());
		}
		virtual COMPARE_t CompareKey(KEY_t key1, REF_t pObj) const override
		{
			ASSERT(key1 != nullptr);
			ASSERT(pObj != nullptr);
			return StrT::CmpI<_TYPECH>(key1, pObj->get_Name());
		}

	public:
		virtual ~CArraySortPtrName()
		{}

		ITERATE_t FindIForAK(REF_t pBase) const
		{
			if (pBase == nullptr)
				return k_ITERATE_BAD;
			return SUPER_t::FindIForKey(pBase->get_Name());
		}
		bool RemoveArgKey(KEY_t pBase)
		{
			if (pBase == nullptr)
				return false;
			return SUPER_t::RemoveArgKey(pBase, pBase->get_Name());
		}
	};

	//*************************************************

	template<class TYPE, typename TYPE_KEY>
	class CArraySortSmart : public CArraySortFacade < CSmartPtr<TYPE>, TYPE*, TYPE_KEY >
	{
		//! @class Gray::CArraySortSmart
		//! A sorted array of CSmartPtr<TYPE> objects.
		//! the array has a reference to the element. similar to CArraySmart but sorted
		//! It will get deleted when the reference count is 0.
		//! default sort by memcmp() pointers.

		typedef CArraySortFacade< CSmartPtr<TYPE>, TYPE*, TYPE_KEY > SUPER_t;

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
					CArraySmart<TYPE> orig;
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
	class CArraySortHash : public CArraySortSmart < TYPE, _TYPE_HASH >
	{
		//! @class Gray::CArraySortHash
		//! A _TYPE_HASH get_HashCode() sorted array of CSmartPtr<TYPE>
		//! TYPE based on IScriptableObj typically
		//! does NOT allow dupe hash codes !

	public:
		typedef CArraySortSmart<TYPE, _TYPE_HASH> SUPER_t;
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
			return CValT::Compare(key1, key2);
		}
		virtual COMPARE_t CompareKey(KEY_t key1, REF_t pBase) const override
		{
			//! INT_MAX - INT_MIN must be positive !
			//! @note x-y will not work for extreme values so we use CValT::Compare
			ASSERT_N(pBase != nullptr);
			KEY_t key2 = pBase->get_HashCode();
			return CValT::Compare(key1, key2);
		}
	public:
		virtual ~CArraySortHash()
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
	class CArraySortValue : public CArraySortSmart < TYPE, TYPE_KEY >
	{
		//! @class Gray::CArraySortValue
		//! A TYPE_KEY get_SortValue() sorted array of CSmartPtr<TYPE>. sort low to high
		//! TYPE based on CSmartBase
		//! TYPE based on IScriptableObj typically
		//! @note allow duplicate get_SortValue() but NOT duplicate objects!
		//! Similar to HashCode but different in the it can be any type. (float,etc)

	public:
		typedef CArraySortSmart<TYPE, TYPE_KEY> SUPER_t;
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
			COMPARE_t iDiff = CValT::Compare(key1, key2);
			if (iDiff == COMPARE_Equal)	// allow duplicate get_SortValue() but NOT duplicate objects!
				return CValT::Compare((INT_PTR)pData1, (INT_PTR)pData2);
			return iDiff;
		}
		virtual COMPARE_t CompareKey(KEY_t key1, REF_t pBase) const override
		{
			if (pBase == nullptr)
				return COMPARE_Greater;
			TYPE_KEY key2 = pBase->get_SortValue();
			return CValT::Compare(key1, key2);
		}
	public:
		virtual ~CArraySortValue()
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
				CSmartPtr<TYPE> pBase2 = this->GetAtCheck(i);
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
	class CArraySortName : public CArraySortSmart < TYPE, const _TYPECH* >
	{
		//! @class Gray::CArraySortName
		//! get_Name() sorted array of CSmartPtr<TYPE>.
		//! TYPE must support get_Name() and be CSmartBase
		//! does  NOT allow dupe names !

	public:
		typedef CArraySortSmart<TYPE, const _TYPECH*> SUPER_t;
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
		virtual ~CArraySortName()
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

};
#endif // _INC_CArraySort_H
