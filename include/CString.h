//
//! @file cString.h
//! Create a UTF8/UNICODE interchangeable string.
//! ref counted string class.
//! STL C string emulating.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cString_H
#define _INC_cString_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cHeapObject.h"
#include "cRefPtr.h"
#include "StrT.h"
#include "cUnitTestDecl.h"

#if defined(_MFC_VER) // Only need minimal sub-set if using MFC. __ATLSTR_H__
#include <cstringt.h>	// __ATLSIMPSTR_H__
#endif

namespace Gray
{
	class GRAYCORE_LINK cStreamInput;
	class GRAYCORE_LINK cStreamOutput;
	class GRAYCORE_LINK cArchive;

#if defined(_MFC_VER) // Only need minimal sub-set if using MFC. __ATLSTR_H__
	// NOTE: TCHAR is always wchar_t for MFC
#define cStringT_DEF(_TYPE_CH) ATL::CStringT< _TYPE_CH, StrTraitMFC_DLL< _TYPE_CH > >

#else

	class GRAYCORE_LINK CStringData : public cRefBase, public cHeapObject	// ref count
	{
		//! @class Gray::CStringData
		//! Equiv to the MFC ATL:CStringData. Variable size allocation for this.
		//! May contain UTF8 or UNICODE string.
		CHEAPOBJECT_IMPL;

	private:
		//! m_nCharCount in sizeof(BaseType) (character) units. NOT always the same as bytes.
		StrLen_t m_nCharCount;	//!< count of chars (NOT including '\0' terminator) (not the same as allocation size _msize())
		// BaseType m_data[ m_nCharCount * sizeof(BaseType) ];
		// BaseType m_nullchar;	// terminated with '\0' char

	public:
		void* GetString() const noexcept          // char/wchar_* to managed data
		{
			//! Get a pointer to the characters of the string. Stored in the space allocated after this class.
			return (void*)(this + 1); // const_cast
		}
		void* operator new(size_t stAllocateBlock, StrLen_t iStringLengthBytes)
		{
			//! @note Make sure this is compatible with realloc() !!
			//! must set m_nCharCount after this !
			//! iStringLengthBytes = length in bytes includes space for '\0'.
			ASSERT(iStringLengthBytes >= 2);	// X + '\0'
			ASSERT(stAllocateBlock == sizeof(CStringData));
			return cHeap::AllocPtr(stAllocateBlock + iStringLengthBytes);
		}

		void operator delete(void* pObj, StrLen_t iStringLengthBytes)
		{
			UNREFERENCED_PARAMETER(iStringLengthBytes);
			cHeap::FreePtr(pObj);
		}
		void operator delete(void* pObj)
		{
			cHeap::FreePtr(pObj);
		}
		StrLen_t get_CharCount() const noexcept
		{
			//! @return Number of chars. (not necessarily bytes)
			return m_nCharCount;
		}
		void put_CharCount(StrLen_t nCount) noexcept
		{
			//! @return Number of chars. (not necessarily bytes). NOT including '/0'
			m_nCharCount = nCount;
		}
	};

	// make sure the new length of the string is set correctly on destruct !
	typedef cRefPtr<CStringData> CStringDataPtr;	//!< Pointer to hold a ref to the string data so i can thread lock it.

	template< typename _TYPE_CH = char >
	class GRAYCORE_LINK CStringT
	{
		//! @class Gray::CStringT
		//! Mimic the MFC ATL::CStringT<> functionality.

		typedef CStringT<_TYPE_CH> THIS_t;

	protected:
		_TYPE_CH* m_pchData;	//!< points into CStringData[1]
		static const _TYPE_CH m_Nil;		//!< Use this instead of nullptr. ala MFC. also like _afxDataNil. AKA cStrConst::k_Empty ?

