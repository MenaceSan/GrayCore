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
#include "cDebugAssert.h"		// THROW_IF_NOT

namespace Gray
{
	/// <summary>
	/// a variable size allocated, reference counted array of some data.
	/// used with cString. Variable size allocation for this. Equiv to the MFC ATL:CStringData. Variable size allocation for this.
	/// (ALWAYS) dynamically allocated
	/// </summary>
	/// <typeparam name="_TYPE"></typeparam>
	template <class _TYPE>
	class cArrayDataT : public cRefBase, public cHeapObject	// ref count and dynamic allocate
	{
		CHEAPOBJECT_IMPL;
		typedef cArrayDataT<_TYPE> THIS_t;

	protected:
		ITERATE_t m_nCount = 0;		//!< Number of elements (upperBound - 1). m_pData MUST hold at least this many
		mutable HASHCODE32_t m_HashCode;	//!< Some hash code of the data to follow. 0 = not yet calculated or empty.
		// TYPE m_a[ m_nCount * sizeof(_TYPE) ];

		static void* operator new(size_t stAllocateBlock, size_t sizePayload)
		{
			//! @note Make sure this is compatible with realloc() !!
			//! must set m_nCount after this !
			//! sizePayload = length in bytes 
			ASSERT(stAllocateBlock == sizeof(cArrayDataT));
			return cHeap::AllocPtr(stAllocateBlock + sizePayload);
		}
		cArrayDataT(ITERATE_t nCount) noexcept
			: m_nCount(nCount)
			, m_HashCode(k_HASHCODE_CLEAR)
		{
		}

	public:
		static void operator delete(void* pObj, size_t sizePayload)
		{
			UNREFERENCED_PARAMETER(sizePayload);
			cHeap::FreePtr(pObj);
		}
		static void operator delete(void* pObj)
		{
			cHeap::FreePtr(pObj);
		}

		/// <summary>
		/// allocate space and call its constructor. RefCount = 0.
		/// </summary>
		/// <param name="nCount"></param>
		/// <returns></returns>
		static THIS_t* CreateData(ITERATE_t nCount)
		{
			if (nCount == 0)
				return nullptr;
			THIS_t* p = new(nCount * sizeof(_TYPE)) THIS_t(nCount);	// allocate space 
			ASSERT(p != nullptr);
			return p;
		}
		/// <summary>
		/// call child constructors for array elements.
		/// </summary>
		/// <param name="nCount"></param>
		/// <returns></returns>
		static THIS_t* CreateData2(ITERATE_t nCount)
		{
			THIS_t* p = CreateData(nCount);	// allocate space 
			if (nCount > 0)
			{
				cValArray::ConstructElementsX(p->get_Data(), nCount);
			}
			return p;
		}

		/// <summary>
		/// Get a pointer to the payload array of TYPE. Stored in the space allocated after this class.
		/// </summary>
		/// <returns></returns>
		const _TYPE* get_Data() const noexcept
		{
			return reinterpret_cast<const _TYPE*>(this + 1);
		}
		_TYPE* get_Data() noexcept
		{
			return reinterpret_cast<_TYPE*>(this + 1);
		}

		/// <summary>
		/// get Number of array elements of _TYPE in payload. 
		/// </summary>
		/// <returns></returns>
		inline ITERATE_t get_Count() const noexcept
		{
			return m_nCount;
		}
		inline void put_Count(ITERATE_t nCount) noexcept
		{
			//! DANGER set array size . assume allocation is valid.
			m_nCount = nCount;
			m_HashCode = k_HASHCODE_CLEAR; // invalidate hash.
		}

		inline size_t get_BytesSize() const noexcept
		{
			return m_nCount * sizeof(_TYPE);
		}

		/// <summary>
		/// Get quantity of objects truly allocated. (may not be same as m_nSize or even properly aligned with TYPE)
		/// like STL capacity()
		/// </summary>
		/// <returns></returns>
		inline size_t get_BytesMalloc() const noexcept
		{
			return cHeap::GetSize(this) - sizeof(cArrayDataT);
		}
		inline ITERATE_t get_CountMalloc() const noexcept
		{
			return (ITERATE_t)(get_BytesMalloc() / sizeof(_TYPE));
		}

		inline bool IsHashCodeSet() const noexcept
		{
			return m_HashCode != k_HASHCODE_CLEAR && m_nCount > 0;
		}
		inline HASHCODE32_t get_HashCode() const
		{
			// hides get_HashCode() implemented by cRefBase
			return m_HashCode;
		}
	};

	/// <summary>
	/// An array that is contained in a single pointer like cString.
	/// NO MFC compatibility.
	/// </summary>
	/// <typeparam name="TYPE"></typeparam>
	template <class TYPE >
	class cArrayT : public cRefPtr< cArrayDataT<TYPE> >
	{
		typedef cRefPtr< cArrayDataT<TYPE> > SUPER_t;
		typedef cArrayT<TYPE> THIS_t;
		typedef cArrayDataT<TYPE> DATA_t;

		void SetCopyFrom(const DATA_t* pOld, ITERATE_t nCountNew, ITERATE_t nCountOld)
		{
			//! deep copy (and resize) the array. private reference.
			//! like CopyBeforeWrite()

			DATA_t* pNew = DATA_t::CreateData2(nCountNew);
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
		explicit cArrayT(ITERATE_t nCount) : cRefPtr(DATA_t::CreateData2(nCount))
		{
		}

		/// <summary>
		/// get sizeof() all children alloc(s). not size of *this
		/// </summary>
		/// <param name="iAllocCount"></param>
		/// <returns></returns>
		size_t GetHeapStats(OUT ITERATE_t& iAllocCount) const noexcept
		{
			DATA_t* pData = this->get_Ptr();
			if (pData == nullptr)
				return 0;
			return pData->GetHeapStatsThis(iAllocCount); // just the alloc for the array
		}

		inline ITERATE_t get_Count() const noexcept
		{
			DATA_t* pData = this->get_Ptr();
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

			DATA_t* pOld = this->get_Ptr();
			if (pOld == nullptr)
			{
				// Make a new array.
				if (nCountNew == 0)
					return;
				this->put_Ptr(DATA_t::CreateData2(nCountNew));
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

			DATA_t* pNew = nullptr;
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

				pNew = (DATA_t*)cHeap::ReAllocPtr(pOld, sizeof(DATA_t) + allocateCount * sizeof(TYPE));
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

			DATA_t* pNew = nullptr;
			DATA_t* pOld = this->get_Ptr();
			if (pOld == nullptr)
			{
				// i = 0;
				this->put_Ptr(pNew = DATA_t::CreateData2(countCopy));
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

				pNew = (DATA_t*)cHeap::ReAllocPtr(pOld, sizeof(DATA_t) + allocateCount * sizeof(TYPE));
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
