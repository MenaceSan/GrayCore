//
//! @file cAtom.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cAtom_H
#define _INC_cAtom_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cString.h"
#include "HResult.h"
#include "cUnitTestDecl.h"

namespace Gray
{
	UNITTEST2_PREDEF(cAtom);

	typedef HASHCODE32_t ATOMCODE_t;	//!< Encode a atom as a 32 bit hashcode instead of using its name/pointer. StrT::GetHashCode32()

#define CATOM_STR(a)		a	//!< Part of a static atom quoted string. for concatenate use. e.g. "Tag_XX"
#define CATOM_CAT(a,b)		a##b	//!< https://gcc.gnu.org/onlinedocs/cpp/Concatenation.html#Concatenation
#define CATOM_STATIC(a)		#a	//!< A static atom defined in the code. (e.g."SymName","Root")
#define CATOM_STATSTR(a)	#a	//!< A static atom i know is defined some place, but i just want to use the string here.

	class GRAYCORE_LINK cAtomDef : public cRefBase
	{
		//! @class Gray::cAtomDef
		//! Internal holder for the atom. Don't use this publicly. Use cAtomRef.
		//! A single string name shared by all.

		friend class cAtomRef;
		friend class cAtomManager;
		UNITTEST2_FRIEND(cAtom);

	private:
		cStringA m_s;				//!< the string being represented.
		ATOMCODE_t m_nHashCode;		//!< GetHashCode32() for m_s; case independent. e.g. THIS==this==same atom.

	private:
		cAtomDef(cStringA s) noexcept
			: m_s(s)
			, m_nHashCode(StrT::GetHashCode32<ATOMCHAR_t>(s, k_StrLen_UNK, 0))
		{
			// Private construct.
		}

	public:
		ATOMCODE_t get_HashCode() const noexcept
		{
			//! GetHashCode32() for m_s; case independent. e.g. THIS==this==same atom.
			return m_nHashCode;
		}
		const ATOMCHAR_t* get_Name() const noexcept
		{
			return m_s;
		}
	};
	typedef cRefPtr<cAtomDef> cAtomDefPtr;

	class GRAYCORE_LINK cAtomRef
	{
		//! @class Gray::cAtomRef
		//! A single string name shared by all.
		//! Similar to the _WIN32 ATOM GlobalAddAtom(). but not system shared of course.
		//! case independent. e.g. THIS==this=>same atom.

		friend class cAtomManager;
		typedef cStringA STR_t;
		typedef cAtomRef THIS_t;
		UNITTEST2_FRIEND(cAtom);

	private:
		cAtomDefPtr	m_pDef;		//!< a counted reference to an atom. NOT allowed to be nullptr!

	private:
		static cAtomDefPtr GRAYCALL FindorCreateAtomStr(const ATOMCHAR_t* pszText);
		static cAtomDefPtr GRAYCALL FindorCreateAtomStr(const STR_t& sText);

		explicit inline cAtomRef(cAtomDef* pDef)  	// cAtomManager only.
			: m_pDef(pDef)
		{
		}

		void EmptyAtom(bool isLast);

	public:
		cAtomRef(const THIS_t& ref) noexcept
			: m_pDef(ref.m_pDef)
		{
			// copy
			DEBUG_CHECK(m_pDef != nullptr);
		}
		cAtomRef(const STR_t& sName) noexcept
			: m_pDef(FindorCreateAtomStr(sName))
		{
			DEBUG_CHECK(m_pDef != nullptr);
		}
		cAtomRef(const ATOMCHAR_t* pszName = "") noexcept
			: m_pDef(FindorCreateAtomStr(pszName))
		{
			DEBUG_CHECK(m_pDef != nullptr);
		}
		~cAtomRef()
		{
			EmptyAtom(true);
		}

		size_t GetHeapStats(OUT ITERATE_t& iAllocCount) const;

		ATOMCODE_t get_HashCode() const noexcept
		{
			//! particular hash value is not important.
			//! Value just needs to be unique and consistent on a single machine.
			DEBUG_CHECK(m_pDef != nullptr);
			return m_pDef->get_HashCode();
		}

		const STR_t& get_StrA() const noexcept
		{
			return m_pDef->m_s;
		}
		const ATOMCHAR_t* get_CPtr() const noexcept       //!< as a C string
		{
			return m_pDef->m_s;
		}
		operator const ATOMCHAR_t* () const noexcept       //!< as a C string
		{
			return m_pDef->m_s;
		}

		bool isValidCheck() const noexcept
		{
			return m_pDef != nullptr && m_pDef->m_s.isValidCheck();
		}
		bool IsEmpty() const 
		{
			return m_pDef->m_s.IsEmpty();
		}
		StrLen_t GetLength() const
		{
			return m_pDef->m_s.GetLength();
		}

		bool operator==(const cAtomRef& atom) const noexcept
		{
			return(m_pDef == atom.m_pDef);
		}
		COMPARE_t CompareNoCase(const ATOMCHAR_t* pStr) const
		{
			return m_pDef->m_s.CompareNoCase(pStr);
		}
		bool operator==(const ATOMCHAR_t* pStr) const
		{
			return !CompareNoCase(pStr);
		}

		const THIS_t& operator=(const THIS_t& atom)
		{
			if (m_pDef != atom.m_pDef)
			{
				EmptyAtom(true);
				m_pDef = atom.m_pDef;
			}
			return *this;
		}
		const THIS_t& operator=(const ATOMCHAR_t* pStr)
		{
			if (CompareNoCase(pStr) != COMPARE_Equal)
			{
				EmptyAtom(true);
				m_pDef = FindorCreateAtomStr(pStr);
			}
			return *this;
		}
		const THIS_t& operator=(const STR_t& sStr)
		{
			if (CompareNoCase(sStr) != COMPARE_Equal)
			{
				EmptyAtom(true);
				m_pDef = FindorCreateAtomStr(sStr);
			}
			return *this;
		}

		void EmptyAtom()
		{
			EmptyAtom(false);
		}
		void SetAtomStatic();

		static void GRAYCALL CreateStaticAtoms(const ATOMCHAR_t** ppAtoms);
		static cAtomRef GRAYCALL FindAtomStr(const ATOMCHAR_t* pszText);
		static cAtomRef GRAYCALL FindAtomHashCode(ATOMCODE_t idAtomCode);

		static HRESULT GRAYCALL CheckSymbolicStr(const ATOMCHAR_t* pszTag, bool bAllowDots = false);
		static StrLen_t GRAYCALL GetSymbolicStr(OUT ATOMCHAR_t* pszTag, const ATOMCHAR_t* pszExp, bool bAllowDots = false);

#ifdef _DEBUG
		static HRESULT GRAYCALL DebugDumpFile(const FILECHAR_t* pszFilePath);
#endif
	};
}
#endif // _INC_cAtom_H
