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
static const cRandomDef::RAND_t k_Seed1 = CastN(cRandomDef::RAND_t, 0xECC440A96968484A);   // Some random seed
GRAYCORE_LINK cRandomDef g_Rand;  //! the default random number generator. NOT Thread Safe!

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
    return GetRandUX(UINT_MAX); // default implementation.
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

bool cRandomBase::GetNoise(cMemSpan& m) {  // override
    //! fill a buffer with random data from get_RandUns().
    //! < 0 = error.
    RAND_t* puData = m.get_DataW<RAND_t>();
    size_t iSizeLeft = m.get_DataSize();
    while (iSizeLeft > 0) {
        RAND_t uVal = get_RandUns();  // virtual.
        if (iSizeLeft < sizeof(uVal)) {
            // running out of room.
            cMem::Copy(puData, &uVal, iSizeLeft);  // partial fill.
            break;                                 // done.
        }
        puData[0] = uVal;
        puData++;
        iSizeLeft -= sizeof(uVal);
    }
    return true;
}

//*************************************************************
cRandomPerf::cRandomPerf() : cSingleton<cRandomPerf>(this, typeid(cRandomPerf)) {}

void GRAYCALL cRandomPerf::GetNoisePerf(cMemSpan& m) {  // static
    //! Get noise via low bits of cTimePerf
    //! Prefer GetNoiseOS noise over this, but use this as a fallback or XOR.
    UINT* puData = m.get_DataW<UINT>();
    size_t iSizeLeft = m.get_DataSize();
    while (iSizeLeft > 0) {
        cTimePerf tStart(true);                                 // the low bits of high performance timer should be random-ish.
        UINT32 uVal = (UINT32)(tStart.m_nTime ^ k_Seed1);  // use default XOR pattern
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

cRandomOS::cRandomOS() : cSingleton<cRandomOS>(this, typeid(cRandomOS)) {}

HRESULT GRAYCALL cRandomOS::GetNoiseOS(cMemSpan& m) {  // static. fill array with random. return # filled.
    //! Try to use the OS supplied noise generator. It may not work.
#ifdef __linux__
    // Is the Linux version high enough for random function ?

#if defined(SYS_getrandom)
    if (cSystemInfo::I().isVer3_17_plus()) {
        // long sys_getrandom(char __user *buf, size_t count, unsigned int flags)
        int iRet = ::syscall(SYS_getrandom, m.get_DataW(), m.get_DataSize(), 0);
        if (iRet >= 0) return (HRESULT)m.get_DataSize();
    }
#endif

    cOSHandle fd;
    fd.OpenHandle("/dev/random", O_RDONLY);
    if (!fd.isValidHandle()) fd.OpenHandle("/dev/urandom", O_RDONLY);
    if (fd.isValidHandle()) {
        int iSizeRet = fd.ReadX(m.get_DataW(), m.get_DataSize());
        if (iSizeRet == (int)m.get_DataSize()) return (HRESULT)m.get_DataSize();
    }

#elif defined(_WIN32) && defined(_MSC_VER) && defined(WINCRYPT32API)
    // WinCrypt
    ::HCRYPTPROV hProvider;  // NOT a pointer so its not a type-ablefor cOSHandle or cHandlePtr
    if (_FNF(::CryptAcquireContext)(&hProvider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        bool bRet = ::CryptGenRandom(hProvider, (DWORD)m.get_DataSize(), m.get_DataW());
        ::CryptReleaseContext(hProvider, 0);
        if (bRet) return (HRESULT)m.get_DataSize();
    }
#endif

    // OS noise not available. Try some other pseudo random source.
    // Just use the time??

    return E_NOTIMPL;
}

bool cRandomOS::GetNoise(cMemSpan& m) {  // override. fill array with random. return # filled.
    //! Get the best source or random noise the system can supply.
    //! Make a random seed from the most random source we can find.
    if (m.isEmpty()) return true;
    HRESULT hRes = GetNoiseOS(m);
    if (SUCCEEDED(hRes)) return true;  // all good.
    // else fallback to try the high perf timer.
    cRandomPerf::GetNoisePerf(m);
    return true;
}

cRandomDef::RAND_t cRandomOS::get_RandUns() {  // virtual
    RAND_t uVal = 0;
    GetNoise(TOSPANT(uVal));
    return uVal;
}

//*************************************************************

cRandomDef::cRandomDef(RAND_t nSeed) : m_nSeed(nSeed) {
    //! like ::srand()
    //! http://stackoverflow.com/questions/4768180/rand-implementation
}

void cRandomDef::InitSeed(const cMemSpan& seed) {  // override
    //! Start a repeatable series of pseudo random numbers
    //! like ::srand()
    m_nSeed = k_Seed1;  // Clear any under flow.
    cMem::Copy(&m_nSeed, seed, cValT::Min(sizeof(m_nSeed), seed.get_DataSize()));
}

cRandomDef::RAND_t cRandomDef::get_RandUns() {
    // https://stackoverflow.com/questions/18969783/how-can-i-get-the-sourcecode-for-rand-c
    m_nSeed = (m_nSeed * CastN(RAND_t, 1103515245)) + CastN(RAND_t, 12345);
    // Low bits tend to oscillate. so swap.
    m_nSeed = cMemT::ReverseType(m_nSeed);
    return m_nSeed;
}
}  // namespace Gray
