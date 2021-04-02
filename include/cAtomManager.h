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
#include "cArraySortRef.h"
#include "cSingleton.h"

namespace Gray
{
	class GRAYCORE_LINK cAtomManager
		: public cSingleton < cAtomManager >
	{
		//! @class Gray::cAtomManager
		//! an alpha sorted string lookup table. CASE IGNORED !
		//! @note Internal collection for the atoms. Don't use this publicly. Use cAtomRef.
		//! @note cAtomRef can be defined in the C++ init code. So the cAtomManager must be safe to run at init time.
		//! e.g. static const cAtomRef a_Root("Root");

		friend class cSingleton < cAtomManager >;
		friend class cAtomRef;

	private:
		static const int kRefsBase = 3;	// We only have 3 refs = we can be deleted. Magic number.

		mutable cThreadLockCount m_Lock;	// make it thread safe.
		cArraySortName< cAtomDef, ATOMCHAR_t > m_aName;	//!< Sorted by ATOMCHAR_t Text. NO DUPES
		cArraySortHash< cAtomDef, ATOMCODE_t > m_aHash;	//!< Sorted by ATOMCODE_t GetHashCode32(s) NO DUPES.
		cArraySortHash< cAtomDef, ATOMCODE_t > m_aStatic;	//!< Put atoms here to persist. NO DUPES.

	public:
		const cAtomRef m_aEmpty;	//!< static "\0" atom. never use nullptr. ATOMCODE_t = 0. same as SetAtomStatic

	protected:
		bool RemoveAtom(cAtomDef* pDef);
		cAtomRef CreateAtom(ITERATE_t index, COMPARE_t iCompareRes, cStringA sVal);
		cAtomRef FindorCreateAtomStr(const cStringA& sName) noexcept;
		cAtomRef FindorCreateAtomStr(const ATOMCHAR_t* pszName) noexcept;
		void SetAtomStatic(cAtomDef* pDef);

	public:
		cAtomManager();
		~cAtomManager() noexcept;

		cAtomRef FindAtomStr(const ATOMCHAR_t* pszText) const;
		cAtomRef FindAtomHashCode(ATOMCODE_t idAtom) const;

		HRESULT DebugDumpFile(cStreamOutput& o) const;
		bool DebugTest() const;

		CHEAPOBJECT_IMPL;
	};
}

#endif
