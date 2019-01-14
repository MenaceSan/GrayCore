//
//! @file CList.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CList_H
#define _INC_CList_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"
#include "HResult.h"
#include "CUnitTestDecl.h"
#include "CDebugAssert.h"
#include "CHeapObject.h"

UNITTEST_PREDEF(CListBase)

namespace Gray
{
	class CListBase;

	class GRAYCORE_LINK CListNodeBase : public CHeapObject
	{
		//! @class Gray::CListNodeBase
		//! base class for a single node in a CListBase.
		//! derive a class from CListNodeBase to be a node member in a CListBase.
		//! Single owner = This item belongs to JUST ONE CListBase (m_pParent)
		//! Double linked list.
		//! NOT circular. head and tail are nullptr.
		friend class CListBase; // so m_pNext and m_pPrev can be manipulated directly.

	private:
		CListBase* m_pParent;		//!< link me back to my parent object.
		CListNodeBase* m_pNext;	//!< next sibling
		CListNodeBase* m_pPrev;	//!< previous sibling

	protected:
		virtual void put_Parent(CListBase* pParent)
		{
			//! I am being added to a list. (or nullptr = no list)
			ASSERT(m_pParent == nullptr || pParent == nullptr || m_pParent == pParent);
			m_pParent = pParent;		// link me back to my parent object.
		}

	protected:
		CListNodeBase() 	// always just a base class.
			: m_pParent(nullptr)	// not linked yet.
			, m_pNext(nullptr)
			, m_pPrev(nullptr)
		{
		}

	public:
		virtual ~CListNodeBase()
		{
			//! ASSUME: RemoveFromParent() was already called! (virtuals don't work in destruct!)
			ASSERT(!hasParent());
		}

		CListBase* get_Parent() const
		{
			return m_pParent;
		}
		CListNodeBase* get_Next() const
		{
			return m_pNext;
		}
		CListNodeBase* get_Prev() const
		{
			return m_pPrev;
		}

		bool hasParent() const
		{
			//! is this in a list?
			if (m_pParent != nullptr)
			{
				return true;
			}
			ASSERT(m_pNext == nullptr);
			ASSERT(m_pPrev == nullptr);
			return false;
		}

		//! remove ListNode from List.
		//! @note Must define body of this function after CListBase has been defined.
		void RemoveFromParent();

		virtual HRESULT DisposeThis()	// delete myself from the system.
		{
			//! Pre-destructor cleanup things can be done since virtuals don't work in destructors.
			//! @note this does not free the memory. override to do this.
			RemoveFromParent();	// called before destruct so virtual RemoveListNode() is called correctly.
			return S_OK;
		}
	};

	//*************************************************

	class GRAYCORE_LINK CListBase	// generic list of objects based on CListNodeBase.
	{
		//! @class Gray::CListBase
		//! Double linked list. NOT circular. head and tail are nullptr.
		//! @note Lists are primarily used if inserts and deletes for large sets occurs frequently.
		//! Objects should remove themselves from the list when deleted.
		//! Similar to the MFC CList, or std::list<T>, std::deque

		friend class CListNodeBase; // so it can call RemoveListNode() to remove self.

	protected:
		ITERATE_t m_iCount;		//!< how many children? nice to get read only direct access to this for scripting.
	private:
		CListNodeBase* m_pHead;	//!< Head of my list.
		CListNodeBase* m_pTail;	//!< Tail of my list.

	protected:
		//! Override this to get called when an item is removed from this list.
		//! Never called directly. ALWAYS called from pObRec->RemoveFromParent()
		virtual void RemoveListNode(CListNodeBase* pNode);	//!< allow Override of this. called when child pObRec removed from list.

	public:
		CListBase()
			: m_iCount(0)
			, m_pHead(nullptr)
			, m_pTail(nullptr)
		{
		}
		virtual ~CListBase()
		{
			//! @note virtuals do not work in destructors !
			//! ASSUME: DisposeAll() or Empty() is called from higher levels
			//!  it is important for virtual RemoveListNode() callback
			ASSERT(isEmpty());
		}

