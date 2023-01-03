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
		, m_aEmpty(DATA_t::CreateStringData2(0, &cStrConst::k_EmptyA))
	{
		//! created on demand to prevent any race conditions at static create time.
		m_aName.InsertAt(cHashIterator(), COMPARE_Equal, m_aEmpty.m_p);
		m_aHash.Add(m_aEmpty.m_p);
		SetAtomStatic(m_aEmpty);
	}

	cAtomManager::~cAtomManager() noexcept
	{
		DEBUG_MSG(("~cAtomManager"));
	}

	cAtomRef cAtomManager::FindAtomStr(const ATOMCHAR_t* pszText) const
	{
		if (StrT::IsNullOrEmpty(pszText))
			return m_aEmpty;
		cThreadGuard lock(m_Lock);
		cAtomRef pDef(m_aName.FindArgForKey(pszText));
		if (!pDef.isValidPtr())
			return m_aEmpty;
		return cAtomRef(pDef);
	}

	cAtomRef cAtomManager::FindAtomHashCode(ATOMCODE_t idAtom) const
	{
		if (idAtom == k_HASHCODE_CLEAR)
			return m_aEmpty;
		cThreadGuard lock(m_Lock);
		cAtomRef pDef(m_aHash.FindArgForKey(idAtom));
		if (!pDef.isValidPtr())
			return m_aEmpty;
		return cAtomRef(pDef);
	}

	bool cAtomManager::RemoveAtom(DATA_t* pDef)
	{
		// below kRefsBase
		if (pDef == nullptr)
			return false;
		cThreadGuard lock(m_Lock);
		bool bRetRemove = m_aHash.DeleteArg(pDef);
		ASSERT(bRetRemove);
		bRetRemove = m_aName.DeleteArg(pDef);
		ASSERT(bRetRemove);
#ifdef _DEBUG
		int iRefCount = pDef->get_RefCount();
		ASSERT(iRefCount == 1);
#endif
		return bRetRemove;
	}

	cAtomRef cAtomManager::CreateAtom(const cHashIterator& index, COMPARE_t iCompareRes, DATA_t* pData)
	{
		//! Insertion sort.
		m_aName.InsertAt(index, iCompareRes,pData);
		// const ITERATE_t nHashSize = m_aHash.GetSize();	// previous size.
		const ITERATE_t nHashRet = m_aHash.Add(pData);
		ASSERT(nHashRet >= 0);
		// ASSERT(m_aHash.GetSize() > nHashSize);	// Hash collision?!
		UNREFERENCED_PARAMETER(nHashRet);	// release mode.
		return cAtomRef(pData);
	}

	cAtomRef cAtomManager::FindorCreateAtomStr(const cStringA& sName) noexcept
	{
		if (sName.IsEmpty())
			return m_aEmpty;

		cThreadGuard lock(m_Lock);

		COMPARE_t iCompareRes;
		cHashIterator index = m_aName.FindINearKey(sName, iCompareRes);
		if (iCompareRes == COMPARE_Equal)
		{
			// already here.
			return cAtomRef(m_aName.GetAtHash(index));
		}
		return CreateAtom(index, iCompareRes, const_cast<cStringA&>(sName).GetData());
	}

	cAtomRef cAtomManager::FindorCreateAtomStr(const ATOMCHAR_t* pszName) noexcept
	{
		//! Find the atom in the atom table if it exists else create a new one.
		if (StrT::IsNullOrEmpty(pszName))
			return m_aEmpty;

		cThreadGuard lock(m_Lock);

		COMPARE_t iCompareRes;
		cHashIterator index = m_aName.FindINearKey(pszName, iCompareRes);
		if (iCompareRes == COMPARE_Equal)
		{
			// already here.
			return cAtomRef(m_aName.GetAtHash(index));
		}
		return CreateAtom(index, iCompareRes, DATA_t::CreateStringData2(StrT::Len(pszName), pszName));
	}

	void cAtomManager::SetAtomStatic(DATA_t* pDef)
	{
		m_aStatic.Add(pDef);
	}

	HRESULT cAtomManager::DebugDumpFile(cStreamOutput& o) const
	{
		cThreadGuard lock(m_Lock);

		// Order by name
		FOR_HASH_TABLE(m_aName, i)
		{
			auto pDef = m_aName.GetAtHash(i);
			o.Printf("%s" FILE_EOL, StrArg<char>(pDef->get_Name()));
		}

		// Order by hash
		FOR_HASH_TABLE(m_aHash, k)
		{
			auto pDef = m_aHash.GetAtHash(k);
			o.Printf("%x = '%s'" FILE_EOL, pDef->get_HashCode(), StrArg<char>(pDef->get_Name()));
		}
		return S_OK;
	}

	bool cAtomManager::DebugTest() const
	{
		// Is the atom manager ok ?
		return m_aName.isArraySortedND();
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
		const int iRefCount = get_RefCount();
		if (iRefCount <= cAtomManager::kRefsBase)
		{
			// Remove me from the cAtomManager tables.
			rAM.RemoveAtom(m_p);
		}

		// if nullptr then caller will free ref. or we are in destruct.
		put_Ptr(isLast ? nullptr : rAM.m_aEmpty.m_p);
	}

	size_t cAtomRef::GetHeapStats(OUT ITERATE_t& iAllocCount) const
	{
		//! Every user of the atom dears 1/n of the usage of the memory
		//! return SUPER_t::GetHeapStats(iAllocCount);
		CODEPROFILEFUNC();
		if (IsEmpty())	// already empty
			return 0;
		int iRefs = get_RefCount();
		ASSERT(iRefs >= 2);
		return get_Ptr()->GetHeapStatsThis(iAllocCount) / (iRefs - 1);
	}

	cAtomRef GRAYCALL cAtomRef::FindAtomStr(const ATOMCHAR_t* pszText) // static
	{
		CODEPROFILEFUNC();
		return cAtomManager::I().FindAtomStr(pszText);
	}
	cAtomRef GRAYCALL cAtomRef::FindAtomHashCode(ATOMCODE_t idAtom) // static
	{
		CODEPROFILEFUNC();
		return cAtomManager::I().FindAtomHashCode(idAtom);
	}

	void cAtomRef::SetAtomStatic()
	{
		cAtomManager::I().SetAtomStatic(get_Ptr());
	}

	cAtomRef GRAYCALL cAtomRef::FindorCreateAtomStr(const ATOMCHAR_t* pszText) noexcept // static
	{
		return cAtomManager::I().FindorCreateAtomStr(pszText);
	}

	cAtomRef GRAYCALL cAtomRef::FindorCreateAtomStr(const STR_t& sText) noexcept // static
	{
		return cAtomManager::I().FindorCreateAtomStr(sText);
	}

#ifdef _DEBUG
	HRESULT GRAYCALL cAtomRef::DebugDumpFile(const FILECHAR_t* pszFilePath) // static
	{
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
		ASSERT_NN(pszTagRet);

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
				return 0;
			}

			pszTagRet[i] = ch;
		}

		pszTagRet[i] = '\0';
		return i;
	}
}
