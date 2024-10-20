//! @file GrayVersion.h
//! Can be included in RC file as well! RC_INVOKED
//! version broken out into a separate file for minimal impact on the Version Control System.
//! Use the first 2 digits for data format breaking changes. 3rd digit and beyond is for new functionality. 4th = builds.
//! @note Windows PE only supports the first 2 digits.
//! @note Obscure rule in .MSI forces use of versions >= 1.0.0. NO version 0. allowed.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef GRAY_VERSION_N
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#define GRAY_COMPANY "Menasoft"
#define GRAY_COPYRIGHT "Copyright 1992 - 2023 Dennis Robinson (http://www.menasoft.com)"

// NOTE: Obscure rule in .MSI forces use of versions >= 1.0.0
#define GRAY_VERSION_N 106800       /// numeric version id. used for easy compares. ToVersion32() X.2.3 digit format. Drop 4th digit.
#define GRAY_VERSION_S "1.6.8.0"    /// share string version with all files. ToVersionStr()
#define GRAY_VERSION_RC 1, 6, 8, 0  /// RC version id. *.rc FILEVERSION wants this format.

#endif
