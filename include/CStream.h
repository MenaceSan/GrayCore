//
//! @file cStream.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cStream_H
#define _INC_cStream_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "StrT.h"
#include "cHeap.h"
#include "cBlob.h"
#include "cStreamProgress.h"  // STREAM_OFFSET_t , STREAM_POS_t, SEEK_t
#include "cTimeSys.h"         // cTimeSys
#include "cSingleton.h"

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
    STREAM_POS_t m_nCount;  /// Keep arbitrary stats on how much i move (bytes).
    cTimeSys m_tLast;       /// When did i last move data?

    cStreamStat() noexcept : m_nCount(0), m_tLast(cTimeSys::k_CLEAR) {}
    void ResetStat() noexcept {
        m_nCount = 0;  //  Keep arbitrary stats on how much i TX/RX.
        m_tLast.InitTime();
    }
    void UpdateStat(size_t n) noexcept {
        m_nCount += n;
        m_tLast.InitTimeNow();
    }
    void Add(const cStreamStat& n) noexcept {
        m_nCount += n.m_nCount;
        if (n.m_tLast.get_TimeSys() > m_tLast.get_TimeSys()) {
            m_tLast = n.m_tLast;
        }
    }
};

/// <summary>
/// track how much data is read and written and when.
/// </summary>
struct GRAYCORE_LINK cStreamStats {
    cStreamStat m_StatOut;
    cStreamStat m_StatInp;

    void Add(const cStreamStats& n) noexcept {
        m_StatOut.Add(n.m_StatOut);
        m_StatInp.Add(n.m_StatInp);
    }
};

/// <summary>
/// base class for cStreamOutput or cStreamInput.
/// </summary>
struct GRAYCORE_LINK cStreamBase {
    static const BYTE k_SIZE_MASK = 0x80;                 /// Used for WriteSize()
    static const size_t k_FILE_BLOCK_SIZE = (32 * 1024);  /// default arbitrary transfer block size. more than this is NOT more efficient.

    virtual ~cStreamBase() noexcept {}

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
    virtual STREAM_POS_t GetPosition() const {
        return k_STREAM_POS_ERR;  // It doesn't work on this type of stream!
    }

    /// <summary>
    /// get total length of the stream in bytes. if available. not the same as Read Length.
    /// default implementation using SeekX and GetPosition. override this for better implementation.
    /// </summary>
    /// <returns></returns>
    virtual STREAM_POS_t GetLength() const;

    /// <summary>
    /// Helper functions for SeekX
    /// ala MFC. SeekX to start of file/stream.
    /// </summary>
    void SeekToBegin() noexcept {
        SeekX(0, SEEK_t::_Set);
    }
    STREAM_POS_t SeekToEnd() {
        //! ala MFC. SeekX to end of file/stream.
        SeekX(0, SEEK_t::_End);
        return GetPosition();
    }
};

/// <summary>
/// Write a stream of data/text out to some arbitrary destination.
/// i.e. console, file, socket, telnet, game client, web page client, etc..
/// May be no SeekX() method available / implemented.
/// similar to STL std::ostream, and IWriteStream
/// </summary>
struct GRAYCORE_LINK cStreamOutput : public cStreamBase {
    cStreamOutput() noexcept {}
    ~cStreamOutput() noexcept override {}

    /// <summary>
    /// Write a data block to the stream.
    /// NOT pure virtual function. stub implementation.
    /// @note In string only protocols this might not be supported. (in favor of WriteString() only)
    /// </summary>
    /// <param name="pData"></param>
    /// <param name="nDataSize"></param>
    /// <returns>Number of bytes actually written. -lt- 0 = error.</returns>
    virtual HRESULT WriteX(const void* pData, size_t nDataSize) { // = 0;
        ASSERT(0);  // should never get called. (should always be overloaded)
        UNREFERENCED_PARAMETER(pData);
        UNREFERENCED_PARAMETER(nDataSize);
        return HRESULT_WIN32_C(ERROR_WRITE_FAULT);  // E_NOTIMPL
    }

