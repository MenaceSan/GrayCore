//
//! @file cStream.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cStream_H
#define _INC_cStream_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrT.h"
#include "cStreamProgress.h"
#include "cTimeSys.h"
#include "cMem.h"
#include "cHeap.h"
#include "HResult.h"
#include "cUnitTestDecl.h"

namespace Gray
{
	UNITTEST2_PREDEF(cStream);

#ifdef _WIN32
#define FILE_EOL	STR_CRLF	//!< CRLF for DOS/Windows format text files. (13,10)
#else
#define FILE_EOL	STR_NL		//!< __linux__ format new line. (10)
#endif

	class cStreamInput;

	class GRAYCORE_LINK cStreamStat
	{
		//! @class Gray::cStreamStat
		//! track how much data is read or written and when.

	public:
		STREAM_POS_t m_nCount;		//!< Keep arbitrary stats on how much i move (bytes).
		cTimeSys m_tLast;		//!< When did i last move data?

	public:
		cStreamStat() noexcept
			: m_nCount(0)
			, m_tLast(cTimeSys::k_CLEAR)
		{
		}
		void ResetStat() noexcept
		{
			m_nCount = 0;		//!< Keep arbitrary stats on how much i TX/RX.
			m_tLast.InitTime();
		}
		void UpdateStat(size_t n) noexcept
		{
			m_nCount += n;
			m_tLast.InitTimeNow();
		}
		void Add(const cStreamStat& n) noexcept
		{
			m_nCount += n.m_nCount;
			if (n.m_tLast.get_TimeSys() > m_tLast.get_TimeSys())
			{
				m_tLast = n.m_tLast;
			}
		}
	};

	class GRAYCORE_LINK cStreamStats
	{
		//! @class Gray::cStreamStats
		//! track how much data is read and written and when.

	public:
		cStreamStat m_StatOut;
		cStreamStat m_StatInp;

	public:
		void Add(const cStreamStats& n)
		{
			m_StatOut.Add(n.m_StatOut);
			m_StatInp.Add(n.m_StatInp);
		}
	};

	class GRAYCORE_LINK cStreamBase
	{
		//! @class Gray::cStreamBase
		//! base class for cStreamOutput or cStreamInput.

	public:
		static const BYTE k_SIZE_MASK = 0x80;		//!< Used for WriteSize()
		static const size_t k_FILE_BLOCK_SIZE = (32 * 1024);	//!< default arbitrary transfer block size. more than this is NOT more efficient.

	public:
		virtual ~cStreamBase()
		{
		}
		virtual STREAM_SEEKRET_t Seek(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set)
		{
			//! Try to change position in a stream.
			//! TODO MOVE THIS TO RX ONLY ??
			//! Maybe try to 'unread' to a previous position in the stream.
			//! This may not be possible if the data has been lost!
			//! @arg
			//!  eSeekOrigin = SEEK_ORIGIN_TYPE SEEK_Cur etc.
			//! @return
			//!  the New position,  <0=FAILED = INVALID_SET_FILE_POINTER
			UNREFERENCED_PARAMETER(iOffset);
			UNREFERENCED_PARAMETER(eSeekOrigin);
			return (STREAM_SEEKRET_t)E_NOTIMPL;	// It doesn't work on this type of stream!
		}

		void SeekToBegin()
		{
			//! ala MFC. Seek to start of file/stream.
			Seek(0, SEEK_Set);
		}
		STREAM_POS_t SeekToEnd()
		{
			//! ala MFC. Seek to end of file/stream.
			return((STREAM_POS_t)Seek(0, SEEK_End));
		}

		virtual STREAM_POS_t GetPosition() const;
		virtual STREAM_POS_t GetLength() const;
	};

	class GRAYCORE_LINK cStreamOutput : public cStreamBase
	{
		//! @class Gray::cStreamOutput
		//! Write a stream of data/text out to some arbitrary destination.
		//! i.e. console, file, socket, telnet, game client, web page client, etc..
		//! May be no Seek() method available / implemented.
		//! similar to STL std::ostream, and IWriteStream

	public:
		cStreamOutput() noexcept
		{
		}
		virtual ~cStreamOutput()
		{
		}

