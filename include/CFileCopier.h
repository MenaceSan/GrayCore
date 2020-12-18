//
//! @file cFileCopier.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cFileCopier_H
#define _INC_cFileCopier_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cFile.h"

namespace Gray
{
	struct GRAYCORE_LINK DECLSPEC_NOVTABLE IFileCopier
	{
		//! @struct Gray::IFileCopier
		//! An Abstract/interface to request/send a file from/to a remote source. (e.g. HTTP,FTP,another file device,etc)
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
		virtual HRESULT SendAttr(const FILECHAR_t* pszDestName, cTimeFile timeChanged)
		{
			//! Optionally set the remote side time stamp for a file.
			//! ASSUME blocking call.
			UNREFERENCED_PARAMETER(pszDestName);
			UNREFERENCED_PARAMETER(timeChanged);
			return E_NOTIMPL;
		}
	};

	class GRAYCORE_LINK cFileCopier : public IFileCopier
	{
		//! @class Gray::cFileCopier
		//! Implement the IFileCopier for the local file system. Copy single files.

	public:
		cStringF m_sServerRoot;	//!< Prefix all server side paths with this.

	protected:
		cStringF makeFilePath(const FILECHAR_t* pszFileName) const
		{
			return cFilePath::CombineFilePathX(m_sServerRoot, pszFileName);
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
		virtual HRESULT SendAttr(const FILECHAR_t* pszDestName, cTimeFile timeChanged) override;


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

		static HRESULT GRAYCALL CopyFileStream(cStreamInput& stmIn, const FILECHAR_t* pszDstFileName, bool bFailIfExists = false, IStreamProgressCallback* pProgress = nullptr);

		static HRESULT GRAYCALL CopyFileX(const FILECHAR_t* pszExistingName, const FILECHAR_t* pszNewName, IStreamProgressCallback* pProgress = nullptr, bool bFailIfExists = false);
		static HRESULT GRAYCALL RenamePath(const FILECHAR_t* pszOldName, const FILECHAR_t* pszNewName, IStreamProgressCallback* pProgress = nullptr);
 
	};
}

#endif


