//
//! @file cArray.h
//! c++ Collections. MFC compatible.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cArray_H
#define _INC_cArray_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cHeap.h"
#include "cObject.h"
#include "cValT.h"
#ifdef _MFC_VER
#include <afxtempl.h>	// CArray
#else
#include <new>	// STL overload the new operator to allow call of constructor directly.
#endif

namespace Gray
{
	// similar to BOOST_FOREACH()
#define GRAY_FOREACH( _ITYPE, i, a ) for ( ITERATE_t index_##i=0, _foreach_int=true; index_##i < a.GetSize(); index_##i++, _foreach_int=true ) \
	for ( _ITYPE i=a[index_##i]; _foreach_int; _foreach_int=false )

#ifndef _MFC_VER

	template <class TYPE>
	void __cdecl ConstructElements(TYPE* pElements, ITERATE_t nCount)
	{
		//! 'new' an array of elements. For use with CArray
		//! @note we cannot always ASSUME that the constructor actually does anything. (i.e. new int)
		//! for instance void* has no constructor and will be left as CDCDCDCD NOT nullptr as expected.

		if (nCount <= 0)
			return;

#ifdef _DEBUG
		ASSERT(cMem::IsValid(pElements, nCount * sizeof(TYPE), true));
		// for debug. first do bit-wise init (or zero) initialization
		cValArray::FillSize<BYTE>(pElements, nCount * sizeof(TYPE), cHeap::FILL_Alloc);
#endif

		for (; (nCount--) != 0; pElements++)
		{
			::new((void*)pElements) TYPE;
		}
	}

	template <class TYPE>
	void __cdecl DestructElements(TYPE* pElements, ITERATE_t nCount)
	{
		//! Call the destructor(s) for an array. Does not explicitly free anything.
		if (nCount <= 0)
			return;

#ifdef _DEBUG
		ASSERT(cMem::IsValid(pElements, nCount * sizeof(TYPE), true));
		const ITERATE_t nCountPrev = nCount;	// just for debug
		UNREFERENCED_PARAMETER(nCountPrev);
#endif

		for (; (nCount--) != 0; pElements++)
		{
			(pElements)->~TYPE();
		}
	}

	template <class TYPE>
	void __cdecl CopyElements(TYPE* pDest, const TYPE* pSrc, ITERATE_t nCount)
	{
		//! element-copy using class assignment operators
		if (nCount <= 0)
			return;

		ASSERT(cMem::IsValid(pDest, nCount * sizeof(TYPE), true));
		ASSERT(cMem::IsValid(pSrc, nCount * sizeof(TYPE), false));

		while ((nCount--) != 0)
		{
			*pDest++ = *pSrc++;
		}
	}

	// Override implementation of templates
	template <>
	inline void __cdecl CopyElements<BYTE>(BYTE* pDest, const BYTE* pSrc, ITERATE_t nCount)
	{
		//! Any integral type could use this ?
		cMem::Copy(pDest, pSrc, nCount);
	}

	/////////////////////////////////////////////////////////////////////////////

	template <class TYPE, class ARG_TYPE = const TYPE& >
	class CArray : public CObject
	{
		//! @class Gray::CArray
		//! Minimal array template of elements. like MFC version.
		//! @note MFC 8.0 uses INT_PTR for GetSize()
		//! ARG_TYPE = const TYPE&	typically

		typedef CArray<TYPE, ARG_TYPE> THIS_t;

	protected:
		TYPE* m_pData;			//!< the actual array of data
		ITERATE_t m_nSize;		//!< Number of elements (upperBound - 1)

	protected:
		bool IsValidIndex(ITERATE_t i) const noexcept
		{
			return IS_INDEX_GOOD(i, this->m_nSize);
		}

	public:
		CArray() noexcept
			: m_pData(nullptr)
			, m_nSize(0)
		{
		}
		CArray(THIS_t&& ref) noexcept
		{
			//! move constructor.
			m_pData = ref.m_pData; ref.m_pData = nullptr;
			m_nSize = ref.m_nSize; ref.m_nSize = 0;
		}
		virtual ~CArray()
		{
			RemoveAll();
		}

		bool IsValidMallocSize() const noexcept;

		// Attributes
		inline ITERATE_t GetSize() const noexcept
		{
			return m_nSize;
		}
		inline ITERATE_t GetUpperBound() const noexcept
		{
			return m_nSize - 1;
		}
		bool IsEmpty() const noexcept
		{
			return this->m_nSize <= 0;
		}
		void SetSize(ITERATE_t nNewSize);
		ITERATE_t GetMallocSize() const noexcept
		{
			//! Get quantity of objects truly allocated. (may not be whole number)
			//! like STL capacity()
			return (ITERATE_t)(cHeap::GetSize(m_pData) / sizeof(TYPE));
		}

		// Operations
		//! Clean up
		void RemoveAll();

		// Accessing elements
		const TYPE& GetAt(ITERATE_t nIndex) const
		{
			ASSERT(IsValidIndex(nIndex));
			return m_pData[nIndex];
		}
		TYPE& ElementAt(ITERATE_t nIndex)
		{
			ASSERT(IsValidIndex(nIndex));
			return m_pData[nIndex];
		}

		void SetAt(ITERATE_t nIndex, ARG_TYPE newElement) // throw
		{
			//! @note Destructor is automatically called.
			ASSERT(IsValidIndex(nIndex));
			m_pData[nIndex] = newElement;	// may call a copy constructor.
		}

		// Direct Access to the element data (may return nullptr)
		inline const TYPE* GetData() const
		{
			return m_pData;
		}
		inline TYPE* GetData()
		{
			return m_pData;
		}
		void SetDataArrayPtr(TYPE* pData, ITERATE_t nSize)
		{
			//! set internal pointer. (dangerous)
			ASSERT(!cMem::IsCorrupt(pData, nSize));
			m_pData = pData;
			m_nSize = nSize;
		}

		// Potentially growing the array
		void SetAtGrow(ITERATE_t nIndex, ARG_TYPE newElement);
		ITERATE_t Add(ARG_TYPE newElement)
		{
			//! Add to the end.
			const ITERATE_t nIndex = m_nSize;
			SetAtGrow(nIndex, newElement);
			return nIndex;
		}

		void Copy(const CArray& src);

		// overloaded operator helpers
		inline TYPE& operator[](ITERATE_t nIndex)
		{
			return ElementAt(nIndex);
		}
		inline const TYPE& operator[](ITERATE_t nIndex) const
		{
			return GetAt(nIndex);
		}

		// Operations that move elements around
		void InsertAt(ITERATE_t nIndex, ARG_TYPE newElement);
		void RemoveAt(ITERATE_t nIndex);
		void RemoveAt(ITERATE_t nIndex, ITERATE_t iQty);

		void MoveElement(ITERATE_t iFrom, ITERATE_t iTo); // throw
	};

	//************************************************************************

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::RemoveAll()
	{
		//! AKA SetEmpty, Empty
		//! @note SetSize(0) is slightly more efficient than RemoveAll() if u plan to re-use the array.
#ifdef _DEBUG
		// AssertSize();
#endif
		const ITERATE_t iSize = m_nSize;
		m_nSize = 0;
		if (m_pData != nullptr)
		{
			DestructElements<TYPE>(m_pData, iSize);
			cHeap::FreePtr((void*)(m_pData));	// const_cast
			m_pData = nullptr;
		}
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::SetSize(ITERATE_t nNewSize)
	{
		//! @note SetSize(0) is slightly more efficient than RemoveAll() if u plan to re-use the array.
		ASSERT_VALID(this);
		ASSERT(nNewSize >= 0);
		if (nNewSize <= 0)
		{
			// shrink to nothing but keep any allocated memory.
			goto do_try_resize;
		}
		else if (m_pData == nullptr)
		{
			// create one with exact size
			m_pData = (TYPE*)cHeap::AllocPtr(nNewSize * sizeof(TYPE));
			ASSERT(m_pData != nullptr);
			ConstructElements<TYPE>(m_pData, nNewSize);
		}
		else if (nNewSize <= GetMallocSize())
		{
			// it fits. don't shrink the allocated array. we may expand again some day.
		do_try_resize:
			if (nNewSize > m_nSize)
			{
				// initialize the new elements
				ConstructElements<TYPE>(&m_pData[m_nSize], nNewSize - m_nSize);
			}
			else if (m_nSize > nNewSize)
			{
				// destroy the old elements
				DestructElements<TYPE>(&m_pData[nNewSize], m_nSize - nNewSize);
			}
		}
		else
		{
			// otherwise, grow array
			// MFC will heuristically determine growth when nGrowBy == 0 (this avoids heap fragmentation in many situations)
#if 1
			m_pData = (TYPE*)cHeap::ReAllocPtr(m_pData, (nNewSize + (nNewSize / 16)) * sizeof(TYPE));
#else
			m_pData = (TYPE*)cHeap::ReAllocPtr(m_pData, nNewSize * sizeof(TYPE));
#endif
			ASSERT_N(m_pData != nullptr);

			// construct remaining elements
			ASSERT(nNewSize > m_nSize);
			ConstructElements<TYPE>(&m_pData[m_nSize], nNewSize - m_nSize);
		}
		m_nSize = nNewSize;
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::Copy(const CArray& src)
	{
		ASSERT_VALID(this);
		ASSERT(this != &src);   // ASSUME Not copy to self.
		SetSize(src.m_nSize);
		CopyElements<TYPE>(m_pData, src.m_pData, src.m_nSize);
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::SetAtGrow(ITERATE_t nIndex, ARG_TYPE newElement)
	{
		ASSERT_VALID(this);
		ASSERT(nIndex >= 0);
		if (nIndex < m_nSize)
		{
			m_pData[nIndex] = newElement;
			return;
		}

		SetSize(nIndex + 1);
		ASSERT_N(m_pData != nullptr);
		m_pData[nIndex] = newElement;
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::MoveElement(ITERATE_t iFrom, ITERATE_t iTo) // throw
	{
		//! Similar to Swap() but only one element is moved. (sort of)
		//! re-order the whole array. move an index to another place.
		//! dangerous for types that have internal pointers !
		BYTE bTmp[sizeof(TYPE)];
		ASSERT(IsValidIndex(iFrom));
		cMem::Copy(bTmp, &m_pData[iFrom], sizeof(TYPE));
		// shift old data to fill gap
		ASSERT(IsValidIndex(iTo));
		cMem::CopyOverlap(&m_pData[iTo + 1], &m_pData[iTo],
			(iFrom - iTo) * sizeof(TYPE));
		// re-init slots we copied from
		cMem::Copy(&m_pData[iTo], bTmp, sizeof(TYPE));
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::InsertAt(ITERATE_t nIndex, ARG_TYPE newElement)
	{
		//! Insert at this location, move anything after this.
		ASSERT_VALID(this);
		ASSERT(nIndex >= 0);    // will expand to meet need

		if (nIndex >= m_nSize)
		{
			// adding after the end of the array
			SetSize(nIndex + 1);   // grow so nIndex is valid
		}
		else
		{
			// inserting in the middle of the array
			ITERATE_t nOldSize = m_nSize;
			SetSize(m_nSize + 1);  // grow it to new size
			// destroy initial data before copying over it (inefficient i know but is very convenient)
			MoveElement(nOldSize, nIndex);
		}

		// insert new value in the gap
		ASSERT(nIndex + 1 <= m_nSize);
		m_pData[nIndex] = newElement;	// ASSUME copy constructor will be called!
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::RemoveAt(ITERATE_t nIndex)
	{
		// NOTE: Any destructor effecting the array will be reentrant ?!
		ASSERT_VALID(this);
		if (nIndex < 0)
			return;
		ITERATE_t nMoveCount = m_nSize - (nIndex + 1);
		if (nMoveCount < 0)
			return;
		DestructElements<TYPE>(&m_pData[nIndex], 1);
		m_nSize--;
		if (nMoveCount > 0) // not last.
		{
			cMem::CopyOverlap(&m_pData[nIndex], &m_pData[nIndex + 1], nMoveCount * sizeof(TYPE));
		}
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::RemoveAt(ITERATE_t nIndex, ITERATE_t iQty)
	{
		// NOTE: Any destructor effecting the array will be reentrant ?!
		ASSERT_VALID(this);
		if (iQty <= 0 || nIndex < 0)
			return;
		ITERATE_t nMoveCount = m_nSize - (nIndex + iQty);
		if (nMoveCount < 0)	// iQty beyond the end!
		{
			ASSERT(nMoveCount >= 0);
			nMoveCount = 0;	// Last element.
			iQty = m_nSize - nIndex;
		}
		if (iQty >= m_nSize)
		{
			// ASSERT(nIndex == 0);	// assumed.
			RemoveAll();
			return;
		}
		// just remove a range
		DestructElements<TYPE>(&m_pData[nIndex], iQty);
		if (nMoveCount > 0)	// not last.
		{
			cMem::CopyOverlap(&m_pData[nIndex], &m_pData[nIndex + iQty], nMoveCount * sizeof(TYPE));
		}
		m_nSize -= iQty;
	}

	template <class TYPE, class ARG_TYPE>
	bool CArray<TYPE, ARG_TYPE>::IsValidMallocSize() const noexcept
	{
		// Make sure the alloc is actually bigger than the declared size.
		if (m_pData == nullptr)
		{
			if (m_nSize != 0)
				return false;
		}
		else
		{
			// ASSERT(m_nSize>0);
			if (m_nSize > GetMallocSize())	// NOTE: GetMallocSize will check m_pData
				return false;
		}
		return true;
	}

#endif // _MFC_VER

	//*************************************************

	template <class TYPE, class ARG_TYPE = const TYPE&>
	class cArrayTyped : public CArray < TYPE, ARG_TYPE >
	{
		//! @class Gray::cArrayTyped
		//! Equivalent to MFC CArray with added functions not in MFC
		//! TYPE = the type stored. ARG_TYPE = how it should be referenced. const TYPE&	typically
	public:
		typedef CArray<TYPE, ARG_TYPE> SUPER_t;
		typedef cArrayTyped<TYPE, ARG_TYPE> THIS_t;

		typedef ITERATE_t iterator;			// like STL
		typedef ITERATE_t const_iterator;	// like STL

		typedef TYPE ELEM_t;				//!< What type is stored.
		typedef ARG_TYPE REF_t;				//!< How to refer to this? value or ref or pointer?

	protected:
		virtual COMPARE_t CompareData(REF_t Data1, REF_t Data2) const noexcept
		{
			//! Compare a data record to another data record.
			//! ASSUME this is the same as comparing keys. Otherwise you must overload this.
			//! Default implementation. Override this for proper implementation. This probably won't work for most cases.
			// return cValT::Compare(Data1,Data2);
			return cMem::Compare(&Data1, &Data2, sizeof(REF_t));	// should we use use cValT::Compare()??
		}

		ITERATE_t QSortPartition(ITERATE_t iLeft, ITERATE_t iRight);
		void QSort(ITERATE_t iLeft, ITERATE_t iRight);

	public:
		cArrayTyped() noexcept
		{
		}
		cArrayTyped(const THIS_t& rArray)
		{
			//! Force copies to be explicit!
			this->SetSize(rArray.GetSize());
			CopyElements(this->GetData(), rArray.GetData(), this->GetSize());
		}
		explicit cArrayTyped(ITERATE_t iSize)
		{
			this->SetSize(iSize);
		}
		virtual ~cArrayTyped()
		{
		}

		bool isValidCheck() const noexcept
		{
			if (!COBJECT_IsValidCheck())
				return false;
			// ASSERT( ISTYPE_OF(CArray<TYPE, ARG_TYPE>,this));
#ifndef _MFC_VER
			if (!this->IsValidMallocSize())
				return false;
#endif
			return true;
		}

		// New
		bool IsValidIndex(ITERATE_t i) const noexcept
		{
			return IS_INDEX_GOOD(i, this->m_nSize);
		}
		ITERATE_t ClampValidIndex(ITERATE_t i) const noexcept
		{
			//! @return -1 = empty array.
			if (i < 0)
				i = 0;
			if (i >= this->m_nSize)
				return(this->m_nSize - 1);
			return i;
		}
		size_t GetHeapStats(OUT ITERATE_t& iAllocCount) const noexcept
		{
			//! @return sizeof all children alloc(s). not size of *this
			if (this->m_pData == nullptr)
				return 0;
			iAllocCount++; // just the alloc for the array
			return cHeap::GetSize(this->m_pData);
		}

		void AssertValidIndex(ITERATE_t nIndex) const
		{
			ASSERT_THROW(IsValidIndex(nIndex));
		}
		const TYPE& GetAtSecure(ITERATE_t nIndex) const // throw
		{
			//! throw an exception if we are out or range.
			AssertValidIndex(nIndex);
			return this->m_pData[nIndex];
		}
		TYPE& ElementAtSecure(ITERATE_t nIndex) // throw
		{
			//! throw an exception if we are out or range.
			AssertValidIndex(nIndex);
			return this->m_pData[nIndex];
		}

		REF_t ConstElementAt(ITERATE_t nIndex) const // throw
		{
			// Same as GetAt ?
			AssertValidIndex(nIndex);
			return this->m_pData[nIndex];
		}
		inline TYPE& operator[](ITERATE_t nIndex) // throw
		{
			return this->ElementAt(nIndex);
		}
		inline const TYPE& operator[](ITERATE_t nIndex) const // throw
		{
			// Const array should return const values.
			AssertValidIndex(nIndex);
			return this->m_pData[nIndex];
		}

		TYPE& Head() // throw
		{
			return this->ElementAt(0);
		}
		TYPE& Tail()
		{
			return this->ElementAt(this->GetSize() - 1);
		}
		TYPE& ElementAtHead()
		{
			return this->ElementAt(0);
		}
		TYPE& ElementAtTail()
		{
			return this->ElementAt(this->GetSize() - 1);
		}
		REF_t ConstHead() const
		{
			return ConstElementAt(0);
		}
		REF_t ConstTail() const
		{
			return ConstElementAt(this->GetSize() - 1);
		}

		REF_t GetAtTail()
		{
			return this->GetAt(this->GetSize() - 1);
		}

		void UnLinkIndex(ITERATE_t nIndex)
		{
			//! Remove the object from the list.
			//! DO NOT call its normal destructor!
			ASSERT(IsValidIndex(nIndex));
			cMem::CopyOverlap(&(this->m_pData[nIndex]), &(this->m_pData[nIndex + 1]), ((this->m_nSize - nIndex) - 1) * sizeof(TYPE));
			this->m_nSize--;
			if (this->m_nSize == 0)
				this->RemoveAll();
		}
		void Swap(ITERATE_t i, ITERATE_t j)
		{
			//! like cMemT::Swap(). dangerous for types that have pointers to themselves. self referenced.
			if (i == j)
				return;
			cMem::Swap((BYTE*)&this->ElementAt(i), (BYTE*)&(this->ElementAt(j)), sizeof(TYPE));
		}
		void SetCopy(const cArrayTyped<TYPE, ARG_TYPE>& aValues)
		{
			//! @note This will call empty constructors.
			if (this == &aValues)
				return;
			this->RemoveAll();	// destruct any previous data.
			this->SetSize(aValues.GetSize());
			CopyElements(this->GetData(), aValues.GetData(), this->GetSize());
		}

		const cArrayTyped<TYPE, ARG_TYPE>& operator = (const cArrayTyped<TYPE, ARG_TYPE>& aValues)
		{
			SetCopy(aValues);
			return *this;
		}

		ITERATE_t FindIFor(ARG_TYPE arg) const
		{
			//! Find the index of a specified entry arg.
			//! @return index, -1 = k_ITERATE_BAD = not found.
			for (ITERATE_t nIndex = 0; nIndex < this->GetSize(); nIndex++)
			{
				if (this->m_pData[nIndex] == arg)
					return nIndex;
			}
			return k_ITERATE_BAD;
		}
		bool HasArg(ARG_TYPE arg) const
		{
			//! Find the index of a specified entry.
			return FindIFor(arg) >= 0;
		}

		void RemoveLast()
		{
			this->RemoveAt((this->GetSize()) - 1);
		}
		ELEM_t PopHead()
		{
			ASSERT(this->GetSize() > 0);
			ELEM_t tmp = this->GetAt(0);	// copy it.
			this->RemoveAt(0);
			return tmp;
		}
		ELEM_t PopTail()
		{
			ASSERT(this->GetSize() > 0);
			ITERATE_t i = (this->GetSize()) - 1;
			ELEM_t tmp = this->GetAt(i);	// copy it.
			this->RemoveAt(i);
			return tmp;
		}
		bool RemoveArg(ARG_TYPE arg)
		{
			//! @return true = removed. false = was not here.
			ITERATE_t nIndex = FindIFor(arg);
			if (nIndex < 0)
				return false;
			this->RemoveAt(nIndex);
			return true;
		}
		void UnLinkArg(ARG_TYPE arg)
		{
			ITERATE_t nIndex = FindIFor(arg);
			if (nIndex < 0)
				return;
			UnLinkIndex(nIndex);
		}
		ITERATE_t AddTail(ARG_TYPE newElement) // a "Push"
		{
			// add to tail. alias
			return this->Add(newElement);
		}
		ITERATE_t PushTail(ARG_TYPE newElement)
		{
			// add to tail. alias
			return this->Add(newElement);
		}
		void AddHead(ARG_TYPE newElement)
		{
			// NOT a normal stack or queue. Adds are usually to the tail.
			this->InsertAt(0, newElement);
		}

		TYPE* get_DataWork() const
		{
			return this->m_pData;
		}

		void AddArray(const SUPER_t& src)
		{
			//! Append an array to the end of this.
			//! Like MFC CArray::Append()
			ASSERT_VALID(this);
			ASSERT(this != &src);   // cannot append to itself
			ITERATE_t nOldSize = this->GetSize();
			ITERATE_t nSrcSize = src.GetSize();
			if (nSrcSize > 0)
			{
				this->SetSize(nOldSize + nSrcSize);
				CopyElements<TYPE>(this->GetData() + nOldSize, src.GetData(), nSrcSize);
			}
		}

		bool IsEqualArray(const SUPER_t& aValues) const
		{
			//! Compare 2 arrays, just look for the first not same.
			//! @note Must be the same order too !
			//! @return false = different
			if (this == &aValues)
				return true;
			if (this->GetSize() != aValues.GetSize())
				return false;
			for (ITERATE_t i = 0; i < this->GetSize(); i++)
			{
				if (!(this->GetAt(i) == aValues.GetAt(i)))	// Assume everything supports the == operator.
					return false;
			}
			return true; // looks the same to me.
		}

		bool isArraySorted() const;
		bool isArraySortedND() const;	// no dupes

		void QSort()
		{
			ITERATE_t iSize = this->GetSize() - 1;
			if (iSize <= 0)
				return;
			QSort(0, iSize);
		}
	};

	template<class TYPE, class ARG_TYPE>
	bool cArrayTyped<TYPE, ARG_TYPE>::isArraySorted() const
	{
		//! Hard check for sorted. Allow dupes!
		ITERATE_t iQty = this->GetSize() - 1;
		for (ITERATE_t i = 0; i < iQty; i++)
		{
			REF_t a = this->ConstElementAt(i);
			REF_t b = this->ConstElementAt(i + 1);
			if (CompareData(a, b) > 0)
			{
				return false;
			}
		}
		return true;
	}

	template<class TYPE, class ARG_TYPE>
	bool cArrayTyped<TYPE, ARG_TYPE>::isArraySortedND() const
	{
		//! Hard check for sorted. Allow NO dupes!
		ITERATE_t iQty = this->GetSize() - 1;
		for (ITERATE_t i = 0; i < iQty; i++)
		{
			const TYPE& a = this->ConstElementAt(i);
			const TYPE& b = this->ConstElementAt(i + 1);
			if (CompareData(a, b) >= 0)
			{
				return false;
			}
		}
		return true;
	}

	template<class TYPE, class ARG_TYPE>
	ITERATE_t cArrayTyped<TYPE, ARG_TYPE>::QSortPartition(ITERATE_t iLeft, ITERATE_t iRight)
	{
		ASSERT(iLeft < iRight);
		for (;;)
		{
			// Do right side.
			while (iLeft < iRight && CompareData(this->ElementAt(iLeft), this->ElementAt(iRight)) <= COMPARE_Equal) // skip stuff already in order.
				iRight--;
			if (iLeft >= iRight)
				break;
			this->Swap(iRight, iLeft);

			// Do left side.
			while (iLeft < iRight && CompareData(this->ElementAt(iLeft), this->ElementAt(iRight)) <= COMPARE_Equal)
				iLeft++;
			if (iLeft >= iRight)
				break;
			this->Swap(iLeft, iRight);
		}
		return iLeft;	// Next mid point.
	}

	template<class TYPE, class ARG_TYPE>
	void cArrayTyped<TYPE, ARG_TYPE>::QSort(ITERATE_t iLeft, ITERATE_t iRight)
	{
		//! Re-sort- might have become unsorted for some reason.
		//! similar to std::sort()
		ITERATE_t iMid = QSortPartition(iLeft, iRight);
		if (iLeft < iMid - 1)
			QSort(iLeft, iMid - 1);
		if (iMid + 1 < iRight)
			QSort(iMid + 1, iRight);
	}

	//*************************************************

	template <class TYPE, class TYPE_ARG = TYPE*>
	class cArrayFacade : public cArrayTyped < TYPE, TYPE_ARG >
	{
		//! @class Gray::cArrayFacade
		//! An array of some type of pointer using cPtrFacade. Allow dupes.
		//! base for cArrayPtr, cArryNew, cArrayIUnk and cArrayRef

	public:
		typedef cArrayTyped<TYPE, TYPE_ARG> SUPER_t;
		typedef cArrayFacade<TYPE, TYPE_ARG> THIS_t;

		typedef typename SUPER_t::ELEM_t ELEM_t;			// GCC needs this silliness.
		typedef typename SUPER_t::REF_t REF_t;				// 

	public:
		virtual ~cArrayFacade()
		{
			//! Make sure the virtuals get called correctly.
			this->RemoveAll();
		}

		virtual COMPARE_t CompareData(REF_t pData1, REF_t pData2) const noexcept override
		{
			//! Compare a data record to another data record. Use cValT::Compare()??
			// return cValT::Compare(Data1,Data2);
			return cMem::Compare(pData1, pData2, sizeof(*pData2));
		}

		REF_t GetAt(ITERATE_t index) const // Override
		{
			return SUPER_t::ConstElementAt(index);
		}
		REF_t GetAtCheck(ITERATE_t index) const
		{
			//! Just return nullptr if index out of bounds. Safe. GetAtSafe()
			if (!SUPER_t::IsValidIndex(index))
			{
				return nullptr;
			}
			return SUPER_t::ConstElementAt(index);
		}

		TYPE PopHead()
		{
			if (!SUPER_t::GetSize())
			{
				return nullptr;
			}
			return SUPER_t::PopHead();
		}
		TYPE PopTail()
		{
			if (!SUPER_t::GetSize())
			{
				return nullptr;
			}
			return SUPER_t::PopTail();
		}
	};

	template <class TYPE>
	class cArrayPtr : public cArrayFacade < TYPE*, TYPE* >
	{
		//! @class Gray::cArrayPtr
		//! An array of some sort of dumb pointer. Pointer memory ownership is unknown. Do not free it.
		//! TYPE is allowed to be const X 

	public:
		typedef cArrayFacade<TYPE*, TYPE*> SUPER_t;
		typedef typename SUPER_t::REF_t REF_t;				// How to refer to this? value or ref or pointer?
		typedef typename SUPER_t::ELEM_t ELEM_t;			// How to refer to this? value or ref or pointer?

	public:
		void DeleteAt(ITERATE_t i)
		{
			//! Dynamic heap allocated object is deleted.
			REF_t pObj = this->GetAt(i);
			this->RemoveAt(i);
			delete pObj;
		}
		void DeleteAll()
		{
			//! Similar to RemoveAll(), DisposeAll() except it calls 'delete' to try to dereference all the entries.
			//! @note often delete has the effect of removing itself from the list. Beware of this.

			ITERATE_t iSize = this->GetSize();
			if (iSize <= 0)
				return;
			{
				// save original list, call DisposeThis on everything from original list
				SUPER_t orig;
				orig.SetCopy(*this);

				ASSERT(orig.GetSize() == iSize);
				for (ITERATE_t i = iSize - 1; i >= 0; i--)	// reverse order they got added?
				{
					REF_t pObj = orig.GetAt(i);
					if (pObj != nullptr)
					{
						delete pObj;
					}
				}
			}
			SUPER_t::RemoveAll();
		}
	};

	//*************************************************
	// cArrayVal

	template<class TYPE>
	class cArrayVal : public cArrayTyped < TYPE, TYPE >
	{
		//! @class Gray::cArrayVal
		//! An array of some simple value type that is easy to copy.
		//! Using a Reference is a waste if the object is small. Just use a copy for small objects.
	public:
		cArrayVal()
		{
		}
		explicit cArrayVal(ITERATE_t iSize) : cArrayTyped<TYPE, TYPE>(iSize)
		{
		}
	};

	//*************************************************
	// cArrayStruct

	template<class TYPE>
	class cArrayStruct : public cArrayTyped < TYPE, const TYPE& >
	{
		//! @class Gray::cArrayStruct
		//! An array of some bigger value type.
		//! maybe TYPE has constructor/destructor that does real work.
		//! should be referenced instead of copied.
	public:
		cArrayStruct() noexcept
		{
		}
		explicit cArrayStruct(ITERATE_t iSize) : cArrayTyped<TYPE, const TYPE&>(iSize)
		{
		}
	};
}
#endif	// _INC_cArray_H
