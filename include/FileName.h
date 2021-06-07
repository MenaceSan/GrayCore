//
//! @file FileName.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_FileName_H
#define _INC_FileName_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray
{
	// File names might be _UNICODE wchar_t or UTF8 characters.
#if USE_UNICODE_FN
	typedef wchar_t FILECHAR_t;		//!< a _UNICODE char in a file name. like TCHAR. default for _WIN32 OS file names.
#define _FN(x)		__TOW(x)	//!< like _T(x) macro for static text file names.
#define _FNF(c)		c##W		//!< _WIN32 name has a A or W for UTF8 or UNICODE
#define _FNFW(c)	c##W		//!< _WIN32 name has a W for UNICODE but not UTF8
#else
	typedef char FILECHAR_t;		//!< a UTF8 char in a file name. like TCHAR
#define _FN(x)		__TOA(x)	//!< like _T(x) macro for static text file names.
#define _FNF(c)		c##A		//!< _WIN32 name has a A or W for UTF8 or UNICODE
#define _FNFW(c)	c			//!< _WIN32 name has a W for UNICODE but not UTF8
#endif

	enum FILEOP_TYPE
	{
		//! @enum Gray::FILEOP_TYPE
		//! Operations on files. for use with FILEOP_FLAGS ?
		//! Same as WIN32 "shellapi.h" FO_MOVE, FO_COPY, FO_DELETE, FO_RENAME   for cFileDirDlg SHFileOperation
		FILEOP_MOVE = 0x0001,	// AKA FO_MOVE
		FILEOP_COPY = 0x0002,
		FILEOP_DELETE = 0x0003,
		FILEOP_RENAME = 0x0004,		// Similar to FILEOP_MOVE
	};
}

#endif
