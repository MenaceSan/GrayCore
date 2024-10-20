//! @file cFileText.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cFileText_H
#define _INC_cFileText_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cFile.h"
#include "cHandlePtr.h"
#include "cTextReader.h"
#include "cTextPos.h"

namespace Gray {
class GRAYCORE_LINK cFileTextBase : public cFile {
    typedef cFile SUPER_t;

 public:
    /// <summary>
    /// Read an ASCII or UTF8 string/line from the file.
    /// Read to the end of the single line.
    /// </summary>
    HRESULT ReadStringLineA(OUT cStringA& r);
    // HRESULT ReadStringLineW(OUT cStringW &r);
};

/// <summary>
/// read text lines from a file stream.
/// @note Try to use this instead of cFileText/USE_CRT below.
/// Replace the FILE* streaming file i/o reader fread() with something more under our control.
/// </summary>
class GRAYCORE_LINK cFileTextReader : public cFileTextBase {
    typedef cFileTextBase SUPER_t;

 public:
    cTextReaderStream _Reader;

 public:
    cFileTextReader(size_t nSizeLineMax = cStream::k_FILE_BLOCK_SIZE * 2) noexcept : _Reader(*this, nSizeLineMax) {}
    ~cFileTextReader() override {}

    inline ITERATE_t get_CurrentLineNumber() const noexcept {
        return _Reader.get_CurrentLineNumber();
    }
    cTextPos get_TextPos() const {
        return cTextPos(GetPosition(), _Reader.get_CurrentLineNumber(), 0);
    }

    HRESULT ReadStringLine(cSpanX<char> ret) override {
        return _Reader.ReadStringLine(ret);
    }
};

#if USE_CRT

/// <summary>
/// A file stream read/writer with special processing for detecting and converting text "\r\n" chars
/// dont bother if file is write only. cFile is fine.
/// @note use cFileTextReader instead.
/// like MFC CStdioFile. Compatible with C standard FILE,stdin,stdout,stderr.
/// like cTextReaderStream
/// use OF_TEXT_A or OF_TEXT_W for format ??
/// </summary>
class GRAYCORE_LINK cFileText : public cFileTextBase { // Try to be compatible with MFC CStdioFile
    typedef cFileTextBase SUPER_t;
 protected:
    ::FILE* _pStream = nullptr;  /// the open CRT script/text type file. named as MFC CStdioFile.

 private:
    ITERATE_t _iLineNumCur = 0;  /// track the line number we are on currently. (0 based) (for cTextPos)

 private:
    HRESULT OpenCreate(cStringF sFilePath, OF_FLAGS_t nOpenFlags = OF_CREATE | OF_WRITE, _SECURITY_ATTRIBUTES* pSa = nullptr) {
        //! Don't use this. deleted.
        ASSERT(0);
        UNREFERENCED_PARAMETER(sFilePath);
        UNREFERENCED_PARAMETER(nOpenFlags);
        UNREFERENCED_PARAMETER(pSa);
        return E_NOTIMPL;
    }

    HRESULT ReadX(cMemSpan ret) noexcept override;  // dont call this directly.
    HRESULT WriteX(const cMemSpan& m) override;                          // dont call this directly.

 public:
    cFileText();
    cFileText(cStringF sFilePath, OF_FLAGS_t nOpenFlags);
    ~cFileText() noexcept override;

    inline ITERATE_t get_CurrentLineNumber() const noexcept {
        return _iLineNumCur;
    }

    bool isEOF() const;
    HRESULT GetStreamError() const;
    HRESULT OpenFileHandle(::HANDLE h, OF_FLAGS_t nOpenFlags);
 
    /// <summary>
    /// Open a text file. OF_TEXT \n processing is weird.
    /// </summary>
    /// <param name="sFilePath"></param>
    /// <param name="nOpenFlags">OF_READ|OF_WRITE|OF_READWRITE</param>
    /// <returns></returns>
    HRESULT OpenX(cStringF sFilePath = _FN(""), OF_FLAGS_t nOpenFlags = OF_READ | OF_SHARE_DENY_NONE) override;

    void Close() noexcept override;
    HRESULT SeekX(STREAM_OFFSET_t nOffset = 0, SEEK_t eSeekOrigin = SEEK_t::_Set) noexcept override;

    ::FILE* get_FileStream() const noexcept {
        // operator FILE* () const { return _pStream; }	// Dangerous.
        return _pStream;
    }
    const FILECHAR_t* get_ModeCPtr() const;
    ::FILE* DetachFileStream() noexcept;

    // override
    STREAM_POS_t GetPosition() const noexcept override;
    HRESULT FlushX() override;

    HRESULT WriteString(const char* pszStr) override;
    HRESULT WriteString(const wchar_t* pszStr) override {
        return SUPER_t::WriteString(pszStr);
    }
    HRESULT ReadStringLine(cSpanX<char> ret) override;
    HRESULT ReadStringLine(cSpanX<wchar_t> ret) override {
        return SUPER_t::ReadStringLine(ret);
    }

    bool put_TextPos(const cTextPos& rPos);
    cTextPos get_TextPos() const {
        return cTextPos(GetPosition(), _iLineNumCur, 0);
    }
};
#endif
}  // namespace Gray

#if !defined(_MFC_VER) && USE_CRT
typedef Gray::cFileText CStdioFile;  // emulate  _MFC_VER
#endif
#endif  // _INC_cFileText_H