		virtual HRESULT WriteX(const void* pData, size_t nDataSize) // = 0;
		{
			//! Write a data block to the stream.
			//! NOT pure virtual function. stub implementation.
			//! @note In string only protocols this might not be supported. (in favor of WriteString() only)
			//! @return Number of bytes written. <0 = error.
			ASSERT(0);	// should never get called. (should always be overloaded)
			UNREFERENCED_PARAMETER(pData);
			UNREFERENCED_PARAMETER(nDataSize);
			return HRESULT_WIN32_C(ERROR_WRITE_FAULT);
		}

		HRESULT WriteT(const void* pVal, size_t nDataSize)
		{
			//! Write all or nothing (fail). otherwise same as WriteX() (where partials are allowed)
			//! @return Number of bytes written. <0 = error.
			HRESULT hRes = WriteX(pVal, nDataSize);
			if (SUCCEEDED(hRes) && hRes != (HRESULT)nDataSize)
				return HRESULT_WIN32_C(ERROR_WRITE_FAULT); // STG_WRITEFAULT
			return hRes;
		}

		// Support the base types directly.
		template< typename TYPE >
		HRESULT WriteT(TYPE val);

		HRESULT WriteSize(size_t nSize);
		HRESULT WriteHashCode(HASHCODE_t nHashCode)
		{
			//! opposite of ReadHashCode()
			return WriteSize(nHashCode);
		}

		HRESULT WriteN(const void* pBuffer, size_t nSize)
		{
			//! Write a block prefixed by its size (Bytes).
			//! Write out a string with the length prefix.
			//! @return <0 = error.
			HRESULT hRes = WriteSize(nSize);
			if (FAILED(hRes))
				return hRes;
			if (nSize == 0)
				return S_OK;
			return WriteT(pBuffer, nSize);
		}

		template< typename _CH >
		HRESULT WriteStringN(const _CH* pszStr)
		{
			//! Write out a string with the length prefix. ReadStringN()
			//! @return <0 = error.
			return WriteN(pszStr, (pszStr == nullptr) ? 0 : (StrT::Len(pszStr) * sizeof(_CH)));
		}
		template< typename _CH >
		HRESULT WriteCharRepeat(_CH nChar, int nCount = 1)
		{
			//! Repeat writing of a char/wchar_t * nCount.
			//! @return <0 = error.
			ASSERT(nCount >= 0);
			_CH szTmp[2];
			szTmp[0] = nChar;
			szTmp[1] = '\0';
			for (; nCount-- > 0;)
			{
				HRESULT hRes = WriteString(szTmp);
				if (FAILED(hRes))
					return hRes;
			}
			return S_OK;
		}

		virtual HRESULT WriteString(const char* pszStr)
		{
			//! Write just the chars of the string. NOT nullptr. like fputs()
			//! Does NOT assume include NewLine or automatically add one.
			//! @note This can get overloaded for string only protocols. like FILE, fopen()
			//! @note MFC CStdioFile has void return for this.
			//! @return Number of chars written. <0 = error.
			if (pszStr == nullptr)
				return 0;	// write nothing = S_OK.
			StrLen_t iLen = StrT::Len(pszStr);
			return WriteT(pszStr, iLen * sizeof(char));
		}
		virtual HRESULT WriteString(const wchar_t* pszStr)
		{
			//! Write just the chars of the string. NOT nullptr. like fputs()
			//! Does NOT assume include NewLine or automatically add one.
			//! @note This can get overloaded for string only protocols. like FILE, fopen()
			//! @note MFC CStdioFile has void return for this.
			//! @return Number of chars written. <0 = error.
			if (pszStr == nullptr)
				return 0;	// write nothing = S_OK.
			StrLen_t iLen = StrT::Len(pszStr);
			HRESULT hRes = WriteT(pszStr, iLen * sizeof(wchar_t));
			if (FAILED(hRes))
				return hRes;
			return(hRes / sizeof(wchar_t));
		}

