//
//! @file cString.h
//! Create a UTF8/UNICODE interchangeable string.
//! ref counted string class.
//! STL C string emulating. NOT MFC.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cString_H
#define _INC_cString_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArrayT.h"
#include "StrT.h"

namespace Gray
{
	class GRAYCORE_LINK cStreamInput;
	class GRAYCORE_LINK cStreamOutput;
	class GRAYCORE_LINK cArchive;

	template< typename _TYPE_CH = char >
	class GRAYCORE_LINK cStringDataT final : public cArrayDataT<_TYPE_CH>
	{
		typedef cArrayDataT<_TYPE_CH> SUPER_t;
		typedef cStringDataT<_TYPE_CH> THIS_t;
		CHEAPOBJECT_IMPL;

	public:
		/// <summary>
		/// Allocate space for chars plus '\0'
		/// </summary>
		/// <param name="nCharCount"></param>
		/// <returns></returns>
		static THIS_t* CreateStringData(StrLen_t nCharCount)
		{
			ASSERT(nCharCount >= 0);
			return reinterpret_cast<THIS_t*>(SUPER_t::CreateData(nCharCount + 1));
		}
		static THIS_t* CreateStringData2(StrLen_t nCharCount, const _TYPE_CH* pSrc)
		{
			THIS_t* p = CreateStringData(nCharCount);
			if (pSrc != nullptr)
			{
				// init
				StrT::CopyLen(p->get_Data(), pSrc, nCharCount + 1);
			}
			return p;
		}

		inline const _TYPE_CH* get_Name() const noexcept
		{
			//! supported for cArraySort. NON hash sort.
			return get_Data();
		}
		inline StrLen_t get_CharCount() const noexcept
		{
			return get_Count() - 1;	// remove '\0';
		}
		void put_CharCount(StrLen_t len)
		{
			put_Count(len + 1);
		}
		bool isValidString() const noexcept
		{
			const StrLen_t iLen = get_CharCount();
			if (!IsValidInsideN(iLen * sizeof(_TYPE_CH)))
				return false;		// should never happen!
			if (get_RefCount() <= 0)
				return false;		// should never happen!
			return get_Name()[iLen] == '\0';
		}
		HASHCODE32_t get_HashCode() const noexcept
		{
			const StrLen_t iLen = get_CharCount();
			if (iLen <= 0)
			{
				return k_HASHCODE_CLEAR;
			}
			if (m_HashCode == k_HASHCODE_CLEAR)
			{
				const_cast<THIS_t*>(this)->m_HashCode = StrT::GetHashCode32(get_Data(), iLen);
			}
			return m_HashCode;
		}

		COMPARE_t CompareNoCase(const ATOMCHAR_t* pStr) const noexcept
		{
			return StrT::CmpI(get_Data(), pStr);
		}
		bool IsEqualNoCase(const ATOMCHAR_t* pStr) const noexcept
		{
			// TODO: Use HashCode for speed compares.
			return StrT::CmpI(get_Data(), pStr) == COMPARE_Equal;
		}
	};

	/// <summary>
	/// Manage a pointer to cArrayDataT<_TYPE_CH>. Is reference counted to a string array that is dynamically allocated.
	/// Mimic the MFC ATL::CStringT<> functionality.
	/// </summary>
	/// <typeparam name="_TYPE_CH">char or wchar_t</typeparam>

	template< typename _TYPE_CH = char >
	class GRAYCORE_LINK cStringBaseT
	{
		typedef cStringBaseT<_TYPE_CH> THIS_t;

	protected:
		typedef cStringDataT<_TYPE_CH> DATA_t;
		_TYPE_CH* m_pchData;	//!< points into DATA_t[1] like cRefPtr<> but is offset.
		static const _TYPE_CH m_Nil;		//!< '\0' Use this instead of nullptr. ala MFC. also like _afxDataNil. AKA cStrConst::k_Empty ?

	public:
		cStringBaseT() noexcept
		{
			Init();
		}
		cStringBaseT(const wchar_t* pwText)
		{
			//! Init and convert UNICODE -> UTF8 if needed.
			Init();
			Assign(pwText);
		}
		cStringBaseT(const wchar_t* pwText, StrLen_t iLenMax)
		{
			//! @arg iLenMax = STRMAX = _countof(StrT::k_LEN_MAX)-1 default
			//! @arg iLenMax = max chars, not including '\0' ! NOT sizeof() or _countof() like StrT::CopyLen
			Init();
			AssignLen(pwText, iLenMax);
		}
		cStringBaseT(const char* pszStr)
		{
			//! Init and convert UNICODE is needed.
			Init();
			Assign(pszStr);
		}
		cStringBaseT(const char* pszStr, StrLen_t iLenMax)
		{
			//! @arg iLenMax = _countof(StrT::k_LEN_MAX)-1 default
			//! @arg iLenMax = max chars, not including '\0' ! NOT sizeof() or _countof() like StrT::CopyLen
			Init();
			AssignLen(pszStr, iLenMax);
		}
		cStringBaseT(const THIS_t& ref) noexcept
		{
			AssignFirst(ref);
		}
		cStringBaseT(THIS_t&& ref) noexcept
		{
			//! Move constructor
			m_pchData = ref.m_pchData;
			ref.Init();
		}
		~cStringBaseT()
		{
			Empty();
		}

