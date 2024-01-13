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

#include "cFilePath.h"
#include "cMime.h"
#include "cObject.h"

namespace Gray {
#define RESNAME(_n) _n  /// an embedded hard-coded resource file name. RESCHAR_t/ATOMCHAR_t. May be a relative file path with '/' embedded.

/// <summary>
/// A resource in an app resource pool. a type specific id.
/// WIN32 use .RC attached to a file/module.
/// use with MAKEINTRESOURCE() to convert to char*. https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-makeintresourcea
/// a resource id as 16 valid bits only. enum space is type specific. e.g. RT_STRING, RT_ICON, etc (rough equiv of MIME_t)
/// BEWARE: some ids that overlap this are UINT32. see DLGID_t, CommandId_t
/// </summary>
typedef WORD RESOURCEID_t;

/// <summary>
/// This factory object will load resource blobs from some source by name.
/// Load by name and MIME type. From _WIN32 resource DLL, Directory of files, db, or Zip file.
/// @note Does NOT support loading windows resources and getting a GDI handle back. Use _WIN32 resource wrapper directly for this (if you need a GDI handle. e.g. LoadIcon()).
/// </summary>
interface IResourceLoader {
    HRESULT LoadResource(const RESCHAR_t* pszName, MIME_t eMime, OUT cObject** ppObject);
};

/// <summary>
/// Find and Load a resource from a directory of files.
/// </summary>
class cResourceDir : public IResourceLoader {
 public:
    cFilePath m_Dir;  // directory holding the files i want. Can also look in attached .RC .
 public:
    virtual HRESULT LoadResource(const RESCHAR_t* pszName, MIME_t eMime, OUT cObject** ppObject);
};
}  // namespace Gray
#endif