		StrLen_t VPrintf(const char* pszFormat, va_list args)
		{
			//! Write just the chars of the string. NOT nullptr
			//! @return
			//!  <0 = error. else number of chars written
			ASSERT(pszFormat != nullptr);
			char szTemp[StrT::k_LEN_MAX];
			StrLen_t iLenRet = StrT::vsprintfN(szTemp, STRMAX(szTemp), pszFormat, args);
			UNREFERENCED_PARAMETER(iLenRet);
			return WriteString(szTemp);
		}
		StrLen_t VPrintf(const wchar_t* pszFormat, va_list args)
		{
			//! Write just the chars of the string. NOT nullptr
			//! @return
			//!  <0 = error. else number of chars written
			ASSERT(pszFormat != nullptr);
			wchar_t szTemp[StrT::k_LEN_MAX];
			StrLen_t iLenRet = StrT::vsprintfN(szTemp, STRMAX(szTemp), pszFormat, args);
			UNREFERENCED_PARAMETER(iLenRet);
			return WriteString(szTemp);
		}
		StrLen_t _cdecl Printf(const char* pszFormat, ...)
		{
			//! Write just the chars of the string. NOT nullptr
			//! Does NOT assume include NewLine or automatically add one.
			//! @note Use StrArg<GChar_t>(s) for safe "%s" args.
			//! @return
			//!  <0 = error. else number of chars written
			ASSERT(pszFormat != nullptr);
			va_list vargs;
			va_start(vargs, pszFormat);
			StrLen_t iLenRet = VPrintf(pszFormat, vargs);
			va_end(vargs);
			return iLenRet;
		}
		StrLen_t _cdecl Printf(const wchar_t* pszFormat, ...)
		{
			//! Write just the chars of the string. NOT nullptr
			//! @note Use StrArg<GChar_t>(s) for safe "%s" args.
			//! @return
			//!  <0 = error. else number of chars written
			ASSERT(pszFormat != nullptr);
			va_list vargs;
			va_start(vargs, pszFormat);
			StrLen_t iLenRet = VPrintf(pszFormat, vargs);
			va_end(vargs);
			return iLenRet;
		}

		//! Copy cStreamInput to this stream.
		HRESULT WriteStream(cStreamInput& sInp, STREAM_POS_t nSizeMax = k_FILE_BLOCK_SIZE, IStreamProgressCallback* pProgress = nullptr, TIMESYSD_t nTimeout = 0);

		virtual HRESULT FlushX()
		{
			//! virtualized fflush() or FlushFileBuffers()
			return S_OK;
		}
	};

	// Write all my types bool, char, int, float, double, _int64. (short, long, signed, unsigned)
#define CTYPE_DEF(a,_TYPE,c,d,e,f,g,h) template<> inline HRESULT cStreamOutput::WriteT<_TYPE>( _TYPE val ) { return WriteT(&val,sizeof(val)); }
#include "cTypes.tbl"
#undef CTYPE_DEF

	class GRAYCORE_LINK cStreamInput : public cStreamBase
	{
		//! @class Gray::cStreamInput
		//! Generic input stream of data.
		//! @note Seek() not always available from this interface. ReadX(nullptr) = skip over but not true seek.
	public:
		cStreamInput() noexcept
		{
		}
		virtual ~cStreamInput()
		{
		}

		virtual size_t SetSeekSizeMin(size_t nSizeMin = k_FILE_BLOCK_SIZE)
		{
			//! similar to ReadCommit (put_AutoReadCommitSize) size. Used by cStreamTransaction.
			//! Leave a certain amount of data (max message size for current protocol)
			//! such that we could Seek() back for incomplete messages.
			//! @arg nSizeMin = 0 = don't commit/lost any data until we have a complete message/block.

			UNREFERENCED_PARAMETER(nSizeMin);
			ASSERT(false);	// Must implement this ?
			return 0;	// Previous commit size.
		}

		virtual HRESULT ReadX(OUT void* pData, size_t nDataSize)
		{
			//! Just read a block from the stream. // must support this.
			//! Similar to MFC CFile::Read()
			//! @arg
			//!  pData = nullptr = skip over nDataSize in read stream. Same as Seek().
			//! @return
			//!  Length of the stuff read.
			//!  <0 = error
			//! @return HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = need more data.
			UNREFERENCED_PARAMETER(pData);
			UNREFERENCED_PARAMETER(nDataSize);
			return 0;
		}

		HRESULT ReadAll(OUT cHeapBlock& block, size_t nSizeExtra = 0)
		{
			//! Read the whole stream as a single allocated block in memory.
			//! @arg nSizeExtra = extra memory allocation.
			//! @return length read. (Not including nSizeExtra). or < 0 = error.
			STREAM_POS_t nLengthStream = this->GetLength();
			size_t nLengthAlloc = (size_t)nLengthStream;
			if (!block.Alloc(nLengthAlloc + nSizeExtra))
				return E_OUTOFMEMORY;
			return ReadT(block.get_Data(), nLengthAlloc);	// must get all.
		}

