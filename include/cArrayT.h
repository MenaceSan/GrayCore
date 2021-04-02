//
//! @file cArrayT.h
//! c++ Collections.  Simple.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cArrayT_H
#define _INC_cArrayT_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cRefPtr.h"
#include "cHeapObject.h"

namespace Gray
{
	template <class TYPE>
	class cArrayDataT : public cRefBase, public cHeapObject	// ref count
	{
		//! @class Gray::cArrayDataT
		//! similar to CStringData. Variable size allocation for this.
		CHEAPOBJECT_IMPL;

	private:
		ITERATE_t m_nCount;		//!< Number of elements (upperBound - 1). m_pData MUST hold at least this many
		// TYPE m_data[ m_nCount * sizeof(TYPE) ];

	public:
		inline TYPE* get_Data() const noexcept          // get array of data
		{
			//! Get a pointer to the data. Stored in the space allocated after this class.
			return (TYPE*)(this + 1); // const_cast
		}

		static void* operator new(size_t stAllocateBlock, size_t sizePayload)
		{
			//! @note Make sure this is compatible with realloc() !!
			//! must set m_nCount after this !
			//! sizePayload = length in bytes 
			ASSERT(stAllocateBlock == sizeof(cArrayDataT));
			return cHeap::AllocPtr(stAllocateBlock + sizePayload);
		}

		static void operator delete(void* pObj, StrLen_t sizePayload)
		{
			UNREFERENCED_PARAMETER(sizePayload);
			cHeap::FreePtr(pObj);
		}
		static void operator delete(void* pObj)
		{
			cHeap::FreePtr(pObj);
		}

		inline ITERATE_t get_Count() const noexcept
		{
			//! @return Number of  
			return m_nCount;
		}
		inline void put_Count(ITERATE_t nCount) noexcept
		{
			//! DANGER
			//! @return Number 
			m_nCount = nCount;
		}
		inline ITERATE_t get_CountMalloc() const noexcept
		{
			//! Get quantity of objects truly allocated. (may not be same as m_nSize or even properly aligned with TYPE)
			//! like STL capacity()
			return (ITERATE_t)((cHeap::GetSize(this) - sizeof(cArrayDataT)) / sizeof(TYPE));
		}
	};

	template <class TYPE >
	class cArrayT : public cRefPtr< cArrayDataT<TYPE> >
	{
		//! @class Gray::cArrayT
		//! An array that is contained in a single pointer like cString.
		//! NO MFC compatibility.

		static cArrayDataT<TYPE>* CreateData(ITERATE_t nCount)
		{
			// allocate space and call its constructor
			if (nCount == 0)
				return nullptr;
			cArrayDataT<TYPE>* pDataNew = new(nCount * sizeof(TYPE)) cArrayDataT<TYPE>;	// allocate space 
			ASSERT(pDataNew != nullptr);
			pDataNew->put_Count(nCount);
			cValArray::ConstructElementsX(pDataNew->get_Data(), nCount);
			return pDataNew;
		}

		void SetCopyFrom(const cArrayDataT<TYPE>* pOld, ITERATE_t nCountNew, ITERATE_t nCountOld)
		{
			//! deep copy the array. private reference.
			//! like CopyBeforeWrite()

			cArrayDataT<TYPE>* pNew = CreateData(nCountNew);
			ASSERT_N(pNew != nullptr || nCountNew == 0);

			if (pNew != nullptr)
			{
				cValArray::CopyQty(pNew->get_Data(), pOld->get_Data(), MIN(nCountNew, nCountOld)); // Copy from old
			}

			this->put_Ptr(pNew);
		}

	public:
		cArrayT() noexcept
		{
		}
		cArrayT(ITERATE_t nCount) : cRefPtr(CreateData(nCount))
		{
		}

		size_t GetHeapStats(OUT ITERATE_t& iAllocCount) const noexcept
		{
			//! @return sizeof all children alloc(s). not size of *this
			if (this->get_Ptr() == nullptr)
				return 0;
			iAllocCount++; // just the alloc for the array
			return cHeap::GetSize(this->get_Ptr());
		}

		inline ITERATE_t get_Count() const noexcept
		{
			cArrayDataT<TYPE>* pData = this->get_Ptr();
			if (pData == nullptr)
				return 0;
			return pData->get_Count();
		}
		inline bool IsValidIndex(ITERATE_t i) const noexcept
		{
			return IS_INDEX_GOOD(i, this->get_Count());
		}
		inline bool isEmpty() const noexcept
		{
			return get_Count() == 0;
		}

		inline TYPE* get_DataWork() const noexcept
		{
			// Get a pointer to the array.
			auto p = get_Ptr();
			if (p == nullptr)
				return nullptr;
			return p->get_Data();
		}

		inline TYPE GetAt(ITERATE_t nIndex) const noexcept
		{
			DEBUG_CHECK(IsValidIndex(nIndex));
			return get_Ptr()->get_Data()[nIndex];
		}
		inline TYPE& ElementAt(ITERATE_t nIndex) noexcept
		{
			DEBUG_CHECK(IsValidIndex(nIndex));
			return get_Ptr()->get_Data()[nIndex];
		}
		inline const TYPE& ConstElementAt(ITERATE_t nIndex) const noexcept
		{
			// same as GetAt() but more explicit
			DEBUG_CHECK(IsValidIndex(nIndex));
			return get_Ptr()->get_Data()[nIndex];
		}
		inline void SetAt(ITERATE_t nIndex, const TYPE& newElement) noexcept
		{
			// If multiple refs to this then we should copy/split it ?
			// @note DANGER - this is a reference counted object. Any changes to it will make changes to all references !! Make a deep copy if required.
			DEBUG_CHECK(IsValidIndex(nIndex));
			get_Ptr()->get_Data()[nIndex] = newElement;
		}

