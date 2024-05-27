//! @file cList.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cDebugAssert.h"
#include "cList.h"

namespace Gray {
void cList::SetEmptyList() {
    cListNode* pNodeLast = nullptr;
    UNREFERENCED_PARAMETER(pNodeLast);  // _DEBUG only.
    for (;;) {  // iterate the list.
        cListNode* pNode = get_Head();
        if (pNode == nullptr) break;
        ASSERT(pNodeLast != pNode);
        pNodeLast = pNode;
        ASSERT(IsMyChild(pNode));
        pNode->RemoveFromParent();  // ASSUME RemoveListNode(pNode) will be called.
    }
    ClearList();  // this should not be needed. but just in case of leaks.
}

void cList::DisposeAll() {
    cListNode* pNodeLast = nullptr;
    UNREFERENCED_PARAMETER(pNodeLast);  // _DEBUG only.
    for (;;) {                          // iterate the list.
        cListNode* pNode = get_Head();
        if (pNode == nullptr) break;
        ASSERT(pNodeLast != pNode);
        pNodeLast = pNode;
        ASSERT(IsMyChild(pNode));
        pNode->DisposeThis();  // ASSUME RemoveFromParent() then RemoveListNode(pNode) will be called.
    }
    ClearList();
}

void cList::RemoveListNode(cListNode* pNodeRemove) {  // virtual Override this = called when removed from list.
    //! Some child object is being removed from my list.
    //! Override this to get called when an item is removed from this list.
    //! just remove from list. It may or may not be deleting itself.
    //! @note Only called from RemoveFromParent(). this is protected.
    //! Never called directly. ALWAYS called from pNodeRemove->RemoveFromParent()

    ASSERT_NN(pNodeRemove);
    ASSERT(IsMyChild(pNodeRemove));

    cListNode* pNodeNext = pNodeRemove->get_Next();
    cListNode* pNodePrev = pNodeRemove->get_Prev();

    if (pNodeNext != nullptr) {
        // ASSUME pNodeNext->m_pPrev was pointing at this.
        pNodeNext->m_pPrev = pNodePrev;
    } else {
        m_pTail = pNodePrev;
    }
    if (pNodePrev != nullptr) {
        // ASSUME pNodePrev->m_pNext was pointing at this.
        pNodePrev->m_pNext = pNodeNext;
    } else {
        m_pHead = pNodeNext;
    }

    m_iCount--;
    pNodeRemove->m_pNext = nullptr;
    pNodeRemove->m_pPrev = nullptr;
    pNodeRemove->OnChangeListParent(nullptr);  // officially removed from list. may be virtual. may delete object!
}

void cList::InsertListNode(cListNode* pNodeNew, cListNode* pNodePrev) {  // virtual Override this = called when add to list.
    //! Add pNodeNew after pNodePrev.
    //! pNodePrev = nullptr == add to the start.
    //! @note If the item is already here, it will NOT be removed then re-added.

    if (pNodeNew == nullptr) return;
    DEBUG_CHECK(pNodePrev != pNodeNew);
    if (pNodeNew->hasParent()) {          // currently in a list.
        if (IsMyChild(pNodeNew)) return;  // already here. // allow a change in order?
        pNodeNew->RemoveFromParent();     // Remove from any previous list first.
        DEBUG_CHECK(!pNodeNew->hasParent());
    }

    cListNode* pNodeNext;
    if (pNodePrev != nullptr) {  // put after some other node?
        DEBUG_CHECK(IsMyChild(pNodePrev));
        pNodeNext = pNodePrev->get_Next();
        pNodePrev->m_pNext = pNodeNew;
    } else {
        pNodeNext = get_Head();  // put at head.
        m_pHead = pNodeNew;
    }

    pNodeNew->m_pPrev = pNodePrev;

    if (pNodeNext != nullptr) {
        DEBUG_CHECK(IsMyChild(pNodeNext));
        pNodeNext->m_pPrev = pNodeNew;
    } else {
        m_pTail = pNodeNew;
    }

    m_iCount++;
    pNodeNew->m_pNext = pNodeNext;
    pNodeNew->OnChangeListParent(this);
    DEBUG_CHECK(pNodeNew->hasParent());
}

void cList::MoveListNodes(cList* pListSrc, cListNode* pNodePrev) {
    if (pListSrc == this || pListSrc == nullptr) return;  // not really a transfer at all
    cListNode* pNode = pListSrc->get_Head();
    while (pNode != nullptr) {
        cListNode* pNodeNext = pNode->get_Next();
        pNode->RemoveFromParent();  // should not be needed
        this->InsertListNode(pNode, pNodePrev);
        pNodePrev = pNode;
        pNode = pNodeNext;
    }
}

cListNode* cList::GetAt(ITERATE_t index) const {
    if (IS_INDEX_BAD(index, get_Count())) return nullptr;
    cListNode* pNode = get_Head();
    while (index-- > 0 && pNode != nullptr) {
        pNode = pNode->get_Next();
    }
    return pNode;
}

#if 0
cListNode* cList::GetNext(cListNode* pNode) const	{
	//! iterate the linked list.
	if (pNode == nullptr) return get_Head();
	ASSERT(pNode->get_Parent() == this);
	return pNode->get_Next();
}
#endif
}  // namespace Gray
