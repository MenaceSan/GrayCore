//
//! @file cAtom.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cAtom.h"
#include "cAtomManager.h"
#include "cFile.h"
#include "cCodeProfiler.h"
#include "cThreadLock.h"

namespace Gray
{
	cAtomManager::cAtomManager()
		: cSingleton<cAtomManager>(this, typeid(cAtomManager))
		, m_aEmpty(new cAtomDef(""))
	{
		//! created on demand to prevent any race conditions at static create time.
		m_aName.Add(m_aEmpty.m_pDef);
		m_aHash.Add(m_aEmpty.m_pDef);
	}

	cAtomManager::~cAtomManager()
	{
		DEBUG_MSG(("~cAtomManager"));
	}

	cAtomRef cAtomManager::FindAtomStr(const ATOMCHAR_t* pszText) const
	{
		if (StrT::IsNullOrEmpty(pszText))
			return m_aEmpty;
		cThreadGuard lock(m_Lock);
		cAtomDefPtr pDef = m_aName.FindArgForKey(pszText);
		if (pDef == nullptr)
			return m_aEmpty;
		return cAtomRef(pDef);
	}
	cAtomRef cAtomManager::FindAtomHashCode(ATOMCODE_t idAtom) const
	{
		//! Is this hash id a valid atom hash ?
		//! this is 32 bit specific?
		if (idAtom == k_HASHCODE_CLEAR)
			return m_aEmpty;
		cThreadGuard lock(m_Lock);
		cAtomDefPtr pDef = m_aHash.FindArgForKey(idAtom);
		if (pDef == nullptr)
			return m_aEmpty;
		return cAtomRef(pDef);
	}

	bool cAtomManager::RemoveAtom(cAtomDef* pDef)
	{
		// kRefsBase

		if (pDef == nullptr)
			return false;
		cThreadGuard lock(m_Lock);
		bool bRetRemove = m_aHash.RemoveArgKey(pDef);
		ASSERT(bRetRemove);
		bRetRemove = m_aName.RemoveArgKey(pDef);
		ASSERT(bRetRemove);
#ifdef _DEBUG
		int iRefCount = pDef->get_RefCount();
		ASSERT(iRefCount == 1);
#endif
		return bRetRemove;
	}

	cAtomDefPtr cAtomManager::CreateAtom(ITERATE_t index, COMPARE_t iCompareRes, cStringA sVal)
	{
		//! Insertion sort.
		ASSERT(sVal.get_RefCount() >= 1);
		ASSERT(!sVal.IsEmpty());
		cAtomDefPtr pDef(new cAtomDef(sVal));
		ASSERT(sVal.get_RefCount() >= 2);
		m_aName.AddPresorted(index, iCompareRes, pDef);
		ITERATE_t iHashRet = m_aHash.Add(pDef);
		ASSERT(iHashRet >= 0);
		UNREFERENCED_PARAMETER(iHashRet);
		return pDef;
	}

	cAtomDefPtr cAtomManager::FindorCreateAtomStr(const cStringA& sName)
	{
		//! Find the atom in the atom table if it exists else create a new one.
		//! Does NOT return nullptr

		if (sName.IsEmpty())
			return m_aEmpty.m_pDef;
		cThreadGuard lock(m_Lock);

		COMPARE_t iCompareRes;
		ITERATE_t index = m_aName.FindINearKey(sName, iCompareRes);
		if (iCompareRes == COMPARE_Equal)
		{
			// already here.
			return cAtomDefPtr(m_aName.GetAt(index));
		}
		return CreateAtom(index, iCompareRes, sName);
	}

	cAtomDefPtr cAtomManager::FindorCreateAtomStr(const ATOMCHAR_t* pszName)
	{
		//! Find the atom in the atom table if it exists else create a new one.
		if (StrT::IsNullOrEmpty(pszName))
			return m_aEmpty.m_pDef;
		cThreadGuard lock(m_Lock);

		COMPARE_t iCompareRes;
		ITERATE_t index = m_aName.FindINearKey(pszName, iCompareRes);
		if (iCompareRes == COMPARE_Equal)
		{
			// already here.
			return cAtomDefPtr(m_aName.GetAt(index));
		}
		return CreateAtom(index, iCompareRes, pszName);
	}

