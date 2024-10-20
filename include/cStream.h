//! @file cStream.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cStream_H
#define _INC_cStream_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "HResult.h"
#include "ITextWriter.h"
#include "StrArg.h"
#include "cBlob.h"
#include "cHeap.h"
#include "cSingleton.h"
#include "cStreamProgress.h"  // STREAM_OFFSET_t , STREAM_POS_t, SEEK_t
#include "cTimeSys.h"         // cTimeSys

namespace Gray {
#ifdef _WIN32
#define FILE_EOL STR_CRLF  /// CRLF for DOS/Windows format text files. (13,10)
#else
#define FILE_EOL STR_NL  /// __linux__ format new line. (10)
#endif

struct cStreamInput;

/// <summary>
/// track how much data is read or written and when.
/// </summary>
struct GRAYCORE_LINK cStreamStat {
    STREAM_POS_t _nCount = 0;             /// Keep arbitrary stats on how much i move (bytes).
    cTimeSys _tLast = cTimeSys::k_CLEAR;  /// When did i last move data?

    void ResetStat() noexcept {
        _nCount = 0;  //  Keep arbitrary stats on how much i TX/RX.
        _tLast.InitTime();
    }
    void UpdateStat(size_t n) noexcept {
        _nCount += n;
        _tLast.InitTimeNow();
    }
    void Add(const cStreamStat& n) noexcept {
        _nCount += n._nCount;
        if (n._tLast.get_TimeSys() > _tLast.get_TimeSys()) {
            _tLast = n._tLast;
        }
    }
};

/// <summary>
/// track how much data is read and written and when.
/// </summary>
struct GRAYCORE_LINK cStreamStats {
    cStreamStat _StatOut;
    cStreamStat _StatInp;

    void Add(const cStreamStats& n) noexcept {
        _StatOut.Add(n._StatOut);
        _StatInp.Add(n._StatInp);
    }
};

/// <summary>
/// base class for binary cStreamOutput or cStreamInput.
/// </summary>
struct GRAYCORE_LINK cStreamBase {
    static const BYTE k_SIZE_MASK = 0x80;                 /// Used for WriteSize(). 7 bits.
    static const size_t k_FILE_BLOCK_SIZE = (32 * 1024);  /// default arbitrary transfer block size. more than this is NOT more efficient.

    virtual ~cStreamBase() {}

    /// <summary>
    /// Change position in a stream. Success or failure. NO partial success.
    /// Maybe try to 'unread' to a previous position in the stream.
    /// This may not be possible if the data has been lost!
    /// </summary>
    /// <param name="nOffset"></param>
    /// <param name="eSeekOrigin">SEEK_t SEEK_t::_Cur etc.</param>
    /// <returns>the New position,  -lt- 0=FAILED = INVALID_SET_FILE_POINTER</returns>
    virtual HRESULT SeekX(STREAM_OFFSET_t nOffset, SEEK_t eSeekOrigin = SEEK_t::_Set) noexcept {
        UNREFERENCED_PARAMETER(nOffset);
        UNREFERENCED_PARAMETER(eSeekOrigin);
        return E_NOTIMPL;  // It doesn't work on this type of stream!
    }
    /// <summary>
    /// Must override this. like: SeekX(SEEK_t::_Cur,0)
    /// </summary>
    /// <returns></returns>
    virtual STREAM_POS_t GetPosition() const noexcept {
        return k_STREAM_POS_ERR;  // It doesn't work on this type of stream!
    }

    /// <summary>
    /// get total length of the stream in bytes. if available. not the same as Read Length.
    /// default implementation using SeekX and GetPosition. override this for better implementation.
    /// </summary>
    /// <returns></returns>
    virtual STREAM_POS_t GetLength() const noexcept;

