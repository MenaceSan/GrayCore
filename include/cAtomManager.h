//! @file cAtomManager.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cAtomManager_H
#define _INC_cAtomManager_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cAtom.h"
#include "cHashTable.h"
#include "cLogMgr.h"
#include "cSingleton.h"

namespace Gray {
/// <summary>
/// an alpha sorted string lookup table. CASE IGNORED !
/// @note Internal collection for the atoms. Don't use this publicly. Use cAtomRef.
/// @note cAtomRef can be defined in the C++ init code. So the cAtomManager must be safe to run at init time.
/// e.g. static const cAtomRef a_Root(CATOM_N(Root)); //
/// </summary>
struct GRAYCORE_LINK cAtomManager final : public cSingleton<cAtomManager> {
    SINGLETON_IMPL(cAtomManager);

    friend struct cAtomRef;
    typedef cStringHeadT<ATOMCHAR_t> DATA_t;    // HEAD_t
    typedef cRefPtr<DATA_t> REF_t;

 private:
    static const int kRefsBase = 3;  // We only have 3 refs = we can be deleted. Magic number.

    mutable cThreadLockableX m_Lock;               /// make it thread safe.
    cHashTableName<DATA_t, 4> m_aName;             /// Sorted by ATOMCHAR_t Text. NO DUPES
    cHashTableRef<DATA_t, ATOMCODE_t, 5> m_aHash;  /// Sorted by ATOMCODE_t GetHashCode32(s) DUPES?
    cArraySortHash<DATA_t, ATOMCODE_t> m_aStatic;  /// Put atoms here to persist even with no refs. NO DUPES.

 protected:
    bool RemoveAtom(DATA_t* pDef);
    cAtomRef CreateAtom(const cHashIterator& index, COMPARE_t iCompareRes, DATA_t* pData);

    /// <summary>
    /// Make this atom permanent. never removed from the atom tables. adds an extra ref.
    /// </summary>
    /// <param name="pDef"></param>
    void SetAtomStatic(DATA_t* pDef);

    cAtomManager();

 public:
    /// <summary>
    /// Get the atom that corresponds to a string. do not create it.
    /// </summary>
    /// <param name="pszText"></param>
    /// <returns></returns>
    cAtomRef FindAtomStr(const ATOMCHAR_t* pszText) const;
    /// <summary>
    /// Get atom ONLY if this hash is a valid atom hash.
    /// </summary>
    /// <param name="idAtom"></param>
    /// <returns>cAtomRef</returns>
    cAtomRef FindAtomHashCode(ATOMCODE_t idAtom) const;

    /// <summary>
    /// Find the atom in the atom table if it exists else create a new one.
    /// </summary>
    /// <param name="sName"></param>
    /// <returns></returns>
    cAtomRef FindorCreateAtom(const cStringA& sName) noexcept;
    cAtomRef FindorCreateAtom(const cSpan<ATOMCHAR_t>& src) noexcept;

    HRESULT DebugDumpFile(ITextWriter& o) const;
    bool DebugTest() const;
};
}  // namespace Gray
#endif
