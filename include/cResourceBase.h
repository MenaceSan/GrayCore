//
//! @file cResourceBase.h
//! Abstraction to load resources from windows resource modules, directories or Zip files.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cResourceBase_H
#define _INC_cResourceBase_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cMime.h"
#include "cFilePath.h"

namespace Gray
{
	//! A resource in .RC attached to a file/module. like MAKEINTRESOURCE(). https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-makeintresourcea
	typedef WORD RESOURCEID_t;	//!< a resource id as 16 valid bits only. enum space is type specific. e.g. RT_STRING, RT_ICON, etc

	interface IResourceLoader
	{
		//! @interface Gray::IResourceLoader
		//! This object will load resource blobs from some source.
		//! Load by name and MIME type. From _WIN32 resource DLL, Directory of files or Zip file.
		//! @note Does NOT support loading windows resources and getting a GDI handle. Use _WIN32 resource wrapper directly for this.

		HRESULT LoadResource(const char* pszName, MIME_TYPE eMime);
	};

	class cResourceBase : public IResourceLoader
	{
		//! @class Gray::cResourceBase
		//! Load a resource from a directory of files.
		//! 
	public:
		cFilePath m_Dir;

	public:
		virtual HRESULT LoadResource(const char* pszName, MIME_TYPE eMime);
	};
}
#endif