		virtual HRESULT ReadStringLine(OUT char* pszBuffer, StrLen_t iSizeMax);
		virtual HRESULT ReadStringLine(OUT wchar_t* pszBuffer, StrLen_t iSizeMax);

		// Support the base types directly.
		HRESULT ReadT(OUT void* pVal, size_t nSize)
		{
			//! Read all nSize or fail HRESULT_WIN32_C(ERROR_IO_INCOMPLETE).
			//! @return >= 0 = actual size read. or < 0 = error
			//!  HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = need more data.

			HRESULT hRes = ReadX(pVal, nSize);
			if (SUCCEEDED(hRes) && hRes != (HRESULT)nSize)
			{
				return HRESULT_WIN32_C(ERROR_IO_INCOMPLETE);	// maybe HRESULT_WIN32_C(ERROR_HANDLE_EOF) ?
			}
			return hRes;
		}
		template< typename TYPE >
		HRESULT ReadT(OUT TYPE& val);

		template< typename TYPE >
		HRESULT ReadTN(OUT TYPE& val)
		{
			//! Read a type value in network order. convert to host order.
			HRESULT hRes = ReadT(val);
			if (FAILED(hRes))
				return hRes;
			val = cMemT::NtoH(val);
			return hRes;
		}

		HRESULT ReadSize(OUT size_t& nSize);

		template< typename TYPE >
		HRESULT ReadSizeT(OUT TYPE& n)
		{
			// cast size_t to some other type.
			size_t nSize = 0;
			HRESULT hRes = ReadSize(nSize);
			n = (TYPE)nSize;
			return hRes;
		}

		HRESULT ReadHashCode(OUT UINT32& nHashCode)
		{
			return ReadSizeT<UINT32>(nHashCode);
		}
#ifdef USE_INT64
		HRESULT ReadHashCode(OUT UINT64& nHashCode)
		{
			return ReadSizeT<UINT64>(nHashCode);
		}
#endif
		HRESULT ReadN(OUT BYTE* pBuffer, size_t nSizeMax)
		{
			//! Read a block with a leading size field.
			//! @return >= 0 = actual size read.
			size_t nSize;
			HRESULT hRes = ReadSize(nSize);
			if (FAILED(hRes))
				return hRes;
			if (nSize > nSizeMax)
			{
				// ASSERT(0);
				return HRESULT_WIN32_C(ERROR_FILE_CORRUPT);	// corrupt data.
			}
			return ReadT(pBuffer, nSize);
		}

		template< typename _CH >
		HRESULT ReadStringN(OUT _CH* pszStr, StrLen_t iSizeMax)
		{
			//! Read a string that is prefixed by its size.
			//! iSizeMax = _countof(pszStr), includes space for '\0'.
			//!  e.g. _countof("abc") = 4
			//! @return
			//!  The size of the string (in chars) + including '\0'.
			//! HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = need more data.

			HRESULT hResRead = ReadN((BYTE*)pszStr, (size_t)(iSizeMax - 1) * sizeof(_CH));
			if (FAILED(hResRead))
				return hResRead;
			StrLen_t nSizeRead = hResRead / sizeof(_CH);
			ASSERT(nSizeRead < iSizeMax);
			pszStr[nSizeRead] = '\0';
			return nSizeRead + 1;
		}

		virtual HRESULT ReadPeek(void* pData, size_t nDataSize);
	};

	// Read all my types bool, char, int, float, double, _int64. (short, long, signed, unsigned)
#define CTYPE_DEF(a,_TYPE,c,d,e,f,g,h) template<> inline HRESULT cStreamInput::ReadT<_TYPE>( OUT _TYPE& rval ) { return ReadT(&rval,sizeof(rval)); }
#include "cTypes.tbl"
#undef CTYPE_DEF