    /// <summary>
    /// Write all or nothing (fail). otherwise same as WriteX() (where partials are allowed)
    /// </summary>
    /// <param name="pVal"></param>
    /// <param name="nDataSize"></param>
    /// <returns>Number of bytes written. -lt- 0 = error.</returns>
    HRESULT WriteT(const void* pVal, size_t nDataSize) {
        const HRESULT hRes = WriteX(pVal, nDataSize);
        if (SUCCEEDED(hRes) && hRes != CastN(HRESULT, nDataSize)) return HRESULT_WIN32_C(ERROR_WRITE_FAULT);  // STG_WRITEFAULT
        return hRes;
    }

    // Support the base types directly. Host endian order.
    template <typename TYPE>
    HRESULT WriteT(TYPE val);

    HRESULT WriteSize(size_t nSize);
    HRESULT WriteHashCode(HASHCODE_t nHashCode) {
        //! opposite of ReadHashCode()
        return WriteSize(nHashCode);
    }

    /// <summary>
    /// Write a block prefixed by its size (Bytes).
    /// Write out a string with the length prefix.
    /// </summary>
    /// <param name="pBuffer"></param>
    /// <param name="nSize"></param>
    /// <returns>-lt- 0 = error</returns>
    HRESULT WriteBlob(const void* pBuffer, size_t nSize) {
        const HRESULT hRes = WriteSize(nSize);
        if (FAILED(hRes)) return hRes;
        if (nSize == 0) return S_OK;
        return WriteT(pBuffer, nSize);
    }

    virtual HRESULT WriteString(const char* pszStr);
    virtual HRESULT WriteString(const wchar_t* pszStr);

    /// <summary>
    /// Write out a string with the length prefix. ReadBlobStr()
    /// </summary>
    /// <typeparam name="_CH"></typeparam>
    /// <param name="pszStr"></param>
    /// <returns>-lt- 0 = error</returns>
    template <typename _CH>
    HRESULT WriteBlobStr(const _CH* pszStr) {
        return WriteBlob(pszStr, (pszStr == nullptr) ? 0 : (StrT::Len(pszStr) * sizeof(_CH)));
    }

    /// <summary>
    /// Repeat writing of a char/wchar_t * nCount.
    /// </summary>
    /// <typeparam name="_CH"></typeparam>
    /// <param name="nChar"></param>
    /// <param name="nCount"></param>
    /// <returns>-lt- 0 = error.</returns>
    template <typename _CH>
    HRESULT WriteCharRepeat(_CH nChar, int nCount = 1) {
        ASSERT(nCount >= 0);
        _CH szTmp[2];
        szTmp[0] = nChar;
        szTmp[1] = '\0';
        for (; nCount-- > 0;) {
            const HRESULT hRes = WriteString(szTmp);
            if (FAILED(hRes)) return hRes;
        }
        return S_OK;
    }

    /// <summary>
    /// Write just the chars of the string. NOT nullptr
    /// </summary>
    /// <returns>-lt- 0 = error. else number of chars written</returns>
    template <typename _CH>
    HRESULT VPrintf(const _CH* pszFormat, va_list args) {
        ASSERT_NN(pszFormat);
        _CH szTemp[StrT::k_LEN_Default];
        const StrLen_t iLenRet = StrT::vsprintfN(szTemp, STRMAX(szTemp), pszFormat, args);
        UNREFERENCED_PARAMETER(iLenRet);
        return WriteString(szTemp); // WriteT ??
    }

