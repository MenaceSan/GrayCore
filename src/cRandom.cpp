//! @file cRandom.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cBlob.h"
#include "cRandom.h"

#ifdef __linux__
#include <fcntl.h>  // open()
#endif

namespace Gray {
cSingleton_IMPL(cRandomPerf);
cSingleton_IMPL(cRandomOS);

static const cRandomDef::RAND_t k_Seed1 = CastN(cRandomDef::RAND_t, 0xECC440A96968484AULL);  /// a random seed. Truncated in 32 bit mode.
GRAYCORE_LINK cRandomDef g_Rand;                                                          //! the default random number generator. NOT Thread Safe!

#if defined(_WIN32) && !defined(_MSC_VER)
// extern for CryptAcquireContext
#endif

//*************************************************************

void cRandomBase::InitSeed(IRandomNoise* pSrc, size_t iSize) {
    cBlob blob(iSize);
    pSrc->GetNoise(blob);
    InitSeed(blob);
}

void cRandomBase::InitSeedOS(size_t iSize) {
    //! Initialize random sequence randomly.
    //! Seed the random generator with time or some mix of more random data. NOT Deterministic!
    InitSeed(cRandomOS::get_Single(), iSize);
}

void cRandomBase::InitSeedUns(RAND_t nSeed) {
    //! set seed from UINT. Similar to RAND_t
    InitSeed(cMemSpan(&nSeed, sizeof(nSeed)));
}

cRandomBase::RAND_t cRandomBase::get_RandUns() {  // virtual
    return GetRandUX(UINT_MAX);                   // default implementation.
}

cRandomBase::RAND_t cRandomBase::GetRandUX(RAND_t nScale) {  // virtual
    //! get random integer number in 0 to < nScale range
    //! NON inclusive
    //! default implementation.
    //! @note derived implementation MUST override get_RandUns() or GetRandUX()
    if (nScale <= 1) return 0;      // error
    return get_RandUns() % nScale;  // shave/modulus off the part i don't want.
}

cRandomBase::RAND_t cRandomBase::GetRandRange(RAND_t iRangeLo, RAND_t iRangeHi) {  // output random int
    //! get a random integer inclusive inside range. default implementation.
    //! @return a random number in an inclusive range of integers.
    if (iRangeHi <= iRangeLo) return iRangeLo;
    return iRangeLo + GetRandUX((iRangeHi - iRangeLo) + 1);
}

bool cRandomBase::GetNoise(cMemSpan ret) {  // override
    RAND_t* puData = ret.GetTPtrW<RAND_t>();
    size_t iSizeLeft = ret.get_SizeBytes();
    while (iSizeLeft > 0) {
        const RAND_t uVal = get_RandUns();  // virtual.
        if (iSizeLeft < sizeof(uVal)) {
            // running out of room.
            cMem::Copy(puData, &uVal, iSizeLeft);  // partial fill.
            break;                                 // done.
        }
        *puData = uVal;
        puData++;
        iSizeLeft -= sizeof(uVal);
    }
    return true;
}

//*************************************************************
cRandomPerf::cRandomPerf() : cSingleton<cRandomPerf>(this ) {}

void GRAYCALL cRandomPerf::GetNoisePerf(cMemSpan ret) {  // static
    //! Get noise via low bits of cTimePerf
    //! Prefer GetNoiseOS noise over this, but use this as a fallback or XOR.
    UINT* puData = ret.GetTPtrW<UINT>();
    size_t iSizeLeft = ret.get_SizeBytes();
    while (iSizeLeft > 0) {
        cTimePerf tStart(true);                            // the low bits of high performance timer should be random-ish.
        UINT32 uVal = (UINT32)(tStart._nTimePerf ^ k_Seed1);  // use default XOR pattern
        if (iSizeLeft < sizeof(uVal)) {
            // running out of room.
            cMem::Copy(puData, &uVal, iSizeLeft);
            break;  // done.
        }
        puData[0] = uVal;
        puData++;
        iSizeLeft -= sizeof(uVal);
    }
}

//*************************************************************

cRandomOS::cRandomOS() : cSingleton<cRandomOS>(this ) {}

HRESULT GRAYCALL cRandomOS::GetNoiseOS(cMemSpan ret) {  // static. fill array with random. return # filled.
    //! Try to use the OS supplied noise generator. It may not work.
#ifdef __linux__
    // Is the Linux version high enough for random function ?

#if defined(SYS_getrandom)
    if (cSystemInfo::I().isVer3_17_plus()) {
        // long sys_getrandom(char __user *buf, size_t count, unsigned int flags)
        int iRet = ::syscall(SYS_getrandom, ret.GetTPtrW(), ret.get_SizeBytes(), 0);
        if (iRet >= 0) return (HRESULT)ret.get_SizeBytes();
    }
#endif

    cOSHandle fd;
    fd.OpenHandle("/dev/random", O_RDONLY);
    if (!fd.isValidHandle()) fd.OpenHandle("/dev/urandom", O_RDONLY);
    if (fd.isValidHandle()) {
        int iSizeRet = fd.ReadX(ret);
        if (iSizeRet == (int)ret.get_SizeBytes()) return (HRESULT)ret.get_SizeBytes();
    }

#elif defined(_WIN32) && defined(_MSC_VER) && defined(WINCRYPT32API)
    // WinCrypt
    ::HCRYPTPROV hProvider;  // NOT a pointer so its not a type-ablefor cOSHandle or cHandlePtr
    if (_FNF(::CryptAcquireContext)(&hProvider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        bool bRet = ::CryptGenRandom(hProvider, (DWORD)ret.get_SizeBytes(), ret.GetTPtrW());
        ::CryptReleaseContext(hProvider, 0);
        if (bRet) return (HRESULT)ret.get_SizeBytes();
    }
#endif

    // OS noise not available. Try some other pseudo random source.
    // Just use the time??

    return E_NOTIMPL;
}

bool cRandomOS::GetNoise(cMemSpan ret) {  // override. fill array with random. return # filled.
    //! Get the best source or random noise the system can supply.
    //! Make a random seed from the most random source we can find.
    if (ret.isEmpty()) return true;
    HRESULT hRes = GetNoiseOS(ret);
    if (SUCCEEDED(hRes)) return true;  // all good.
    // else fallback to try the high perf timer.
    cRandomPerf::GetNoisePerf(ret);
    return true;
}

cRandomDef::RAND_t cRandomOS::get_RandUns() {  // virtual
    RAND_t uVal = 0;
    GetNoise(TOSPANT(uVal));
    return uVal;
}

//*************************************************************

cRandomDef::cRandomDef(RAND_t nSeed) : _nSeed(nSeed) {
    //! like ::srand()
    //! http://stackoverflow.com/questions/4768180/rand-implementation
}

void cRandomDef::InitSeed(const cMemSpan& seed) {  // override
    _nSeed = k_Seed1;  // Clear any under flow.
    TOSPANT(_nSeed).SetCopySpan(seed);
}

cRandomDef::RAND_t cRandomDef::get_RandUns() {
    _nSeed = (_nSeed * CastN(RAND_t, 1103515245)) + CastN(RAND_t, 12345);
    // Low bits tend to oscillate. so swap.
    _nSeed = cValT::ReverseBytes(_nSeed);
    return _nSeed;
}
}  // namespace Gray
