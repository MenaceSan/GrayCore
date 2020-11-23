//
//! @file cPair.h
//! Associate 2 arbitrary typed values.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cPair_H
#define _INC_cPair_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cMem.h"
#include "cDebugAssert.h"

namespace Gray
{
	template<typename TYPE>
	class cRangeT
	{
		//! @class Gray::RangeT
		//! Simple linearity range from m_Lo to m_Hi. Similar to cStreamProgressT<>
		//! @note assume Normalized Hi>=Lo.
		//! POD class should allow static init
		typedef cRangeT<TYPE> THIS_t;

	public:
		TYPE m_Lo;		//!< low range value.
		TYPE m_Hi;		//!< inclusive high side of range. int size = (hi-lo)+1, float size = hi-lo ?? weird.
	public:
		TYPE get_Min() const
		{
			return m_Lo;
		}
		TYPE get_Max() const
		{
			return m_Hi;
		}
		TYPE get_Avg() const
		{
			return (m_Lo + m_Hi) / 2;
		}
		TYPE GetClampValue(TYPE nVal) const
		{
			// assume normalized.
			if (nVal < m_Lo)
				return m_Lo;
			if (nVal > m_Hi)
				return m_Hi;
			return nVal;
		}
		bool IsInsideI(TYPE nVal) const
		{
			//! Is the index in the range? inclusive.
			// assume normalized.
			return nVal >= m_Lo && nVal <= m_Hi;
		}
		bool IsInsideX(TYPE nVal) const
		{
			//! Is the index in the range? Non inclusive.
			//! @note if size 0 then this is never true !
			// assume normalized.
			return nVal >= m_Lo && nVal < m_Hi;
		}

		TYPE get_RangeI() const
		{
			//! Typically used for inclusive int types.
			// assume normalized.
			return (m_Hi - m_Lo) + 1;	// inclusive. integer
		}
		TYPE get_RangeX() const
		{
			//! Typically used for exclusive float types.
			// assume normalized.
			return m_Hi - m_Lo;	// exclusive.
		}

		TYPE GetLinear1(float fOne) const
		{
			//! @arg fOne = 0.0 to 1.0
			return (TYPE)(m_Lo + (fOne * get_RangeI()));
		}
		int GetSpinValueI(int iVal) const
		{
			//! @return a modulus of the range.
			iVal -= (int)m_Lo;
			int iRange = (int)get_RangeI();
			iVal %= iRange;
			if (iVal < 0)
				iVal += (int)(m_Hi + 1);
			else
				iVal += (int)(m_Lo);
#ifdef _DEBUG
			TYPE iValClamp = (TYPE)GetClampValue((TYPE)iVal);
			ASSERT(iVal == iValClamp);
#endif
			return iVal;
		}

		// Setters.
		void SetZero()
		{
			m_Hi = m_Lo = 0;
		}
		void put_Min(TYPE iLo)
		{
			m_Lo = iLo;
		}
		void put_Max(TYPE iHi)
		{
			m_Hi = iHi;
		}
		void SetRange(TYPE iLo, TYPE iHi)
		{
			//! May not be normalized ?
			m_Lo = iLo;
			m_Hi = iHi;
		}
		void NormalizeRange()
		{
			if (m_Lo > m_Hi)
			{
				cValT::Swap<TYPE>(m_Lo, m_Hi);
			}
		}
		void UnionValue(TYPE nVal)
		{
			//! Expand the range to include this value. Normalized
			if (nVal < m_Lo)
				m_Lo = nVal;
			if (nVal > m_Hi)
				m_Hi = nVal;
		}

		bool IsRangeOverlapI(const THIS_t& x) const
		{
			// Do 2 ranges (assume Normalized/proper ordered ranges) overlap ?
			if (x.m_Lo > m_Hi)
				return false;
			if (x.m_Hi < m_Lo)
				return false;
			return true;
		}
		void SetUnionRange(const THIS_t& x)
		{
			// assume Normalized/proper ordered ranges
			if (x.m_Hi > m_Hi)
			{
				m_Hi = x.m_Hi;
			}
			if (x.m_Lo < m_Lo)
			{
				m_Lo = x.m_Lo;
			}
		}
	};

	template< class _TYPE_A, class _TYPE_B >
	class cPairT
	{
		//! @class Gray::cPairA
		//! The aggregate/simple type for simple static const init. AKA Tuple.
		//! similar to "std::pair" or "std::tuple<>" or "System.Collections.Generic.KeyValuePair<>"
		//! not the same as cValueRange (same type)
		//! @note if i give this a constructor then the compiler won't allow it to be static initialized.

	public:
		_TYPE_A m_a;	//!< nullptr or 0 = last in array. (typically sorted by A as primary key)
		_TYPE_B m_b;	//!< nullptr or 0 = last in array.

