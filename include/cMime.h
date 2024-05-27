//! @file cMime.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cMime_H
#define _INC_cMime_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray {
typedef char RESCHAR_t;  // resource name is not the same as ATOMCHAR_t exactly.

// List of known file extensions. Was FILE_EXT_*
// NOT .dmp .dat .riff
#define MIME_EXT_unk ""
#define MIME_EXT_3ds ".3ds"  // 3d Studio max model.
#define MIME_EXT_7z ".7z"
#define MIME_EXT_avi ".avi"
#define MIME_EXT_bin ".bin"  // no mime type.
#define MIME_EXT_bmp ".bmp"
#define MIME_EXT_cer ".cer"
#define MIME_EXT_crl ".crl"
#define MIME_EXT_crt ".crt"
#define MIME_EXT_css ".css"
#define MIME_EXT_csv ".csv"
#define MIME_EXT_dae ".dae"  // Collada 3d file.
#define MIME_EXT_dds ".dds"
#define MIME_EXT_dll ".dll"
#define MIME_EXT_doc ".doc"
#define MIME_EXT_docx ".docx"
#define MIME_EXT_exe ".exe"
#define MIME_EXT_fbx ".fbx"  // Film Box 3d model
#define MIME_EXT_flv ".flv"
#define MIME_EXT_gif ".gif"
#define MIME_EXT_htm ".htm"
#define MIME_EXT_html ".html"
#define MIME_EXT_htt ".htt"
#define MIME_EXT_ico ".ico"
#define MIME_EXT_ini ".ini"
#define MIME_EXT_jar ".jar"
#define MIME_EXT_jp2 ".jp2"
#define MIME_EXT_jpg ".jpg"
#define MIME_EXT_jpeg ".jpeg"
#define MIME_EXT_js ".js"
#define MIME_EXT_json ".json"
#define MIME_EXT_log ".log"
#define MIME_EXT_lua ".lua"
#define MIME_EXT_mp3 ".mp3"
#define MIME_EXT_mp4 ".mp4"
#define MIME_EXT_mpeg ".mpeg"
#define MIME_EXT_mpg ".mpg"
// #define MIME_EXT_obj ".obj"	 // Mesh model.
#define MIME_EXT_ocx ".ocx"
#define MIME_EXT_pdf ".pdf"
#define MIME_EXT_pem ".pem"
#define MIME_EXT_png ".png"
#define MIME_EXT_ppt ".ppt"
#define MIME_EXT_pptx ".pptx"
#define MIME_EXT_pvk ".pvk"
// #define MIME_EXT_raw		".raw"	// no mime type . binary.
#define MIME_EXT_rtf ".rtf"
#define MIME_EXT_scp ".scp"
#define MIME_EXT_so ".so"  // dll
#define MIME_EXT_swf ".swf"
#define MIME_EXT_tga ".tga"
#define MIME_EXT_tif ".tif"
#define MIME_EXT_tiff ".tiff"
#define MIME_EXT_ttf ".ttf"
#define MIME_EXT_txt ".txt"
#define MIME_EXT_wav ".wav"
#define MIME_EXT_xls ".xls"
#define MIME_EXT_xlsx ".xlsx"
#define MIME_EXT_xml ".xml"
#define MIME_EXT_zip ".zip"
 
/// <summary>
/// Enumerate of recognized MIME types. Linux version of file types.
/// This is similar to the _WIN32 file extension. Registry "Content Type".
/// same as HTTP "Content-Type"
/// </summary>
enum class MIME_t {
#define cMimeType(a, b, c, d, e) _##a,
#include "cMimeTypes.tbl"
#undef cMimeType
    _QTY,
};

/// <summary>
/// Declare all the file types that the app might want to use. all as char/ATOMCHAR_t.
/// _WIN32 registry calls MIME name strings "Content Type"
/// HTTP calls this "Content-Type"
/// </summary>
struct GRAYCORE_LINK cMime {
    static const RESCHAR_t* GRAYCALL GetMimeTypeName(MIME_t eMimeType);
    static MIME_t GRAYCALL FindMimeTypeName(const RESCHAR_t* pszName);

    static MIME_t GRAYCALL FindMimeTypeForExt(const RESCHAR_t* pszExt, MIME_t eMimeTypeDefault = MIME_t::_UNKNOWN);

    const RESCHAR_t* m_pszName;  /// IANA name for predefined/known MIME types.
    const RESCHAR_t* m_pExt;
    const RESCHAR_t* m_pExt2;  // Alternate extension. // Allow an alternate extension for each MIME type.

    static const cMime k_Type[static_cast<int>(MIME_t::_QTY)];  // Define info for all the types i support.
};
}  // namespace Gray
#endif
