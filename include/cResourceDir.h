//
//! @file cResourceDir.h
//! Abstraction to load resources from windows resource modules, directories or Zip files.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cResourceDir_H
#define _INC_cResourceDir_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cMime.h"
#include "cFilePath.h"
#include "cObject.h"

namespace Gray
{
	//! A resource in .RC attached to a file/module. like MAKEINTRESOURCE(). https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-makeintresourcea
	typedef WORD RESOURCEID_t;	//!< a resource id as 16 valid bits only. enum space is type specific. e.g. RT_STRING, RT_ICON, etc

	interface IResourceLoader
	{
		//! @interface Gray::IResourceLoader
		//! This factory object will load resource blobs from some source by name.
		//! Load by name and MIME type. From _WIN32 resource DLL, Directory of files, db, or Zip file.
		//! @note Does NOT support loading windows resources and getting a GDI handle back. Use _WIN32 resource wrapper directly for this (if you need a GDI handle. e.g. LoadIcon()).

		HRESULT LoadResource(const char* pszName, MIME_TYPE eMime, OUT CObject** ppObject);
	};

	class cResourceDir : public IResourceLoader
	{
		//! @class Gray::cResourceDir
		//! Load a resource from a directory of files.

	public:
		cFilePath m_Dir;	// directory holding the files i want.

	public:
		virtual HRESULT LoadResource(const char* pszName, MIME_TYPE eMime, OUT CObject** ppObject);
	};
}
#endif
