//
//! @file cFileStatus.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CFileStatus_H
#define _INC_CFileStatus_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "FileName.h"
#include "cTimeFile.h"
#include "cTimeInt.h"
#include "cValArray.h"
#include "cStreamProgress.h"
#include "HResult.h"

#ifdef _WIN32
typedef WIN32_FIND_DATAW cFileStatusSys;	// or BY_HANDLE_FILE_INFORMATION ?
#else
struct stat;
typedef struct stat cFileStatusSys;		// from stat(), lstat() or fstat() 
#endif

namespace Gray
{
#ifdef _WIN32
	typedef ULONGLONG FILE_SIZE_t;	//!< similar to STREAM_POS_t size_t
#else
	typedef UINT64 FILE_SIZE_t;	//!< similar to STREAM_POS_t size_t
#endif

	enum FILEATTR_TYPE_
	{
		//! @enum Gray::FILEATTR_TYPE_
		//! FAT, FAT32 and NTFS file attribute flags. translated to approximate __linux__ NFS
		FILEATTR_None,
		FILEATTR_ReadOnly = 0x0001,	//!< FILE_ATTRIBUTE_READONLY. __linux__ permissions for user ?
		FILEATTR_Hidden = 0x0002,	//!< FILE_ATTRIBUTE_HIDDEN. __linux__ starts with .
		FILEATTR_System = 0x0004,	//!< FILE_ATTRIBUTE_SYSTEM

		FILEATTR_NormalMask = 0x000F,	//!< (FILEATTR_ReadOnly|FILEATTR_Hidden|FILEATTR_System)

		FILEATTR_Directory = 0x0010,	//!< FILE_ATTRIBUTE_DIRECTORY
		FILEATTR_Archive = 0x0020,	//!< FILE_ATTRIBUTE_ARCHIVE = this has been changed. (needs to be archived) not yet backed up.
		FILEATTR_Volume = 0x0040,	//!< FILE_ATTRIBUTE_DEVICE = some sort of device. not a file or dir. e.g. COM1
		FILEATTR_Normal = 0x0080,	//!< FILE_ATTRIBUTE_NORMAL = just a file.

		// NTFS only flags. (maybe Linux)
		FILEATTR_Temporary = 0x0100,	//!< FILE_ATTRIBUTE_TEMPORARY
		FILEATTR_Link = 0x0400,			//!< FILE_ATTRIBUTE_REPARSE_POINT = a link. This file doesn't really exist locally but is listed in the directory anyhow.
		FILEATTR_Compress = 0x0800,		//!< FILE_ATTRIBUTE_COMPRESSED. this is a file that will act like a ATTR_directory. (sort of)
	};
	typedef UINT32 FILEATTR_MASK_t;	// FILEATTR_TYPE_ mask

	class GRAYCORE_LINK cFileStatus
	{
		//! @class Gray::cFileStatus
		//! Attributes for a file (or directory) in a directory. Does NOT store the name.
		//! Support of fields varies based on file system. FAT,FAT32,NTFS, etc
		//! Similar to ANSI (or POSIX) stat() _stat
		//! Similar to MFC cFileStatus

		typedef cFileStatus THIS_t;

	public:
		cTimeFile m_timeCreate;		//!< m_ctime  = (may not be supported by file system).
		cTimeFile m_timeChange;		//!< m_mtime = real world time/date of last modification. (FAT32 only accurate to 2 seconds) // All OS support this.
		cTimeFile m_timeLastAccess;	//!< m_atime = time of last access/Open. (For Caching). (may not be supported by file system)
		FILE_SIZE_t m_Size;			//!< file size in bytes. size_t. not always accurate for directories. (-1)
		FILEATTR_MASK_t m_Attributes;		//!< Mask of FILEATTR_TYPE_ attribute bits. FILEATTR_None

	public:
		cFileStatus() noexcept;
		cFileStatus(const FILECHAR_t* pszFilePath);

		void InitFileStatus() noexcept;
		void InitFileStatus(const cFileStatusSys& statusSys);

#if defined(__linux__)
		static HRESULT GRAYCALL GetStatusSys(OUT cFileStatusSys& statusSys, const FILECHAR_t* pszName, bool bFollowLinks = false);
#endif

