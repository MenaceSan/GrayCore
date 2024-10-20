//! @file cStreamStack.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cStreamStack_H
#define _INC_cStreamStack_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cStreamQueue.h"

namespace Gray {
/// <summary>
/// Stack of input streams. Acts like a codec, decompressor, decipher, etc.
/// This input stream will grab data and process it from some other stream. holding it for when someone calls this->ReadX()
/// ASSUME derived class overrides ReadX and calls ReadFill
/// cStreamReader = source input stream. called by ReadFill()
/// </summary>
class GRAYCORE_LINK cStreamStackInp : public cStreamQueue, public cStreamReader {
 protected:
    HRESULT ReadFill();
    HRESULT ReadFillAligned(size_t nSizeBlockAlign = 1);

 public:
    cStreamStackInp(cStreamInput* pStreamInp = nullptr, size_t nSizeMaxBuffer = cStream::k_FILE_BLOCK_SIZE) noexcept
        : cStreamQueue(cValT::Min<size_t>(nSizeMaxBuffer / 2U, 8U * 1024U), nSizeMaxBuffer),  // chunk size, max size.
          cStreamReader(pStreamInp) {
        DEBUG_CHECK(nSizeMaxBuffer == 0 || get_AutoReadCommit() > 0);
    }

    /// <summary>
    /// read processed/decoded data from cStreamReader cStreamInput and SeekX
    /// </summary>
    /// <param name="ret">cMemSpan</param>
    /// <returns></returns>
    HRESULT ReadX(cMemSpan ret) noexcept override = 0;  // MUST be overridden. and call ReadFill() at some point.
};

/// <summary>
/// Stack of output streams. Acts like a codec, compressor, cipher, etc.
/// This output stream will process data and push it along to another output stream via _pOut->WriteX().
/// @note WriteX() MUST take all data passed to it and cQueueRW it up if it cant process immediately.
/// ASSUME derived class overrides WriteX and calls WriteFlush
/// </summary>
class GRAYCORE_LINK cStreamStackOut : public cStreamQueue { // cStreamOutput, protected cStreamStack
 protected:
    cStreamOutput* _pStreamOut;  /// End result output stream. called by WriteFlush()
 protected:
    HRESULT WriteFlush();
 public:
    cStreamStackOut(cStreamOutput* pStreamOut = nullptr, size_t nSizeBuffer = cStream::k_FILE_BLOCK_SIZE) noexcept : cStreamQueue(8 * 1024, nSizeBuffer), _pStreamOut(pStreamOut) {}
    HRESULT WriteX(const cMemSpan& m) override = 0;  // cStreamOutput override calls WriteFlush() // MUST be overridden
};

/// <summary>
/// Stream out to a cStreamOutput that might not take anything but whole packets.
/// call _pStreamOut->WriteX() multiple times for multiple whole packets.
/// save unfinished packets in buffer.
/// nSizeBuffer = the size of the largest possible whole packet.
/// </summary>
struct GRAYCORE_LINK cStreamStackPackets : public cStreamStackOut {
    cStreamStackPackets(cStreamOutput* pStreamOut = nullptr, size_t nSizeBuffer = cStream::k_FILE_BLOCK_SIZE) noexcept : cStreamStackOut(pStreamOut, nSizeBuffer) {}
    HRESULT WriteX(const cMemSpan& m) override;
};
}  // namespace Gray

#endif