	public:
		CStringT() noexcept
		{
			Init();
		}
		CStringT(const wchar_t* pwText)
		{
			//!? Init and convert UNICODE is needed.
			Init();
			Assign(pwText);
		}
		CStringT(const wchar_t* pwText, StrLen_t iLenMax)
		{
			//! @arg iLenMax = STRMAX = _countof(StrT::k_LEN_MAX)-1 default
			//! @arg iLenMax = max chars, not including '\0' ! NOT sizeof() or _countof() like StrT::CopyLen
			Init();
			AssignLen(pwText, iLenMax);
		}
		CStringT(const char* pszStr)
		{
			//! Init and convert UNICODE is needed.
			Init();
			Assign(pszStr);
		}
		CStringT(const char* pszStr, StrLen_t iLenMax)
		{
			//! @arg iLenMax = _countof(StrT::k_LEN_MAX)-1 default
			//! @arg iLenMax = max chars, not including '\0' ! NOT sizeof() or _countof() like StrT::CopyLen
			Init();
			AssignLen(pszStr, iLenMax);
		}
		CStringT(const THIS_t& ref) noexcept
		{
			AssignFirst(ref);
		}
		CStringT(THIS_t&& ref) noexcept
		{
			//! Move constructor
			m_pchData = ref.m_pchData;
			ref.Init();
		}
		~CStringT()
		{
			Empty();
		}

		CStringData* GetData() const noexcept
		{
			//! like MFC
			DEBUG_CHECK(m_pchData != &m_Nil);
			DEBUG_CHECK(m_pchData != nullptr);
			return (reinterpret_cast<CStringData*>(m_pchData)) - 1;	// the block before this pointer.
		}
		const _TYPE_CH* GetString() const noexcept
		{
			//! like MFC
			DEBUG_CHECK(isValidString());
			return m_pchData;
		}

		bool isValidString() const noexcept
		{
			//! Is the string properly terminated?
			if (m_pchData == &m_Nil)
				return true;
			CStringData* const pData = GetData();
			if (pData == nullptr)
				return false;		// should never happen!
			const StrLen_t iLen = pData->get_CharCount();
			if (!pData->IsValidInsideN(iLen * sizeof(_TYPE_CH)))
				return false;		// should never happen!
			if (pData->get_RefCount() <= 0)
				return false;		// should never happen!
			return m_pchData[iLen] == '\0';
		}

		bool IsEmpty() const noexcept
		{
			// like MFC.
			DEBUG_CHECK(isValidString());
			return m_pchData == &m_Nil;
		}

		StrLen_t GetLength() const noexcept
		{
			//! @return Number of chars. (not necessarily bytes)
			if (m_pchData == &m_Nil)
				return 0;
			// DEBUG_CHECK(isValidString());
			const CStringData* pData = GetData();
			DEBUG_CHECK(pData != nullptr);
			return pData->get_CharCount();
		}
		void Empty()
		{
			if (m_pchData == nullptr)	// certain off instances where it could be nullptr. arrays
				return;
			if (IsEmpty())
				return;
			EmptyValid();
		}

		const _TYPE_CH& ReferenceAt(StrLen_t nIndex) const       // 0 based
		{
			ASSERT(nIndex <= GetLength());
			return m_pchData[nIndex];
		}
		_TYPE_CH GetAt(StrLen_t nIndex) const      // 0 based
		{
			ASSERT(nIndex <= GetLength());	// allow to get the '\0' char
			return m_pchData[nIndex];
		}
		void SetAt(StrLen_t nIndex, _TYPE_CH ch)
		{
			ASSERT(IS_INDEX_GOOD(nIndex, GetLength()));
			CopyBeforeWrite();
			m_pchData[nIndex] = ch;
			ASSERT(isValidString());
		}

		_TYPE_CH* GetBuffer(StrLen_t iMinLength);
		void ReleaseBuffer(StrLen_t nNewLength = k_StrLen_UNK);

