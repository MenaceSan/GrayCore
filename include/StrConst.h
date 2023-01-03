//
//! @file StrConst.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_StrConst_H
#define _INC_StrConst_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray
{
	typedef char ATOMCHAR_t;		//!< the char form (UNICODE or not) for an atom. (for symbolic names)

#define _AT(x)		x			//!< like __T(x) macro but a stub for no change to ATOMCHAR_t text. like CATOM_STR()
#define __TOA(x)	x			//!< like __T(x) macro for UTF8. Second layer "#define" to catch macro string arguments for x.
#define __TOW(x)	L##x		//!< convert literal "" to UNICODE. like __T(x) macro for UNICODE. or OLESTR()

#if USE_UNICODE
	typedef wchar_t GChar_t;		//!< My version of TCHAR, _TCHAR for UI operations.
#define _GT(x)		__TOW(x)	//!< like _T(x) macro for static text.
#define _GTN(c)		c##W		//!< _WIN32 name has a A or W for UTF8 or UNICODE (like _FNF)
#else
	typedef char GChar_t;			//!< My version of TCHAR, _TCHAR for UI operations.
#define _GT(x)		__TOA(x)	//!< like _T(x) macro for static text.
#define _GTN(c)		c##A		//!< _WIN32 name has a A or W for UTF8 or UNICODE (like _FNF)
#endif
#define _GTNPST(c,p)		_GTN(c)##p		//!< add a postfix.

	typedef int StrLen_t;	//!< the length of a string in chars (bytes for UTF8, wchar_t for UNICODE). or offset in characters. NOT always valid for (p1-p2) for 32vs64 bit code.
#define STRMAX(x) ((::Gray::StrLen_t)(_countof(x)-1))	//!< Get Max size of static string space. minus the '\0' terminator character.
	constexpr StrLen_t k_StrLen_UNK = -1;		//!< use the default/current length of the string argument.

#define STRLIT2(s)	(s), STRMAX(s)		//!< Macro to automatically add the size of literal string or fixed size buffer.

#define STR_NL		"\n"			//!< NL/LF for Linux format text files. (10) Use "#define" so we can concatenate strings at compile time.
#define STR_CRLF	"\r\n"			//!< CR+LF for DOS/Windows format text files. (13,10)

	class GRAYCORE_LINK cStrConst
	{
		//! @class Gray::cStrConst
		//! Produce a string constant of either UNICODE or UTF8. For use inside templates.

	public:
		const char* m_A;		//!< a UTF8 string.
		const wchar_t* m_W;		//!< the UNICODE version of m_A;

		static const StrLen_t k_TabSize = 4;	//!< default desired spaces for a tab.

		static const char k_EmptyA = '\0';		//!< like CString::m_Nil
		static const wchar_t k_EmptyW = '\0';		//!< like CString::m_Nil

		static const cStrConst k_Empty;			//!< Empty cStrConst string as &k_EmptyA or &k_EmptyW. like CString::m_Nil
		static const cStrConst k_CrLf;			//!< STR_CRLF

	public:
		cStrConst(const char* a, const wchar_t* w) noexcept
			: m_A(a), m_W(w)
		{}

		inline bool isNull() const noexcept
		{
			return m_A == nullptr;
		}

		inline operator const char*() const noexcept
		{
			// Implied type
			return m_A;
		}
		inline operator const wchar_t*() const noexcept
		{
			// Implied type
			return m_W;
		}
		inline const char* get_StrA() const noexcept
		{
			// Explicit type
			return m_A;
		}
		inline const wchar_t* get_StrW() const noexcept
		{
			// Explicit type
			return m_W;
		}
		inline const GChar_t* get_CPtr() const noexcept
		{
			// Explicit type
			//! Get the default GChar_t type.
#if USE_UNICODE
			return m_W;
#else
			return m_A;
#endif
		}

		// template derived type
		template <typename TYPE> const TYPE* Get() const noexcept;
	};

	template <>	inline const char* cStrConst::Get<char>() const noexcept { return m_A; }
	template <>	inline const wchar_t* cStrConst::Get<wchar_t>() const noexcept { return m_W; }

#define CSTRCONST(t) ::Gray::cStrConst(t,__TOW(t))	//!< define a const for both Unicode and UTF8 in templates. used at run-time not just compile time.
}
#endif
