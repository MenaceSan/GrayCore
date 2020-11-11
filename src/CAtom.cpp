//
//! @file cAtom.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cAtom.h"
#include "cArraySortRef.h"
#include "cSingleton.h"
#include "CFile.h"
#include "cCodeProfiler.h"
#include "cThreadLock.h"
#include "cLogMgr.h"

namespace Gray
{
	class cAtomManager
		: public cSingleton < cAtomManager >
	{
		//! @class Gray::cAtomManager
		//! an alpha sorted string lookup table. CASE IGNORED !
		friend class cSingleton < cAtomManager >;
		friend class cAtomRef;

	private:
		cAtomDefPtr m_pDefEmpty;	//!< "\0" atom. never use nullptr. ATOMCODE_t = 0.

		mutable cThreadLockCount m_Lock;	// make it thread safe.
		cArraySortName< cAtomDef, ATOMCHAR_t > m_aName;	//!< Sorted by ATOMCHAR_t Text. NO DUPES
		cArraySortHash< cAtomDef, ATOMCODE_t > m_aHash;	//!< Sorted by ATOMCODE_t GetHashCode32(s) NO DUPES.
		cArraySortHash< cAtomDef, ATOMCODE_t > m_aStatic;	//!< Put atoms here to persist. NO DUPES.

	public:
		cAtomManager()
			: cSingleton<cAtomManager>(this, typeid(cAtomManager))
		{
			//! created on demand to prevent any race conditions at static create time.
			m_pDefEmpty = new cAtomDef("");
			m_aName.Add(m_pDefEmpty);
			m_aHash.Add(m_pDefEmpty);
		}
		~cAtomManager()
		{
			DEBUG_MSG(("~cAtomManager"));
		}

		cAtomDefPtr FindAtomStr(const ATOMCHAR_t* pszText) const
		{
			if (StrT::IsNullOrEmpty(pszText))
				return m_pDefEmpty;
			cThreadGuard lock(m_Lock);
			cAtomDefPtr pDef = m_aName.FindArgForKey(pszText);
			if (pDef == nullptr)
				return m_pDefEmpty;
			return pDef;
		}
		cAtomDefPtr FindAtomHashCode(ATOMCODE_t idAtom) const
		{
			//! Is this hash id a valid atom hash ?
			//! this is 32 bit specific?
			if (idAtom == k_HASHCODE_CLEAR)
				return m_pDefEmpty;
			cThreadGuard lock(m_Lock);
			cAtomDefPtr pDef = m_aHash.FindArgForKey(idAtom);
			if (pDef == nullptr)
				return m_pDefEmpty;
			return pDef;
		}

		bool RemoveAtom(cAtomDef* pDef)
		{
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

		cAtomDefPtr CreateAtom(ITERATE_t index, COMPARE_t iCompareRes, cStringA sVal)
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
			return(pDef);
		}

		cAtomDefPtr FindorCreateAtomStr(const cStringA& sName)
		{
			//! Find the atom in the atom table if it exists else create a new one.
			//! Does NOT return nullptr

			if (sName.IsEmpty())
				return m_pDefEmpty;
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

		cAtomDefPtr FindorCreateAtomStr(const ATOMCHAR_t* pszName)
		{
			//! Find the atom in the atom table if it exists else create a new one.
			if (StrT::IsNullOrEmpty(pszName))
				return m_pDefEmpty;
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

		void SetAtomStatic(cAtomDef* pDef)
		{
			//! Make this atom permanent. never removed from the atom table.
			m_aStatic.Add(pDef);
		}

#ifdef _DEBUG
		HRESULT DebugDumpFile(cStreamOutput& o) const
		{
			cThreadGuard lock(m_Lock);
			for (ITERATE_t i = 0; i < m_aName.GetSize(); i++)
			{
				o.Printf("%s" FILE_EOL, StrArg<char>(m_aName.GetAt(i)->get_Name()));
			}
			return S_OK;
		}
#endif
		CHEAPOBJECT_IMPL;
		UNITTEST_FRIEND(cAtomRef)
	};

	//*********************************
	// cAtomRef

	void cAtomRef::EmptyAtom()
	{
		//! Delete if there are no more refs
		CODEPROFILEFUNC();
		if (IsEmpty())	// already empty
			return;

		cAtomManager& rAM = cAtomManager::I();

		// Remove the cAtomRef from the table if last use.
		// is this is good idea ??
		int iRefCount = m_pDef->get_RefCount();
		if (iRefCount <= 3)
		{
			// Remove me from the table.
			// NOTE: This can leak if there are outstanding cString referenced versions ?
			rAM.RemoveAtom(m_pDef);
		}

		m_pDef = rAM.m_pDefEmpty;
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
		//! @return "\0" atom if not found.
		CODEPROFILEFUNC();
		return cAtomRef(cAtomManager::I().FindAtomStr(pszText));
	}

	cAtomRef GRAYCALL cAtomRef::FindAtomHashCode(ATOMCODE_t idAtom) // static
	{
		//! Is this hash id a valid atom hash ?
		//! Don't create a new atom if don't find it.
		CODEPROFILEFUNC();
		return cAtomRef(cAtomManager::I().FindAtomHashCode(idAtom));
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
};

//*************************************************************************
#if USE_UNITTESTS
#include "cUnitTest.h"

UNITTEST_CLASS(cAtomRef)
{
	UNITTEST_METHOD(cAtomRef)
	{
		//! k_asTextLines
		cAtomManager& rAM = cAtomManager::I();
		cAtomRef a0 = CATOM_STR("");	// empty hash.
		UNITTEST_TRUE(a0.get_HashCode() == 0);
		// UNITTEST_TRUE( rAM.m_pDefEmpty == a0 );

		cAtomRef a2(CATOM_STR("test1"));
		cAtomRef a1 = CATOM_STR("test2");
		a2 = CATOM_STR("test3");
		a2 = a1;
		a1 = CATOM_STR("test3");

		UNITTEST_TRUE(cAtomRef::FindAtomStr(CATOM_STR("test1")).IsEmpty());	// should be gone!
		UNITTEST_TRUE(!cAtomRef::FindAtomStr(CATOM_STR("test3")).IsEmpty());	// should find it.

		{
			cAtomRef aa[k_TEXTLINES_QTY];
			for (ITERATE_t i = 0; i < k_TEXTLINES_QTY; i++)
			{
				aa[i] = k_asTextLines[i];
			}
		}

		UNITTEST_TRUE(rAM.m_aName.isArraySorted());

		// Test hash codes.
		ATOMCODE_t nHashCode = a1.get_HashCode();
		a2 = cAtomRef::FindAtomHashCode(nHashCode);
		UNITTEST_TRUE(!a2.IsEmpty());

		// Test bad hash code.
		a2 = cAtomRef::FindAtomHashCode(0x123123);
		UNITTEST_TRUE(a2.IsEmpty());

		ATOMCHAR_t szTmp[_MAX_PATH];
		static const ATOMCHAR_t* k_t2 = CATOM_STR("sdfsdF23 5");	// lower case = higher number ASCII.
		StrLen_t nLen = cAtomRef::GetSymbolicStr(szTmp, k_t2, false);
		UNITTEST_TRUE(nLen == 8);

		HRESULT hRes = cAtomRef::CheckSymbolicStr(szTmp);
		UNITTEST_TRUE(SUCCEEDED(hRes));

		hRes = cAtomRef::CheckSymbolicStr(CATOM_STR("sdfsdf"));
		UNITTEST_TRUE(hRes == 6);

		hRes = cAtomRef::CheckSymbolicStr(CATOM_STR("sdfsdf s"));
		UNITTEST_TRUE(hRes == E_INVALIDARG);

	}
};
UNITTEST_REGISTER(cAtomRef, UNITTEST_LEVEL_Core);
#endif
