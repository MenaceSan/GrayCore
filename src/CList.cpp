//
//! @file cList.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cList.h"
#include "cDebugAssert.h"

namespace Gray
{
	void cListBase::Empty()
	{
		//! empty the list. but don't necessarily DisposeThis() the objects.
		cListNodeBase* pNodeLast = nullptr;
		for (;;)	// iterate the list.
		{
			cListNodeBase* pNode = get_Head();
			if (pNode == nullptr)
				break;
			ASSERT(pNodeLast != pNode);
			pNodeLast = pNode;
			ASSERT(IsMyChild(pNode));
			pNode->RemoveFromParent();	// ASSUME RemoveListNode(pNode) will be called.
		}
		DEBUG_ASSERT(m_iCount == 0, "List not cleaned up properly!");
		m_iCount = 0;	// this should not be needed. but just in case of leaks.
		m_pHead = nullptr;
		m_pTail = nullptr;
	}

	void cListBase::DisposeAll(void)
	{
		//! call DisposeThis() for all entries.
		cListNodeBase* pNodeLast = nullptr;
		for (;;)	// iterate the list.
		{
			cListNodeBase* pNode = get_Head();
			if (pNode == nullptr)
				break;
			ASSERT(pNodeLast != pNode);
			pNodeLast = pNode;
			ASSERT(IsMyChild(pNode));
			pNode->DisposeThis();	// ASSUME RemoveFromParent() then RemoveListNode(pNode) will be called.
		}
		DEBUG_ASSERT(m_iCount == 0, "List not cleaned up properly!");
		m_iCount = 0;	// this should not be needed. but just in case of leaks.
		m_pHead = nullptr;
		m_pTail = nullptr;
	}

	void cListBase::RemoveListNode(cListNodeBase* pNodeRemove) // virtual Override this = called when removed from list.
	{
		//! Some child object is being removed from my list.
		//! Override this to get called when an item is removed from this list.
		//! just remove from list. It may or may not be deleting itself.
		//! @note Only called from RemoveFromParent(). this is protected.
		//! Never called directly. ALWAYS called from pNodeRemove->RemoveFromParent()

		ASSERT_N(pNodeRemove != nullptr);
		ASSERT(IsMyChild(pNodeRemove));

		cListNodeBase* pNodeNext = pNodeRemove->get_Next();
		cListNodeBase* pNodePrev = pNodeRemove->get_Prev();

		if (pNodeNext != nullptr)
		{
			// ASSUME pNodeNext->m_pPrev was pointing at this.
			pNodeNext->m_pPrev = pNodePrev;
		}
		else
		{
			m_pTail = pNodePrev;
		}
		if (pNodePrev != nullptr)
		{
			// ASSUME pNodePrev->m_pNext was pointing at this.
			pNodePrev->m_pNext = pNodeNext;
		}
		else
		{
			m_pHead = pNodeNext;
		}

		m_iCount--;
		pNodeRemove->m_pNext = nullptr;
		pNodeRemove->m_pPrev = nullptr;
		pNodeRemove->put_Parent(nullptr);	// officially removed from list. may be virtual.
		ASSERT(!pNodeRemove->hasParent());
	}

	void cListBase::InsertListNode(cListNodeBase* pNodeNew, cListNodeBase* pNodePrev) // virtual Override this = called when add to list.
	{
		//! Add pNodeNew after pNodePrev.
		//! pNodePrev = nullptr == add to the start.
		//! @note If the item is already here, it will NOT be removed then re-added.

		if (pNodeNew == nullptr)
			return;
		DEBUG_CHECK(pNodePrev != pNodeNew);
		if (pNodeNew->hasParent())	// currently in a list.
		{
			if (IsMyChild(pNodeNew))	// already here.
			{
				// allow a change in order??
				return;
			}
			pNodeNew->RemoveFromParent();	// Get out of any previous list first.
			DEBUG_CHECK(!pNodeNew->hasParent());
		}

		cListNodeBase* pNodeNext;
		if (pNodePrev != nullptr)		// put after some other node?
		{
			DEBUG_CHECK(IsMyChild(pNodePrev));
			pNodeNext = pNodePrev->get_Next();
			pNodePrev->m_pNext = pNodeNew;
		}
		else
		{
			// put at head.
			pNodeNext = get_Head();
			m_pHead = pNodeNew;
		}

		pNodeNew->m_pPrev = pNodePrev;

		if (pNodeNext != nullptr)
		{
			DEBUG_CHECK(IsMyChild(pNodeNext));
			pNodeNext->m_pPrev = pNodeNew;
		}
		else
		{
			m_pTail = pNodeNew;
		}

		pNodeNew->m_pNext = pNodeNext;
		pNodeNew->put_Parent(this);
		m_iCount++;
		DEBUG_CHECK(pNodeNew->hasParent());
		// DEBUG_CHECK(pNodeNew->isValidCheck());
	}

	void cListBase::InsertList(cListBase* pListSrc, cListNodeBase* pNodePrev)
	{
		//! Transfer the contents of another list into this one.
		if (pListSrc == this || pListSrc == nullptr)
		{
			// not really a transfer at all
			return;
		}
		cListNodeBase* pNode = pListSrc->get_Head();
		while (pNode != nullptr)
		{
			cListNodeBase* pNodeNext = pNode->get_Next();
			pNode->RemoveFromParent();	// should not be needed
			this->InsertListNode(pNode, pNodePrev);
			pNodePrev = pNode;
			pNode = pNodeNext;
		}
	}

	cListNodeBase* cListBase::GetAt(ITERATE_t index) const
	{
		//! iterate the linked list.
		//! Not very efficient. iterative.
		//! @return nullptr = past end of list.
		if (IS_INDEX_BAD(index, get_Count()))
			return nullptr;
		cListNodeBase* pNode = get_Head();
		while (index-- > 0 && pNode != nullptr)
		{
			pNode = pNode->get_Next();
		}
		return(pNode);
	}

#if 0
	cListNodeBase* cListBase::GetNext(cListNodeBase* pNode) const
	{
		//! iterate the linked list.
		if (pNode == nullptr)
			return get_Head();
		ASSERT(pNode->get_Parent() == this);
		return pNode->get_Next();
	}
#endif
}
 