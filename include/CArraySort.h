//
//! @file cArraySort.h
//! c++ sorted collections.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cArraySort_H
#define _INC_cArraySort_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArray.h"
#include "StrT.h"
#include "cValArray.h"

namespace Gray
{
	template<class TYPE, class TYPE_ARG, typename TYPE_KEY>
	class cArraySorted : public cArrayTyped < TYPE, TYPE_ARG >
	{
		//! @class Gray::cArraySorted
		//! An array of some sorted TYPE. duplicates are destroyed.
		//! default is that it is just sorted by its bytes.
		//! Similar to .NET HashSet

	public:
		typedef cArrayTyped<TYPE, TYPE_ARG> SUPER_t;
		typedef TYPE_KEY KEY_t;		// make a typedef for this type.
		typedef TYPE_ARG ARG_t;

	protected:
		virtual COMPARE_t CompareKey(KEY_t key1, TYPE_ARG Data2) const noexcept
		{
			//! Compare by a key that may not be part of a data record (yet).
			//! Default implementation. Overload this for proper implementation.
			//! @note If we reach here assume the key is &reference to the whole record !
			return cMem::Compare(&key1, &Data2, sizeof(TYPE_KEY));
		}

		bool RemoveArgKey(TYPE_ARG pObj, KEY_t key)
		{
			//! Can't use this for arrays that allow dupes ! (e.g. get_Value()) Use FindIForAK() instead.
			ITERATE_t index = FindIForKey(key);
			if (index <= k_ITERATE_BAD)
				return false;
			TYPE_ARG pObjOld = this->GetAt(index);
			if (!(pObjOld == pObj))	// not the current one !! weird! we don't allow dupes !
			{
				ASSERT(0);
				return false;
			}
			this->RemoveAt(index);
			return true;
		}

	public:
		virtual ~cArraySorted()
		{
			// Make this virtual to allow derived classes to override this and make destructors work.
		}

		ITERATE_t FindINear(TYPE_ARG pNew, COMPARE_t& iCompareRes) const;
		ITERATE_t FindINearKey(KEY_t key, COMPARE_t& iCompareRes) const noexcept;
		ITERATE_t FindIForKey(KEY_t key) const noexcept
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
			return i + 1;
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
			return i - 1;	// last
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

		ITERATE_t Add(TYPE_ARG pNew);	// add in sorted order.

		void AddArray(const SUPER_t& src)
		{
			//! Add all the entries in array a to this array. sorted add. like InsertArray() sort of.
			for (ITERATE_t i = 0; i < src.GetSize(); i++)
			{
				Add(src[i]);
			}
		}
	};

