//
//! @file cFile.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cFile_H
#define _INC_cFile_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cObject.h"
#include "cStream.h"
#include "cTimeInt.h"
#include "cFilePath.h"
#include "cFileStatus.h"
#include "cOSHandle.h"
#include "cException.h"

#if defined(__linux__)
//#include <sys/types.h>
#include <fcntl.h>	// O_RDONLY
//#include <unistd.h>
struct _SECURITY_ATTRIBUTES;	// stub this out
#endif

namespace Gray
{
	enum OF_FLAGS_TYPE_
	{
		//! @enum Gray::OF_FLAGS_TYPE_
		//! File open mode flags.
#if defined(__linux__)
		OF_READ = O_RDONLY,	//!< _O_RDONLY
		OF_WRITE = O_WRONLY,	//!< _O_WRONLY
		OF_READWRITE = O_RDWR,	//!< _O_RDWR
		OF_APPEND = O_APPEND, //!< writes starting at EOF
		OF_CREATE = O_CREAT,	//!< _O_CREAT = create the file if it doesn't exist. overwrite it if it does.

		OF_SHARE_COMPAT = 0x00000000,	//!< no Linux function.
		OF_SHARE_EXCLUSIVE = 0x00000000,	//!< no Linux function. O_EXCL
		OF_SHARE_DENY_WRITE = 0x00000000,	//!< no Linux function.
		OF_SHARE_DENY_READ = 0x00000000,	//!< no Linux function.
		OF_SHARE_DENY_NONE = 0x00000000,	//!< no Linux function.

		OF_EXIST = 0x00000000,	//!< just test if it exists. like _access()
#endif

#if defined(UNDER_CE)
		OF_READ = 0x0000,	//!< _O_RDONLY
		OF_WRITE = 0x0001,	//!< _O_WRONLY
		OF_READWRITE = 0x0002,	//!< _O_RDWR
		OF_APPEND = 0x0008,	//!< O_APPEND writes done at EOF
		OF_CREATE = 0x0100,	//!< _O_CREAT = create the file if it doesn't exist. overwrite it if it does.

		OF_SHARE_COMPAT = 0x00000000,
		OF_SHARE_EXCLUSIVE = 0x00000010,	//!< O_EXCL
		OF_SHARE_DENY_WRITE = 0x00000020,	//!< not defined in __linux__ O_EXCL
		OF_SHARE_DENY_READ = 0x00000030,
		OF_SHARE_DENY_NONE = 0x00000040,

		OF_EXIST = 0x00004000,	//!< just test if it exists. like _access()
#endif

		//! High flags not supported by POSIX open().
		OF_OPEN_MASK = 0x00FFFFFF,	//!< flags used by open().
		// OF_CACHE_RAND		= 0x04000000,
		OF_CACHE_SEQ = 0x08000000,	//!< O_DIRECT for __linux__ ??
		OF_BINARY = 0x10000000,	//!< for using FILE* in non text mode. (Default)
		OF_TEXT = 0x20000000,	//!< UTF8 or plain ASCII text file. (set by use of char Read/WriteString functions)
		OF_NONCRIT = 0x40000000,	//!< Not a real failure if it doesn't exist.
		// OF_TEXT_W		= 0x80000000,	//!< UNICODE text file. (set by use of wchar_t Read/WriteString functions)
	};

	typedef UINT32 OF_FLAGS_t;	//!< Mask of file open flags OF_FLAGS_TYPE_

	class GRAYCORE_LINK cFileStatus;
	
#ifndef _MFC_VER
	typedef cExceptionHResult CFileException;
	class GRAYCORE_LINK CFile : public CObject	// replace the MFC version
	{
		//! @class Gray::CFile
		//! try to be compatible with MFC CFile class.
		//! @note Don't use this directly. Use cFile.

	public:
		cOSHandle m_hFile;		//!< OSHandle for the open file.
	protected:
		cStringF m_strFileName;		//!< store a copy of the full file path. MFC defined name. 

	protected:
		// virtual BOOL Open( const TCHAR* lpszFileName, UINT nOpenFlags, CFileException* pError = nullptr ) = 0; // MFC def.
		HRESULT OpenCreate(cStringF sFilePath = _FN(""), OF_FLAGS_t nOpenFlags = OF_CREATE | OF_WRITE, _SECURITY_ATTRIBUTES* pSa = nullptr);

	public:
		virtual ~CFile()
		{
			Close();
		}

		bool isFileOpen() const noexcept
		{
			return m_hFile.isValidHandle();
		}

		// virtual CString GetFilePath() const	// DO NOT USE this. It conflicts with the messed up version of MFC. 

#if defined(__linux__)
		HRESULT GetStatusSys(OUT cFileStatusSys& statusSys);
#endif

		// File Access
		virtual STREAM_POS_t GetPosition() const;
		virtual STREAM_POS_t GetLength() const;

		virtual void SetLength(STREAM_POS_t dwNewLen);
		virtual void Close()
		{
			m_hFile.CloseHandle();
		}

		HRESULT Write(const void* pData, size_t nDataSize);
		HRESULT Read(void* pData, size_t nDataSize)
		{
			// don't call this directly use ReadX
			// ERROR_HANDLE_EOF
			return m_hFile.ReadX(pData, nDataSize);
		}
	};
#endif	// _MFC_VER

