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
/// Basic interface for getting noise (random data) from some entropy source.
/// </summary>
DECLARE_INTERFACE(IRandomNoise) {
    typedef UINT RAND_t;  /// default value/seed size might be 32 or 64 bit .
    IGNORE_WARN_INTERFACE(IRandomNoise);
    virtual bool GetNoise(cMemSpan & m) = 0;  /// fill array with random bytes.
};

/// <summary>
/// Generic abstract base class for a integer/binary (pseudo) random number generator.
/// Derived Providers will natively give up N bits of randomness per call/tick.
/// Similar to .NET System.Random
/// @note derived class MUST implement get_RandUns or GetRandUX to generate at least 32 bits or 31 bits of random data.
/// </summary>
struct GRAYCORE_LINK cRandomBase : public IRandomNoise {
    cRandomBase() noexcept {}
    virtual ~cRandomBase() {}

    virtual void InitSeed(const cMemSpan& seed) = 0;  // all implementations must support this.
    void InitSeed(IRandomNoise* pSrc, size_t iSize);
    void InitSeedOS(size_t iSize = sizeof(RAND_t));
    void InitSeedUns(RAND_t uSeed);

    bool GetNoise(cMemSpan& m) override;  /// fill array with random. return # filled.

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
    SINGLETON_IMPL(cRandomPerf);

 protected:
    cRandomPerf();

 public:
    static void GRAYCALL GetNoisePerf(cMemSpan& m);
    bool GetNoise(cMemSpan& m) override {  // fill array with random. return # filled.
        GetNoisePerf(m);
        return true;
    }
};

/// <summary>
/// Get Low level Hardware based noise supplied by the OS. NO SEED. NOT Deterministic (in theory)
/// __linux__ use "/dev/urandom" as a get_RandomSeed().
/// </summary>
class GRAYCORE_LINK cRandomOS final : public cRandomBase, public cSingleton<cRandomOS> {
    SINGLETON_IMPL(cRandomOS);

    void InitSeed(const cMemSpan& seed) override {
        // No way to seed this.
        UNREFERENCED_PARAMETER(seed);
    }

 protected:
    cRandomOS();

 public:
    static HRESULT GRAYCALL GetNoiseOS(cMemSpan& m);
    bool GetNoise(cMemSpan& m) override;  // fill array with random. return # filled.
    RAND_t get_RandUns() override;        // UINT_MAX
};

/// <summary>
/// Hold a blob of random data. Acts as a one time cipher.
/// Supply test 'random' data. (e.g. maybe not random at all). acts as a one time cipher pad.
/// </summary>
class GRAYCORE_LINK cRandomBlock : public IRandomNoise {
    size_t m_nReadIndex;  /// How far have we read in m_Data? recycle when at end ? like cQueueIndex
    cBlob m_Data;         /// a block of 'random' test data.

 public:
    cRandomBlock(const cMemSpan& m) noexcept : m_Data(m, false), m_nReadIndex(0) {}

    /// <summary>
    /// Get sample random data bytes
    /// </summary>
    bool GetNoise(cMemSpan& m) override {  // IRandomNoise
        if (m_Data.isValidPtr()) {
            m_nReadIndex = cMem::CopyRepeat(m.get_DataW(), m.get_DataSize(), m_Data, m_Data.get_DataSize(), m_nReadIndex);
        } else {
            cMem::Fill(m.get_DataW(), m.get_DataSize(), 0x2a);  // No m_Src supplied so fill with fixed data.
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
    RAND_t m_nSeed;  /// Control the pattern of random numbers via the seed. may be globally/thread shared.

 public:
    cRandomDef(RAND_t nSeed = 1);
    void InitSeed(const cMemSpan& seed) override;  /// Start a repeatable seeded series
    /// <summary>
    /// Get next pseudo random number like ::rand();
    /// </summary>
    RAND_t get_RandUns() override;
};

extern GRAYCORE_LINK cRandomDef g_Rand;  /// the global random number generator. NOT thread safe. but does that matter?
}  // namespace Gray
#endif