    /// <summary>
    /// Write just the chars of the string. NOT nullptr
    /// Does NOT assume include NewLine or automatically add one.
    /// @note Use StrArg(s) for safe "%s" args.
    /// </summary>
    /// <typeparam name="_CH">char or wchar_t</typeparam>
    /// <param name="pszFormat"></param>
    /// <param name=""></param>
    /// <returns>-lt- 0 = error. else number of chars written</returns>
    template <typename _CH>
    HRESULT _cdecl Printf(const _CH* pszFormat, ...) {
        ASSERT_NN(pszFormat);
        va_list vargs;
        va_start(vargs, pszFormat);
        const HRESULT hResLen = VPrintf(pszFormat, vargs);
        va_end(vargs);
        return hResLen;
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
    /// optional, virtualized fflush() or FlushFileBuffers()
    /// </summary>
    /// <returns></returns>
    virtual HRESULT FlushX() {
        return S_OK;
    }
};

// Write all my types bool, char, int, float, double, _int64. (short, long, signed, unsigned)
#define CTYPE_DEF(a, _TYPE, c, d, e, f, g, h)                \
    template <>                                              \
    inline HRESULT cStreamOutput::WriteT<_TYPE>(_TYPE val) { \
        return WriteT(&val, sizeof(val));                    \
    }
#include "cTypes.tbl"
#undef CTYPE_DEF

/// <summary>
/// Generic input stream of data.
/// @note SeekX() not always available from this interface. ReadX(nullptr) = skip over but not true seek.
/// </summary>
struct GRAYCORE_LINK cStreamInput : public cStreamBase {
    cStreamInput() noexcept {}
    ~cStreamInput() override {}

    /// <summary>
    /// similar to ReadCommit (put_AutoReadCommitSize) size. Used by cStreamTransaction.
    /// Leave a certain amount of data (max message size for current protocol)
    /// such that we could SeekX() back for incomplete messages.
    /// </summary>
    /// <param name="nSizeMin">0 = don't commit/lost any data until we have a complete message/block.</param>
    /// <returns></returns>
    virtual size_t SetSeekSizeMin(size_t nSizeMin = k_FILE_BLOCK_SIZE) {
        UNREFERENCED_PARAMETER(nSizeMin);
        return 0;       // Previous commit size.
    }

    /// <summary>
    /// Read a block from the stream. // must support this.
    /// Similar to MFC CFile::Read()
    /// </summary>
    /// <param name="pData">nullptr = skip over nDataSize in read stream. Same as SeekX().</param>
    /// <param name="nDataSize"></param>
    /// <returns>Length of the stuff read. -lt- 0 = error. HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = need more data.</returns>
    virtual HRESULT ReadX(OUT void* pData, size_t nDataSize) noexcept {
        UNREFERENCED_PARAMETER(pData);
        UNREFERENCED_PARAMETER(nDataSize);
        return E_NOTIMPL;  // nothing read.
    }

    /// <summary>
    /// Read the whole stream as a single allocated block in memory.
    /// </summary>
    /// <param name="blob">cBlob</param>
    /// <param name="nSizeExtra">extra memory allocation.</param>
    /// <returns>length read. (Not including nSizeExtra). or -lt- 0 = error.</returns>
    HRESULT ReadAll(OUT cBlob& blob, size_t nSizeExtra = 0);

    virtual HRESULT ReadStringLine(OUT char* pszBuffer, StrLen_t iSizeMax);
    virtual HRESULT ReadStringLine(OUT wchar_t* pszBuffer, StrLen_t iSizeMax);

    // Support the base types directly.

    /// <summary>
    /// Read all of nSize or fail HRESULT_WIN32_C(ERROR_IO_INCOMPLETE). ASSUME host endian
    /// </summary>
    /// <param name="pVal"></param>
    /// <param name="nSize">0 = S_OK</param>
    /// <returns>-lte- 0 = actual size read. or -lt- 0 = error. HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = need more data.</returns>
    HRESULT ReadT(OUT void* pVal, size_t nSize) {
        const HRESULT hRes = ReadX(pVal, nSize);
        if (SUCCEEDED(hRes) && hRes != CastN(HRESULT, nSize)) {
            return HRESULT_WIN32_C(ERROR_IO_INCOMPLETE);  // maybe HRESULT_WIN32_C(ERROR_HANDLE_EOF) ?
        }
        return hRes;
    }
    template <typename TYPE = BYTE>
    HRESULT ReadT(OUT TYPE& val);

