//! @file cStreamProgress.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cStreamProgress_H
#define _INC_cStreamProgress_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"  // S_OK
#include "Index.h"
#include "PtrCast.h"
#include "cDebugAssert.h"

namespace Gray {
/// <summary>
/// What are we moving relative to ? SEEK_SET,SEEK_CUR,SEEK_END or FILE_BEGIN,FILE_CURRENT,FILE_END
/// SEEK_SET defined for both __linux__ and _WIN32
/// same as enum tagSTREAM_SEEK
/// </summary>
enum class SEEK_t {
    _Set = 0,  /// SEEK_SET = FILE_BEGIN = STREAM_SEEK_SET = 0 = relative to the start of the file. (default)
    _Cur = 1,  /// SEEK_CUR = FILE_CURRENT = STREAM_SEEK_CUR = 1 = relative to the current position.
    _End = 2,  /// SEEK_END = FILE_END = STREAM_SEEK_END = 2 = relative to the end of the file.
};

#if defined(_MFC_VER) && (_MFC_VER > 0x0600)
typedef LONGLONG STREAM_OFFSET_t;  // AKA off_t
typedef ULONGLONG STREAM_POS_t;    // same as FILE_SIZE_t.
#define USE_FILE_POS64

#else
typedef LONG_PTR STREAM_OFFSET_t;  /// Might be 64 or 32 bit relative value (signed). TODO SET USE_FILE_POS64. AKA off_t (linux)
typedef ULONG_PTR STREAM_POS_t;    /// NOT same as FILE_SIZE_t in 32 bit? Why not ?

#endif  // ! _MFC_VER

constexpr STREAM_POS_t k_STREAM_POS_ERR = CastN(STREAM_POS_t, -1);  // like INVALID_SET_FILE_POINTER

/// <summary>
/// How much of some total has been processed? Similar to like cRangeT
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <typename TYPE = STREAM_POS_t>
struct cStreamProgressT {
    TYPE _nCurrent;  /// How far the stream has progressed toward _nTotal. Current Value.
    TYPE _nTotal;    /// Total/Max size of the stream. 0 = i have no idea how big the total is.

    cStreamProgressT(TYPE nCurrent = 0, TYPE nTotal = 0) noexcept : _nCurrent(nCurrent), _nTotal(nTotal) {}
    bool isComplete() const noexcept {
        if (_nTotal == 0) return true;  // no idea.
        if (_nCurrent >= _nTotal) return true;
        return false;
    }
    float get_PercentFloat() const noexcept {
        //! Get percent of total.
        //! @return 0.0f to 1.0f
        if (_nTotal == 0) return 0;  // no idea.
        return CastN(float, CastN(double, _nCurrent) / CastN(double, _nTotal));
    }
    int get_PercentInt() const noexcept {
        //! @return From 0 to 100. MULDIV
        if (_nTotal == 0) return 0;  // no idea.
        return CastN(int, (_nCurrent * 100) / _nTotal);
    }
    bool isValidPercent() const noexcept {
        if (_nTotal <= 0) return false;  // no idea.
        return _nCurrent <= _nTotal;
    }
    void InitZero() noexcept {
        _nCurrent = 0;
        _nTotal = 0;  // 0 = i have no idea how big the total is.
    }
};
typedef cStreamProgressT<STREAM_POS_t> cStreamProgress;

/// <summary>
/// We are descending into nested tasks we have not fully measured.
/// i.e. enumerating subdirectories i have not yet counted.
/// TODO This can be a used as a time throbber. the task time is just an estimate. we should never actually reach it.
/// _nTotal = Estimated Value of the directory we are processing. (1.0=total of all files)
/// _nCurrent = Current progress 0 to 1.0 (_nTotal)
/// </summary>
struct GRAYCORE_LINK cStreamProgressF : cStreamProgressT<float> {
    friend class cStreamProgressChunk;

    cStreamProgressF() noexcept {
        InitPercent();
    }
    void InitPercent() noexcept {
        _nTotal = 1.0f;  // of everything.
        _nCurrent = 0.0f;
    }
#if 0
	void put_PercentComplete(float fComplete = 1.0f) noexcept {
		_nCurrent = fComplete;	// indicate we are done.
	}
#endif
    float get_PercentComplete() const noexcept {
        return _nCurrent;  // return value <= 1.0f. ASSUME _nTotal = 1.
    }
    float get_PercentChunk() const noexcept {
        return _nTotal;
    }
};

/// <summary>
/// Track nested work load. Processing a tree.
/// </summary>
class GRAYCORE_LINK cStreamProgressChunk {
    cStreamProgressF& _rProg;
    cStreamProgressF _ProgPrev;  /// _rProg at start.
    int _nChunk = 0;                /// What chunk are we on ?
    int _nChunks;               /// how many chunks is this supposed to be ?

 public:
    cStreamProgressChunk(cStreamProgressF& prog, int iSubChunks, int iParentChunks = 1) : _rProg(prog), _nChunks(iSubChunks) {
        //! Start a sub-chunk of the task. expect iChunks in this task. IncChunk() will be called.
        //! iParentChunks = the number of _nTotal we represent.
        //! ASSUME caller will IncChunk(iParentChunks) after this is destructed.
        _ProgPrev = _rProg;
        ASSERT(iSubChunks >= 0);
        if (_nChunks == 0) {
            prog._nTotal = 0;
        } else {
            prog._nTotal = (CastN(float, iParentChunks) * prog._nTotal) / CastN(float, _nChunks);  // MULDIV
        }
    }
    ~cStreamProgressChunk() noexcept {
        //! complete the task.
        if (_ProgPrev._nTotal >= 1.0f) {
            _rProg._nTotal = _rProg._nCurrent = 1.0f;  // i have no parent. we are done.
        } else {
            _rProg = _ProgPrev;  // back out my changes and assume IncChunk() will be called from my parent.
        }
    }
    void IncChunk(int iChunks = 1) noexcept {
        //! We are making some progress at the current task.
        //! ASSERT(prog._nTotal);
        _nChunk += iChunks;
        if (_nChunk > _nChunks) {
            // this really shouldn't happen!
            _nChunk = _nChunks;
        }
        _rProg._nCurrent += iChunks * _rProg._nTotal;
    }
};

/// <summary>
/// Abstract Base class. Get call backs indicating the overall progress of some action.
/// Similar to .NET System.IProgress
/// This can be used as ICancellable with cThreadState. The caller may decide to cancel the function via the onProgressCallback return.
/// </summary>
struct GRAYCORE_LINK DECLSPEC_NOVTABLE IStreamProgressCallback {
    IGNORE_WARN_ABSTRACT(IStreamProgressCallback);

    virtual HRESULT _stdcall onProgressCallback(const cStreamProgress& progress) {
        //! Some synchronous process is notifying us how far it has gone.
        //! @return
        //!  S_OK = just keep going
        //!  FAILED(hRes) = stop the action. e.g. HRESULT_WIN32_C(ERROR_CANCELED) = stop
        UNREFERENCED_REFERENCE(progress);
        return S_OK;  // S_OK=just keep going.
    }
};
}  // namespace Gray
#endif
