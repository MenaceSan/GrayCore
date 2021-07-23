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
#include "cExceptionAssert.h"		// THROW_IF_NOT

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

		typedef cRefPtr< cArrayDataT<TYPE> > SUPER_t;
		typedef cArrayT<TYPE> THIS_t;

		static cArrayDataT<TYPE>* CreateData(ITERATE_t nCount)
		{
			// allocate space and call its constructor. assume will call put_Ptr()
			if (nCount == 0)
				return nullptr;
			cArrayDataT<TYPE>* pNew = new(nCount * sizeof(TYPE)) cArrayDataT<TYPE>;	// allocate space 
			ASSERT(pNew != nullptr);
			pNew->put_Count(nCount);
			cValArray::ConstructElementsX(pNew->get_Data(), nCount);
			return pNew;
		}

		void SetCopyFrom(const cArrayDataT<TYPE>* pOld, ITERATE_t nCountNew, ITERATE_t nCountOld)
		{
			//! deep copy (and resize) the array. private reference.
			//! like CopyBeforeWrite()

			cArrayDataT<TYPE>* pNew = CreateData(nCountNew);
			ASSERT(pNew != nullptr || nCountNew == 0);

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
		explicit cArrayT(ITERATE_t nCount) : cRefPtr(CreateData(nCount))
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
		inline ITERATE_t GetSize() const noexcept
		{
			// ALIAS for MFC compat.
			return get_Count();
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
			auto p = this->get_Ptr();
			if (p == nullptr)
				return nullptr;
			return p->get_Data();
		}
		inline TYPE* GetData() const noexcept
		{
			// ALIAS for MFC compat.
			return get_DataWork();
		}

		inline const TYPE& GetAt(ITERATE_t nIndex) const noexcept
		{
			// I can make a const pointer from this and know what its life span is. NOT just a copy.
			DEBUG_CHECK(IsValidIndex(nIndex));
			return this->get_Ptr()->get_Data()[nIndex];
		}
		inline TYPE& ElementAt(ITERATE_t nIndex) noexcept
		{
			DEBUG_CHECK(IsValidIndex(nIndex));
			return this->get_Ptr()->get_Data()[nIndex];
		}
		inline void SetAt(ITERATE_t nIndex, const TYPE& newElement) noexcept
		{
			// If multiple refs to this then we should copy/split it ?
			// @note DANGER - this is a reference counted object. Any changes to it will make changes to all references !! Make a deep copy if required.
			DEBUG_CHECK(IsValidIndex(nIndex));
			this->get_Ptr()->get_Data()[nIndex] = newElement;
		}

		void AssertValidIndex(ITERATE_t nIndex) const // throw
		{
			THROW_IF_NOT(IsValidIndex(nIndex));
		}
		inline TYPE& operator[](ITERATE_t nIndex) // throw
		{
			//! throw an exception if we are out of range.
			AssertValidIndex(nIndex);
			return this->get_Ptr()->get_Data()[nIndex];
		}
		inline const TYPE& operator[](ITERATE_t nIndex) const // throw
		{
			//! throw an exception if we are out of range.
			AssertValidIndex(nIndex);
			return this->get_Ptr()->get_Data()[nIndex];
		}

		ITERATE_t get_CountMalloc() const noexcept
		{
			//! Get quantity of objects truly allocated. (may not be same as m_nSize or even properly aligned with TYPE)
			//! like STL capacity()
			if (!this->isValidPtr())
				return 0;
			return this->get_Ptr()->get_CountMalloc();
		}

		void SetCopy(const cArrayT<TYPE>& a)
		{
			// deep copy the array. private reference.
			const ITERATE_t nCount = get_Count();
			SetCopyFrom(a.get_Ptr(), nCount, nCount);
		}

		inline static ITERATE_t GetCountMalloc(ITERATE_t i)
		{
			//! over allocate to allow room to grow.
			return i + (i / 16);
		}

		void put_Count(ITERATE_t nCountNew)
		{
			// realloc my size for array. all reference counts will see this change!

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

				ITERATE_t allocateCount = nCountNew;
				if (nCountOld != 0)	// not the first time we have done this.
				{
					allocateCount = GetCountMalloc(allocateCount);
				}

				pNew = (cArrayDataT<TYPE>*)cHeap::ReAllocPtr(pOld, sizeof(cArrayDataT<TYPE>) + allocateCount * sizeof(TYPE));
				ASSERT_NN(pNew);

				// construct new elements
				cValArray::ConstructElementsX<TYPE>(pNew->get_Data() + nCountOld, nCountNew - nCountOld);
			}

			pNew->put_Count(nCountNew);
			this->AttachPtr(pNew);		// replace realloced pointer. (if it changed at all) No ref count change.
		}

		void SetAtGrow(ITERATE_t nIndex, const TYPE& newElement)
		{
			const ITERATE_t nCountOld = get_Count();
			if (nIndex >= nCountOld)
			{
				// must grow.
				put_Count(nIndex + 1);
			}
			SetAt(nIndex, newElement);
		}

		ITERATE_t Add(const TYPE& newElement)
		{
			//! Add to the end. AKA push_back()
			const ITERATE_t nCountOld = get_Count();
			put_Count(nCountOld + 1);	// Grow.
			SetAt(nCountOld, newElement);
			return nCountOld;
		}

		void InsertAt(ITERATE_t nIndex, const TYPE& newElement)
		{
			// newElement as interior pointer is ok.
			ASSERT(nIndex >= 0);    // will expand to meet need
			const ITERATE_t nCountOld = get_Count();
			if (nIndex >= nCountOld)
			{
				put_Count(nIndex + 1);  // grow it to new size
			}
			else
			{
				put_Count(nCountOld + 1);  // grow it to new size
				TYPE* pData = this->get_Ptr()->get_Data();
				cValArray::MoveElement1(pData + nCountOld, pData + nIndex);	// make space.
			}

			// insert new value in the gap
			SetAt(nIndex, newElement);	
		}

		void InsertArray(ITERATE_t i, const TYPE* pCopy, ITERATE_t countCopy)
		{
			//! Add array to the end. 
			//! Like MFC CArray::Append() ?
			if (countCopy <= 0)
				return;

			cArrayDataT<TYPE>* pNew = nullptr;
			cArrayDataT<TYPE>* pOld = this->get_Ptr();
			if (pOld == nullptr)
			{
				// i = 0;
				this->put_Ptr(pNew = CreateData(countCopy));
			}
			else
			{
				const ITERATE_t nCountOld = pOld->get_Count();
				if (i < 0 || i > nCountOld)
					i = nCountOld;

				ASSERT(!cMem::IsInsideBlock(pCopy, pOld->get_Data() + i, nCountOld - i));   // append to self not supported.

				const int iRefCounts = pOld->get_RefCount();
				if (iRefCounts != 1)
				{
					// TODO MUST MAKE PRIVATE COPY !!!
				}

				ITERATE_t nCountNew = nCountOld + countCopy;		// new size.
				ITERATE_t allocateCount = GetCountMalloc(nCountNew);

				pNew = (cArrayDataT<TYPE>*)cHeap::ReAllocPtr(pOld, sizeof(cArrayDataT<TYPE>) + allocateCount * sizeof(TYPE));
				ASSERT_NN(pNew);
				pNew->put_Count(nCountNew);
				this->AttachPtr(pNew);		// replace realloced pointer. (if it changed at all) No ref count change.

				TYPE* pData = pNew->get_Data();
				// Move existing elements.
				cMem::CopyOverlap(pData + i + countCopy, pData + i, (nCountOld - i) * sizeof(TYPE));
				// construct new elements
				cValArray::ConstructElementsX<TYPE>(pData + i, countCopy);
			}

			if (pCopy != nullptr)
			{
				// Copy new.
				cValArray::CopyQty(pNew->get_Data() + i, pCopy, countCopy);
			}
		}
		void InsertArray(ITERATE_t i, const THIS_t& src)
		{
			InsertArray(i, src.get_DataWork(), src.GetSize());
		}

		void RemoveAt(ITERATE_t nIndex)
		{
			//! NOTE: Any destructor effecting the array MAY be reentrant ?!
			if (nIndex < 0)
				return;
			auto p = this->get_Ptr();
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