		const THIS_t& operator = (const THIS_t& ref)
		{
			//! Copy assignment
			Assign(ref);
			return *this;
		}
		const THIS_t& operator = (THIS_t&& ref)
		{
			//! Move assignment
			m_pchData = ref.m_pchData;
			ref.Init();
			return *this;
		}

		void AssignLenT(const _TYPE_CH* pszStr, StrLen_t iLenMax);

		// UTF8. auto converted
		void AssignLen(const char* pszStr, StrLen_t iSizeMax = StrT::k_LEN_MAX);
		const THIS_t& operator = (const char* pStr)
		{
			Assign(pStr);
			return *this;
		}

		// UNICODE. auto converted
		void AssignLen(const wchar_t* pwText, StrLen_t iSizeMax = StrT::k_LEN_MAX);
		const THIS_t& operator = (const wchar_t* pStr)
		{
			Assign(pStr);
			return *this;
		}

		void FormatV(const _TYPE_CH* pszStr, va_list args);
		void _cdecl Format(const _TYPE_CH* pszStr, ...)
		{
			//! use the normal sprintf() style.
			//! @note Use StrArg<GChar_t>(s) for safe "%s" args.
			va_list vargs;
			va_start(vargs, pszStr);
			FormatV(pszStr, vargs);
			va_end(vargs);
		}
		COMPARE_t Compare(const _TYPE_CH* pszStr) const noexcept
		{
			return StrT::Cmp(GetString(), pszStr);
		}
		COMPARE_t CompareNoCase(const _TYPE_CH* pszStr) const noexcept
		{
			return StrT::CmpI(GetString(), pszStr);
		}
		void MakeUpper();	// MFC like not .NET like.
		void MakeLower();
		THIS_t Left(StrLen_t nCount) const;
		THIS_t Right(StrLen_t nCount) const;
		THIS_t Mid(StrLen_t nFirst, StrLen_t nCount = StrT::k_LEN_MAX) const;

		void TrimRight();
		void TrimLeft();
		StrLen_t Find(_TYPE_CH ch, StrLen_t nPosStart = 0) const;

		_TYPE_CH operator[](StrLen_t nIndex) const // same as GetAt
		{
			return GetAt(nIndex);
		}
		const _TYPE_CH& operator[](StrLen_t nIndex)
		{
			return ReferenceAt(nIndex);
		}
		operator const _TYPE_CH* () const       // as a C string
		{
			return GetString();
		}

		friend bool operator==(const THIS_t& str1, const _TYPE_CH* str2) THROW_DEF
		{
			return str1.Compare(str2) == COMPARE_Equal;
		}
		friend bool operator!=(const THIS_t& str1, const _TYPE_CH* str2) THROW_DEF
		{
			return str1.Compare(str2) != COMPARE_Equal;
		}

		// insert character at zero-based index; concatenates
		// if index is past end of string
		StrLen_t Insert(StrLen_t nIndex, _TYPE_CH ch);
		const THIS_t& operator += (_TYPE_CH ch)
		{
			Insert(GetLength(), ch);
			return *this;
		}

		//! insert substring at zero-based index; concatenates
		//! if index is past end of string
		StrLen_t Insert(StrLen_t nIndex, const _TYPE_CH* pszStr, StrLen_t iLenCat);
		StrLen_t Insert(StrLen_t nIndex, const _TYPE_CH* pszStr)
		{
			return Insert(nIndex, pszStr, k_StrLen_UNK);
		}
		const THIS_t& operator += (const _TYPE_CH* psz)	// like strcat()
		{
			Insert(GetLength(), psz, k_StrLen_UNK);
			return *this;
		}

#if 0
		THIS_t operator + (const _TYPE_CH* pszStr) const
		{
			THIS_t sTmp(*this);
			sTmp.Insert(GetLength(), pszStr, k_StrLen_UNK);
			return sTmp;
		}
#endif