		/// <summary>
		/// Get my internal storage object pointer. like MFC
		/// </summary>
		/// <returns></returns>
		const DATA_t* GetData() const noexcept
		{
			DEBUG_CHECK(m_pchData != &m_Nil);
			DEBUG_CHECK(m_pchData != nullptr);
			return (reinterpret_cast<const DATA_t*>(m_pchData)) - 1;	// the block before this pointer.
		}
		DATA_t* GetData() noexcept
		{
			DEBUG_CHECK(m_pchData != &m_Nil);
			DEBUG_CHECK(m_pchData != nullptr);
			return (reinterpret_cast<DATA_t*>(m_pchData)) - 1;	// the block before this pointer.
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
			const DATA_t* const pData = GetData();
			if (pData == nullptr)
				return false;		// should never happen!
			return pData->isValidString();
		}

		bool IsEmpty() const noexcept
		{
			// like MFC.
			DEBUG_CHECK(isValidString());
			return m_pchData == &m_Nil;
		}

		/// <summary>
		/// get Number of chars. (not necessarily bytes)
		/// </summary>
		/// <returns></returns>
		StrLen_t GetLength() const noexcept
		{
			if (m_pchData == &m_Nil)
				return 0;
			const DATA_t* pData = GetData();
			DEBUG_CHECK(pData != nullptr);
			return pData->get_CharCount();
		}
		void Empty() noexcept
		{
			if (m_pchData == nullptr)	// certain off instances where it could be nullptr. arrays
				return;
			if (IsEmpty())
				return;
			EmptyValid();
		}

		const _TYPE_CH& ReferenceAt(StrLen_t nIndex) const       // 0 based
		{
			// AKA ElementAt()
			ASSERT(nIndex <= GetLength());
			return m_pchData[nIndex];
		}

		/// <summary>
		/// Get a character.
		/// </summary>
		/// <param name="nIndex"></param>
		/// <returns></returns>
		_TYPE_CH GetAt(StrLen_t nIndex) const      // 0 based
		{
			ASSERT(nIndex <= GetLength());	// allow to get the '\0' char
			return m_pchData[nIndex];
		}
		/// <summary>
		/// Set a character.
		/// </summary>
		/// <param name="nIndex"></param>
		/// <param name="ch"></param>
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