	class GRAYCORE_LINK cStream
		: public cStreamInput
		, public cStreamOutput
	{
		//! @class Gray::cStream
		//! This is a bi-directional stream. RX and TX
		//! Sequential = seek May not be avail from this interface. or only partial support.
		//! Similar to MFC cArchive, COM ISequentialStream, std::basic_streambuf ?
		//! ASSUME this overrides ReadX() and WriteX()
		//! GetLength() optionally avail for this stream. I can move to any place in the stream.

	public:
		//! Disambiguate Seek for cStreamBase to cStreamInput for stupid compiler.
		virtual STREAM_SEEKRET_t Seek(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) override
		{
			return cStreamInput::Seek(iOffset, eSeekOrigin);
		}
		virtual STREAM_POS_t GetPosition() const override
		{
			return cStreamInput::GetPosition();
		}
		virtual STREAM_POS_t GetLength() const override
		{
			return cStreamInput::GetLength();
		}
		void SeekToBegin()
		{
			cStreamInput::SeekToBegin();
		}
		STREAM_POS_t SeekToEnd()
		{
			return cStreamInput::SeekToEnd();
		}

		UNITTEST2_FRIEND(cStream);
	};

	class GRAYCORE_LINK cStreamTransaction
	{
		//! @class Gray::cStreamTransaction
		//! we are reading a single message / Transaction from the stream. We need to read all of it or roll back.

	public:
		cStreamInput* m_pInp;		//!< Pull transaction data from this stream.
		STREAM_SEEKRET_t m_lPosStart;
		size_t m_nSeekSizeMinPrev;	//!< Previous value. Maybe nested transactions !

	protected:
		HRESULT TransactionRollback()
		{
			// Roll back to m_lPosStart
			ASSERT(isTransactionActive());
			STREAM_SEEKRET_t lPosRet = m_pInp->Seek(m_lPosStart, SEEK_Set);
			if (lPosRet == m_lPosStart)
				return S_OK;
			ASSERT(lPosRet == m_lPosStart);
			return HResult::GetDef((HRESULT)lPosRet, HRESULT_WIN32_C(ERROR_READ_FAULT));
		}

	public:
		cStreamTransaction(cStreamInput* pInp)
			: m_pInp(pInp)
		{
			ASSERT(m_pInp != nullptr);
			m_lPosStart = m_pInp->GetPosition();
			ASSERT(m_lPosStart >= 0 && m_lPosStart <= (STREAM_SEEKRET_t)cHeap::k_ALLOC_MAX);
			m_nSeekSizeMinPrev = m_pInp->SetSeekSizeMin(0);	// Don't use AutoReadCommit inside cStreamTransaction.
			ASSERT(m_nSeekSizeMinPrev >= 0 && m_nSeekSizeMinPrev <= cHeap::k_ALLOC_MAX);
			ASSERT(isTransactionActive());
		}
		~cStreamTransaction()
		{
			//! if we didn't say it was a success, do a rollback on destruct.
			if (m_pInp == nullptr)
				return;
			if (isTransactionActive())	// We failed ! didn't call SetTransactionComplete or SetTransactionFailed()
			{
				TransactionRollback();
			}
			// Restore commit ability
			m_pInp->SetSeekSizeMin(m_nSeekSizeMinPrev);	// Complete. we can now commit reads. e.g. toss data we have declared read.
		}

		bool isTransactionActive() const
		{
			//! Was SetTransactionComplete called ?
			return m_lPosStart != ((STREAM_SEEKRET_t)-1);
		}
		void SetTransactionComplete()
		{
			//! Success. we got what we wanted. no rollback.
			ASSERT(isTransactionActive());
			m_lPosStart = ((STREAM_SEEKRET_t)-1);
			ASSERT(!isTransactionActive());
		}
		void SetTransactionCompleteN(size_t nSize)
		{
			// I got a partial success. I used some of the data. maybe not all.
			ASSERT(isTransactionActive());
			m_lPosStart += nSize;	// roll back to here.
		}
		void SetTransactionFailed()
		{
			//! The stream broke in some way. e.g. socket close.
			//! assume connection is broken. no rollback.
			if (m_pInp == nullptr)
				return;
			m_pInp = nullptr;
		}
		void SetTransactionRollback()
		{
			//! default behavior if closed without calling SetTransactionComplete() or SetTransactionFailed().
			//! if we didn't say it was a success, do a rollback on destruct.
			ASSERT(isTransactionActive());
		}
	};

	class GRAYCORE_LINK cStreamNull : public cStream
	{
		//! @class Gray::cStreamNull
		//! A junk/null cStream that just tosses write data and has no read data. For testing.

	public:
		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override // = 0;
		{
			//! Write a data block to null.
			UNREFERENCED_PARAMETER(pData);
			return (HRESULT)nDataSize;
		}
	};
};

#endif // _INC_cStream_H
