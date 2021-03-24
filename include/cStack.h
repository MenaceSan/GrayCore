//
//! @file cStack.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cStack_H
#define _INC_cStack_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cHeap.h"
 
namespace Gray
{

	template<class TYPE = BYTE, ITERATE_t _QTY = 1024>
	class cStackStatic
	{
		//! @class Gray::cStackStatic
		//! Create a generic thread/multi process safe (static sized) stack.

		TYPE m_Data[_QTY];		//!< Not heap allocation. static/inline allocated
		ITERATE_t m_iWriteNext;	//!< last .

	public:
		inline cStackStatic() noexcept
			: m_iWriteNext(0)
		{
			STATIC_ASSERT(_QTY > 0, cStackStatic);
		}
		inline bool isEmpty() const noexcept
		{
			return m_iWriteNext == 0;
		}
		inline bool isFull() const noexcept
		{
			return m_iWriteNext >= _QTY;
		}
		inline TYPE Pop()
		{
			ASSERT(m_iWriteNext >= 1);
			return m_Data[--m_iWriteNext];
		}
		inline void Push(TYPE v)
		{
			ASSERT(m_iWriteNext < _QTY);
			m_Data[m_iWriteNext++] = v;
		}
	};
}

#endif