		void Assign(const THIS_t& str)
		{
			if (m_pchData == str.GetString()) // already same.
				return;
			Empty();
			AssignFirst(str);
		}
		void Assign(const wchar_t* pwText);
		void Assign(const char* pszStr);

	protected:
		void Init() noexcept
		{
			m_pchData = const_cast<_TYPE_CH*>(&m_Nil);
		}
		void EmptyValid()
		{
			// Use m_Nil for empty.
			DEBUG_CHECK(isValidString());
			GetData()->DecRefCount();
			Init();
		}

		void AssignFirst(const THIS_t& s) noexcept
		{
			m_pchData = s.m_pchData;
			if (m_pchData == &m_Nil)
				return;
			GetData()->IncRefCount();
			DEBUG_CHECK(isValidString());
		}

		void AllocBuffer(StrLen_t iStrLength);
		void CopyBeforeWrite();
	};

#define cStringT_DEF(t) CStringT<t>

#endif // ! _MFC_VER

	//***********************************************************************************************************

	template< typename _TYPE_CH = char >
	class GRAYCORE_LINK cStringT
		: public cStringT_DEF(_TYPE_CH)
	{
		//! @class Gray::cStringT
		//! A string with added functions over the MFC CString core set.
		//! Use this for best string functionality.

		typedef cStringT_DEF(_TYPE_CH) SUPER_t;
		typedef cStringT<_TYPE_CH> THIS_t;

	public:
		typedef _TYPE_CH CharType_t;		//!< ALA std::string::value_type

		cStringT() noexcept
		{}
		cStringT(SUPER_t & str) noexcept : SUPER_t(str)
		{}
		cStringT(const char* pszText) : SUPER_t(pszText)
		{}
		cStringT(const char* pszText, StrLen_t iLenMax) : SUPER_t(pszText, iLenMax)
		{
			//! iLenMax = _countof(StrT::k_LEN_MAX)-1 default. not including '\0'
			//! iLenMax = max chars, not including '\0' ! NOT sizeof() or _countof() like StrT::CopyLen
		}
		cStringT(const wchar_t* pwText) : SUPER_t(pwText)
		{}
		cStringT(const wchar_t* pwText, StrLen_t iLenMax) : SUPER_t(pwText, iLenMax)
		{
			//! iLenMax = _countof(StrT::k_LEN_MAX)-1 default
			//! iLenMax = max chars, not including '\0' ! NOT sizeof() or _countof() like StrT::CopyLen
		}

#if defined(_MFC_VER)
		CStringData* GetData() const
		{
			//! In some versions of MFC GetData() is protected!? This Fixes that.
			return(((CStringData*)(const _TYPE_CH*)SUPER_t::GetString()) - 1);
		}
#endif

		const _TYPE_CH* get_CPtr() const noexcept
		{
			return SUPER_t::GetString();
		}

		bool isPrintableString() const noexcept
		{
#if defined(_MFC_VER)
			return true;
#else
			if (SUPER_t::m_pchData == &SUPER_t::m_Nil)
				return true;
			CStringData* pData = this->GetData();
			StrLen_t iLen = pData->get_CharCount();
			ASSERT(pData->IsValidInsideN(iLen * sizeof(_TYPE_CH)));
			ASSERT(pData->get_RefCount() > 0);
			return StrT::IsPrintable(SUPER_t::m_pchData, iLen) && (SUPER_t::m_pchData[iLen] == '\0');
#endif
		}
		bool isValidString() const noexcept
		{
#ifdef _MFC_VER
			return true;
#elif 0 // defined(_DEBUG) && defined(DEBUG_VALIDATE_ALLOC)
			return IsPrintableString();
#else
			return SUPER_t::isValidString();
#endif
		}
		bool isValidCheck() const noexcept
		{
			return isValidString();
		}
		bool IsWhitespace() const
		{
			//! Like .NET String.IsNullOrWhitespace
			return StrT::IsWhitespace(this->get_CPtr(), this->length());
		}

		HRESULT ReadZ(cStreamInput & File, StrLen_t iLenMax);
		bool WriteZ(cStreamOutput & File) const;

		HASHCODE_t get_HashCode() const noexcept
		{
			//! Get a hash code good on this machine alone.
			//! Must use something like StrT::GetHashCode32() for use on multiple machines or processes.
			return (HASHCODE_t)(UINT_PTR)(void*)get_CPtr() ;	// just use the pointer.
		}
		size_t GetHeapStats(OUT ITERATE_t & iAllocCount) const
		{
			//! Get data allocations for all children. does not include sizeof(*this)
			if (SUPER_t::IsEmpty())
				return 0;
#if defined(_MFC_VER)	// Only need minimal sub-set if using MFC.
			iAllocCount++;
			return cHeap::GetSize(GetData());
#else
			return this->GetData()->GetHeapStatsThis(iAllocCount);
#endif
		}
		int get_RefCount() const
		{
#if defined(_MFC_VER)
			return this->GetData()->nRefs;
#else
			return this->GetData()->get_RefCount();
#endif
		}
		void SetStringStatic()
		{
			//! Make this string permanent. never removed from memory.
#if defined(_MFC_VER)
			this->GetData()->nRefs++;
#else
			this->GetData()->IncRefCount();
#endif
		}

		StrLen_t SetCodePage(const wchar_t* pwText, CODEPAGE_t uCodePage = CP_UTF8);
		StrLen_t GetCodePage(OUT wchar_t* pwText, StrLen_t iLenMax, CODEPAGE_t uCodePage = CP_UTF8) const;

#ifdef _MFC_VER
		void Assign(const SUPER_t & str)
		{
			// For use with MFC and cAtomRef
			*static_cast<SUPER_t*>(this) = str;
		}
#endif

		THIS_t GetTrimWhitespace() const;

		HRESULT SerializeInput(cStreamInput & File, StrLen_t iLenMax = StrT::k_LEN_MAX);
		HRESULT SerializeOutput(cStreamOutput & File) const;
		HRESULT SerializeOutput(cArchive & a) const;
		HRESULT Serialize(cArchive & a);

		const THIS_t& operator = (const THIS_t & s)
		{
			this->Assign(s);
			return *this;
		}
		const THIS_t& operator = (const char* pszStr)
		{
			SUPER_t::operator=(pszStr);
			return *this;
		}
		const THIS_t& operator = (const wchar_t* pwStr)
		{
			SUPER_t::operator=(pwStr);
			return *this;
		}

		void SetErase()
		{
			//! Clear this more thoroughly for security reasons. passwords, etc
			//! ZeroSecure ?
			SUPER_t::Empty();
		}

		bool Contains(const _TYPE_CH * pSubStr)
		{
			// Like .NET
			return StrT::FindStr(get_CPtr(), pSubStr) != nullptr;
		}
		bool ContainsI(const _TYPE_CH * pSubStr)
		{
			// Like .NET
			return StrT::FindStrI(get_CPtr(), pSubStr) != nullptr;
		}
		bool StartsWithI(const _TYPE_CH * pSubStr)
		{
			// Like .NET
			return StrT::StartsWithI(get_CPtr(), pSubStr);
		}
		bool EndsWithI(const _TYPE_CH * pSubStr) const
		{
			// Like .NET 
			return StrT::EndsWithI<_TYPE_CH>(get_CPtr(), pSubStr, this->GetLength());
		}

		//! emulate STL string operators.
		//! Is any of this needed ??
		static const int npos = -1; // k_ITERATE_BAD. same name as STL.

		const _TYPE_CH* c_str() const
		{
			return SUPER_t::GetString();
		}
		StrLen_t size() const
		{
			// basic_string::size
			return SUPER_t::GetLength();
		}
		StrLen_t length() const
		{
			// char_traits::length
			return SUPER_t::GetLength();
		}
		bool empty() const
		{
			return SUPER_t::IsEmpty();
		}
		StrLen_t find(_TYPE_CH ch) const
		{
			//! @return npos = -1 = not found.
			return SUPER_t::Find(ch, 0);
		}
		void assign(const _TYPE_CH * pszStr, StrLen_t iLenCat)
		{
			*this = THIS_t(pszStr, iLenCat);
		}
		void append(const _TYPE_CH * pszStr, StrLen_t iLenCat)
		{
#ifdef _MFC_VER
			* this += THIS_t(pszStr, iLenCat);
#else
			this->Insert(this->GetLength(), pszStr, iLenCat);
#endif
		}
		void push_back(_TYPE_CH ch)
		{
			this->Insert(this->GetLength(), ch);
		}

#ifndef _MFC_VER
		void resize(StrLen_t iSize)
		{
			SUPER_t::AllocBuffer(iSize);
		}
#endif
		void reserve(StrLen_t iSize)
		{
			// optimize that this is the end length.
			UNREFERENCED_PARAMETER(iSize);
		}
		THIS_t substr(StrLen_t nFirst, StrLen_t nCount = StrT::k_LEN_MAX) const
		{
			// Like return SUPER_t::Mid(nFirst, nCount)
			if (nFirst >= this->GetLength())
				return cStrConst::k_Empty.Get<_TYPE_CH>();
			return THIS_t(this->GetString() + nFirst, nCount);
		}

		static THIS_t _cdecl GetFormatf(const _TYPE_CH * pszFormat, ...);

		// TODO MOVE THESE SOME OTHER PLACE >?
		static THIS_t GRAYCALL GetErrorStringV(HRESULT nFormatID, void* pSource, va_list vargs);
		static THIS_t GRAYCALL GetErrorString(HRESULT nFormatID, void* pSource = nullptr);
		static THIS_t _cdecl GetErrorStringf(HRESULT nFormatID, void* pSource, ...);

		static THIS_t GRAYCALL GetSizeK(UINT64 uVal, UINT nKUnit = 1024, bool bSpace = false);

		UNITTEST_FRIEND(cString);
	};

