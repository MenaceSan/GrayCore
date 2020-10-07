//
//! @file CFileStatus.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CFileStatus_H
#define _INC_CFileStatus_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "FileName.h"
#include "CTimeFile.h"
#include "CTimeInt.h"
#include "CValT.h"
#include "CStreamProgress.h"
#include "HResult.h"

#ifdef _WIN32
typedef WIN32_FIND_DATAW CFileStatusSys;
#else
struct stat;
typedef struct stat CFileStatusSys;
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
		//! Similar to MFC CFileStatus

		typedef cFileStatus THIS_t;

	public:
		CTimeFile m_timeCreate;		//!< m_ctime  = (may not be supported by file system).
		CTimeFile m_timeChange;		//!< m_mtime = real world time/date of last modification. (FAT32 only accurate to 2 seconds) // All OS support this.
		CTimeFile m_timeLastAccess;	//!< m_atime = time of last access/Open. (For Caching). (may not be supported by file system)
		FILE_SIZE_t m_Size;			//!< file size in bytes. size_t. not always accurate for directories. (-1)
		FILEATTR_MASK_t m_Attributes;		//!< Mask of FILEATTR_TYPE_ attribute bits. FILEATTR_None

	public:
		cFileStatus();
		cFileStatus(const FILECHAR_t* pszFilePath);

		void InitFileStatus();
		void InitFileStatus(const CFileStatusSys& filestat);

		static bool IsLinuxHidden(const FILECHAR_t* pszName)
		{
			//! Is this a hidden file on __linux__ (NFS) ?
			if (pszName == nullptr)
				return true;
			return(pszName[0] == '.');
		}
		bool UpdateLinuxHidden(const FILECHAR_t* pszName)
		{
			//! Is this a __linux__ (NFS) hidden file name ? starts with dot.
			if (IsLinuxHidden(pszName))
			{
				m_Attributes |= FILEATTR_Hidden;
				return true;
			}
			return false;
		}

		bool isFileValid() const
		{
			//! did i get file data ?
			//! @return false = bad file like 'com1:' 'lpt:' etc.
			//! @note asking for a 'devicename' is BAD! i.e. http://myserver/com5.txt (this will trap that!)
			return(m_timeChange.isValid());
		}

		static COMPARE_t GRAYCALL CompareChangeFileTime(const CTimeFile& t1, const CTimeFile& t2)	//! (accurate to 2 seconds)
		{
			//! ~2 sec accurate for FAT32
			return CValT::Compare(t1.get_FAT32(), t2.get_FAT32());
		}
		bool IsSameChangeFileTime(const CTimeFile& t2) const	//! (accurate to 2 seconds)
		{
			//! ~2 sec accurate for FAT32
			return CValT::Compare(m_timeChange.get_FAT32(), t2.get_FAT32()) == COMPARE_Equal;
		}
		static TIMESEC_t GRAYCALL MakeFatTime(TIMESEC_t tTime)
		{
			//! (accurate to 2 seconds)
			return tTime &~1;
		}
		static COMPARE_t GRAYCALL CompareChangeTime(const CTimeInt& t1, const CTimeInt& t2)
		{
			//! ~2 second accurate for FAT32
			return CValT::Compare(MakeFatTime(t1.GetTime()), MakeFatTime(t2.GetTime()));
		}
		bool IsSameChangeTime(const CTimeInt& t2) const
		{
			return CompareChangeTime(m_timeChange, t2) == COMPARE_Equal;
		}

		bool IsFileEqualTo(const THIS_t& rFileStatus) const
		{
			if (CValT::Compare(m_timeCreate.get_Val(), rFileStatus.m_timeCreate.get_Val()) != COMPARE_Equal)
				return false;
			if (!IsSameChangeFileTime(rFileStatus.m_timeChange))
				return false;
			if (m_Size != rFileStatus.m_Size)
				return false;
			return true;
		}
		bool IsFileEqualTo(const THIS_t* pFileStatus) const
		{
			//! do these 2 files have the same attributes.
			if (pFileStatus == nullptr)
				return false;
			return IsFileEqualTo(*pFileStatus);
		}
		bool IsAttrMask(FILEATTR_MASK_t dwAttrMask = FILEATTR_ReadOnly) const
		{
			//! have this attribute? e.g. FILEATTR_ReadOnly
			return(m_Attributes&dwAttrMask) ? true : false;
		}
		bool isAttrDir() const
		{
			return IsAttrMask(FILEATTR_Directory);
		}
		bool isAttrHidden() const
		{
			// for __linux__ starts with .
			return IsAttrMask(FILEATTR_Hidden);
		}
		FILE_SIZE_t GetFileLength() const
		{
			//! get the 64 bit length of the file.
			//! -1 = size not available for directories.
			return m_Size;
		}

		static HRESULT GRAYCALL WriteFileAttributes(const FILECHAR_t* pszFilePath, FILEATTR_MASK_t dwAttributes);
		static HRESULT GRAYCALL WriteFileTimes(const FILECHAR_t* pszFilePath, const CTimeFile* pTimeCreate, const CTimeFile* pTimeChange = nullptr);
		static HRESULT GRAYCALL ReadFileStatus2(const FILECHAR_t* pszFilePath, cFileStatus* pFileStatus=nullptr, bool bFollowLink = false);

		static bool GRAYCALL Exists(const FILECHAR_t* pszFilePath)
		{
			//! boolean true if this file exists? I can read it. Does not mean I can write to it.
			HRESULT hRes = ReadFileStatus2(pszFilePath, nullptr, true);
			return SUCCEEDED(hRes);
		}

		HRESULT ReadFileStatus(const FILECHAR_t* pszFilePath, bool bFollowLink = false)
		{
			return ReadFileStatus2(pszFilePath, this, bFollowLink);
		}
	};
}

#endif