	public:
		_TYPE_A get_A() const
		{
			return this->m_a;	// Key
		}
		_TYPE_B get_B() const
		{
			return this->m_b;	// Value
		}
		_TYPE_A get_HashCode() const
		{
			//! Support this in case anyone wants to use it.
			return m_a;
		}

		void put_A(_TYPE_A a)
		{
			m_a = a;
		}
		void put_B(_TYPE_B b)
		{
			m_b = b;
		}
	};

	template< class _TYPE_A, class _TYPE_ARG_A, class _TYPE_B, class _TYPE_ARG_B >
	class cPairX : public cPairT < _TYPE_A, _TYPE_B >
	{
		//! @class Gray::cPairX
		//! Template to associate a complex something with another something.
		//! _TYPE_ARG_A = Allow an alternate argument type for _TYPE_A. Usually a reference for _TYPE_A if complex.
		//! typically sorted by _TYPE_A. but not assumed/enforced.
		//! typically in a static table!
		//! typically LAST ENTRY in static table = { 0  or nullptr }, in either place.
		//! Like cArrayVal2<> is to cArrayVal<>

		typedef cPairT<_TYPE_A, _TYPE_B> SUPER_t;

	public:
		cPairX()	// Undefined values for dynamic arrays.
		{}
		cPairX(_TYPE_ARG_A a, _TYPE_ARG_B b)
		{
			this->m_a = a;
			this->m_b = b;
		}

		void SetValues(_TYPE_ARG_A a, _TYPE_ARG_B b)
		{
			this->m_a = a;
			this->m_b = b;
		}
		_TYPE_ARG_A get_HashCode() const
		{
			//! Support this in case anyone wants to use it.
			return this->m_a;
		}
		const _TYPE_A& get_A() const
		{
			return this->m_a;
		}
		void put_A(_TYPE_ARG_A a)
		{
			this->m_a = a;
		}
		const _TYPE_A& get_B() const
		{
			return this->m_b;
		}
		void put_B(_TYPE_ARG_B b)
		{
			this->m_b = b;
		}
	};

	template< class _TYPE_A, class _TYPE_B >
	class cPair : public cPairT < _TYPE_A, _TYPE_B >
	{
		//! @class Gray::cPair
		//! Associated pair of simple things. Like cArrayVal<> is to cArrayVal2<>

		typedef cPairT<_TYPE_A, _TYPE_B> SUPER_t;

	public:
		cPair()	// Undefined values for dynamic arrays.
		{}
		cPair(_TYPE_A a, _TYPE_B b)
		{
			this->m_a = a;
			this->m_b = b;
		}

		// If element is a member of a static array.

		bool IsValidIndex(ITERATE_t i) const
		{
			//! ASSUME static array.
			//! either value is non zero?
			if (i < 0)
				return false;
			return this[i].m_a || this[i].m_b;
		}

		ITERATE_t FindIA(_TYPE_A a) const
		{
			//! ASSUME static array.
			//! brute force lookup A
			for (ITERATE_t i = 0; IsValidIndex(i); i++)
			{
				if (this[i].m_a == a)
					return i;
			}
			return k_ITERATE_BAD;
		}
		ITERATE_t FindIB(_TYPE_B b) const
		{
			//! ASSUME static array.
			//! brute force lookup B return index
			//! @return <0 = can't find it.
			for (ITERATE_t i = 0; IsValidIndex(i); i++)
			{
				if (this[i].m_b == b)
					return i;
			}
			return k_ITERATE_BAD;
		}

		bool FindARetB(_TYPE_A a, _TYPE_B* pb) const
		{
			//! ASSUME static array.
			//! brute force lookup A to return corresponding B
			//! @return <0 = can't find it.
			ITERATE_t i = FindIA(a);
			if (i >= 0)
			{
				*pb = this[i].m_b;
				return true;
			}
			return false;
		}
		bool FindBRetA(_TYPE_B b, _TYPE_A* pa) const
		{
			//! ASSUME static array.
			//! brute force lookup B to return corresponding A
			ITERATE_t i = FindIB(b);
			if (i >= 0)
			{
				*pa = this[i].m_a;
				return true;
			}
			return false;
		}

		_TYPE_B FindSortedARetB(_TYPE_A a) const
		{
			//! ASSUME sorted static array.
			//! Assume _TYPE_A sorted values from min to max.
			//! @todo binary search?
			ITERATE_t i = 0;
			for (; IsValidIndex(i); i++)
			{
				if (a < this[i + 1].m_a)
					return(this[i].m_b);
			}
			return(this[i - 1].m_b);
		}
	};
};
#endif // _INC_cPair_H
