//! @file cRandom.h
//! Basic random number generator.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cRandom_H
#define _INC_cRandom_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "IUnknown.h"
#include "cBlob.h"
#include "cSingleton.h"

namespace Gray {
/// <summary>
/// Basic interface for getting noise (random data) from some entropy source. like: std::random_device ?
/// </summary>
DECLARE_INTERFACE(IRandomNoise) {
    typedef UINT RAND_t;  /// default value/seed size might be 32 or 64 bit . RAND_MAX = max value of UINT.
    IGNORE_WARN_INTERFACE(IRandomNoise);
    virtual bool GetNoise(cMemSpan ret) = 0;  /// fill array with random bytes.
};

/// <summary>
/// Generic abstract base class for a integer/binary (pseudo) random number generator.
/// Derived Providers will natively give up N bits of randomness per call/tick.
/// Similar to .NET System.Random
/// @note derived class MUST implement get_RandUns or GetRandUX to generate at least 32 bits or 31 bits of random data.
/// </summary>
struct GRAYCORE_LINK cRandomBase : public cObject, public IRandomNoise {
    cRandomBase() noexcept {}
    virtual ~cRandomBase() {}

    virtual void InitSeed(const cMemSpan& seed) = 0;  // all implementations must support this.
    void InitSeed(IRandomNoise* pSrc, size_t iSize);
    void InitSeedOS(size_t iSize = sizeof(RAND_t));
    void InitSeedUns(RAND_t uSeed);

    /// <summary>
    /// fill a buffer with random data from get_RandUns().
    /// </summary>
    /// <param name="ret"></param>
    /// <returns>-lt- 0 = error.</returns>
    bool GetNoise(cMemSpan ret) override;  /// fill array with random. return # filled.

    // Type helpers.

    /// <summary>
    /// get unsigned random number in 0 to -lt- UINT_MAX range
    /// @note derived implementation MUST override get_RandUns or GetRandUX
    /// </summary>
    /// <returns>RAND_t = UINT</returns>
    virtual RAND_t get_RandUns();

    /// <summary>
    /// flip a coin.
    /// </summary>
    bool GetRandBool() {
        return GetRandUX(2) == 1;
    }

    //! Get random number in scale. 0 to scale.
    //! unsigned integer is NON inclusive range. from 0 to nScale-1
    virtual RAND_t GetRandUX(RAND_t nScale);                // get integer random number in desired interval. (Non inclusive)
    RAND_t GetRandRange(RAND_t nRangeLo, RAND_t nRangeHi);  // output random int
};

/// <summary>
/// Get randomness from perf data. prefer cRandomOS but use this as fallback
/// </summary>
class GRAYCORE_LINK cRandomPerf final : public IRandomNoise, public cSingleton<cRandomPerf> {
    cRandomPerf();

 public:
    DECLARE_cSingleton(cRandomPerf);
    static void GRAYCALL GetNoisePerf(cMemSpan ret);
    bool GetNoise(cMemSpan ret) override {  // fill array with random. return # filled.
        GetNoisePerf(ret);
        return true;
    }
};

/// <summary>
/// Get Low level Hardware based noise supplied by the OS. NO SEED. NOT Deterministic (in theory)
/// __linux__ use "/dev/urandom" as a get_RandomSeed().
/// </summary>
class GRAYCORE_LINK cRandomOS final : public cRandomBase, public cSingleton<cRandomOS> {
    void InitSeed(const cMemSpan& seed) override {
        // No way to seed this.
        UNREFERENCED_PARAMETER(seed);
    }

 protected:
    cRandomOS();

 public:
    DECLARE_cSingleton(cRandomOS);
    static HRESULT GRAYCALL GetNoiseOS(cMemSpan ret);

    bool GetNoise(cMemSpan ret) override;  // fill array with random. return # filled.
    RAND_t get_RandUns() override;         // UINT_MAX
};

/// <summary>
/// Hold a blob of random data. Acts as a 'one time' cipher.
/// Supply test 'random' data. (e.g. maybe not random at all). acts as a one time cipher pad.
/// </summary>
class GRAYCORE_LINK cRandomBlock : public IRandomNoise {
    size_t _nReadIndex = 0;  /// How far have we read in _Data? recycle when at end ? like cQueueIndex
    cBlob _Data;             /// a block of 'random' test data. Act as 'one time' or pad cipher.

 public:
    cRandomBlock(const cMemSpan& m) noexcept : _Data(m, false) {}

    /// <summary>
    /// Get sample random data bytes
    /// </summary>
    bool GetNoise(cMemSpan ret) override {  // IRandomNoise
        if (_Data.isValidPtr()) {
            _nReadIndex = cMem::CopyRepeat(ret.GetTPtrW(), ret.get_SizeBytes(), _Data, _Data.get_SizeBytes(), _nReadIndex);
        } else {
            cMem::Fill(ret.GetTPtrW(), ret.get_SizeBytes(), 0x2a);  // No _Src supplied so fill with fixed data.
        }
        return true;
    }
};

/// <summary>
/// Like the default 'C' library seeded pseudo-random number generator ::srand() and ::rand()
/// Control a series of pseudo random numbers via a seed.
/// not thread safe. Use cThreadLocal to make thread safe version.
/// </summary>
class GRAYCORE_LINK cRandomDef : public cRandomBase {
    RAND_t _nSeed;  /// Control the pattern of random numbers via this seed. may be globally/thread shared. RAND_MAX = all bits.

 public:
    cRandomDef(RAND_t nSeed = 1);
    /// <summary>
    /// Start a repeatable series of pseudo random numbers. like ::srand()
    /// </summary>
    void InitSeed(const cMemSpan& seed) override;  /// Start a repeatable seeded series
    /// <summary>
    /// Get next pseudo random number like ::rand(); RAND_MAX = full UINT
    /// https://stackoverflow.com/questions/18969783/how-can-i-get-the-sourcecode-for-rand-c
    /// </summary>
    RAND_t get_RandUns() override;
};

extern GRAYCORE_LINK cRandomDef g_Rand;  /// the global random number generator with single _nSeed. NOT thread safe? but does that matter?
}  // namespace Gray
#endif
