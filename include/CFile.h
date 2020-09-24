//
//! @file CFile.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CFile_H
#define _INC_CFile_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CObject.h"
#include "CStream.h"
#include "CTimeInt.h"
#include "CFilePath.h"
#include "CFileStatus.h"
#include "COSHandle.h"
#include "CException.h"

#if defined(__linux__)
//#include <sys/types.h>
#include <fcntl.h>	// O_RDONLY
//#include <unistd.h>
struct _SECURITY_ATTRIBUTES;	// stub this out
#endif

UNITTEST_PREDEF(cFile)

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

	struct GRAYCORE_LINK DECLSPEC_NOVTABLE IFileCopier
	{
		//! @struct Gray::IFileCopier
		//! An Abstract/interface to request/send a file from/to a remote source. (e.g. HTTP,FTP,etc)
		//! And track it's progress.
		//! similar to __linux__ curl lib

		IGNORE_WARN_ABSTRACT(IFileCopier);

		//! get a debug name for the connection.
		virtual cStringA get_ConnectName() const = 0;

		virtual HRESULT Connect(const FILECHAR_t* pszDevice)
		{
			//! Connect to the device by its name. Maybe URI or File device to transfer files to/from. ASSUME blocking call.
			//! @arg pszDevice = can be prefixed by "http://host" or "ftp://host" or "C:\etc"
			//! nullptr = It might be needed to re-connect to a device if it disconnects spontaneously.
			UNREFERENCED_PARAMETER(pszDevice);
			return S_FALSE;	// not needed i guess.
		}
		virtual HRESULT RequestFile(const FILECHAR_t* pszSrcName, const FILECHAR_t* pszDestPath, IStreamProgressCallback* pProgress = nullptr, FILE_SIZE_t nOffsetStart = 0, FILE_SIZE_t* pnRequestSizeEst = nullptr)
		{
			//! Request a file from a server to be brought back to me/local. ASSUME blocking call.
			//! @arg pszDestPath = a local file. nullptr = query only. don't actually get the file.
			//! @arg pnRequestSizeEst = nullptr = i don't care. -1 = request/return the size.
			UNREFERENCED_PARAMETER(pszSrcName);
			UNREFERENCED_PARAMETER(pszDestPath);
			UNREFERENCED_PARAMETER(pProgress);
			UNREFERENCED_PARAMETER(nOffsetStart);
			UNREFERENCED_PARAMETER(pnRequestSizeEst);
			return E_NOTIMPL;
		}
		virtual HRESULT SendFile(const FILECHAR_t* pszSrcPath, const FILECHAR_t* pszDestName, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart = 0, FILE_SIZE_t nSize = (FILE_SIZE_t)-1)
		{
			//! Send a file to a remote/server from local storage. or delete the remote side file. (pszSrcPath=nullptr,nSize=0)
			//! ASSUME blocking call.
			//! @arg pszSrcPath = a local file.
			//! @note I cannot set the modification time stamp for the file here.
			UNREFERENCED_PARAMETER(pszSrcPath);
			UNREFERENCED_PARAMETER(pszDestName);
			UNREFERENCED_PARAMETER(pProgress);
			UNREFERENCED_PARAMETER(nOffsetStart);
			UNREFERENCED_PARAMETER(nSize);
			return E_NOTIMPL;
		}
		virtual HRESULT SendAttr(const FILECHAR_t* pszDestName, CTimeFile timeChanged)
		{
			//! Optionally set the remote side time stamp for a file.
			//! ASSUME blocking call.
			UNREFERENCED_PARAMETER(pszDestName);
			UNREFERENCED_PARAMETER(timeChanged);
			return E_NOTIMPL;
		}
	};

#ifndef _MFC_VER
	typedef cExceptionHResult CFileException;
	class GRAYCORE_LINK CFile : public CObject	// replace the MFC version
	{
		//! @class Gray::CFile
		//! try to be compatible with MFC CFile class.
		//! @note Don't use this directly. Use cFile.

	public:
		COSHandle m_hFile;		//!< OSHandle for the open file.
	protected:
		CStringF m_strFileName;		//!< full file path. 

	protected:
		bool isFileOpen() const
		{
			return m_hFile.isValidHandle();
		}

		// virtual BOOL Open( const TCHAR* lpszFileName, UINT nOpenFlags, CFileException* pError = nullptr ) = 0; // MFC def.
		HRESULT OpenCreate(CStringF sFilePath = "", OF_FLAGS_t nOpenFlags = OF_CREATE | OF_WRITE, _SECURITY_ATTRIBUTES* pSa = nullptr);
	public:
		virtual ~CFile()
		{
			Close();
		}

		// virtual CString GetFilePath() const	// DO NOT USE this. It conflicts with the messed up version of MFC. 

		// File Access
		virtual STREAM_SEEKRET_t Seek(STREAM_OFFSET_t lOffset = 0, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set);	// should be const ? but not in MFC !
		virtual STREAM_POS_t GetPosition() const;
		virtual STREAM_POS_t GetLength() const;

		virtual void SetLength(STREAM_SEEKRET_t dwNewLen);
		virtual void Close()
		{
			m_hFile.CloseHandle();
		}

		HRESULT Write(const void* pData, size_t nDataSize);
		HRESULT Read(void* pData, size_t nDataSize)
		{
			// don't call this directly use ReadX
			return m_hFile.ReadX(pData, nDataSize);
		}
	};
