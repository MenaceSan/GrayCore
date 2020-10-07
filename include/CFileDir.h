//
//! @file CFileDir.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CFileDir_H
#define _INC_CFileDir_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CArrayString.h"
#include "CFile.h"
#include "CFileStatus.h"
#include "CTimeInt.h"
#include "CString.h"
#include "HResult.h"

#ifdef __linux__
#include <dirent.h>	// DIR
#endif

enum FOF_TYPE_
{
	//! @enum FOF_TYPE_
	//! DWORD of flags to control directory listing. FileFlags
	//! like FILEOP_FLAGS from include <shellapi.h>	
#ifdef __linux__
	FOF_ALLOWUNDO = 0x0040,		// 0x0040 in WIN32 <shellapi.h>	
	FOF_FILESONLY = 0x0080,		// 0x0080 in WIN32 <shellapi.h>	
	FOF_RENAMEONCOLLISION = 0x0100,	// 8 in WIN32 (FOF_SIMPLEPROGRESS)
	FOF_NOERRORUI = 0x400,		// 0x0400  in WIN32 <shellapi.h>
#endif
	// NON WIN32 standard.
	FOF_X_FollowLinks = 0x10000,
	FOF_X_WantDots = 0x20000,
#ifndef USE_UNICODE_FN
	FOF_X_UTF8 = 0x40000,			//!< store file names as UTF8. even if USE_UNICODE.
#endif
};

UNITTEST_PREDEF(CFileDir)

namespace Gray
{

#ifdef _WIN32
#define FILEDEVICE_PREFIX	"\\\\.\\"	// usually _FN(FILEDEVICE_PREFIX). similar to "\\Device\\"
#elif defined(__linux__)
#define FILEDEVICE_PREFIX	"/dev/"
#endif

	class GRAYCORE_LINK cFileDevice
	{
		//! @class Gray::cFileDevice
		//! Info for a particular Disk/Device/Volume. e.g. 'C:\'
	public:
		static const char* k_FileSysName[FILESYS_QTY];		//!< File system types i might support.

		// _WIN32 Info from GetVolumeInformation();
		CStringF m_sVolumeName;		//!< can be empty.
		CStringF m_sTypeName;		//!< File system format/type e.g. "NTFS", "FAT"
		FILESYS_TYPE m_eType;		//!< Enumerate known types for m_sTypeName (file system type)
		UINT64 m_nSerialNumber;		//!< Volume serial number (time stamp of last format) e.g. 0x0ca0e613 for _WIN32.
		DWORD m_dwMaximumComponentLength;	//!< block size? e.g. 255 bytes
		bool m_bCaseSensitive;			//!< e.g. 0x03e700ff, FILE_CASE_SENSITIVE_SEARCH. else IgnoreCase

	public:
		cFileDevice();
		~cFileDevice();

		HRESULT UpdateInfo(const FILECHAR_t* pszDeviceId = nullptr);

		FILESYS_TYPE get_FileSysType() const
		{
			return m_eType;
		}
		bool isCaseSensitive() const;

		static UINT GRAYCALL GetDeviceType(const FILECHAR_t* pszDeviceId);
		static FILE_SIZE_t GRAYCALL GetDeviceFreeSpace(const FILECHAR_t* pszDeviceId = nullptr);
		static HRESULT GRAYCALL GetSystemDeviceList(CArrayString<FILECHAR_t>& a);
	};

	class GRAYCORE_LINK cFileFindEntry
		: public cFileStatus
	{
		//! @class Gray::cFileFindEntry
		//! The file as part of a directory listing.
		//! Assume this is part of a list.
		typedef cFileStatus SUPER_t;
		typedef cFileFindEntry THIS_t;

	public:
		CStringF m_sFileName;	//!< relative file title. (NOT FULL PATH) checks USE_UNICODE_FN and FILECHAR_t.

	public:
		cFileFindEntry()
		{
			InitFileStatus();
		}
		explicit cFileFindEntry(const FILECHAR_t* pszFileName)
			: m_sFileName(pszFileName)
		{
			InitFileStatus();
		}
		cFileFindEntry(const FILECHAR_t* pszFileName, const cFileStatus& status)
			: cFileStatus(status)
			, m_sFileName(pszFileName)
		{
		}
		bool IsFileEqualTo(const THIS_t& rEntry) const
		{
			// Does file system use case ?
			if (m_sFileName.CompareNoCase(rEntry.m_sFileName) != 0)
				return false;
			return SUPER_t::IsFileEqualTo(rEntry);
		}
		bool IsFileEqualTo(const THIS_t* pEntry) const
		{
			// Does file system use case ?
			if (pEntry == nullptr)
				return false;
			return IsFileEqualTo(*pEntry);
		}
		bool operator == (const THIS_t& rEntry) const
		{
			return IsFileEqualTo(rEntry);
		}
		bool operator != (const THIS_t& rEntry) const
		{
			return !IsFileEqualTo(rEntry);
		}
		bool isDot() const
		{
			if (m_sFileName[0] != '.')
				return false;
			if (m_sFileName[1] == '\0')
				return true;
			return false;
		}
		bool isDots() const
		{
			//! ignore the . and .. that old systems can give us.
			if (m_sFileName[0] != '.')
				return false;
			if (m_sFileName[1] == '\0')
				return true;
			if (m_sFileName[1] != '.')
				return false;
			if (m_sFileName[2] == '\0')
				return true;
			return false;
		}
		CStringF get_Name() const
		{
			return m_sFileName;
		}
	};