		void AssertValidIndex(ITERATE_t nIndex) const // throw
		{
			ASSERT_THROW(IsValidIndex(nIndex));
		}
		inline TYPE& operator[](ITERATE_t nIndex) // throw
		{
			//! throw an exception if we are out of range.
			AssertValidIndex(nIndex);
			return get_Ptr()->get_Data()[nIndex];
		}
		inline const TYPE& operator[](ITERATE_t nIndex) const // throw
		{
			//! throw an exception if we are out of range.
			AssertValidIndex(nIndex);
			return get_Ptr()->get_Data()[nIndex];
		}

		ITERATE_t get_CountMalloc() const noexcept
		{
			//! Get quantity of objects truly allocated. (may not be same as m_nSize or even properly aligned with TYPE)
			//! like STL capacity()
			if (!isValidPtr())
				return 0;
			return get_Ptr()->get_CountMalloc();
		}

		void SetCopy(const cArrayT<TYPE>& a)
		{
			// deep copy the array. private reference.
			const ITERATE_t nCount = get_Count();
			SetCopyFrom(a.get_Ptr(), nCount, nCount);
		}

		void put_Count(ITERATE_t nCountNew)
		{
			// realloc my size for array.

			ASSERT(IS_INDEX_GOOD(nCountNew, cHeap::k_ALLOC_MAX)); // reasonable arbitrary limit.

			cArrayDataT<TYPE>* pOld = this->get_Ptr();
			if (pOld == nullptr)
			{
				// Make a new array.
				if (nCountNew == 0)
					return;
				this->put_Ptr(CreateData(nCountNew));
				return;
			}

			const ITERATE_t nCountOld = pOld->get_Count();
			if (nCountNew == nCountOld)		// no change.
				return;

			const int iRefCounts = pOld->get_RefCount();
			if (iRefCounts != 1)	// other refs exist. so i must make a private copy.
			{
				// Make a new array. Copy from old. So we can change size.
				// NOTE: we may be duping our self. (to change length)
				ASSERT(iRefCounts > 1);
				SetCopyFrom(pOld, nCountNew, nCountOld);
				return;
			}

			// I have an exclusive copy that no other is using.
			// just change/resize the existing heap alloc instance. 

			cArrayDataT<TYPE>* pNew = nullptr;
			if (nCountNew <= get_CountMalloc())
			{
				// it fits. don't shrink the allocated array. we may expand again some day.
				cValArray::Resize<TYPE>(pOld->get_Data(), nCountNew, nCountOld);
				pNew = pOld;
			}
			else
			{
				// otherwise, grow array
				// MFC will heuristically determine growth when nGrowBy == 0 (this avoids heap fragmentation in many situations)
				ASSERT(nCountNew > nCountOld);

				ITERATE_t allocateExtra = 0;
				if (nCountOld != 0)	// not the first time we have done this.
				{
					allocateExtra = (nCountNew / 16);
				}

				pNew = (cArrayDataT<TYPE>*)cHeap::ReAllocPtr(pOld, sizeof(cArrayDataT<TYPE>) + (nCountNew + allocateExtra) * sizeof(TYPE));
				ASSERT_N(pNew != nullptr);

				// construct new elements
				cValArray::ConstructElementsX<TYPE>(pNew->get_Data() + nCountOld, nCountNew - nCountOld);
			}

			pNew->put_Count(nCountNew);
			this->AttachPtr(pNew);		// replace realloced pointer. (if it changed at all)
		}

		// Potentially growing the array
		void SetAtGrow(ITERATE_t nIndex, const TYPE& newElement)
		{
			const ITERATE_t nCount = get_Count();
			if (nIndex >= nCount)
			{
				put_Count(nIndex + 1);	// Grow.
			}
			get_Ptr()->get_Data()[nIndex] = newElement;	// SetAt
		}
		ITERATE_t Add(const TYPE& newElement)
		{
			//! Add to the end.
			const ITERATE_t nCount = get_Count();
			SetAtGrow(nCount, newElement);
			return nCount;
		}

		void InsertAt(ITERATE_t nIndex, const TYPE& newElement)
		{
			ASSERT(nIndex >= 0);    // will expand to meet need
			const ITERATE_t nCount = get_Count();
			put_Count(nCount + 1);  // grow it to new size
			auto p = get_Ptr();
			if (nIndex < nCount)
			{
				cValArray::MoveElement(p->get_Data() + nIndex, p->get_Data() + nCount);	// make space.
			}
			// insert new value in the gap
			ASSERT(nIndex + 1 <= nCount);
			p->get_Data()[nIndex] = newElement;	// ASSUME copy constructor will be called!
		}
		void RemoveAt(ITERATE_t nIndex)
		{
			//! NOTE: Any destructor effecting the array MAY be reentrant ?!
			if (nIndex < 0)
				return;
			auto p = get_Ptr();
			if (p == nullptr)
				return;
			const ITERATE_t nCount = p->get_Count();
			ITERATE_t nMoveCount = nCount - (nIndex + 1);
			if (nMoveCount < 0)
				return;
			auto pData = p->get_Data();
			cValArray::DestructElementsX<TYPE>(pData + nIndex, 1);
			p->put_Count(nCount - 1);
			if (nMoveCount > 0) // not last.
			{
				cMem::CopyOverlap(pData + nIndex, pData + nIndex + 1, nMoveCount * sizeof(TYPE));
			}
		}
	};
}

#endif