		//! Override this to check items being added.
		//! pPrev = nullptr = first
		virtual void InsertListNode(CListNodeBase* pNodeNew, CListNodeBase* pNodePrev = nullptr);
		void InsertList(CListBase* pListSrc, CListNodeBase* pNodePrev = nullptr);

		void InsertBefore(CListNodeBase* pNodeNew, CListNodeBase* pNodeNext)
		{
			//! @arg pNext = nullptr = insert last
			InsertListNode(pNodeNew, (pNodeNext != nullptr) ? (pNodeNext->get_Prev()) : get_Tail());
		}
		void InsertHead(CListNodeBase* pNodeNew)
		{
			InsertListNode(pNodeNew, nullptr);
		}
		void InsertTail(CListNodeBase* pNodeNew)
		{
			InsertListNode(pNodeNew, get_Tail());
		}

		void DisposeAll();
		void Empty();

		CListNodeBase* get_Head(void) const
		{
			return m_pHead;
		}
		CListNodeBase* get_Tail(void) const
		{
			return m_pTail;
		}
		ITERATE_t get_Count() const
		{
			return m_iCount;
		}
		bool isEmpty() const
		{
			return(!get_Count());
		}

		//! iterate the linked list.
		CListNodeBase* GetAt(ITERATE_t index) const;

		bool IsMyChild(const CListNodeBase* pNode) const
		{
			if (pNode == nullptr)
				return false;
			return(pNode->get_Parent() == this);
		}

		UNITTEST_FRIEND(CListBase);
	};

	//*************************************************

	inline void CListNodeBase::RemoveFromParent()
	{
		//! Remove this(myself) from my parent list (if i have one)
		if (m_pParent != nullptr)
		{
			m_pParent->RemoveListNode(this); // only call this from here !
			// ASSERT( m_pParent != pParentPrev );	// We are now unlinked. (or deleted)
		}
	}

	//*************************************************
	// template type casting for lists.

	template<class _TYPE_REC = CListNodeBase>
	class CListNodeT : public CListNodeBase
	{
		//! @class Gray::CListNodeT
		//! Assume this is a node of type _TYPE_REC. e.g. _TYPE_REC is based on CListNodeT<> which is based on CListNodeBase.
		typedef CListNodeBase SUPER_t;
	public:
		/*
		operator _TYPE_REC*()
		{
			//! This shouldn't really be needed.
			return static_cast<_TYPE_REC*>(this);
		}
		*/
		_TYPE_REC* get_Next() const
		{
			//! _TYPE_REC cast version of get_Next
			return static_cast<_TYPE_REC*>(SUPER_t::get_Next());
		}
		_TYPE_REC* get_Prev() const
		{
			//! _TYPE_REC cast version of get_Next
			return static_cast<_TYPE_REC*>(SUPER_t::get_Prev());
		}
	};

	template<class _TYPE_REC /* = CListNodeBase */ >
	class CListT : public CListBase
	{
		//! @class Gray::CListT
		//! Hold a List of _TYPE_REC things.
		//! @note _TYPE_REC is the type of class this list contains. _TYPE_REC is based on CListNodeT<_TYPE_REC> and/or CListNodeBase
		typedef CListBase SUPER_t;
		typedef CListNodeT<_TYPE_REC> NODEBASE_t;
	public:
		_TYPE_REC* GetAt(ITERATE_t index) const
		{
			//! iterate the linked list.
			return static_cast<_TYPE_REC*>(SUPER_t::GetAt(index));
		}
		_TYPE_REC* get_Head() const
		{
			return static_cast<_TYPE_REC*>(SUPER_t::get_Head());
		}
		_TYPE_REC* get_Tail() const
		{
			return static_cast<_TYPE_REC*>(SUPER_t::get_Tail());
		}
	};
};
#endif // _INC_CList_H