	class GRAYCORE_LINK cFileFind
	{
		//! @class Gray::cFileFind
		//! Read/Browse/Enumerate directory in ongoing/serial state. use FindFileNext() to get next file.
		//! Similar to MFC CFileFind.
		//! @note Don't delete files while reading here. no idea what effect that has. Use CFileDir.

	public:
		cFileFindEntry m_FileEntry;	//!< The current entry. by calls to FindFile() and FindFileNext()

	private:
		CStringF m_sDirPath;			//!< Assume it ends with k_DirSep
		DWORD m_nFileFlags;				//!< Options such as follow the links in the directory. Act as though these are regular files.
#ifdef _WIN32
		WIN32_FIND_DATAW m_FindInfo;	//!< Always USE_UNICODE as base.
		HANDLE m_hContext;				//!< Handle for my search. NOT OSHandle, uses FindClose()
#elif defined(__linux__)
		CStringF m_sWildcardFilter;		//!< Need to perform wildcard (strip out the *.EXT part) later/manually in Linux.
	public:
		bool m_bReadStats;				//!< e.g. "/proc" directory has no extra stats. don't read them.
	private:
		DIR* m_hContext;				//!< Handle for my search/enum.
#else
#error NOOS
#endif

	public:
		explicit cFileFind(CStringF sDirPath = "", DWORD nFileFlags = 0);
		~cFileFind()
		{
			CloseContext();
		}

		CStringF get_DirPath() const
		{
			return m_sDirPath;
		}
		CStringF GetFilePath(const FILECHAR_t* pszFileTitle) const
		{
			//! Create a full file path with directory and file name/title.
			return CFilePath::CombineFilePathX(m_sDirPath, pszFileTitle);
		}
		CStringF get_FilePath() const
		{
			//! Get Full file path.
			//! like MFC CFileFind::GetFilePath()
			return CFilePath::CombineFilePathX(m_sDirPath, m_FileEntry.m_sFileName);
		}
		bool isDots() const
		{
			return m_FileEntry.isDots();
		}

		HRESULT FindOpen(const FILECHAR_t* pszDirPath = nullptr, const FILECHAR_t* pszWildcardFile = nullptr);
		HRESULT FindFile(const FILECHAR_t* pszDirPath = nullptr, const FILECHAR_t* pszWildcardFile = nullptr);
		HRESULT FindFileNext(bool bFirst = false);

		bool isContextOpen() const;
		void CloseContext();
	};

	class GRAYCORE_LINK CFileDir
	{
		//! @class Gray::CFileDir
		//! A file folder or directory. read/cached as a single action.
		//! Stores a list of the files as a single action.
		//! @note i CAN delete files without harming the list. (unlike cFileFind)
	public:
		static const int k_FilesMax = 64 * 1024;
		static const LOGCHAR_t k_szCantMoveFile[];	//!< if MoveDirFiles failed for this.

		CArrayVal2<cFileFindEntry> m_aFiles;	//!< Array of the files we found matching the ReadDir criteria.

	protected:
		CStringF m_sDirPath;	//!< Does NOT include the wild card.

	protected:
		virtual HRESULT AddFileDirEntry(cFileFindEntry& FileEntry)
		{
			//! Just add the file to a list. Overload this to do extra filtering.
			if (!FileEntry.isDots())
			{
				m_aFiles.Add(FileEntry);
			}
			return S_OK;
		}

	public:
		explicit CFileDir(CStringF sDirPath = _FN(""))
			: m_sDirPath(sDirPath)
		{
		}
		virtual ~CFileDir()
		{
		}