    /// <summary>
    /// Read a type value in network order. convert to host order.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="val"></param>
    /// <returns></returns>
    template <typename TYPE>
    HRESULT ReadTN(OUT TYPE& val) {
        const HRESULT hRes = ReadT(val);
        if (FAILED(hRes)) return hRes;
        val = cMemT::NtoH(val);
        return hRes;
    }
    template <typename TYPE>
    HRESULT ReadTLE(OUT TYPE& val) {
        const HRESULT hRes = ReadT(val);
        if (FAILED(hRes)) return hRes;
        val = cMemT::LEtoH(val);
        return hRes;
    }

    HRESULT ReadSize(OUT size_t& nSize);

    /// <summary>
    /// Read Variable length size_t field and cast to final type.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="n"></param>
    /// <returns>-lt- 0 = fail</returns>
    template <typename TYPE>
    HRESULT ReadSizeT(OUT TYPE& n) {
        size_t nSize = 0;
        const HRESULT hRes = ReadSize(nSize);
        n = CastN(TYPE, nSize);
        return hRes;
    }

    HRESULT ReadHashCode(OUT UINT32& nHashCode) {
        return ReadSizeT<UINT32>(nHashCode);
    }
#ifdef USE_INT64
    HRESULT ReadHashCode(OUT UINT64& nHashCode) {
        return ReadSizeT<UINT64>(nHashCode);
    }
#endif
    /// <summary>
    /// Read a block/blob with a leading size field.
    /// </summary>
    /// <param name="pBuffer"></param>
    /// <param name="nSizeMax"></param>
    /// <returns>-gte- 0 = actual size read.</returns>
    HRESULT ReadBlob(OUT BYTE* pBuffer, size_t nSizeMax) {
        size_t nSize = 0;
        const HRESULT hRes = ReadSize(nSize);
        if (FAILED(hRes)) return hRes;
        if (nSize > nSizeMax) { 
            // ASSERT(0);            
            return HRESULT_WIN32_C(ERROR_FILE_CORRUPT);  // corrupt data.
        }
        return ReadT(pBuffer, nSize);
    }

    /// <summary>
    /// Read a variable length string that is prefixed by its size.
    /// </summary>
    /// <typeparam name="_CH"></typeparam>
    /// <param name="pszStr"></param>
    /// <param name="iSizeMax">_countof(pszStr), includes space for '\0'. e.g. _countof("abc") = 4</param>
    /// <returns>The size of the string (in chars) + including '\0'. HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = need more data.</returns>
    template <typename _CH>
    HRESULT ReadBlobStr(OUT _CH* pszStr, StrLen_t iSizeMax) {
        const HRESULT hResRead = ReadBlob((BYTE*)pszStr, (size_t)(iSizeMax - 1) * sizeof(_CH));
        if (FAILED(hResRead)) return hResRead;
        const StrLen_t nSizeRead = hResRead / sizeof(_CH);
        ASSERT(nSizeRead < iSizeMax);
        pszStr[nSizeRead] = '\0';
        return nSizeRead + 1;
    }

    virtual HRESULT ReadPeek(void* pData, size_t nDataSize);
};

// Read all my types bool, char, int, float, double, _int64. (short, long, signed, unsigned). ASSUME serialized in host/native order. No big/little Endian issues.
#define CTYPE_DEF(a, _TYPE, c, d, e, f, g, h)                     \
    template <>                                                   \
    inline HRESULT cStreamInput::ReadT<_TYPE>(OUT _TYPE & rval) { \
        return ReadT(&rval, sizeof(rval));                        \
    }
#include "cTypes.tbl"
#undef CTYPE_DEF

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
    STREAM_POS_t GetPosition() const override {
        return cStreamInput::GetPosition();
    }
    STREAM_POS_t GetLength() const override {
        return cStreamInput::GetLength();
    }

