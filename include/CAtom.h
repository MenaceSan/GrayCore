//
//! @file cAtom.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cAtom_H
#define _INC_cAtom_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cString.h"
#include "HResult.h"

namespace Gray
{
	typedef HASHCODE32_t ATOMCODE_t;	//!< Hash a (case ignored) atom as a 32 bit hashcode instead of using its name/pointer. using StrT::GetHashCode32().

#define CATOM_STR(a)		a		//!< Part of a static atom quoted string. for concatenate use. e.g. "Tag_%s"
#define CATOM_CAT(a,b)		a##b	//!< https://gcc.gnu.org/onlinedocs/cpp/Concatenation.html#Concatenation
#define CATOM_N(a)	#a		//!< A static atom i know is defined some place, but i just want to use the string here. Typically used by property bag. (e.g."SymName","Root")
 
	/// <summary>
	/// A single string name shared by all.
	/// Similar to the _WIN32 ATOM GlobalAddAtom(). but not system shared of course.
	/// case independent. e.g. "THIS"=="this"=>same atom.
	/// commonly used atoms should be constructed at startup/init time: e.g. static const cAtomRef a_Root("Root");
	/// </summary>
	class GRAYCORE_LINK cAtomRef : public cRefPtr<cStringDataT<ATOMCHAR_t>>
	{
		friend class cAtomManager;

	public:
		typedef cStringA STR_t;
		typedef cAtomRef THIS_t;
		typedef cStringDataT<ATOMCHAR_t> DATA_t;
		typedef cRefPtr<DATA_t> SUPER_t;

	private:
		static cAtomRef GRAYCALL FindorCreateAtomStr(const ATOMCHAR_t* pszText) noexcept;
		static cAtomRef GRAYCALL FindorCreateAtomStr(const STR_t& sText) noexcept;

		explicit inline cAtomRef(DATA_t* pDef) noexcept 	// cAtomManager only.
			: SUPER_t(pDef)
		{
			// this is allowed to be nullptr as a temporary value. in cAtomManager. FindArgForKey, etc.
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
		cAtomRef(const ATOMCHAR_t* pszName = CATOM_STR("")) noexcept
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
			//! particular hash value is not important. Value just needs to be unique and consistent.
			DEBUG_CHECK(isValidPtr());
			return get_Ptr()->get_HashCode();
		}

		STR_t get_StrA() const noexcept
		{
			DEBUG_CHECK(isValidPtr());
			return STR_t(get_Ptr());
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
			return isValidPtr() && get_Ptr()->isValidString();
		}
		bool IsEmpty() const noexcept
		{
			return !isValidPtr() || get_Ptr()->get_CharCount() <= 0;
		}
		StrLen_t GetLength() const noexcept
		{		
			return get_Ptr()->get_CharCount();
		}

#if 0
		COMPARE_t CompareNoCase(const ATOMCHAR_t* pStr) const noexcept
		{
			DEBUG_CHECK(isValidPtr());
			return get_Ptr()->CompareNoCase(pStr);
		}
#endif

		bool IsEqualNoCase(const ATOMCHAR_t* pStr) const noexcept
		{
			DEBUG_CHECK(isValidPtr());
			return get_Ptr()->IsEqualNoCase(pStr);
		}
		bool operator==(const ATOMCHAR_t* pStr) const
		{
			return IsEqualNoCase(pStr);
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
			if (!IsEqualNoCase(pStr))
			{
				EmptyAtom(true);
				put_Ptr(FindorCreateAtomStr(pStr));
			}
			return *this;
		}
		const THIS_t& operator=(const STR_t& sStr)
		{
			if (!IsEqualNoCase(sStr))
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
		/// <summary>
		/// Make this atom permanent. never removed from the atom table.
		/// </summary>
		void SetAtomStatic();

		/// <summary>
		/// Find the atom in the atom table ONLY if it exists. DO NOT CREATE!
		/// </summary>
		/// <param name="pszText"></param>
		/// <returns>m_aEmpty atom if not found.</returns>
		static cAtomRef GRAYCALL FindAtomStr(const ATOMCHAR_t* pszText);

		/// <summary>
		/// Get this hash id if a valid atom hash exists. DO NOT CREATE!
		/// </summary>
		/// <param name="idAtom"></param>
		/// <returns>m_aEmpty atom if not found.</returns>
		static cAtomRef GRAYCALL FindAtomHashCode(ATOMCODE_t idAtomCode);

		static HRESULT GRAYCALL CheckSymbolicStr(const ATOMCHAR_t* pszTag, bool bAllowDots = false);
		static StrLen_t GRAYCALL GetSymbolicStr(OUT ATOMCHAR_t* pszTag, const ATOMCHAR_t* pszExp, bool bAllowDots = false);

#ifdef _DEBUG
		/// <summary>
		/// Dump all the atoms to a file.
		/// </summary>
		/// <param name="pszFilePath"></param>
		/// <returns></returns>
		static HRESULT GRAYCALL DebugDumpFile(const FILECHAR_t* pszFilePath);
#endif
	};
}
#endif // _INC_cAtom_H