		void FormatV(const _TYPE_CH* pszFormat, va_list args);
		void _cdecl Format(const _TYPE_CH* pszFormat, ...)
		{
			//! format a string using the sprintf() style.
			//! @note Use StrArg<_TYPE_CH>(s) for safe "%s" args.
			va_list vargs;
			va_start(vargs, pszFormat);
			FormatV(pszFormat, vargs);
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
		bool IsEqualNoCase(const _TYPE_CH* pszStr) const noexcept
		{
			// TODO: Use HashCode for speed compares.
			return StrT::CmpI(GetString(), pszStr) == 0;
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
		operator const _TYPE_CH* () const noexcept       // as a C string
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

		/// <summary>
		/// insert substring at zero-based index; 
		/// concatenates if index is past end of string
		/// </summary>
		/// <param name="nIndex"></param>
		/// <param name="pszStr"></param>
		/// <param name="iLenCat"></param>
		/// <returns>New length.</returns>
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
		void EmptyValid() noexcept
		{
			// ASSUME NOT m_Nil. Use m_Nil for empty.
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

	//***********************************************************************************************************

	/// <summary>
	/// A string with added functions over the MFC CString core set.
	/// Unlike STL std::string this is shareable via reference count. No dynamic copied each time.
	/// Use this for best string functionality.
	/// </summary>
	/// <typeparam name="_TYPE_CH"></typeparam>
	template< typename _TYPE_CH = char >
	class GRAYCORE_LINK cStringT
		: public cStringBaseT<_TYPE_CH>
	{
		typedef cStringBaseT<_TYPE_CH> SUPER_t;
		typedef cStringT<_TYPE_CH> THIS_t;

	public:
		typedef _TYPE_CH CharType_t;		//!< ALA std::string::value_type

		cStringT() noexcept
		{}
		cStringT(SUPER_t& str) noexcept : SUPER_t(str)	 // copy constructor.
		{}
		cStringT(const char* pszText) : SUPER_t(pszText)
		{}
		explicit cStringT(DATA_t* pData)
		{
			// Assign internal data object directly. weird usage.
			DEBUG_CHECK(pData != nullptr);
			pData->IncRefCount();
			m_pchData = pData->get_Data();
			DEBUG_CHECK(isValidString());
		}

		/// <summary>
		/// Copy string
		/// </summary>
		/// <param name="pszText"></param>
		/// <param name="iLenMax">max chars, not including '\0' ! NOT sizeof() or _countof() like StrT::CopyLen. _countof(StrT::k_LEN_MAX)-1 default.</param>
		cStringT(const char* pszText, StrLen_t iLenMax) : SUPER_t(pszText, iLenMax)
		{
		}
		cStringT(const wchar_t* pwText) : SUPER_t(pwText)
		{}
		cStringT(const wchar_t* pwText, StrLen_t iLenMax) : SUPER_t(pwText, iLenMax)
		{
			//! iLenMax = _countof(StrT::k_LEN_MAX)-1 default
			//! iLenMax = max chars, not including '\0' ! NOT sizeof() or _countof() like StrT::CopyLen
		}

		const _TYPE_CH* get_CPtr() const noexcept
		{
			return SUPER_t::GetString();
		}

		bool isPrintableString() const noexcept
		{
			if (SUPER_t::m_pchData == &SUPER_t::m_Nil)
				return true;
			const DATA_t* pData = this->GetData();
			const StrLen_t iLen = pData->get_CharCount();
			ASSERT(pData->IsValidInsideN(iLen * sizeof(_TYPE_CH)));
			ASSERT(pData->get_RefCount() > 0);
			return StrT::IsPrintable(SUPER_t::m_pchData, iLen) && (SUPER_t::m_pchData[iLen] == '\0');
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

		HRESULT ReadZ(cStreamInput& File, StrLen_t iLenMax);
		bool WriteZ(cStreamOutput& File) const;

		HASHCODE32_t get_HashCode() const noexcept
		{
			if (SUPER_t::IsEmpty())
				return 0;
			return this->GetData()->get_HashCode();
		}
		size_t GetHeapStats(OUT ITERATE_t& iAllocCount) const
		{
			//! Get data allocations for all children. does not include sizeof(*this)
			if (SUPER_t::IsEmpty())
				return 0;
			return this->GetData()->GetHeapStatsThis(iAllocCount);
		}
		int get_RefCount() const
		{
			// expose internal ref count. ASSUME NOT _Nil ?
			return this->GetData()->get_RefCount();
		}
		void SetStringStatic()
		{
			//! Make this string permanent. never removed from memory.
			this->GetData()->IncRefCount();
		}

		StrLen_t SetCodePage(const wchar_t* pwText, CODEPAGE_t uCodePage = CP_UTF8);
		StrLen_t GetCodePage(OUT wchar_t* pwText, StrLen_t iLenMax, CODEPAGE_t uCodePage = CP_UTF8) const;

		THIS_t GetTrimWhitespace() const;

		HRESULT SerializeInput(cStreamInput& File, StrLen_t iLenMax = StrT::k_LEN_MAX);
		HRESULT SerializeOutput(cStreamOutput& File) const;
		HRESULT SerializeOutput(cArchive& a) const;
		HRESULT Serialize(cArchive& a);

		const THIS_t& operator = (const THIS_t& s)
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

		/// <summary>
		/// Clear this more thoroughly for security reasons. passwords, etc. ZeroSecure ?
		/// </summary>
		void SetErase()
		{
			SUPER_t::Empty();
		}

		bool Contains(const _TYPE_CH* pSubStr)
		{
			// Like .NET
			return StrT::FindStr(get_CPtr(), pSubStr) != nullptr;
		}
		bool ContainsI(const _TYPE_CH* pSubStr)
		{
			// Like .NET
			return StrT::FindStrI(get_CPtr(), pSubStr) != nullptr;
		}
		bool StartsWithI(const _TYPE_CH* pSubStr)
		{
			// Like .NET
			return StrT::StartsWithI(get_CPtr(), pSubStr);
		}
		bool EndsWithI(const _TYPE_CH* pSubStr) const
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
		void assign(const _TYPE_CH* pszStr, StrLen_t iLenCat)
		{
			*this = THIS_t(pszStr, iLenCat);
		}
		void append(const _TYPE_CH* pszStr, StrLen_t iLenCat)
		{
			this->Insert(this->GetLength(), pszStr, iLenCat);
		}
		void push_back(_TYPE_CH ch)
		{
			this->Insert(this->GetLength(), ch);
		}

		void resize(StrLen_t iSize)
		{
			SUPER_t::AllocBuffer(iSize);
		}
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

		static THIS_t _cdecl Join(const _TYPE_CH* s1, ...);
		static THIS_t _cdecl GetFormatf(const _TYPE_CH* pszFormat, ...);

		// TODO MOVE THESE SOME OTHER PLACE >?
		static THIS_t GRAYCALL GetErrorStringV(HRESULT nFormatID, void* pSource, va_list vargs);
		static THIS_t GRAYCALL GetErrorString(HRESULT nFormatID, void* pSource = nullptr);
		static THIS_t _cdecl GetErrorStringf(HRESULT nFormatID, void* pSource, ...);

		static THIS_t GRAYCALL GetSizeK(UINT64 uVal, UINT nKUnit = 1024, bool bSpace = false);
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

	template< typename _TYPE_CH >	// "= char" error C4519: default template arguments are only allowed on a class template
	void inline operator >> (cArchive& ar, cStringT< _TYPE_CH >& pOb) { pOb.Serialize(ar); }
	template< typename _TYPE_CH >
	void inline operator << (cArchive& ar, const cStringT< _TYPE_CH >& pOb) { pOb.SerializeOutput(ar); }
}

#endif // _INC_cString_H