		static HRESULT GRAYCALL RemoveDirectory1(const FILECHAR_t* pszDirName);
		static HRESULT GRAYCALL CreateDirectory1(const FILECHAR_t* pszDirName);
		static HRESULT GRAYCALL CreateDirectoryX(const FILECHAR_t* pszDirName);
		static HRESULT GRAYCALL CreateDirForFileX(const FILECHAR_t* pszFilePath);
		static HRESULT GRAYCALL MovePathToTrash(const FILECHAR_t* pszPath, bool bDir);

		static HRESULT GRAYCALL DirFileOp(FILEOP_TYPE eOp, const FILECHAR_t* pszDirSrc, const FILECHAR_t* pszDirDest, DWORD nFileFlags, CLogProcessor* pLog, IStreamProgressCallback* pProgress);
		static HRESULT GRAYCALL MoveDirFiles(const FILECHAR_t* pszDirSrc, const FILECHAR_t* pszDirDest, CLogProcessor* pLog = nullptr, IStreamProgressCallback* pProgress = nullptr)
		{
			//! Move this directory and all its files.
			return DirFileOp(FILEOP_MOVE, pszDirSrc, pszDirDest, 0, pLog, pProgress);
		}
		static HRESULT GRAYCALL CopyDirFiles(const FILECHAR_t* pszDirSrc, const FILECHAR_t* pszDirDest, CLogProcessor* pLog = nullptr, IStreamProgressCallback* pProgress = nullptr)
		{
			//! Copy this directory and all its files.
			return DirFileOp(FILEOP_COPY, pszDirSrc, pszDirDest, 0, pLog, pProgress);
		}
		static HRESULT GRAYCALL DeleteDirFiles(const FILECHAR_t* pszDirName, const FILECHAR_t* pszWildcardFile = nullptr, DWORD nFileFlags = 0)
		{
			//! Delete this directory and all its files.
			//! similar to CFileDirDlg::DeleteDirFiles( FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI )
			//! e.g. CFileDir::DeleteDirFiles( pszDirPath ); = delete directory and all its sub stuff.
			//! e.g. CFileDir::DeleteDirFiles( pszDirPath, "*.h" ); = delete contents of directory and all its wild carded children. leaves directory.
			return DirFileOp(FILEOP_DELETE, pszDirName, pszWildcardFile, nFileFlags, nullptr, nullptr);
		}

		static HRESULT GRAYCALL DeletePathX(const FILECHAR_t* pszPath, DWORD nFileFlags);

		CStringF get_DirPath() const
		{
			return m_sDirPath;
		}
		void put_DirPath(CStringF sDirPath)
		{
			m_sDirPath = sDirPath;
			// clear list only if changed?
			RemoveAll();
		}
		ITERATE_t get_FileCount() const
		{
			return m_aFiles.GetSize();
		}
		const cFileFindEntry& GetEnumFile(ITERATE_t i) const
		{
			return m_aFiles.ConstElementAt(i);
		}
		cFileFindEntry& GetEnumFile(ITERATE_t i)
		{
			return m_aFiles.ElementAt(i);
		}
		CStringF GetEnumTitleX(ITERATE_t i) const
		{
			//! Get the file title + ext.
			ASSERT(m_aFiles.IsValidIndex(i));
			const cFileFindEntry& rFileEntry = m_aFiles.ConstElementAt(i);
			return rFileEntry.m_sFileName;
		}
		CStringF GetEnumPath(ITERATE_t i) const
		{
			//! Get the full path for the file i.
			return GetFilePath(GetEnumTitleX(i));
		}

		CStringF GetFilePath(const FILECHAR_t* pszTitle) const
		{
			//! @return full path.
			return CFilePath::CombineFilePathX(m_sDirPath, pszTitle);
		}
		void RemoveAll()
		{
			//! Dispose of my data.
			m_aFiles.RemoveAll();
		}

		HRESULT ReadDir(const FILECHAR_t* pszDirPath = nullptr, const FILECHAR_t* pszWildcardFile = nullptr, ITERATE_t iFilesMax = k_FilesMax, bool bFollowLink = false);

		HRESULT ReadDirAnyExt(const FILECHAR_t* pszFilePath, ITERATE_t iFilesMax = k_FilesMax);
		HRESULT ReadDirPreferredExt(const FILECHAR_t* pszFilePath, const FILECHAR_t* const* pszExtTable);

		UNITTEST_FRIEND(CFileDir);
	};

#ifdef GRAY_DLL // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK CArray < cFileFindEntry, const cFileFindEntry& >;
	template class GRAYCORE_LINK CArrayTyped < cFileFindEntry, const cFileFindEntry& >;
	template class GRAYCORE_LINK CArrayVal2 < cFileFindEntry >;
#endif

}

#endif	// _INC_CFileDir_H
