//
//! @file cRandom.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cBlob.h"
#include "cRandom.h"

#ifdef __linux__
#include <fcntl.h>  // open()
#endif

namespace Gray {
GRAYCORE_LINK cRandomDef g_Rand;  //! the default random number generator. NOT Thread Safe!

#if defined(_WIN32) && !defined(_MSC_VER)
// extern for CryptAcquireContext
#endif

//*************************************************************

void cRandomBase::InitSeed(IRandomNoise* pSrc, size_t iSize) {
    cBlob blob(iSize);
    HRESULT iSizeRet = pSrc->GetNoise(blob.get_DataW(), iSize);
    InitSeed(cMemSpan(blob.get_DataC(), (size_t)iSizeRet));  // pure virtual
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
    //! get UINT random number in 0 to < UINT_MAX range
    //! default implementation.
    //! @note derived implementation MUST override get_RandUns or GetRandUX
    return GetRandUX(UINT_MAX);
}

cRandomBase::RAND_t cRandomBase::GetRandUX(RAND_t nScale) {  // virtual
    //! get random integer number in 0 to < nScale range
    //! NON inclusive
    //! default implementation.
    //! @note derived implementation MUST override get_RandUns() or GetRandUX()
    if (nScale <= 1) return 0;      // error
    return get_RandUns() % nScale;  // shave off the part i don't want.
}

cRandomBase::RAND_t cRandomBase::GetRandRange(RAND_t iRangeLo, RAND_t iRangeHi) {  // output random int
    //! get a random integer inclusive inside range. default implementation.
    //! @return a random number in an inclusive range of integers.
    if (iRangeHi <= iRangeLo) return iRangeLo;
    return iRangeLo + GetRandUX((iRangeHi - iRangeLo) + 1);
}

HRESULT cRandomBase::GetNoise(void* pvData, size_t iSize) {  // override
    //! fill a buffer with random data from get_RandUns().
    //! < 0 = error.
    RAND_t* puData = (RAND_t*)pvData;
    size_t iSizeLeft = iSize;
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
    return (HRESULT)iSize;
}

//*************************************************************
cRandomPerf::cRandomPerf() : cSingleton<cRandomPerf>(this, typeid(cRandomPerf)) {}

void GRAYCALL cRandomPerf::GetNoisePerf(void* pData, size_t iSize) {  // static
    //! Get noise via low bits of cTimePerf
    //! Prefer GetNoiseOS noise over this, but use this as a fallback or XOR.
    UINT* puData = (UINT*)pData;
    size_t iSizeLeft = iSize;
    while (iSizeLeft > 0) {
        cTimePerf tStart(true);                                 // the low bits of high performance timer should be random-ish.
        UINT32 uVal = (UINT32)(tStart.m_nTime ^ 3141592654UL);  // use PI as default XOR pattern
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

HRESULT GRAYCALL cRandomOS::GetNoiseOS(void* pData, size_t iSize) {  // static. fill array with random. return # filled.
    //! Try to use the OS supplied noise generator. It may not work.
#ifdef __linux__
    // Is the Linux version high enough for random function ?

#if defined(SYS_getrandom)
    if (cSystemInfo::I().isVer3_17_plus()) {
        // long sys_getrandom(char __user *buf, size_t count, unsigned int flags)
        int iRet = ::syscall(SYS_getrandom, pData, iSize, 0);
        if (iRet > 0) return iRet;
    }
#endif

    cOSHandle fd;
    fd.OpenHandle("/dev/random", O_RDONLY);
    if (!fd.isValidHandle()) fd.OpenHandle("/dev/urandom", O_RDONLY);
    if (fd.isValidHandle()) {
        int iSizeRet = fd.ReadX(pData, iSize);
        if (iSizeRet != (int)iSize) return E_FAIL;
        return iSize;
    }

#elif defined(_WIN32) && defined(_MSC_VER) && defined(WINCRYPT32API)
    // WinCrypt
    ::HCRYPTPROV hProvider;  // NOT a pointer so its not a type-ablefor cOSHandle or cHandlePtr
    if (_FNF(::CryptAcquireContext)(&hProvider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        bool bRet = ::CryptGenRandom(hProvider, (DWORD)iSize, (BYTE*)pData);
        ::CryptReleaseContext(hProvider, 0);
        if (bRet) return (HRESULT)iSize;
    }
#endif
    return E_NOTIMPL;  // OS noise not available. Try some other pseudo random source.
}

HRESULT cRandomOS::GetNoise(void* pData, size_t iSize) {  // override. fill array with random. return # filled.
    //! Get the best source or random noise the system can supply.
    //! Make a random seed from the most random source we can find.
    if (iSize <= 0) return (HRESULT)iSize;
    HRESULT hRes = GetNoiseOS(pData, iSize);
    if (hRes >= (HRESULT)iSize) return hRes;    // all good.
    // else fallback to try the high perf timer.
    cRandomPerf::GetNoisePerf(pData, iSize);
    return CastN(HRESULT, iSize);
}

cRandomDef::RAND_t cRandomOS::get_RandUns() {  // virtual
    RAND_t uVal = 0;
    GetNoise(&uVal, sizeof(uVal));
    return uVal;
}

//*************************************************************

static const cRandomDef::RAND_t k_x1 = CastN(cRandomDef::RAND_t, 0xECC440A96968484A);

cRandomDef::cRandomDef(RAND_t nSeed) : m_nSeed(nSeed) {
    //! like ::srand()
    //! http://stackoverflow.com/questions/4768180/rand-implementation
}

void cRandomDef::InitSeed(const cMemSpan& seed) {  // override
    //! Start a repeatable series of pseudo random numbers
    //! like ::srand()
    m_nSeed = k_x1;  // Clear any under flow.
    cMem::Copy(&m_nSeed, seed, cValT::Min(sizeof(m_nSeed), seed.get_DataSize()));
}

cRandomDef::RAND_t cRandomDef::get_RandUns() {
    m_nSeed = (m_nSeed * 214013L + 2531011L) ^ k_x1;
    return m_nSeed;
}
}  // namespace Gray