	class GRAYCORE_LINK cFile
		: public CFile
		, public cStream
	{
		//! @class Gray::cFile
		//! General OS file access interface. Extends MFC functionality
		//! @note Any file can be a cStreamOutput of text as well.
		//! Dupe the MFC functionality we need from CFile. Similar to IStream and CAtlFile

		typedef CFile SUPER_t;

	protected:
		static ITERATE_t sm_iFilesOpen;	//!< global count of all open files for this process.
		OF_FLAGS_t m_nOpenFlags;		//!< MMSYSTEM uses high bits of 32 bit flags. OF_FLAGS_TYPE_ OF_READ etc

	protected:
		HRESULT OpenSetup(cStringF sFilePath, OF_FLAGS_t uModeFlags);

	public:
		cFile() noexcept
			: m_nOpenFlags(0)
		{
		}
		cFile(cStringF sFilePath, OF_FLAGS_t nOpenFlags)
		{
			OpenX(sFilePath, nOpenFlags);
		}
		virtual ~cFile() noexcept
		{
			Close();
		}

		virtual bool isValidCheck() const noexcept	//!< memory allocation and structure definitions are valid.
		{
#ifndef _MFC_VER
			if (!CObject::isValidCheck())
				return false;
#endif
			if (!IS_TYPE_OF(CFile, this))	// structure definitions are valid..
				return false;
			return true;
		}

		virtual STREAM_POS_t GetPosition() const override
		{
			//! disambiguate. cStream
			//! should be same as FILE_SIZE_t?
			return CFile::GetPosition();
		}
		virtual STREAM_POS_t GetLength() const override
		{
			//! disambiguate. cStream 
			//! should be same as FILE_SIZE_t?
			return CFile::GetLength();
		}

#ifdef _MFC_VER
		void SeekToBegin()
		{
			//! disambiguate. cStream
			CFile::SeekToBegin();
		}
		STREAM_POS_t SeekToEnd()
		{
			//! disambiguate. cStream
			return CFile::SeekToEnd();
		}
#else
		int Read(void* pData, size_t nDataSize)
		{
			// Emulate MFC. Try not to call this directly. use ReadX.
			return ReadX(pData, nDataSize);
		}
		void Write(const void* pData, size_t nDataSize)
		{
			// Emulate MFC
			WriteX(pData, nDataSize);
		}
#endif

		cStringF get_FilePath() const noexcept
		{
			//! like _MFC_VER CFile::GetFilePath(); but cStringF
			//! @note Don't use GetFilePath() as it has some weird side effects in MFC.
			//! like WIN32 GetFinalPathNameByHandle()
			//! __linux__ readlink on /proc/self/fd/NNN where NNN is the file descriptor
			return cStringF(m_strFileName);
		}
		cStringF get_FileTitleX() const;	// MFC is CString return
		cStringF get_FileExt() const;
		bool IsFileExt(const FILECHAR_t* pszExt) const noexcept;

		// File Mode stuff.
		OF_FLAGS_t get_Mode() const noexcept
		{
			//! get basic set of OF_FLAGS_t. get rid of OF_NONCRIT type flags. e.g. OF_READ
			return m_nOpenFlags & 0x00FFFFFF;	//
		}
		OF_FLAGS_t get_ModeFlags() const noexcept
		{
			//! Get the full/hidden elements of the OF_FLAGS_t Flags. e.g. OF_NONCRIT
			return m_nOpenFlags;
		}
		bool isModeWrite() const noexcept
		{
			// Can i write ?
			const OF_FLAGS_t nFlagsDir = (m_nOpenFlags & (OF_WRITE | OF_READ | OF_READWRITE));
			return nFlagsDir == OF_WRITE || nFlagsDir == OF_READWRITE;
		}
		bool isModeRead() const noexcept
		{
			// Can i read ?
			const OF_FLAGS_t nFlagsDir = (m_nOpenFlags & (OF_WRITE | OF_READ | OF_READWRITE));
			return nFlagsDir == OF_READ || nFlagsDir == OF_READWRITE;	// assume OF_READ = 0
		}

		// File Open/Close
#ifdef _MFC_VER
		bool isFileOpen() const noexcept
		{
			return m_hFile != CFile::hFileNull;
		}
#endif

		// MFC Open is BOOL return type.
		virtual HRESULT OpenX(cStringF sFilePath = _FN(""), OF_FLAGS_t nOpenFlags = OF_READ | OF_SHARE_DENY_NONE);
		void Close() noexcept override;

		HANDLE DetachFileHandle() noexcept;

		HRESULT OpenWait(cStringF sFilePath = _FN(""), OF_FLAGS_t nOpenFlags = OF_READ | OF_SHARE_DENY_NONE, TIMESYSD_t nWaitTime = 100);
		HRESULT OpenCreate(cStringF sFilePath = _FN(""), OF_FLAGS_t nOpenFlags = OF_CREATE | OF_WRITE, _SECURITY_ATTRIBUTES* pSa = nullptr);

		// File Access
		bool SetFileTime(const cTimeFile* lpCreationTime, const cTimeFile* lpAccessTime, const cTimeFile* lpLastWriteTime);
		bool SetFileTime(cTimeInt timeCreation, cTimeInt timeLastWrite);
		HRESULT GetFileStatus(OUT cFileStatus& attr) const;

		// cStream
		virtual HRESULT ReadX(void* pData, size_t nDataSize) override;
		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override; // disambiguate.
		virtual HRESULT FlushX() override;
		virtual HRESULT SeekX(STREAM_OFFSET_t lOffset = 0, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set);

		static HRESULT GRAYCALL DeletePath(const FILECHAR_t* pszFileName);	// NOTE: MFC Remove() returns void
		static HRESULT GRAYCALL DeletePathX(const FILECHAR_t* pszFilePath, DWORD nFileFlags = 0);
		static HRESULT GRAYCALL LoadFile(const FILECHAR_t* pszFilePath, OUT cHeapBlock& block, size_t nSizeExtra = 0);
 
	};	
} 

#endif // _INC_CFile_H