    /// <summary>
    /// Helper functions for SeekX
    /// ala MFC. SeekX to start of file/stream.
    /// </summary>
    void SeekToBegin() noexcept {
        SeekX(0, SEEK_t::_Set);
    }
    STREAM_POS_t SeekToEnd() noexcept {
        //! ala MFC. SeekX to end of file/stream.
        SeekX(0, SEEK_t::_End);
        return GetPosition();
    }
};

/// <summary>
/// Write a stream of binary data/text out to some arbitrary destination.
/// i.e. console, file, socket, telnet, game client, web page client, etc..
/// May be no SeekX() method available / implemented.
/// similar to STL std::ostream, and IWriteStream
/// </summary>
struct GRAYCORE_LINK cStreamOutput : public cStreamBase, public ITextWriter {
    cStreamOutput() noexcept {}

    /// <summary>
    /// Write a data block to the stream.
    /// NOT pure virtual function. stub implementation.
    /// @note In string only protocols this might not be supported. (in favor of WriteString() only)
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="nDataSize"></param>
    /// <returns>Number of bytes actually written. -lt- 0 = error.</returns>
    virtual HRESULT WriteX(const cMemSpan& m) {  // = 0;
        DEBUG_CHECK(0);                          // should never get called. (should always be overloaded)
        UNREFERENCED_REFERENCE(m);
        return HRESULT_WIN32_C(ERROR_WRITE_FAULT);  // or E_NOTIMPL
    }

    /// <summary>
    /// Write all or nothing (fail). otherwise same as WriteX() (where partials are allowed)
    /// </summary>
    /// <param name="pVal"></param>
    /// <param name="nDataSize"></param>
    /// <returns>Number of bytes written. -lt- 0 = error.</returns>
    HRESULT WriteSpan(const cMemSpan& m) {
        const HRESULT hRes = WriteX(m);
        if (SUCCEEDED(hRes) && hRes != CastN(HRESULT, m.get_SizeBytes())) return HRESULT_WIN32_C(ERROR_WRITE_FAULT);  // STG_WRITEFAULT
        return hRes;
    }

    /// Write the base types directly. Host endian order.
    template <typename TYPE>
    HRESULT WriteT(TYPE val) {
        return WriteSpan(TOSPANT(val));
    }

    /// <summary>
    /// Write a variable length packed (unsigned) size_t. opposite of ReadSize(nSize).
    /// Packed low to high values.
    /// Bit 7 indicates more data needed.
    /// Similar to ASN1 Length packing.
    /// </summary>
    /// <param name="nSize"></param>
    /// <returns> -lt- 0 = error.</returns>
    HRESULT WriteSize(size_t nSize);

    HRESULT WriteHashCode(HASHCODE_t nHashCode) {
        //! opposite of ReadHashCode()
        return WriteSize(nHashCode);
    }

    /// <summary>
    /// Write just the chars of the string. NOT '\0'. like fputs()
    /// Does NOT assume include NewLine or automatically add one.
    /// @note This can get overloaded for string only protocols. like FILE, fopen()
    /// @note MFC CStdioFile has void return for this.
    /// </summary>
    /// <param name="pszStr">nullptr ok</param>
    /// <returns>Number of chars written. -lt- 0 = error.</returns>
    HRESULT WriteString(const char* pszStr) override;
    HRESULT WriteString(const wchar_t* pszStr) override;

    /// <summary>
    /// Write a block prefixed by its size (Bytes).
    /// Write out a string with the length prefix.
    /// </summary>
    /// <param name="pBuffer"></param>
    /// <param name="nSize"></param>
    /// <returns>-lt- 0 = error</returns>
    HRESULT WriteBlob(const cMemSpan& b) {
        const HRESULT hRes = WriteSize(b.get_SizeBytes());
        if (FAILED(hRes)) return hRes;
        if (b.isEmpty()) return S_OK;
        return WriteSpan(b);
    }

    /// <summary>
    /// Write out a string with the length prefix. ReadBlobStr()
    /// </summary>
    /// <typeparam name="_CH"></typeparam>
    /// <param name="pszStr"></param>
    /// <returns>-lt- 0 = error</returns>
    template <typename _CH>
    HRESULT WriteBlobStr(const _CH* pszStr) {
        return WriteBlob(StrT::ToSpanStr(pszStr));
    }

