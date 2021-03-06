//
//! @file cFileDir.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cFileDir_H
#define _INC_cFileDir_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArrayString.h"
#include "cFile.h"
#include "cFileStatus.h"
#include "cTimeInt.h"
#include "cString.h"
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
 
};

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
		cStringF m_sVolumeName;		//!< can be empty.
		cStringF m_sTypeName;		//!< File system format/type e.g. "NTFS", "FAT"
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
		static HRESULT GRAYCALL GetSystemDeviceList(cArrayString<FILECHAR_t>& a);
	};

	class GRAYCORE_LINK cFileFindEntry
		: public cFileStatus
	{
		//! @class Gray::cFileFindEntry
		//! The file as part of a directory listing.
		//! Assume this is part of a list.
		typedef cFileStatus SUPER_t;
		typedef cFileFindEntry THIS_t;

		friend class cFileFind;

	protected:
		cStringF m_sFileName;	//!< relative file title. (NOT FULL PATH) if FILECHAR_t is NOT USE_UNICODE_FN then is UTF8.

	public:
		cFileFindEntry() noexcept
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

		inline cStringF get_Name() const noexcept
		{
			return m_sFileName;
		}

		bool IsFileEqualTo(const THIS_t& rEntry) const noexcept
		{
			// Does file system use case ?
			if (m_sFileName.CompareNoCase(rEntry.m_sFileName) != 0)
				return false;
			return SUPER_t::IsFileEqualTo(rEntry);
		}
		bool IsFileEqualTo(const THIS_t* pEntry) const noexcept
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
		inline bool isDot() const
		{
			if (m_sFileName[0] != '.')
				return false;
			if (m_sFileName[1] == '\0')
				return true;
			return false;
		}
		inline bool isDots() const noexcept
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
	};

	class GRAYCORE_LINK cFileFind
	{
		//! @class Gray::cFileFind
		//! Read/Browse/Enumerate directory in ongoing/serial state. use FindFileNext() to get next file.
		//! Similar to MFC CFileFind.
		//! @note Don't delete files while reading here. no idea what effect that has. Use cFileDir.

	public:
		cFileFindEntry m_FileEntry;		//!< The current entry. by calls to FindFile() and FindFileNext()

	private:
		cStringF m_sDirPath;			//!< Assume it ends with k_DirSep
		DWORD m_nFileFlags;				//!< Options such as follow the links in the directory. Act as though these are regular files.
#ifdef _WIN32
		WIN32_FIND_DATAW m_FindInfo;	//!< Always UNICODE as base.
		HANDLE m_hContext;				//!< Handle for my search. NOT OSHandle, uses FindClose()
#elif defined(__linux__)
		cStringF m_sWildcardFilter;		//!< Need to perform wildcard (strip out the *.EXT part) later/manually in Linux.
	public:
		bool m_bReadStats;				//!< e.g. "/proc" directory has no extra stats. don't read them.
	private:
		DIR* m_hContext;				//!< Handle for my search/enum.
#else
#error NOOS
#endif

	public:
		explicit cFileFind(cStringF sDirPath = _FN(""), DWORD nFileFlags = 0) noexcept;
		~cFileFind()
		{
			CloseContext();
		}

		cStringF get_DirPath() const noexcept
		{
			return m_sDirPath;
		}
		cStringF GetFilePath(const FILECHAR_t* pszFileTitle) const
		{
			//! Create a full file path with directory and file name/title.
			return cFilePath::CombineFilePathX(m_sDirPath, pszFileTitle);
		}
		cStringF get_FilePath() const
		{
			//! Get Full file path.
			//! like MFC CFileFind::GetFilePath()
			return cFilePath::CombineFilePathX(m_sDirPath, m_FileEntry.get_Name());
		}
		bool isDots() const noexcept
		{
			return m_FileEntry.isDots();
		}

		HRESULT FindOpen(const FILECHAR_t* pszDirPath = nullptr, const FILECHAR_t* pszWildcardFile = nullptr);
		HRESULT FindFile(const FILECHAR_t* pszDirPath = nullptr, const FILECHAR_t* pszWildcardFile = nullptr);
		HRESULT FindFileNext(bool bFirst = false);

		bool isContextOpen() const;
		void CloseContext();
	};

	class GRAYCORE_LINK cFileDir
	{
		//! @class Gray::cFileDir
		//! A file folder or directory. read/cached as a single action.
		//! Stores a list of the files as a single action.
		//! @note i CAN delete files without harming the list. (unlike cFileFind)
	public:
		static const int k_FilesMax = 64 * 1024;
		static const LOGCHAR_t k_szCantMoveFile[];	//!< if MoveDirFiles failed for this.

		cArrayStruct<cFileFindEntry> m_aFiles;	//!< Array of the files we found matching the ReadDir criteria.

	protected:
		cStringF m_sDirPath;	//!< Does NOT include the wild card.

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
		explicit cFileDir(cStringF sDirPath = _FN(""))
			: m_sDirPath(sDirPath)
		{
		}
		virtual ~cFileDir()
		{
		}

		static HRESULT GRAYCALL RemoveDirectory1(const FILECHAR_t* pszDirName);
		static HRESULT GRAYCALL CreateDirectory1(const FILECHAR_t* pszDirName);
		static HRESULT GRAYCALL CreateDirectoryX(const FILECHAR_t* pszDirName);
		static HRESULT GRAYCALL CreateDirForFileX(const FILECHAR_t* pszFilePath);
		static HRESULT GRAYCALL MovePathToTrash(const FILECHAR_t* pszPath, bool bDir);

		static HRESULT GRAYCALL DirFileOp(FILEOP_TYPE eOp, const FILECHAR_t* pszDirSrc, const FILECHAR_t* pszDirDest, DWORD nFileFlags, cLogProcessor* pLog, IStreamProgressCallback* pProgress);
		static HRESULT GRAYCALL MoveDirFiles(const FILECHAR_t* pszDirSrc, const FILECHAR_t* pszDirDest, cLogProcessor* pLog = nullptr, IStreamProgressCallback* pProgress = nullptr)
		{
			//! Move this directory and all its files.
			return DirFileOp(FILEOP_MOVE, pszDirSrc, pszDirDest, 0, pLog, pProgress);
		}
		static HRESULT GRAYCALL CopyDirFiles(const FILECHAR_t* pszDirSrc, const FILECHAR_t* pszDirDest, cLogProcessor* pLog = nullptr, IStreamProgressCallback* pProgress = nullptr)
		{
			//! Copy this directory and all its files.
			return DirFileOp(FILEOP_COPY, pszDirSrc, pszDirDest, 0, pLog, pProgress);
		}
		static HRESULT GRAYCALL DeleteDirFiles(const FILECHAR_t* pszDirName, const FILECHAR_t* pszWildcardFile = nullptr, DWORD nFileFlags = 0)
		{
			//! Delete this directory AND all its files.
			//! similar to cFileDirDlg::DeleteDirFiles( FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI )
			//! e.g. cFileDir::DeleteDirFiles( pszDirPath ); = delete directory and all its sub stuff.
			//! e.g. cFileDir::DeleteDirFiles( pszDirPath, "*.h" ); = delete contents of directory and all its wild carded children. leaves directory.
			//! HRESULT_WIN32_C(ERROR_PATH_NOT_FOUND)
			return DirFileOp(FILEOP_DELETE, pszDirName, pszWildcardFile, nFileFlags, nullptr, nullptr);
		}

		static HRESULT GRAYCALL DeletePathX(const FILECHAR_t* pszPath, DWORD nFileFlags = 0);

		cStringF get_DirPath() const noexcept
		{
			return m_sDirPath;
		}
		void put_DirPath(cStringF sDirPath)
		{
			m_sDirPath = sDirPath;
			// clear list only if changed?
			RemoveAll();
		}
		ITERATE_t get_FileCount() const noexcept
		{
			return m_aFiles.GetSize();
		}
		const cFileFindEntry& GetEnumFile(ITERATE_t i) const
		{
			return m_aFiles.GetAt(i);
		}
		cFileFindEntry& RefEnumFile(ITERATE_t i)
		{
			return m_aFiles.ElementAt(i);
		}
		cStringF GetEnumTitleX(ITERATE_t i) const
		{
			//! Get the file title + ext.
			const cFileFindEntry& rFileEntry = m_aFiles.GetAt(i);
			return rFileEntry.get_Name();
		}
		cStringF GetEnumPath(ITERATE_t i) const
		{
			//! Get the full path for the file i.
			return GetFilePath(GetEnumTitleX(i));
		}

		cStringF GetFilePath(const FILECHAR_t* pszTitle) const
		{
			//! @return full path.
			return cFilePath::CombineFilePathX(m_sDirPath, pszTitle);
		}
		void RemoveAll()
		{
			//! Dispose of my data.
			m_aFiles.RemoveAll();
		}

		HRESULT ReadDir(const FILECHAR_t* pszDirPath = nullptr, const FILECHAR_t* pszWildcardFile = nullptr, ITERATE_t iFilesMax = k_FilesMax, bool bFollowLink = false);

		HRESULT ReadDirAnyExt(const FILECHAR_t* pszFilePath, ITERATE_t iFilesMax = k_FilesMax);
		HRESULT ReadDirPreferredExt(const FILECHAR_t* pszFilePath, const FILECHAR_t* const* pszExtTable);
	};

#ifndef GRAY_STATICLIB // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK CArray < cFileFindEntry, const cFileFindEntry& >;
	template class GRAYCORE_LINK cArrayTyped < cFileFindEntry, const cFileFindEntry& >;
	template class GRAYCORE_LINK cArrayStruct < cFileFindEntry >;
#endif

}

#endif	// _INC_cFileDir_H
