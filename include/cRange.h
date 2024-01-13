//
//! @file cRange.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cRange_H
#define _INC_cRange_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"

namespace Gray {
/// <summary>
/// Simple linearity range from m_Lo to m_Hi. Similar to cStreamProgressT<>
/// @note assume Normalized Hi>=Lo.
/// POD class should allow static init
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <typename TYPE = int>
class cRangeT {
    typedef cRangeT<TYPE> THIS_t;

 public:
    TYPE m_Lo;  /// low range value.
    TYPE m_Hi;  /// inclusive high side of range. int size = (hi-lo)+1, float size = hi-lo ?? weird.

 public:
    cRangeT() {}  // not init!
    cRangeT(TYPE lo, TYPE hi) : m_Lo(lo), m_Hi(hi) {}

    inline bool isNormal() const noexcept {
        return m_Lo <= m_Hi;
    }
    TYPE get_Min() const noexcept {
        return m_Lo;
    }
    TYPE get_Max() const noexcept {
        return m_Hi;
    }
    TYPE get_Avg() const noexcept {
        return (m_Lo + m_Hi) / 2;
    }

    /// <summary>
    /// Get value clamped to range.
    /// assume isNormal().
    /// </summary>
    TYPE GetClampValue(TYPE nVal) const noexcept {
        if (nVal < m_Lo) return m_Lo;
        if (nVal > m_Hi) return m_Hi;
        return nVal;
    }
    /// <summary>
    /// Is the index in the range? inclusive.
    /// assume isNormal().
    /// </summary>
    /// <param name="nVal"></param>
    /// <returns></returns>
    bool IsInsideI(TYPE nVal) const noexcept {
        return nVal >= m_Lo && nVal <= m_Hi;
    }
    /// <summary>
    /// Is the index in the range? Non inclusive.
    /// @note if size 0 then this is never true !
    /// assume isNormal().
    /// </summary>
    bool IsInsideX(TYPE nVal) const noexcept {
        return nVal >= m_Lo && nVal < m_Hi;
    }

    /// <summary>
    /// Get range for inclusive int types.
    /// assume isNormal().
    /// </summary>
    TYPE get_RangeI() const noexcept {
        return (m_Hi - m_Lo) + 1;  // inclusive. integer
    }
    /// <summary>
    /// Get range for exclusive float types.
    /// assume isNormal().
    /// </summary>
    TYPE get_RangeX() const noexcept {
        return m_Hi - m_Lo;  // exclusive.
    }

    TYPE GetLinear1(float fOne) const noexcept {
        //! @arg fOne = 0.0 to 1.0
        return CastN(TYPE, m_Lo + (fOne * get_RangeI()));
    }
    int GetSpinValueI(int iVal) const {
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
    void SetZero() noexcept {
        m_Hi = m_Lo = 0;
    }
    void put_Min(TYPE iLo) noexcept {
        m_Lo = iLo;
    }
    void put_Max(TYPE iHi) noexcept {
        m_Hi = iHi;
    }
    void SetRange(TYPE iLo, TYPE iHi) noexcept {
        //! May not be normalized ?
        m_Lo = iLo;
        m_Hi = iHi;
    }
    void NormalizeRange() noexcept {
        if (!isNormal()) {
            cValT::Swap<TYPE>(m_Lo, m_Hi);
        }
    }

    /// <summary>
    /// Expand the range to include this value. isNormal()
    /// </summary>
    void UnionValue(TYPE nVal) noexcept {
        if (nVal < m_Lo) m_Lo = nVal;
        if (nVal > m_Hi) m_Hi = nVal;
    }

    /// <summary>
    /// Do 2 ranges overlap ? (assume isNormal()/proper ordered ranges)
    /// </summary>
    /// <param name="x"></param>
    /// <returns></returns>
    bool IsRangeOverlapI(const THIS_t& x) const noexcept {
        if (x.m_Lo > m_Hi) return false;
        if (x.m_Hi < m_Lo) return false;
        return true;
    }
    void SetUnionRange(const THIS_t& x) noexcept {
        // assume isNormal()/proper ordered ranges
        if (x.m_Hi > m_Hi) {
            m_Hi = x.m_Hi;
        }
        if (x.m_Lo < m_Lo) {
            m_Lo = x.m_Lo;
        }
    }
};
}  // namespace Gray
#endif