    /// <summary>
    /// Copy data from a read stream (stmIn) to this write stream.
    /// like the IStream::CopyTo() or MFC CopyFrom().
    /// </summary>
    /// <param name="stmIn"></param>
    /// <param name="nSizeMax">Length of file or some arbitrary max to the stream size.</param>
    /// <param name="pProgress"></param>
    /// <param name="nTimeout"></param>
    /// <returns>Size of data moved.</returns>
    HRESULT WriteStream(cStreamInput& sInp, STREAM_POS_t nSizeMax = k_FILE_BLOCK_SIZE, IStreamProgressCallback* pProgress = nullptr, TIMESYSD_t nTimeout = 0);

    /// <summary>
    /// optional, virtual fflush() or FlushFileBuffers()
    /// </summary>
    /// <returns></returns>
    virtual HRESULT FlushX() {
        return S_OK;
    }
};

/// <summary>
/// Generic input stream of binary data.
/// @note SeekX() not always available from this interface. ReadX(nullptr) = skip over but not true seek.
/// </summary>
struct GRAYCORE_LINK cStreamInput : public cStreamBase {
    cStreamInput() noexcept {}

    /// <summary>
    /// Leave a certain amount of data (max message size for current protocol)
    /// such that we could SeekX() back for incomplete messages.
    /// similar to ReadCommit (put_AutoReadCommitSize) size. Used by cStreamTransaction.
    /// </summary>
    /// <param name="nSizeMin">0 = don't commit/lost any data until we have a complete message/block.</param>
    /// <returns></returns>
    virtual size_t SetReadCommitSize(size_t nSizeMin = k_FILE_BLOCK_SIZE) {
        UNREFERENCED_PARAMETER(nSizeMin);
        return 0;  // Previous commit size.
    }

    /// <summary>
    /// Read a block from the stream up to size of cMemSpan. // must support this.
    /// Similar to MFC CFile::Read()
    /// </summary>
    /// <param name="ret">max size. nullptr = skip over nDataSize in read stream. Same as SeekX().</param>
    /// <returns>Length of the stuff read. -lt- 0 = error. HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = need more data.</returns>
    virtual HRESULT ReadX(cMemSpan ret) noexcept {
        UNREFERENCED_PARAMETER(ret);
        return E_NOTIMPL;  // nothing read.
    }

    /// <summary>
    /// Read the whole stream as a single allocated block in memory.
    /// </summary>
    /// <param name="blob">cBlob</param>
    /// <param name="nSizeExtra">extra memory allocation.</param>
    /// <returns>length read. (Not including nSizeExtra). or -lt- 0 = error.</returns>
    HRESULT ReadAll(OUT cBlob& blob, size_t nSizeExtra = 0);

    virtual HRESULT ReadStringLine(cSpanX<char> ret);
    virtual HRESULT ReadStringLine(cSpanX<wchar_t> ret);

    // Support the base types directly.

    /// <summary>
    /// Read all of nSize or fail HRESULT_WIN32_C(ERROR_IO_INCOMPLETE). Ignore endian.
    /// </summary>
    /// <param name="pVal"></param>
    /// <param name="nSize">0 = S_OK</param>
    /// <returns>-lte- 0 = actual size read. or -lt- 0 = error. HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = need more data.</returns>
    HRESULT ReadSpan(cMemSpan ret) noexcept {
        const HRESULT hRes = ReadX(ret);
        if (FAILED(hRes)) return hRes;
        if (hRes == CastN(HRESULT, ret.get_SizeBytes())) return hRes;  // OK
        return HRESULT_WIN32_C(ERROR_IO_INCOMPLETE);                   // maybe HRESULT_WIN32_C(ERROR_HANDLE_EOF) ? maybe SeekX back and try again ?
    }
    template <typename TYPE = BYTE>
    HRESULT ReadT(OUT TYPE& val) noexcept {
        return ReadSpan(TOSPANT(val));
    }

