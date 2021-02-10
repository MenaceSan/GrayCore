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
	typedef HASHCODE32_t ATOMCODE_t;	//!< Hash a (case ignored) atom as a 32 bit hashcode instead of using its name/pointer. using StrT::GetHashCode32().

#define CATOM_STR(a)		a		//!< Part of a static atom quoted string. for concatenate use. e.g. "Tag_%s"
#define CATOM_CAT(a,b)		a##b	//!< https://gcc.gnu.org/onlinedocs/cpp/Concatenation.html#Concatenation
#define CATOM_STATSTR(a)	#a		//!< A static atom i know is defined some place, but i just want to use the string here. Typically used by property bag. (e.g."SymName","Root")

	class GRAYCORE_LINK cAtomDef : public cRefBase // CStringData
	{
		//! @class Gray::cAtomDef
		//! A single string name shared by all.
		//! @note Internal holder for the atom. Don't use this publicly. Use cAtomRef.

		friend class cAtomRef;
		friend class cAtomManager;
		UNITTEST_FRIEND(cAtom);

	private:
		cStringA m_s;				//!< the string being represented. should we allocate this differently. is cStringA wasteful ?
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
			//! supported for cArraySort
			return m_nHashCode;
		}
		const ATOMCHAR_t* get_Name() const noexcept
		{
			//! supported for cArraySort
			return m_s.get_CPtr();
		}
	};
 
	class GRAYCORE_LINK cAtomRef : public cRefPtr<cAtomDef> 
	{
		//! @class Gray::cAtomRef
		//! A single string name shared by all.
		//! Similar to the _WIN32 ATOM GlobalAddAtom(). but not system shared of course.
		//! case independent. e.g. THIS==this=>same atom.
		//! commonly used atoms should be constructed at startup/init time: e.g. static const cAtomRef a_Root("Root");

		friend class cAtomManager;
		typedef cStringA STR_t;
		typedef cAtomRef THIS_t;
		typedef cRefPtr<cAtomDef> SUPER_t;

		UNITTEST_FRIEND(cAtom);
 
	private:
		static cAtomRef GRAYCALL FindorCreateAtomStr(const ATOMCHAR_t* pszText) noexcept;
		static cAtomRef GRAYCALL FindorCreateAtomStr(const STR_t& sText) noexcept;

		explicit inline cAtomRef(cAtomDef* pDef) noexcept 	// cAtomManager only.
			: SUPER_t(pDef)
		{
			// this is allowed to be nullptr as a temporary value. in cAtomManager.
		}

		void EmptyAtom(bool isLast);

	public:
		cAtomRef(const THIS_t& ref) noexcept
			: SUPER_t(ref)
		{
			// copy and inc ref.
			DEBUG_CHECK(isValidPtr());
		}
		cAtomRef(const STR_t& sName) noexcept
			: SUPER_t(FindorCreateAtomStr(sName))
		{
			DEBUG_CHECK(isValidPtr());
		}
		cAtomRef(const ATOMCHAR_t* pszName = "") noexcept
			: SUPER_t(FindorCreateAtomStr(pszName))
		{
			//! @note cAtomRef can be defined in the C++ init code. So the cAtomManager must be safe to run at init time.
			DEBUG_CHECK(isValidPtr());
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
			DEBUG_CHECK(isValidPtr());
			return get_Ptr()->get_HashCode();
		}

		const STR_t& get_StrA() const noexcept
		{
			DEBUG_CHECK(isValidPtr());
			return get_Ptr()->m_s;
		}
		const ATOMCHAR_t* get_CPtr() const noexcept       //!< as a C string
		{
			DEBUG_CHECK(isValidPtr());
			return get_Ptr()->get_Name();
		}
		operator const ATOMCHAR_t* () const noexcept       //!< as a C string
		{
			DEBUG_CHECK(isValidPtr());
			return get_Ptr()->get_Name();
		}

		bool isValidCheck() const noexcept
		{
			return isValidPtr() && get_Ptr()->m_s.isValidCheck();
		}
		bool IsEmpty() const noexcept
		{
			return !isValidPtr() || get_Ptr()->m_s.IsEmpty();
		}
		StrLen_t GetLength() const noexcept
		{		
			return get_Ptr()->m_s.GetLength();
		}

		COMPARE_t CompareNoCase(const ATOMCHAR_t* pStr) const
		{
			DEBUG_CHECK(isValidPtr());
			return get_Ptr()->m_s.CompareNoCase(pStr);
		}
		bool operator==(const ATOMCHAR_t* pStr) const
		{
			return !CompareNoCase(pStr);
		}

		const THIS_t& operator=(const THIS_t& atom)
		{
			if (!IsEqual(atom))
			{
				EmptyAtom(true);
				put_Ptr(atom);
			}
			return *this;
		}
		const THIS_t& operator=(const ATOMCHAR_t* pStr)
		{
			if (CompareNoCase(pStr) != COMPARE_Equal)
			{
				EmptyAtom(true);
				put_Ptr(FindorCreateAtomStr(pStr));
			}
			return *this;
		}
		const THIS_t& operator=(const STR_t& sStr)
		{
			if (CompareNoCase(sStr) != COMPARE_Equal)
			{
				EmptyAtom(true);
				put_Ptr(FindorCreateAtomStr(sStr));
			}
			return *this;
		}

		void EmptyAtom()
		{
			EmptyAtom(false);
		}
		void SetAtomStatic();

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