	typedef cStringT< wchar_t > cStringW;
	typedef cStringT< char > cStringA;
	typedef cStringT< GChar_t > cString;

#if ! defined(_MFC_VER) && ! defined(__CLR_VER)
	typedef cString CString;
#endif

	cStringA inline operator +(const char* pStr1, const cStringA& s2) // static global
	{
		cStringA s1(pStr1);
		s1 += s2;
		return s1;
	}
	cStringW inline operator +(const wchar_t* pStr1, const cStringW& s2) // static global
	{
		cStringW s1(pStr1);
		s1 += s2;
		return s1;
	}

#if defined(_MFC_VER) // resolve ambiguity with MFC
	cStringA inline operator +(cStringA s1, const cStringA& s2) // static global
	{
		s1 += s2;
		return s1;
	}
	cStringA inline operator +(cStringA s1, const char* pStr2) // static global
	{
		s1 += pStr2;
		return s1;
	}
	cStringW inline operator +(cStringW s1, const cStringW& s2) // static global
	{
		s1 += s2;
		return s1;
	}
	cStringW inline operator +(cStringW s1, const wchar_t* pStr2) // static global
	{
		s1 += pStr2;
		return s1;
	}
#else 
	template< typename _TYPE_CH >	// "= char" error C4519: default template arguments are only allowed on a class template
	void inline operator >> (cArchive& ar, cStringT< _TYPE_CH >& pOb) { pOb.Serialize(ar); }
	template< typename _TYPE_CH >
	void inline operator << (cArchive& ar, const cStringT< _TYPE_CH >& pOb) { pOb.SerializeOutput(ar); }
#endif

#undef cStringT_DEF

};

#endif // _INC_cString_H