    void SeekToBegin() {
        cStreamInput::SeekToBegin();
    }
    STREAM_POS_t SeekToEnd() {
        return cStreamInput::SeekToEnd();
    }
};

/// Base class for file reader/importer/etc helper.
struct GRAYCORE_LINK cStreamReader {
    cStreamInput* m_pInp;  /// Pull transaction data from this stream.
    explicit cStreamReader(cStreamInput* p) : m_pInp(p) {}
};

/// <summary>
/// we are reading a single message / Transaction from the stream. We need to read all of it or roll back.
/// </summary>
class GRAYCORE_LINK cStreamTransaction : public cStreamReader {
 public:
    STREAM_POS_t m_lPosStart;
    size_t m_nSeekSizeMinPrev;  /// Previous value. Maybe nested transactions !

 protected:
    HRESULT TransactionRollback() {
        // Roll back to m_lPosStart
        ASSERT(isTransactionActive());
        return m_pInp->SeekX(CastN(STREAM_OFFSET_t, m_lPosStart), SEEK_t::_Set);
    }

 public:
    cStreamTransaction(cStreamInput* pInp) : cStreamReader(pInp) {
        ASSERT(m_pInp != nullptr);
        m_lPosStart = m_pInp->GetPosition();
        if (m_lPosStart < 0 || m_lPosStart > CastN(STREAM_POS_t, cHeap::k_ALLOC_MAX)) {
            m_lPosStart = k_STREAM_POS_ERR;  // Rollback not allowed!
            return;
        }
        m_nSeekSizeMinPrev = m_pInp->SetSeekSizeMin(0);  // Don't use AutoReadCommit inside cStreamTransaction.
        ASSERT(m_nSeekSizeMinPrev >= 0 && m_nSeekSizeMinPrev <= cHeap::k_ALLOC_MAX);
        ASSERT(isTransactionActive());
    }
    ~cStreamTransaction() {
        //! if we didn't say it was a success, do a rollback on destruct.
        if (m_pInp == nullptr) return;
        if (isTransactionActive()) {  // We failed ! didn't call SetTransactionComplete or SetTransactionFailed()
            TransactionRollback();
        }
        // Restore commit ability
        m_pInp->SetSeekSizeMin(m_nSeekSizeMinPrev);  // Complete. we can now commit reads. e.g. toss data we have declared read.
    }

    inline bool isTransactionActive() const noexcept {
        //! Was SetTransactionComplete called ?
        return m_lPosStart != k_STREAM_POS_ERR;
    }
    void SetTransactionComplete() {
        //! Success. we got what we wanted. no rollback.
        // ASSERT(isTransactionActive());
        m_lPosStart = k_STREAM_POS_ERR;
        ASSERT(!isTransactionActive());
    }
    /// <summary>
    /// I got a partial success. I used some of the data. maybe not all?
    /// </summary>
    void SetTransactionCompleteN(size_t nSize) {
        if (!isTransactionActive()) return;
        m_lPosStart += nSize;  // roll back to here.
    }
    void SetTransactionFailed() noexcept {
        //! The stream broke in some way. e.g. socket close.
        //! assume connection is broken. no rollback.
        if (m_pInp == nullptr) return;
        m_pInp = nullptr;
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
struct GRAYCORE_LINK cStreamNull : public cStream, public cSingleton<cStreamNull> {
    cStreamNull() noexcept : cSingleton<cStreamNull>(this) {}

    /// <summary>
    /// Write a data block to null.
    /// </summary>
    HRESULT WriteX(const void* pData, size_t nDataSize) override {
        UNREFERENCED_PARAMETER(pData);
        return CastN(HRESULT, nDataSize);
    }
};
}  // namespace Gray
#endif  // _INC_cStream_H