#endif	// _MFC_VER

	class GRAYCORE_LINK cFile
		: public CFile
		, public CStream
	{
		//! @class Gray::cFile
		//! General OS file access interface. Extends MFC functionality
		//! @note Any file can be a CStreamOutput of text as well.
		//! Dupe the MFC functionality we need from CFile. Similar to IStream and CAtlFile

		typedef CFile SUPER_t;

	protected:
		static ITERATE_t sm_iFilesOpen;	//!< statistical count of total open files for this process.
		OF_FLAGS_t m_nOpenFlags;		//!< MMSYSTEM uses high bits of 32 bit flags. OF_FLAGS_TYPE_ OF_READ etc

	protected:
		HRESULT OpenSetup(CStringF sFilePath, OF_FLAGS_t uModeFlags);

	public:
		cFile()
			: m_nOpenFlags(0)
		{
		}
		cFile(CStringF sFilePath, OF_FLAGS_t nOpenFlags)
		{
			OpenX(sFilePath, nOpenFlags);
		}
		virtual ~cFile()
		{
			Close();
		}

		virtual bool isValidCheck() const	//!< memory allocation and structure definitions are valid.
		{
#ifndef _MFC_VER
			if (!CObject::isValidCheck())
				return false;
#endif
			if (!IS_TYPE_OF(CFile, this))	// structure definitions are valid..
				return false;
			return true;
		}

		virtual STREAM_SEEKRET_t Seek(STREAM_OFFSET_t lOffset = 0, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) override
		{
			//! disambiguate CStream
			//! not const in MFC
			//! @return
			//!  the New position,  <0 = FAILED = INVALID_SET_FILE_POINTER
			return CFile::Seek(lOffset, eSeekOrigin);
		}
		virtual STREAM_POS_t GetPosition() const override
		{
			//! disambiguate. CStream
			//! should be same as FILE_SIZE_t?
			return CFile::GetPosition();
		}
		virtual STREAM_POS_t GetLength() const override
		{
			//! disambiguate. CStream 
			//! should be same as FILE_SIZE_t?
			return CFile::GetLength();
		}

#ifdef _MFC_VER
		void SeekToBegin()
		{
			//! disambiguate. CStream
			CFile::SeekToBegin();
		}
		STREAM_POS_t SeekToEnd()
		{
			//! disambiguate. CStream
			return CFile::SeekToEnd();
		}
#else
		int Read(void* pData, size_t nDataSize)
		{
			// Emulate MFC
			return ReadX(pData, nDataSize);
		}
		void Write(const void* pData, size_t nDataSize)
		{
			// Emulate MFC
			WriteX(pData, nDataSize);
		}
#endif

		CStringF get_FilePath() const
		{
			//! like _MFC_VER CFile::GetFilePath(); but CStringF
			//! @note Don't use GetFilePath() as it has some weird side effects in MFC.
			return CStringF(m_strFileName);
		}
		CStringF get_FileTitleX() const;	// MFC is CString return
		CStringF get_FileExt() const;
		bool IsFileExt(const FILECHAR_t* pszExt) const;

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
			OF_FLAGS_t nFlagsDir = (m_nOpenFlags & (OF_WRITE | OF_READ | OF_READWRITE));
			return nFlagsDir == OF_WRITE || nFlagsDir == OF_READWRITE;
		}
		bool isModeRead() const noexcept
		{
			// Can i read ?
			OF_FLAGS_t nFlagsDir = (m_nOpenFlags & (OF_WRITE | OF_READ | OF_READWRITE));
			return nFlagsDir == OF_READ || nFlagsDir == OF_READWRITE;	// assume OF_READ = 0
		}

		// File Open/Close
		virtual bool isFileOpen() const
		{
#ifdef _MFC_VER
			return m_hFile != CFile::hFileNull;
#else
			return CFile::isFileOpen();
#endif
		}

		// MFC Open is BOOL return type.
		virtual HRESULT OpenX(CStringF sFilePath = "", OF_FLAGS_t nOpenFlags = OF_READ | OF_SHARE_DENY_NONE);
		virtual void Close(void) override;

		HANDLE DetachFileHandle();

		HRESULT OpenWait(CStringF sFilePath = "", OF_FLAGS_t nOpenFlags = OF_READ | OF_SHARE_DENY_NONE, TIMESYSD_t nWaitTime = 100);
		HRESULT OpenCreate(CStringF sFilePath = "", OF_FLAGS_t nOpenFlags = OF_CREATE | OF_WRITE, _SECURITY_ATTRIBUTES* pSa = nullptr);

		// File Access
