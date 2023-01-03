//
//! @file cAtomManager.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cAtomManager_H
#define _INC_cAtomManager_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cAtom.h"
#include "cLogMgr.h"
#include "cHashTable.h"
#include "cSingleton.h"

namespace Gray
{
	/// <summary>
	/// an alpha sorted string lookup table. CASE IGNORED !
	/// @note Internal collection for the atoms. Don't use this publicly. Use cAtomRef.
	/// @note cAtomRef can be defined in the C++ init code. So the cAtomManager must be safe to run at init time.
	/// e.g. static const cAtomRef a_Root(CATOM_N(Root)); //  
	/// </summary>
	class GRAYCORE_LINK cAtomManager
		: public cSingleton < cAtomManager >
	{
		friend class cSingleton < cAtomManager >;
		friend class cAtomRef;

	public:
		typedef cStringDataT<ATOMCHAR_t> DATA_t;
		typedef cRefPtr<DATA_t> REF_t;

	private:
		static const int kRefsBase = 3;	// We only have 3 refs = we can be deleted. Magic number.

		mutable cThreadLockCount m_Lock;	// make it thread safe.
		cHashTableName< DATA_t, 4 > m_aName;	//!< Sorted by ATOMCHAR_t Text. NO DUPES
		cHashTableRef< DATA_t, ATOMCODE_t, 5 > m_aHash;	//!< Sorted by ATOMCODE_t GetHashCode32(s) DUPES?
		cArraySortHash< DATA_t, ATOMCODE_t > m_aStatic;	//!< Put atoms here to persist even with no refs. NO DUPES.

	public:
		const cAtomRef m_aEmpty;	//!< static "\0" atom. never use nullptr. ATOMCODE_t = 0. same as SetAtomStatic

	protected:
		bool RemoveAtom(DATA_t* pDef);
		cAtomRef CreateAtom(const cHashIterator& index, COMPARE_t iCompareRes, DATA_t* pData);

		/// <summary>
		/// Make this atom permanent. never removed from the atom tables. adds an extra ref. 
		/// </summary>
		/// <param name="pDef"></param>
		void SetAtomStatic(DATA_t* pDef);

	public:
		cAtomManager();
		~cAtomManager() noexcept;

		/// <summary>
		/// Get the atom that corresponds to a string. do not create it.
		/// </summary>
		/// <param name="pszText"></param>
		/// <returns></returns>
		cAtomRef FindAtomStr(const ATOMCHAR_t* pszText) const;
		/// <summary>
		/// Get atom ONLY if this hash id a valid atom hash.
		/// </summary>
		/// <param name="idAtom"></param>
		/// <returns>cAtomRef</returns>
		cAtomRef FindAtomHashCode(ATOMCODE_t idAtom) const;

		/// <summary>
		/// Find the atom in the atom table if it exists else create a new one.
		/// </summary>
		/// <param name="sName"></param>
		/// <returns></returns>
		cAtomRef FindorCreateAtomStr(const cStringA& sName) noexcept;
		cAtomRef FindorCreateAtomStr(const ATOMCHAR_t* pszName) noexcept;

		HRESULT DebugDumpFile(cStreamOutput& o) const;
		bool DebugTest() const;

		CHEAPOBJECT_IMPL;
	};
}

#endif