	void cAtomManager::SetAtomStatic(cAtomDef* pDef)
	{
		//! Make this atom permanent. never removed from the atom tables. adds an extra ref. 
		m_aStatic.Add(pDef);
	}

	HRESULT cAtomManager::DebugDumpFile(cStreamOutput& o) const
	{
		cThreadGuard lock(m_Lock);

		// Order by name
		for (ITERATE_t i = 0; i < m_aName.GetSize(); i++)
		{
			auto pDef = m_aName.GetAt(i);
			o.Printf("%s" FILE_EOL, StrArg<char>(pDef->get_Name()));
		}

		// Order by hash
		for (ITERATE_t i = 0; i < m_aHash.GetSize(); i++)
		{
			auto pDef = m_aHash.GetAt(i);
			o.Printf("%x = '%s'" FILE_EOL, pDef->get_HashCode(), StrArg<char>(pDef->get_Name()));
		}
		return S_OK;
	}

	//*********************************

	void cAtomRef::EmptyAtom(bool isLast)
	{
		//! Free this ref count. Delete if there are no more refs
		CODEPROFILEFUNC();
		if (IsEmpty())	// already empty. never free the empty atom.
			return;

		cAtomManager& rAM = cAtomManager::I();

		// Remove the cAtomRef from the tables if last use.
		int iRefCount = m_pDef->get_RefCount();
		if (iRefCount <= cAtomManager::kRefsBase)
		{
			// Remove me from the tables.
			// NOTE: This can leak if there are outstanding cString referenced versions ?
			rAM.RemoveAtom(m_pDef);
		}

		// Assume caller will replace m_pDef. or we are in destruct.
		m_pDef = isLast ? nullptr : rAM.m_aEmpty.m_pDef;
	}

	size_t cAtomRef::GetHeapStats(OUT ITERATE_t& iAllocCount) const
	{
		//! Every user of the atom dears 1/n of the usage of the memory
		//! return SUPER_t::GetHeapStats(iAllocCount);
		CODEPROFILEFUNC();
		if (IsEmpty())	// already empty
			return 0;
		int iRefs = m_pDef->get_RefCount();
		ASSERT(iRefs >= 2);
		return(m_pDef->m_s.GetHeapStats(iAllocCount) / (iRefs - 1));
	}

	cAtomRef GRAYCALL cAtomRef::FindAtomStr(const ATOMCHAR_t* pszText) // static
	{
		//! Find the atom in the atom table if it exists.
		//! @note DO NOT CREATE IT!
		//! @return m_aEmpty atom if not found.
		CODEPROFILEFUNC();
		return cAtomManager::I().FindAtomStr(pszText);
	}

	cAtomRef GRAYCALL cAtomRef::FindAtomHashCode(ATOMCODE_t idAtom) // static
	{
		//! Get this hash id if a valid atom hash
		//! @return m_aEmpty atom if not found.
		CODEPROFILEFUNC();
		return cAtomManager::I().FindAtomHashCode(idAtom);
	}

	void cAtomRef::SetAtomStatic()
	{
		//! Make this atom permanent. never removed from the atom table.
		cAtomManager::I().SetAtomStatic(m_pDef);
	}

	void GRAYCALL cAtomRef::CreateStaticAtoms(const ATOMCHAR_t** ppAtoms) // static
	{
		//! For use with CATOM_STATIC()
		//! @arg ppAtoms = A nullptr terminated array of static text to make into atoms.

		CODEPROFILEFUNC();
		if (ppAtoms == nullptr)
			return;
		for (ITERATE_t i = 0; ppAtoms[i] != nullptr; i++)
		{
			cAtomRef aTmp(ppAtoms[i]);
			aTmp.SetAtomStatic();	// Make this permanent.
		}
	}