#if defined(_WIN32) && ! defined(UNDER_CE)
		static DWORD CALLBACK onCopyProgressCallback(LARGE_INTEGER TotalFileSize,
			LARGE_INTEGER TotalBytesTransferred,
			LARGE_INTEGER StreamSize,
			LARGE_INTEGER StreamBytesTransferred,
			DWORD dwStreamNumber,
			DWORD dwCallbackReason,	// CALLBACK_CHUNK_FINISHED or CALLBACK_STREAM_SWITCH
			HANDLE hSourceFile,
			HANDLE hDestinationFile,
			void* lpData);
#endif

		HRESULT CopyFileStream(const FILECHAR_t* pszDstFileName, bool bFailIfExists = false, IStreamProgressCallback* pProgress = nullptr);

		bool SetFileTime(const CTimeFile* lpCreationTime, const CTimeFile* lpAccessTime, const CTimeFile* lpLastWriteTime);
		bool SetFileTime(CTimeInt timeCreation, CTimeInt timeLastWrite);
		HRESULT GetFileStatus(cFileStatus& attr) const;

		// CStream
		virtual HRESULT ReadX(void* pData, size_t nDataSize) override;
		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override; // disambiguate.
		virtual HRESULT FlushX() override;

		static HRESULT GRAYCALL CopyFileX(const FILECHAR_t* pszExistingName, const FILECHAR_t* pszNewName, IStreamProgressCallback* pProgress = nullptr, bool bFailIfExists = false);
		static HRESULT GRAYCALL RenamePath(const FILECHAR_t* pszOldName, const FILECHAR_t* pszNewName, IStreamProgressCallback* pProgress = nullptr);
		static HRESULT GRAYCALL DeletePath(const FILECHAR_t* pszFileName);	// NOTE: MFC Remove() returns void
		static HRESULT GRAYCALL DeletePathX(const FILECHAR_t* pszFilePath, DWORD nFileFlags = 0);
		static HRESULT GRAYCALL LoadFile(const FILECHAR_t* pszFilePath, OUT CHeapBlock& block, size_t nSizeExtra = 0);

#ifdef USE_UNITTESTS
		static void GRAYCALL UnitTest_Write(CStreamOutput& testfile1);
		static void GRAYCALL UnitTest_Read(CStreamInput& testfile1, bool bString);
		static void GRAYCALL UnitTest_CopyTo(IFileCopier* pDst);
		static void GRAYCALL UnitTest_CopyFrom(IFileCopier* pSrc);
		UNITTEST_FRIEND(cFile);
#endif
	};

	class GRAYCORE_LINK cFileCopier : public IFileCopier
	{
		//! @class Gray::cFileCopier
		//! Implement the IFileCopier for the local file system. Copy single files.

	public:
		CStringF m_sServerRoot;	//!< Prefix all server side paths with this.

	protected:
		CStringF makeFilePath(const FILECHAR_t* pszFileName) const
		{
			return CFilePath::CombineFilePathX(m_sServerRoot, pszFileName);
		}

	public:
		virtual cStringA get_ConnectName() const override 
		{
			//! get a debug name for the connection.
			return "File";
		}
		virtual HRESULT Connect(const FILECHAR_t* pszServerRoot) override
		{
			//! @arg pszServerRoot = server side names can be prefixed by "C:\etc"
			m_sServerRoot = pszServerRoot;
			return S_OK;
		}
		virtual HRESULT RequestFile(const FILECHAR_t* pszSrcName, const FILECHAR_t* pszDestPath, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart, FILE_SIZE_t* pnRequestSizeEst) override;
		virtual HRESULT SendFile(const FILECHAR_t* pszSrcPath, const FILECHAR_t* pszDestName, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart, FILE_SIZE_t nSize) override;
		virtual HRESULT SendAttr(const FILECHAR_t* pszDestName, CTimeFile timeChanged) override;
	};
};

#endif // _INC_CFile_H
