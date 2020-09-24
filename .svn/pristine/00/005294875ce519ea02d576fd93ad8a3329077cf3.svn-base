//
//! @file CHeapObject.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CHeapObject_H
#define _INC_CHeapObject_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CHeap.h" // CHeap

#if defined(_DEBUG) || defined(_DEBUG_FAST)
#define USE_HEAPSIG
#endif

namespace Gray
{
	DECLARE_INTERFACE(IHeapObject)
	{
		//! @interface Gray::IHeapObject
		//! This is a base interface supported by objects/classes that are ALWAYS assumed allocated on the heap.
		//! Use this because multiple inheritance can hide my top heap (freeable) pointer.
		//! Top should implement some version of CHeapObject. e.g. "x = new CXObject"

		IGNORE_WARN_INTERFACE(IHeapObject);

		virtual const void* get_HeapPtr() const noexcept = 0;	//!< Get the top level (outermost, freeable) class pointer. I can delete get_HeapPtr().

		// Add this to each IHeapObject rooted object to get the base heap allocation pointer. Avoids problems with multiple inheritance and heap allocated objects.
#define CHEAPOBJECT_IMPL virtual const void* get_HeapPtr() const noexcept override { return this; }
	};

	//*************************************************

	class GRAYCORE_LINK CHeapObject : public IHeapObject
	{
		//! @class Gray::CHeapObject
		//! The base of some class/struct object that is ALWAYS heap allocated.
		//! This item MUST always be dynamically allocated with new/delete!
		//! Never stack (auto) or data segment (static) based.
		//! get_HeapPtr() must be declared by the highest level ! since derived classes wrap their base classes.
		//! multiple inheritance can disguise the base allocated pointer

	protected:
		// Get the top level malloc() pointer in the case of multiple inheritance.
		CHEAPOBJECT_IMPL;
#ifdef USE_HEAPSIG
		CMemSignature<> m_Sig;	//!< may want to have multiple of these ?
#endif

	public:
		CHeapObject()
		{
			//! @note virtuals don't work in constructors or destructors !
		}
		virtual ~CHeapObject()
		{
			//! @note virtuals do not work in destructors ! get_HeapPtr?
			//! so isValidCheck() not possible here !
		}
		bool IsValidInsideN(INT_PTR index) const
		{
			//! Is index a valid offset inside this object?
#ifdef USE_HEAPSIG
			if (!m_Sig.isValidSignature())
				return false;
#endif
			const void* pBase = get_HeapPtr();
			return CHeapAlign::IsValidInside(pBase, index);
		}
		bool IsValidInsidePtr(void const* pTest) const
		{
			//! Is pTest a valid pointer inside the this object ?
			if (pTest == nullptr)
				return false;
#ifdef USE_HEAPSIG
			if (!m_Sig.isValidSignature())
				return false;
#endif
			const void* pBase = get_HeapPtr();
			return CHeapAlign::IsValidInside(pBase, CMem::Diff(pTest, pBase));
		}
		virtual size_t GetHeapStatsThis(OUT ITERATE_t& iAllocCount) const
		{
			//! Not the same as GetHeapStats(). size of *this as opposed to size of children.
#ifdef USE_HEAPSIG
			ASSERT(m_Sig.isValidSignature());
#endif
			iAllocCount++;
			return CHeapAlign::GetSize(get_HeapPtr());
		}
		virtual bool isValidCheck() const
		{
			//! @note can't call this in a destructor since get_HeapPtr() is virtual.
			if (!CMem::IsValidApp(this))	// NOT be based on nullptr ? sanity check.
				return false;
#ifdef USE_HEAPSIG
			if (!m_Sig.isValidSignature())
				return false;
#endif
			if (!CHeapAlign::IsValidHeap(get_HeapPtr()))	// might be aligned.
				return false;
			return true;
		}
	};

	// Create an ^2 aligned pool for allocation of these objects.
#define DECLARE_HEAP_ALIGNED_ALLOCN(_CLASS,_IALIGN) public: \
	static void* operator new( size_t nCount) \
	{ return CHeapAlign::AllocPtr( nCount, _IALIGN ); } \
	static void operator delete(void* pData) \
	{ CHeapAlign::FreePtr(pData); }

#define DECLARE_HEAP_ALIGNED_ALLOC(_CLASS) DECLARE_HEAP_ALIGNED_ALLOCN(_CLASS,__alignof( _CLASS ))
};

#endif // _INC_CHeapObject_H