	cAtomDefPtr GRAYCALL cAtomRef::FindorCreateAtomStr(const ATOMCHAR_t* pszText) // static
	{
		return cAtomManager::I().FindorCreateAtomStr(pszText);
	}

	cAtomDefPtr GRAYCALL cAtomRef::FindorCreateAtomStr(const STR_t& sText) // static
	{
		return cAtomManager::I().FindorCreateAtomStr(sText);
	}

#ifdef _DEBUG
	HRESULT GRAYCALL cAtomRef::DebugDumpFile(const FILECHAR_t* pszFilePath) // static
	{
		//! Dump all the atoms to a file.

		CODEPROFILEFUNC();
		cFile file;
		HRESULT hRes = file.OpenX(pszFilePath, OF_CREATE | OF_WRITE);
		if (FAILED(hRes))
		{
			return hRes;
		}
		return cAtomManager::I().DebugDumpFile(file);
	}
#endif

	HRESULT GRAYCALL cAtomRef::CheckSymbolicStr(const ATOMCHAR_t* pszTag, bool bAllowDots)	// static
	{
		//! Is this a simple 'c' style identifier/symbolic string? starts with a char and can have numbers.
		//! @note JSON allows '.' as part of normal names ??
		//! @arg pszTag = the identifier (valid char set).
		//! @return > 0 = length else < 0 = HRESULT error.

		if (StrT::IsNullOrEmpty(pszTag))
		{
			return HRESULT_WIN32_C(ERROR_EMPTY);
		}

		StrLen_t i = 0;
		if (!bAllowDots)
		{
			if (!StrChar::IsCSymF(pszTag[0]))	// first char of symbol is special.
			{
				return E_INVALIDARG;
			}
			i++;
		}

		for (;; i++)
		{
			if (i >= StrT::k_LEN_MAX_KEY)
				return HRESULT_WIN32_C(ERROR_BAD_LENGTH);
			ATOMCHAR_t ch = pszTag[i];
			if (ch == '\0')
				break;
			if (bAllowDots && ch == '.')
				continue;
			if (StrChar::IsCSym(ch))
				continue;
			return E_INVALIDARG;
		}

		return i;
	}

	StrLen_t GRAYCALL cAtomRef::GetSymbolicStr(OUT ATOMCHAR_t* pszTagRet, const ATOMCHAR_t* pszExp, bool bAllowDots)
	{
		//! Parse the string and make a legal symbolic name using only valid symbols characters.
		//! @arg pszTagRet = Copy the identifier (valid char set) out to this buffer. ASSUME StrT::k_LEN_MAX_KEY
		//! @arg pszExp = point to source string
		//! @arg bAllowDots = allow dots and stuff to follow them. JSON
		//! @return
		//!  <0 = error.
		//!  Length of the tag + args.
		//!  0 = nothing here worth doing anything with.

		if (pszExp == nullptr)
			return 0;
		ASSERT_N(pszTagRet != nullptr);

		ATOMCHAR_t ch = pszExp[0];
		StrLen_t i = 0;
		if (!bAllowDots)	// JSON allows leading numbers and dots.
		{
			if (!StrChar::IsCSymF(ch))	// first char of symbol is special.
			{
				// Insert leading '_' to cover the invalid first char?
				pszTagRet[0] = '\0';
				return 0;	// Can't fix this.
			}
			pszTagRet[0] = ch;	// first is special.
			i++;
		}

		for (; pszExp[i] != '\0'; i++)
		{
			ch = pszExp[i];
			if (!bAllowDots || ch != '.')
			{
				if (!StrChar::IsCSym(ch))
					break;
			}
			if (i >= StrT::k_LEN_MAX_KEY)
			{
				pszTagRet[StrT::k_LEN_MAX_KEY - 1] = '\0';
				return(0);
			}

			pszTagRet[i] = ch;
		}

		pszTagRet[i] = '\0';
		return i;
	}
}