    /// <summary>
    /// Read a type value in network order (Big endian). convert to host order.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="val"></param>
    /// <returns></returns>
    template <typename TYPE>
    HRESULT ReadTN(OUT TYPE& val) noexcept {
        const HRESULT hRes = ReadT(val);
        if (FAILED(hRes)) return hRes;
        val = cValT::NtoH(val);
        return hRes;
    }
    /// read Little Endian value. (Intel)
    template <typename TYPE>
    HRESULT ReadTLE(OUT TYPE& val) noexcept {
        const HRESULT hRes = ReadT(val);
        if (FAILED(hRes)) return hRes;
        val = cValT::LEtoH(val);
        return hRes;
    }

    /// <summary>
    /// Read a packed (variable length) unsigned size.
    /// Packed low to high values.
    /// Bit 7 reserved to indicate more bytes to come.
    /// opposite of WriteSize( size_t ).
    /// similar to ASN1 Length packing.
    /// </summary>
    /// <param name="nSize"></param>
    /// <returns>HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = no more data</returns>
    HRESULT ReadSize(OUT size_t& nSize) noexcept;

    /// <summary>
    /// Read Variable length size_t field and cast to final type.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="n"></param>
    /// <returns>-lt- 0 = fail</returns>
    template <typename TYPE>
    HRESULT ReadSizeT(OUT TYPE& n) noexcept {
        size_t nSizeTmp = 0;
        const HRESULT hRes = ReadSize(nSizeTmp);
        n = CastN(TYPE, nSizeTmp);
        return hRes;
    }

    HRESULT ReadHashCode(OUT UINT32& nHashCode) noexcept {
        return ReadSizeT<UINT32>(nHashCode);
    }
#ifdef USE_INT64
    HRESULT ReadHashCode(OUT UINT64& nHashCode) noexcept {
        return ReadSizeT<UINT64>(nHashCode);
    }
#endif
    /// <summary>
    /// Read a block/blob with a leading size field.
    /// </summary>
    /// <param name="ret">cMemSpan</param>
    /// <returns>-gte- 0 = actual size read.</returns>
    HRESULT ReadBlob(cMemSpan ret) {
        size_t nSize = 0;
        const HRESULT hRes = ReadSize(nSize);
        if (FAILED(hRes)) return hRes;
        if (nSize > ret.get_SizeBytes()) {
            // ASSERT(0);
            return HRESULT_WIN32_C(ERROR_FILE_CORRUPT);  // corrupt data.
        }
        return ReadSpan(cMemSpan(ret, nSize));
    }

    /// <summary>
    /// Read a variable length string that is prefixed by its size.
    /// </summary>
    /// <typeparam name="_CH"></typeparam>
    /// <param name="ret">_countof(pszStr), includes space for '\0'. e.g. _countof("abc") = 4</param>
    /// <returns>The size of the string (in chars) + including '\0'. HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = need more data.</returns>
    template <typename _CH>
    HRESULT ReadBlobStr(cSpanX<_CH> ret) {
        const HRESULT hResRead = ReadBlob(ret);
        if (FAILED(hResRead)) return hResRead;
        const StrLen_t nSizeRead = hResRead / sizeof(_CH);
        ASSERT(nSizeRead < ret.GetSize());
        ret.get_PtrWork()[nSizeRead] = '\0';
        return nSizeRead + 1;
    }

    virtual HRESULT ReadPeek(cMemSpan ret) noexcept;
};

/// <summary>
/// This is a bi-directional serial stream. RX and TX like ISequentialStream.
/// What mode is it actually using ?? use with cArchive?
/// Sequential = seek May not be avail from this interface. or only partial support.
/// Similar to MFC cArchive, COM ISequentialStream, std::basic_streambuf ?
/// ASSUME this overrides ReadX() and WriteX()
/// GetLength() optionally avail for this stream. I can move to any place in the stream.
/// See cStreamStatic for memory stream support.
/// </summary>
struct GRAYCORE_LINK cStream : public cStreamInput, public cStreamOutput {
    //! Disambiguate SeekX for cStreamBase to cStreamInput for stupid compiler.
    HRESULT SeekX(STREAM_OFFSET_t iOffset, SEEK_t eSeekOrigin = SEEK_t::_Set) noexcept override {
        return cStreamInput::SeekX(iOffset, eSeekOrigin);
    }
    STREAM_POS_t GetPosition() const noexcept override {
        return cStreamInput::GetPosition();
    }
    STREAM_POS_t GetLength() const noexcept override {
        return cStreamInput::GetLength();
    }