		static bool IsLinuxHidden(const FILECHAR_t* pszName) noexcept
		{
			//! Is this a hidden file on __linux__ (NFS) ?
			if (pszName == nullptr)
				return true;
			return pszName[0] == '.';
		}
		bool UpdateLinuxHidden(const FILECHAR_t* pszName) noexcept
		{
			//! Is this a __linux__ (NFS) hidden file name ? starts with dot.
			if (IsLinuxHidden(pszName))
			{
				m_Attributes |= FILEATTR_Hidden;
				return true;
			}
			return false;
		}

		bool isFileValid() const noexcept
		{
			//! did i get file data ? is this a file vs a device ?
			//! @return false = bad (or not a) file like 'com1:' 'lpt:' etc.
			//! @note asking for a 'devicename' is BAD! i.e. http://myserver/com5.txt (this will trap that!)
			return m_timeChange.isValid();
		}

		static COMPARE_t GRAYCALL CompareChangeFileTime(const cTimeFile& t1, const cTimeFile& t2)	//! (accurate to 2 seconds)
		{
			//! ~2 sec accurate for FAT32
			return cValT::Compare(t1.get_FAT32(), t2.get_FAT32());
		}
		bool IsSameChangeFileTime(const cTimeFile& t2) const noexcept	//! (accurate to 2 seconds)
		{
			//! ~2 sec accurate for FAT32
			return cValT::Compare(m_timeChange.get_FAT32(), t2.get_FAT32()) == COMPARE_Equal;
		}
		static TIMESEC_t GRAYCALL MakeFatTime(TIMESEC_t tTime) noexcept
		{
			//! (accurate to 2 seconds)
			return tTime & ~1;
		}
		static COMPARE_t GRAYCALL CompareChangeTime(const cTimeInt& t1, const cTimeInt& t2) noexcept
		{
			//! ~2 second accurate for FAT32
			return cValT::Compare(MakeFatTime(t1.GetTime()), MakeFatTime(t2.GetTime()));
		}
		bool IsSameChangeTime(const cTimeInt& t2) const noexcept
		{
			return CompareChangeTime(m_timeChange, t2) == COMPARE_Equal;
		}

		bool IsFileEqualTo(const THIS_t& rFileStatus) const noexcept
		{
			if (cValT::Compare(m_timeCreate.get_Val(), rFileStatus.m_timeCreate.get_Val()) != COMPARE_Equal)
				return false;
			if (!IsSameChangeFileTime(rFileStatus.m_timeChange))
				return false;
			if (m_Size != rFileStatus.m_Size)
				return false;
			return true;
		}
		bool IsFileEqualTo(const THIS_t* pFileStatus) const noexcept
		{
			//! do these 2 files have the same attributes.
			if (pFileStatus == nullptr)
				return false;
			return IsFileEqualTo(*pFileStatus);
		}
		bool IsAttrMask(FILEATTR_MASK_t dwAttrMask = FILEATTR_ReadOnly) const noexcept
		{
			//! have this attribute? e.g. FILEATTR_ReadOnly
			return(m_Attributes & dwAttrMask) ? true : false;
		}
		bool isAttrDir() const noexcept
		{
			return IsAttrMask(FILEATTR_Directory);
		}
		bool isAttrHidden() const noexcept
		{
			// for __linux__ starts with .
			return IsAttrMask(FILEATTR_Hidden);
		}
		FILE_SIZE_t GetFileLength() const noexcept
		{
			//! get the 64 bit length of the file.
			//! -1 = size not available for directories.
			return m_Size;
		}

		static HRESULT GRAYCALL WriteFileAttributes(const FILECHAR_t* pszFilePath, FILEATTR_MASK_t dwAttributes);
		static HRESULT GRAYCALL WriteFileTimes(const FILECHAR_t* pszFilePath, const cTimeFile* pTimeCreate, const cTimeFile* pTimeChange = nullptr);
		static HRESULT GRAYCALL WriteFileTimes(const FILECHAR_t* pszFilePath, const cFileStatus& rFileStatus);
		static HRESULT GRAYCALL ReadFileStatus2(const FILECHAR_t* pszFilePath, cFileStatus* pFileStatus = nullptr, bool bFollowLink = false);

		static bool GRAYCALL Exists(const FILECHAR_t* pszFilePath)
		{
			//! boolean true if this file exists? I can read it. Does not mean I can write to it.
			const HRESULT hRes = ReadFileStatus2(pszFilePath, nullptr, true);
			return SUCCEEDED(hRes);
		}

		HRESULT ReadFileStatus(const FILECHAR_t* pszFilePath, bool bFollowLink = false)
		{
			return ReadFileStatus2(pszFilePath, this, bFollowLink);
		}
	};
}

#endif
