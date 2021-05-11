//
//! @file cList.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cList_H
#define _INC_cList_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
 
#include "HResult.h"
#include "cDebugAssert.h"
#include "cHeapObject.h"

namespace Gray
{
	class GRAYCORE_LINK cListNodeBase : public cHeapObject
	{
		//! @class Gray::cListNodeBase
		//! base class for a single node in a cListBase. AKA 
		//! derive a class from cListNodeBase to be a node member in a cListBase.
		//! Single owner = This item belongs to JUST ONE cListBase (m_pParent)
		//! Double linked list.
		//! NOT circular. head and tail are nullptr.
		
		friend class cListBase; // so m_pNext and m_pPrev can be manipulated directly.

	private:
		cListBase* m_pParent;		//!< link me back to my parent object.
		cListNodeBase* m_pNext;		//!< next sibling
		cListNodeBase* m_pPrev;		//!< previous sibling

	protected:
		virtual void put_Parent(cListBase* pParent)
		{
			//! I am being added to a list. (or nullptr = no list)
			ASSERT(m_pParent == nullptr || pParent == nullptr || m_pParent == pParent);
			m_pParent = pParent;		// link me back to my parent object.
		}

	protected:
		cListNodeBase() noexcept 	// always just a base class.
			: m_pParent(nullptr)	// not linked yet.
			, m_pNext(nullptr)
			, m_pPrev(nullptr)
		{
		}

	public:
		virtual ~cListNodeBase() noexcept
		{
			//! ASSUME: RemoveFromParent() was already called! (virtuals don't work in destruct!)
			DEBUG_CHECK(!hasParent());
		}

		cListBase* get_Parent() const noexcept
		{
			return m_pParent;
		}
		cListNodeBase* get_Next() const noexcept
		{
			return m_pNext;
		}
		cListNodeBase* get_Prev() const noexcept
		{
			return m_pPrev;
		}

		bool hasParent() const noexcept
		{
			//! is this in a list?
			if (m_pParent != nullptr)
			{
				return true;
			}
			// If i have no parent i shouldnt have any siblings either.
			DEBUG_CHECK(m_pNext == nullptr && m_pPrev == nullptr);		 
			return false;
		}

		//! remove ListNode from List.
		//! @note Must define body of this function after cListBase has been defined.
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

	class GRAYCORE_LINK cListBase	// generic list of objects based on cListNodeBase.
	{
		//! @class Gray::cListBase
		//! Double linked list. NOT circular. head and tail are nullptr.
		//! @note Lists are primarily used if inserts and deletes for large sets occurs frequently.
		//! Objects should remove themselves from the list when deleted.
		//! Similar to the MFC CList, or std::list<T>, std::deque

		friend class cListNodeBase; // so it can call RemoveListNode() to remove self.

	protected:
		ITERATE_t m_iCount;		//!< how many children? nice to get read only direct access to this for scripting.
	private:
		cListNodeBase* m_pHead;	//!< Head of my list.
		cListNodeBase* m_pTail;	//!< Tail of my list.

	protected:
		//! Override this to get called when an item is removed from this list.
		//! Never called directly. ALWAYS called from pObRec->RemoveFromParent()
		virtual void RemoveListNode(cListNodeBase* pNode);	//!< allow Override of this. called when child removed from list.

	public:
		cListBase() noexcept
			: m_iCount(0)
			, m_pHead(nullptr)
			, m_pTail(nullptr)
		{
		}
		virtual ~cListBase()
		{
			//! @note virtuals do not work in destructors !
			//! ASSUME: DisposeAll() or Empty() is called from higher levels
			//!  it is important for virtual RemoveListNode() callback
			ASSERT(isEmpty());
		}

		//! Override this to check items being added.
		//! pPrev = nullptr = first
		virtual void InsertListNode(cListNodeBase* pNodeNew, cListNodeBase* pNodePrev = nullptr);
		void InsertList(cListBase* pListSrc, cListNodeBase* pNodePrev = nullptr);

		void InsertBefore(cListNodeBase* pNodeNew, const cListNodeBase* pNodeNext)
		{
			//! @arg pNext = nullptr = insert last
			InsertListNode(pNodeNew, (pNodeNext != nullptr) ? (pNodeNext->get_Prev()) : get_Tail());
		}
		void InsertHead(cListNodeBase* pNodeNew)
		{
			InsertListNode(pNodeNew, nullptr);
		}
		void InsertTail(cListNodeBase* pNodeNew)
		{
			InsertListNode(pNodeNew, get_Tail());
		}

		void DisposeAll();
		void Empty();

		cListNodeBase* get_Head() const noexcept
		{
			return m_pHead;
		}
		cListNodeBase* get_Tail() const noexcept
		{
			return m_pTail;
		} 
		ITERATE_t get_Count() const noexcept
		{
			return m_iCount;
		}
		bool isEmpty() const noexcept
		{
			return get_Count() == 0;
		}

		//! iterate the linked list.
		cListNodeBase* GetAt(ITERATE_t index) const;

		bool IsMyChild(const cListNodeBase* pNode) const noexcept
		{
			if (pNode == nullptr)
				return false;
			return pNode->get_Parent() == this;
		}
	};

	//*************************************************

	inline void cListNodeBase::RemoveFromParent()
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

	template<class _TYPE_REC = cListNodeBase>
	class cListNodeT : public cListNodeBase
	{
		//! @class Gray::cListNodeT
		//! Assume this is a node (in a linked list) of type _TYPE_REC. e.g. _TYPE_REC is based on cListNodeT<> which is based on cListNodeBase.
		typedef cListNodeBase SUPER_t;
	public:
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

	template<class _TYPE_REC /* = cListNodeBase */ >
	class cListT : public cListBase
	{
		//! @class Gray::cListT
		//! Hold a List of _TYPE_REC things.
		//! @note _TYPE_REC is the type of class this list contains. _TYPE_REC is based on cListNodeT<_TYPE_REC> and/or cListNodeBase
		typedef cListBase SUPER_t;
		typedef cListNodeT<_TYPE_REC> NODEBASE_t;
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
}
#endif // _INC_cList_H
