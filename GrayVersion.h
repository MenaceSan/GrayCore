//
//! @file GrayVersion.h
//! Can be included in RC file as well! RC_INVOKED
//! @note version broken out into a separate file for minimal impact on the Version Control System.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#define GRAY_COMPANY		"Menasoft"
#define GRAY_COPYRIGHT		"Copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)"

#define GRAY_VERSION_N		1602		//!< numeric version id. used for easy compares.
#define GRAY_VERSION_RC		1,6,0,2		//!< RC version id. *.rc FILEVERSION wants this format.
#define GRAY_VERSION_S		"1.602"		//!< share string version with all files.
#define GRAY_VERSION_MASK	0xf000		//!< What bits matter for login purposes?

// for VS_VERSION_INFO VERSIONINFO
#define GRAY_VERSION_RES_N  1,6,0,2
#define GRAY_VERSION_RES_S	"1, 6, 0, 2"