	template<class TYPE, class TYPE_ARG, typename TYPE_KEY>
	ITERATE_t cArraySorted<TYPE, TYPE_ARG, TYPE_KEY>::FindINear(TYPE_ARG pNew, COMPARE_t& riCompareRes) const
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
			iCompareRes = this->CompareData(pNew, this->GetAt(i));	// virtual call.
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
	ITERATE_t cArraySorted<TYPE, TYPE_ARG, TYPE_KEY>::Add(TYPE_ARG pNew)
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
			return index;
		}
		return AddPresorted(index, iCompareRes, pNew);
	}

	template<class TYPE, class TYPE_ARG, typename TYPE_KEY>
	ITERATE_t cArraySorted<TYPE, TYPE_ARG, TYPE_KEY>::FindINearKey(KEY_t key, COMPARE_t& riCompareRes) const noexcept
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
			iCompareRes = CompareKey(key, this->GetAt(i));
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
	class cArraySortVal : public cArraySorted < TYPE, TYPE, TYPE >
	{
		//! @class Gray::cArraySortVal
		//! A sorted array of some native/simple TYPE of values (NOT Pointers)
		//! No duplicates allowed.
		//! ASSUME TYPE supports cValT::Compare()
		//! DEFAULT = sort is low to high

	public:
		typedef cArraySorted<TYPE, TYPE, TYPE> SUPER_t;
#ifdef __GNUC__
		typedef typename SUPER_t::KEY_t KEY_t;
		typedef typename SUPER_t::ARG_t ARG_t;
#endif

	protected:
		virtual COMPARE_t CompareData(ARG_t Data1, ARG_t Data2) const noexcept override
		{
			return cValT::Compare(Data1, Data2);
		}
		virtual COMPARE_t CompareKey(KEY_t Data1, ARG_t Data2) const noexcept override
		{
			return cValT::Compare(Data1, Data2);
		}

	public:
		virtual ~cArraySortVal()
		{
		}

		bool RemoveArgKey(TYPE Data1)
		{
			return SUPER_t::RemoveArgKey(Data1, Data1);
		}
	};

	template<class TYPE, typename _TYPECH = char >
	class cArraySortStructName : public cArraySorted < TYPE, const TYPE&, const _TYPECH* >
	{
		//! @class Gray::cArraySortStructName
		//! A get_Name() sorted array of some type of structure (Non dynamic structure,NOT Pointer)
		//! ASSUME TYPE supports get_Name()
		//! DEFAULT = Alphabetic Non cased sort is low to high. A-Z

	public:
		typedef cArraySorted<TYPE, const TYPE&, const _TYPECH*> SUPER_t;
#ifdef __GNUC__
		typedef typename SUPER_t::ARG_t ARG_t;
		typedef typename SUPER_t::KEY_t KEY_t;
#endif

	protected:
		virtual COMPARE_t CompareData(ARG_t Data1, ARG_t Data2) const noexcept override
		{
			return StrT::CmpI<_TYPECH>(Data1.get_Name(), Data2.get_Name());
		}
		virtual COMPARE_t CompareKey(KEY_t key1, ARG_t Data2) const noexcept override
		{
			return StrT::CmpI<_TYPECH>(key1, Data2.get_Name());
		}

	public:
		virtual ~cArraySortStructName()
		{}

		const TYPE* FindArgForKey(KEY_t key1) const noexcept
		{
			//! put the result in TYPE derived pointer.
			ITERATE_t index = FindIForKey(key1);
			if (index < 0)
				return nullptr;
			return &(this->GetAt(index));
		}
	};

	template<class TYPE, typename TYPE_KEY = int >
	class cArraySortStructValue : public cArraySorted < TYPE, const TYPE&, TYPE_KEY >
	{
		//! @class Gray::cArraySortStructValue
		//! A get_SortValue() sorted array of some type of structure (Non dynamic structure,NOT Pointer)
		//! ASSUME TYPE supports get_SortValue()
		//! Similar to HashCode but different in the it can be any type. (float,etc)
		//! @note allow duplicate get_SortValue() but NOT duplicate objects!

	public:
		typedef cArraySorted<TYPE, const TYPE&, TYPE_KEY> SUPER_t;
#ifdef __GNUC__
		typedef typename SUPER_t::ARG_t ARG_t;
		typedef typename SUPER_t::KEY_t KEY_t;
#endif

	protected:
		virtual COMPARE_t CompareData(ARG_t Data1, ARG_t Data2) const noexcept override
		{
			//! Compare a data record to another data record.
			TYPE_KEY key1 = Data1.get_SortValue();
			TYPE_KEY key2 = Data2.get_SortValue();
			COMPARE_t iDiff = cValT::Compare(key1, key2);
			if (iDiff == COMPARE_Equal)	// allow duplicate get_SortValue() but NOT duplicate objects!
				return cValT::Compare((INT_PTR)&Data1, (INT_PTR)&Data2);
			return iDiff;
		}
		virtual COMPARE_t CompareKey(KEY_t key1, ARG_t Base) const noexcept override
		{
			TYPE_KEY key2 = Base.get_SortValue();
			return cValT::Compare(key1, key2);
		}
	public:
		virtual ~cArraySortStructValue()
		{}

		const TYPE* FindArgForKey(KEY_t key1) const
		{
			//! put the result in TYPE derived pointer.
			ITERATE_t index = FindIForKey(key1);
			if (index < 0)
				return nullptr;
			return &(this->GetAt(index));
		}
	};

	template<class TYPE, typename _TYPE_HASH = HASHCODE_t >
	class cArraySortStructHash : public cArraySorted < TYPE, const TYPE&, _TYPE_HASH >
	{
		//! @class Gray::cArraySortStructHash
		//! A get_HashCode() sorted array of some TYPE of structure (Non dynamic structure,NOT Pointer).
		//! Does NOT allow dupe hash codes!

	public:
		typedef cArraySorted<TYPE, const TYPE&, _TYPE_HASH> SUPER_t;
#ifdef __GNUC__
		typedef typename SUPER_t::ARG_t ARG_t;
		typedef typename SUPER_t::KEY_t KEY_t;
#endif

	protected:
		virtual COMPARE_t CompareData(ARG_t Data1, ARG_t Data2) const noexcept override
		{
			//! Compare a data record to another data record.
			_TYPE_HASH key1 = Data1.get_HashCode();
			_TYPE_HASH key2 = Data2.get_HashCode();
			return cValT::Compare(key1, key2);
		}
		virtual COMPARE_t CompareKey(KEY_t key1, ARG_t Data2) const noexcept override
		{
			//! INT_MAX - INT_MIN must be positive !
			//! @note x-y will not work for extreme values! use cMem::Compare()
			_TYPE_HASH key2 = Data2.get_HashCode();
			return cValT::Compare(key1, key2);
		}
	public:
		virtual ~cArraySortStructHash()
		{}

		const TYPE* FindArgForKey(KEY_t key1) const
		{
			//! put the result in TYPE derived pointer.
			ITERATE_t index = FindIForKey(key1);
			if (index < 0)
				return nullptr;
			return &(this->GetAt(index));
		}
	};

	//********************************************************************

	template<class TYPE, class TYPE_PTR = TYPE, typename TYPE_KEY = TYPE>
	class cArraySortFacade : public cArraySorted < TYPE, TYPE_PTR, TYPE_KEY >
	{
		//! @class Gray::cArraySortFacade
		//! TYPE = the pointer or facade we are storing.
		//! A sorted array of some TYPE_PTR pointers. overload this
		//! Sorted by TYPE_KEY get_SortVal
		//! Default Sort by cMem::Compare()

	public:
		typedef cArraySorted<TYPE, TYPE_PTR, TYPE_KEY> SUPER_t;
#ifdef __GNUC__
		typedef typename SUPER_t::ARG_t ARG_t;
#endif

	protected:
		virtual COMPARE_t CompareData(ARG_t pData1, ARG_t pData2) const noexcept override
		{
			//! default = Binary compare the whole thing.
			//! like cValT::Compare(*pData1,*pData2) ? 
			return cMem::Compare(pData1, pData2, sizeof(*pData1));
		}

	public:
		virtual ~cArraySortFacade()
		{
			// Make sure virtuals are called correctly if storing a facade.
			this->RemoveAll();
		}

		bool IsValidIndex(ITERATE_t i) const noexcept
		{
			//! @todo RENAME THIS. Don't overload IsValidIndex. Make IsValidAt() ?
			if (!SUPER_t::IsValidIndex(i))
				return false;
			return this->GetAt(i) != nullptr ;
		}

#if 0
		TYPE_PTR GetAt(ITERATE_t index) const noexcept
		{
			//! Cast to TYPE_PTR helper.
			//! @note caller should put the result in TYPE (cRefPtr) derived pointer.
			return this->GetAt(index);
		}
#endif

		TYPE GetAtCheck(ITERATE_t nIndex) const
		{
			//! Cast to TYPE_PTR ?
			//! @note we should put the result in TYPE derived pointer.
			if (!this->IsValidIndex(nIndex))
			{
				return nullptr;
			}
			return this->GetAt(nIndex);
		}

		TYPE_PTR FindArgForKey(TYPE_KEY key1) const noexcept
		{
			const ITERATE_t index = this->FindIForKey(key1);
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

		TYPE PopHead()
		{
			if (!this->GetSize())
			{
				return nullptr;
			}
			return SUPER_t::PopHead();
		}
		TYPE PopTail()
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
	class cArraySortFacadeValue : public cArraySortFacade < TYPE, TYPE_PTR, TYPE_KEY >
	{
		//! @class Gray::cArraySortFacadeValue
		//! A get_SortValue() sorted array of some TYPE* pointers. overload this
		//! @note allow duplicate get_SortValue() but NOT duplicate objects!

	public:
		typedef cArraySortFacade<TYPE, TYPE_PTR, TYPE_KEY> SUPER_t;
#ifdef __GNUC__
		typedef typename SUPER_t::ARG_t ARG_t;
		typedef typename SUPER_t::KEY_t KEY_t;	// TYPE_KEY
#endif

	protected:
		virtual COMPARE_t CompareData(ARG_t pData1, ARG_t pData2) const noexcept override
		{
			//! Compare a data record to another data record.
			ASSERT(pData1 != nullptr);
			ASSERT(pData2 != nullptr);
			KEY_t key1 = pData1->get_SortValue();
			KEY_t key2 = pData2->get_SortValue();
			COMPARE_t iDiff = cValT::Compare(key1, key2);
			if (iDiff == COMPARE_Equal)	// allow duplicate get_SortValue() but NOT duplicate objects!
				return cValT::Compare((INT_PTR)pData1, (INT_PTR)pData2);
			return iDiff;
		}
		virtual COMPARE_t CompareKey(KEY_t key1, TYPE_PTR pBase) const noexcept override
		{
			if (pBase == nullptr)
				return COMPARE_Greater;
			KEY_t key2 = pBase->get_SortValue();
			return cValT::Compare(key1, key2);
		}

	public:
		virtual ~cArraySortFacadeValue()
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
	class cArraySortPtrValue : public cArraySortFacadeValue < TYPE*, TYPE*, TYPE_KEY >
	{
		//! @class Gray::cArraySortPtrValue
	};

	template<class TYPE, class TYPE_PTR, typename _TYPE_HASH = HASHCODE_t >
	class cArraySortFacadeHash : public cArraySortFacade < TYPE, TYPE_PTR, _TYPE_HASH >
	{
		//! @class Gray::cArraySortFacadeHash
		//! a _TYPE_HASH get_HashCode() sorted array of TYPE* pointers
		//! does NOT allow dupe hash codes !

	public:
		typedef cArraySortFacade<TYPE, TYPE_PTR, _TYPE_HASH> SUPER_t;
#ifdef __GNUC__
		typedef typename SUPER_t::ARG_t ARG_t;
		typedef typename SUPER_t::KEY_t KEY_t;
#endif

	public:
		virtual ~cArraySortFacadeHash()
		{}

		virtual COMPARE_t CompareData(ARG_t pData1, TYPE_PTR pData2) const noexcept override
		{
			//! Compare a data record to another data record.
			_TYPE_HASH key1 = pData1->get_HashCode();
			_TYPE_HASH key2 = pData2->get_HashCode();
			return cValT::Compare(key1, key2);
		}
		virtual COMPARE_t CompareKey(KEY_t key1, TYPE_PTR pData2) const noexcept override
		{
			//! @note x-y will not work for extreme values so we use cValT::Compare
			//! INT_MAX - INT_MIN must be positive !
			_TYPE_HASH key2 = pData2->get_HashCode();
			return cValT::Compare(key1, key2);
		}
	};

	template<class TYPE, typename TYPE_KEY>
	class cArraySortPtrHash : public cArraySortFacadeHash < TYPE*, TYPE*, TYPE_KEY >
	{
		//! @class Gray::cArraySortPtrHash
	};


	template<class TYPE, typename _TYPECH = GChar_t>
	class cArraySortPtrName : public cArraySortFacade < TYPE*, TYPE*, const _TYPECH* >
	{
		//! @class Gray::cArraySortPtrName
		//! A get_Name() sorted array of some TYPE* pointers. overload this
		//! ASSUME supports get_Name()

	public:
		typedef cArraySortFacade<TYPE*, TYPE*, const _TYPECH*> SUPER_t;
#ifdef __GNUC__
		typedef typename SUPER_t::ARG_t ARG_t;
		typedef typename SUPER_t::KEY_t KEY_t;
#endif

	protected:
		virtual COMPARE_t CompareData(ARG_t pData1, ARG_t pData2) const noexcept override
		{
			//! Compare a data record to another data record.
			ASSERT(pData1 != nullptr);
			ASSERT(pData2 != nullptr);
			return StrT::CmpI<_TYPECH>(pData1->get_Name(), pData2->get_Name());
		}
		virtual COMPARE_t CompareKey(KEY_t key1, ARG_t pObj) const noexcept override
		{
			ASSERT(key1 != nullptr);
			ASSERT(pObj != nullptr);
			return StrT::CmpI<_TYPECH>(key1, pObj->get_Name());
		}

	public:
		virtual ~cArraySortPtrName()
		{}

		ITERATE_t FindIForAK(ARG_t pBase) const
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
}
#endif // _INC_cArraySort_H
