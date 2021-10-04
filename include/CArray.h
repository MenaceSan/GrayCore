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
#include "cValArray.h"
#include "cExceptionAssert.h"		// THROW_IF_NOT
#ifdef _MFC_VER
#include <afxtempl.h>	// CArray from _MFC_VER 
#endif

namespace Gray
{
	// similar to BOOST_FOREACH()
#define GRAY_FOREACH( _ITYPE, i, a ) for ( ITERATE_t index_##i=0, _foreach_int=true; index_##i < a.GetSize(); index_##i++, _foreach_int=true ) \
	for ( _ITYPE i=a[index_##i]; _foreach_int; _foreach_int=false )

#ifndef _MFC_VER

	template <class TYPE, class ARG_TYPE = const TYPE& >
	class CArray : public CObject
	{
		//! @class Gray::CArray
		//! Minimal array template of elements. like MFC version. use cArrayTyped<> instead !
		//! @note MFC 8.0 uses INT_PTR for GetSize()
		//! TYPE = what is stored.
		//! ARG_TYPE = const TYPE& or TYPE depending on what makes sense for SetAt() and Add operations.

		typedef CArray<TYPE, ARG_TYPE> THIS_t;

	protected:
		TYPE* m_pData;			//!< the actual array of data
		ITERATE_t m_nSize;		//!< Number of elements (upperBound - 1). m_pData MUST hold at least this many. 

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

		bool IsValidIndex(ITERATE_t i) const noexcept
		{
			return IS_INDEX_GOOD(i, this->m_nSize);
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
		ITERATE_t get_CountMalloc() const noexcept
		{
			//! Get quantity of objects truly allocated. (may not be same as m_nSize or even properly aligned with TYPE)
			//! like STL capacity()
			return (ITERATE_t)(cHeap::GetSize(m_pData) / sizeof(TYPE));
		}

		constexpr static ITERATE_t GetCountMalloc(ITERATE_t i)
		{
			//! over allocate to allow room to grow.
			return i + (i / 16);
		}

		// Operations
		//! Clean up
		void RemoveAll();

		// Accessing elements
		const TYPE& GetAt(ITERATE_t nIndex) const noexcept
		{
			// I can make a const pointer from this. NOT just a copy.
			DEBUG_CHECK(IsValidIndex(nIndex));
			return m_pData[nIndex];
		}
		TYPE& ElementAt(ITERATE_t nIndex) 
		{
			DEBUG_CHECK(IsValidIndex(nIndex));
			return m_pData[nIndex];
		}
		void SetAt(ITERATE_t nIndex, ARG_TYPE newElement) // throw
		{
			//! @note Destructor is automatically called.
			DEBUG_CHECK(IsValidIndex(nIndex));
			m_pData[nIndex] = newElement;	// may call a copy constructor.
		}

		// Direct Access to the element data (may return nullptr)
		inline const TYPE* GetData() const
		{
			// DANGER.
			return m_pData;
		}
		inline TYPE* GetData()
		{
			// Get a pointer that i might be able to change directly.
			// DANGER.
			return m_pData;
		}
		void SetDataArrayPtr(TYPE* pData, ITERATE_t nSize)
		{
			//! set internal pointer. (dangerous)
			ASSERT(!cMem::IsCorrupt(pData, nSize, false));
			m_pData = pData;
			m_nSize = nSize;
		}

		// Potentially growing the array
		void SetAtGrow(ITERATE_t nIndex, ARG_TYPE newElement);
		ITERATE_t Add(ARG_TYPE newElement)
		{
			//! Add to the end. AKA push_back(), Push()
			const ITERATE_t nIndex = m_nSize;
			SetAtGrow(nIndex, newElement);
			return nIndex;
		}

		void Copy(const CArray& src);

		// overloaded operator helpers
		inline TYPE& operator[](ITERATE_t nIndex) // throw
		{
			return ElementAt(nIndex);
		}
		inline const TYPE& operator[](ITERATE_t nIndex) const // throw
		{
			return GetAt(nIndex);
		}

		// Operations that move elements around
		void InsertAt(ITERATE_t nIndex, ARG_TYPE newElement);
		void RemoveAt(ITERATE_t nIndex);
		void RemoveAt(ITERATE_t nIndex, ITERATE_t iQty);

		void MoveElement(ITERATE_t iFrom, ITERATE_t iTo); // throw

		void InsertArray(ITERATE_t i, const TYPE* pCopy, ITERATE_t countCopy);
	};

	//************************************************************************

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::InsertArray(ITERATE_t i, const TYPE* pCopy, ITERATE_t countCopy)
	{
		//! Insert an array into this array.
		//! Like MFC CArray::Append() ? sort of.

		ASSERT_VALID(this);

		if (countCopy <= 0)
			return;
		const ITERATE_t nCountOld = this->GetSize();
		if (i < 0 || i > nCountOld)
			i = nCountOld;

		ASSERT(!cMem::IsInsideBlock(pCopy, this->GetData() + i, nCountOld - i));   // append to self not supported.

		const ITERATE_t nCountNew = nCountOld + countCopy;		// new size.
		const ITERATE_t allocateCount = GetCountMalloc(nCountNew);
		m_pData = (TYPE*)cHeap::ReAllocPtr(m_pData, allocateCount * sizeof(TYPE));
		ASSERT_NN(m_pData);
		m_nSize = nCountNew;

		// Move existing elements.
		cMem::CopyOverlap(m_pData + i + countCopy, m_pData + i, (nCountOld - i) * sizeof(TYPE));
		// construct new elements
		cValArray::ConstructElementsX<TYPE>(m_pData + i, countCopy);
		if (pCopy != nullptr)
		{
			// Copy over new.
			cValArray::CopyQty(m_pData + i, pCopy, countCopy);
		}
	}

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
			cValArray::DestructElementsX<TYPE>(m_pData, iSize);
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
		if (nNewSize <= get_CountMalloc())
		{
			// it fits. don't shrink the allocated array. we may expand again some day.
			cValArray::Resize<TYPE>(m_pData, nNewSize, m_nSize);
		}
		else
		{
			// otherwise, grow array
			// MFC will heuristically determine growth when nGrowBy == 0 (this avoids heap fragmentation in many situations)

			ASSERT(nNewSize > m_nSize);
			ITERATE_t allocateCount = nNewSize;
			if (m_nSize != 0)	// not the first time we have done this.
			{
				allocateCount = GetCountMalloc(allocateCount);
			}

			m_pData = (TYPE*)cHeap::ReAllocPtr(m_pData, allocateCount * sizeof(TYPE));
			ASSERT_NN(m_pData);

			// construct new elements
			cValArray::ConstructElementsX<TYPE>(&m_pData[m_nSize], nNewSize - m_nSize);
		}
		m_nSize = nNewSize;
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::Copy(const CArray& src)
	{
		ASSERT_VALID(this);
		ASSERT(this != &src);   // ASSUME Not copy to self.
		SetSize(src.m_nSize);
		cValArray::CopyQty<TYPE>(m_pData, src.m_pData, src.m_nSize);
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::SetAtGrow(ITERATE_t nIndex, ARG_TYPE newElement)
	{
		ASSERT_VALID(this);
		if (nIndex >= m_nSize)
		{
			// must grow.
			SetSize(nIndex + 1);
		}
		SetAt(nIndex, newElement);
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::MoveElement(ITERATE_t iFrom, ITERATE_t iTo) // throw
	{
		//! shift the whole array. move an index to another place.
		//! Similar to Swap() but only one element is moved. (sort of)
		//! dangerous for types that have internal pointers !
		ASSERT(IsValidIndex(iFrom));
		ASSERT(IsValidIndex(iTo));
		cValArray::MoveElement1(&m_pData[iFrom], &m_pData[iTo]);
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::InsertAt(ITERATE_t nIndex, ARG_TYPE newElement)
	{
		//! Insert at this location, move anything after this.
		// newElement as interior pointer is ok.

		ASSERT_VALID(this);
		ASSERT(nIndex >= 0);    // will expand to meet need

		const ITERATE_t nCountOld = m_nSize;
		if (nIndex >= nCountOld)
		{
			// adding after the end of the array
			SetSize(nIndex + 1);   // grow so nIndex is valid
		}
		else
		{
			// inserting in the middle of the array
			SetSize(m_nSize + 1);  // grow it to new size
			// destroy initial data before copying over it (inefficient i know but is very convenient)
			MoveElement(nCountOld, nIndex);
		}

		// insert new value in the gap
		SetAt(nIndex, newElement);	// ASSUME copy constructor will be called!
	}

	template <class TYPE, class ARG_TYPE>
	void CArray<TYPE, ARG_TYPE>::RemoveAt(ITERATE_t nIndex)
	{
		//! NOTE: Any destructor effecting the array MAY be reentrant ?!
		ASSERT_VALID(this);
		if (nIndex < 0)
			return;
		const ITERATE_t nMoveCount = m_nSize - (nIndex + 1);
		if (nMoveCount < 0)
			return;
		cValArray::DestructElementsX<TYPE>(&m_pData[nIndex], 1);
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
		cValArray::DestructElementsX<TYPE>(&m_pData[nIndex], iQty);
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
			if (m_nSize > get_CountMalloc())	// NOTE: get_CountMalloc will check m_pData
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
		//! @note Use this instead of CArray directly.
		//! TYPE = the type stored. ARG_TYPE = how it should be referenced. const TYPE&	typically
	public:
		typedef CArray<TYPE, ARG_TYPE> SUPER_t;
		typedef cArrayTyped<TYPE, ARG_TYPE> THIS_t;

		typedef ITERATE_t iterator;			// like STL
		typedef ITERATE_t const_iterator;	// like STL

	protected:
		virtual COMPARE_t CompareData(ARG_TYPE Data1, ARG_TYPE Data2) const noexcept
		{
			//! Compare a data record to another data record.
			//! ASSUME this is the same as comparing keys. Otherwise you must overload this.
			//! Default implementation. Override this for proper implementation. This probably won't work for most cases.
			// return cValT::Compare(Data1,Data2);
			return cMem::Compare(&Data1, &Data2, sizeof(Data1));	// should we use use cValT::Compare()??
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
			cValArray::CopyQty(this->GetData(), rArray.GetData(), this->GetSize());
		}
		explicit cArrayTyped(ITERATE_t iSize)
		{
			// set Size to iSize empty entries.
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

		size_t GetHeapStats(OUT ITERATE_t& iAllocCount) const noexcept
		{
			//! @return sizeof all children alloc(s). not size of *this
			if (this->m_pData == nullptr)
				return 0;
			iAllocCount++; // just the alloc for the array
			return cHeap::GetSize(this->m_pData);
		}

		// New
		inline bool IsValidIndex(ITERATE_t i) const noexcept
		{
			return IS_INDEX_GOOD(i, this->m_nSize);
		}
		inline ITERATE_t ClampValidIndex(ITERATE_t i) const noexcept
		{
			//! @return -1 = empty array.
			if (i < 0)
				i = 0;
			if (i >= this->m_nSize)
				return this->m_nSize - 1;
			return i;
		}
		void AssertValidIndex(ITERATE_t nIndex) const // throw
		{
			THROW_IF_NOT(IsValidIndex(nIndex));
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

		inline TYPE& operator[](ITERATE_t nIndex) // throw
		{
			// AKA GetAtSecure
			AssertValidIndex(nIndex);
			return this->ElementAt(nIndex);
		}
		inline const TYPE& operator[](ITERATE_t nIndex) const // throw
		{
			// AKA ElementAtSecure
			// Const array should return const values. GetAt()
			AssertValidIndex(nIndex);
			return this->m_pData[nIndex];
		}

#if 0
		TYPE& Head() // throw
		{
			// aka front()
			return this->ElementAt(0);
		}
		TYPE& Tail()
		{
			// AKA back()
			return this->ElementAt(this->GetSize() - 1);
		}
#endif

		const TYPE& GetAtHead() const
		{
			// aka front()
			return this->GetAt(0);
		}
		const TYPE& GetAtTail() const
		{
			// AKA back()
			return this->GetAt(this->GetSize() - 1);
		}

		void UnLinkIndex(ITERATE_t nIndex)
		{
			//! DANGER. Remove the object from the list. DO NOT call its normal destructor!
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
			cValArray::CopyQty(this->GetData(), aValues.GetData(), this->GetSize());
		}

		const cArrayTyped<TYPE, ARG_TYPE>& operator = (const cArrayTyped<TYPE, ARG_TYPE>& aValues)
		{
			SetCopy(aValues);
			return *this;
		}

		ITERATE_t FindIFor(ARG_TYPE arg) const noexcept
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
		bool HasArg(ARG_TYPE arg) const noexcept
		{
			//! Find the index of a specified value entry match.
			return FindIFor(arg) >= 0;
		}

		void RemoveLast()
		{
			this->RemoveAt(this->GetSize() - 1);
		}
		TYPE PopHead()
		{
			// pop from front of queue.
			ASSERT(this->GetSize() > 0);
			TYPE tmp = this->GetAt(0);	// copy it.
			this->RemoveAt(0);
			return tmp;
		}
		TYPE PopTail()
		{
			// pop from top of stack.
			// AKA Pop()
			ASSERT(this->GetSize() > 0);
			ITERATE_t i = this->GetSize() - 1;
			TYPE tmp = this->GetAt(i);	// copy it.
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
			// DANGER. remove without call to destructor.
			ITERATE_t nIndex = FindIFor(arg);
			if (nIndex < 0)
				return;
			UnLinkIndex(nIndex);
		}

		void AddHead(ARG_TYPE newElement)
		{
			// NOT a normal stack or queue. Adds are usually to the tail.
			SUPER_t::InsertAt(0, newElement);
		}
		void InsertArray(ITERATE_t i, const TYPE* pCopy, ITERATE_t countCopy)
		{
#ifdef _MFC_VER
			ASSERT(0);
			// SUPER_t::InsertAt(i, pCopy, countCopy);
#else
			SUPER_t::InsertArray(i, pCopy, countCopy);
#endif
		}
		void InsertArray(ITERATE_t i, const THIS_t& src)
		{
			// Emulate MFC. but also resolve overload conflict.
			InsertArray(i, src.GetData(), src.GetSize());
		}

		TYPE* get_DataWork() const noexcept
		{
			return this->m_pData;
		}


		bool IsEqualArray(const SUPER_t& aValues) const
		{
			//! Compare 2 arrays, just look for the first not same.
			//! Assume everything supports the == operator.
			//! @note Must be the same order too !
			//! @return false = different

			if (this->GetSize() != aValues.GetSize())
				return false;
			return cValArray::IsEqualQty(this->GetData(), aValues.GetData(), aValues.GetSize());
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
			const TYPE& a = this->GetAt(i);
			const TYPE& b = this->GetAt(i + 1);
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
			const TYPE& a = this->GetAt(i);
			const TYPE& b = this->GetAt(i + 1);
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

	template <class TYPE, class ARG_TYPE = TYPE*>
	class cArrayFacade : public cArrayTyped < TYPE, ARG_TYPE >
	{
		//! @class Gray::cArrayFacade
		//! An array of some type of pointer using cPtrFacade. Allow dupes.
		//! base for cArrayPtr, cArryNew, cArrayIUnk and cArrayRef
		//! TYPE = some cPtrFacade derived

	public:
		typedef cArrayTyped<TYPE, ARG_TYPE> SUPER_t;
		typedef cArrayFacade<TYPE, ARG_TYPE> THIS_t;

	public:
		virtual ~cArrayFacade()
		{
			//! Make sure the virtuals get called correctly.
			this->RemoveAll();
		}

		virtual COMPARE_t CompareData(ARG_TYPE pData1, ARG_TYPE pData2) const noexcept override
		{
			//! Compare a data record to another data record. Use cValT::Compare()??
			// return cValT::Compare(Data1,Data2);
			return cMem::Compare(pData1, pData2, sizeof(*pData2));
		}

#if 0
		const TYPE& GetAt(ITERATE_t index) const
		{
			// Cast to pointer . helper.
			return SUPER_t::GetAt(index);
		}
#endif
		TYPE GetAtCheck(ITERATE_t index) const
		{
			//! Just return nullptr if index out of bounds. Safe. GetAtSafe()
			if (!SUPER_t::IsValidIndex(index))
			{
				return nullptr;
			}
			return SUPER_t::GetAt(index);
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

	public:
		void DeleteAt(ITERATE_t i)
		{
			//! Dynamic heap allocated object is deleted.
			TYPE* pObj = this->GetAt(i);	// make copy.
			this->RemoveAt(i);
			delete pObj;
		}
		void DeleteAll()
		{
			//! Similar to RemoveAll(), DisposeAll() except it calls 'delete' to try to dereference all the entries.
			//! @note often delete has the effect of removing itself from the list. Beware of this.

			const ITERATE_t iSize = this->GetSize();
			if (iSize <= 0)
				return;
			{
				// save original list, call DisposeThis on everything from original list
				SUPER_t orig;
				orig.SetCopy(*this);

				ASSERT(orig.GetSize() == iSize);
				for (ITERATE_t i = iSize - 1; i >= 0; i--)	// reverse order they got added?
				{
					TYPE* pObj = orig.GetAt(i);
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
		cArrayVal() noexcept
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