    void SeekToBegin() noexcept {
        return cStreamInput::SeekToBegin();  // using cStreamInput::SeekToBegin; // doesnt work for gcc!?
    }
    STREAM_POS_t SeekToEnd() noexcept {
        return cStreamInput::SeekToEnd();  // using cStreamInput::SeekToEnd; // doesnt work for gcc!?
    }
};

/// Base class for file reader/importer/etc helper.
struct GRAYCORE_LINK cStreamReader {
    cStreamInput* _pInp;  /// Pull transaction data from this stream.
    explicit cStreamReader(cStreamInput* p) : _pInp(p) {}
};

/// <summary>
/// we are reading a single message / Transaction from the stream. We need to read all of it or roll back.
/// </summary>
class GRAYCORE_LINK cStreamTransaction : public cStreamReader {
 public:
    STREAM_POS_t _nPosStart;
    size_t _nSeekSizeMinPrev;  /// Previous value. Maybe nested transactions !

 protected:
    HRESULT TransactionRollback();

 public:
    cStreamTransaction(cStreamInput* pInp);
    ~cStreamTransaction();

    inline bool isTransactionActive() const noexcept {
        //! Was SetTransactionComplete called ?
        return _nPosStart != k_STREAM_POS_ERR;
    }
    void SetTransactionComplete() {
        //! Success. we got what we wanted. no rollback.
        // ASSERT(isTransactionActive());
        _nPosStart = k_STREAM_POS_ERR;
        ASSERT(!isTransactionActive());
    }
    void SetTransactionFailed() noexcept {
        //! The stream broke in some way. e.g. socket close.
        //! assume connection is broken. no rollback.
        if (_pInp == nullptr) return;
        _pInp = nullptr;
    }
    /// <summary>
    /// I got a partial success. I used some of the data. maybe not all?
    /// </summary>
    void SetTransactionPartial(size_t nSize) {
        if (!isTransactionActive()) return;
        _nPosStart += nSize;  // roll back to here.
    }
    void SetTransactionRollback() {
        //! default behavior if closed without calling SetTransactionComplete() or SetTransactionFailed().
        //! if we didn't say it was a success, do a rollback on destruct.
        ASSERT(isTransactionActive());
    }
};

/// <summary>
/// A junk/null cStream that just tosses write data and has no read data. For testing.
/// </summary>
class GRAYCORE_LINK cStreamNull final : public cStream, public cSingletonStatic<cStreamNull> {
 protected:
    cStreamNull() noexcept : cSingletonStatic<cStreamNull>(this) {}

 public:
    DECLARE_cSingletonStatic(cStreamNull);

    /// <summary>
    /// Write a data block to null.
    /// </summary>
    HRESULT WriteX(const cMemSpan& m) override {
        return CastN(HRESULT, m.get_SizeBytes());
    }
};

/// <summary>
/// unit test helper for streams. cUnitTests
/// </summary>
struct GRAYCORE_LINK cStreamTester final {  // static
    /// <summary>
    /// Write strings to cStreamOutput.
    /// </summary>
    static bool GRAYCALL TestWrites(cStreamOutput& testfile1);
    /// <summary>
    /// Read strings from cStreamInput (as binary or text). Other side of TestWrites()
    /// </summary>
    static bool GRAYCALL TestReads(cStreamInput& stmIn, bool bString);

    /// <summary>
    /// Write to streams in random block sizes and make sure i read the same back.
    /// </summary>
    /// <param name="stmOut"></param>
    /// <param name="stmIn"></param>
    /// <param name="nSizeTotal">How much to write/test total ?</param>
    /// <param name="timeMax"></param>
    static bool GRAYCALL TestReadsAndWrites(cStreamOutput& stmOut, cStreamInput& stmIn, size_t nSizeTotal, TIMESECD_t timeMax);
};

}  // namespace Gray
#endif  // _INC_cStream_H
