//! @file pch.h
//! include file for standard system include files,
//! or project specific include files that are used frequently, but are changed infrequently
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_pch_H
#define _INC_pch_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#ifndef GRAYCORE_LINK
#if defined(_MFC_VER) || defined(GRAY_STATICLIB)  // GRAY_STATICLIB or _MFC_VER can be defined to make Gray* all static lib
#define GRAYCORE_LINK                             // not in a DLL
#else
#define GRAYCORE_LINK __DECL_EXPORT
#endif
#endif

#include "GrayCore.h"
#endif  // !defined(_INC_pch_H)
