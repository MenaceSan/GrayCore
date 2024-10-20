//! @file cHookJump.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cHookJump.h"
#include "cLogMgr.h"
#include "cTypeInfo.h"

#if USE_INTEL

#ifdef _WIN32
#include "cMemPage.h"
#endif

namespace Gray {

bool cHookJump::isChainable() const noexcept {
    // The k_I_JUMP i inserted is just on top of another k_I_JUMP? Normal fixup jump table.
    // I don't need to lock and swap to call the old code. I can just chain jump to it
    const BYTE b0 = _OldCode[0];

    // Look for forms of chainable jmp commands. e.g. 48 ff 25
    if (b0 == k_I_JUMP3 && _OldCode[1] == k_I_JUMP2 && _OldCode[2] == 0x25) {
        return true;
    }
    if (b0 == k_I_JUMP2 && _OldCode[1] == 0x25) {
        return true;  // TODO?
    }

    return b0 == k_I_JUMP;  // the old code was just a jump as well.
}

FUNCPTR_t cHookJump::GetChainFuncInt() const {
    // Get an chain callable function from this JMP. NOT relative address.
    // ASSUME isChainable()

    int lRelAddr = 0;  // not int_ptr. always 32 bit.
    STATIC_ASSERT(sizeof(lRelAddr) == k_LEN_JO, lRelAddr);
    const BYTE b0 = _OldCode[0];

    STATIC_ASSERT(sizeof(_pFuncOrig) == _SIZEOF_PTR, "_pFuncOrig");

    // for k_I_JUMP
    if (b0 == k_I_JUMP) {
        cMem::Copy(&lRelAddr, _OldCode + k_LEN_J, sizeof(lRelAddr));
        return CastFPtrTo<FUNCPTR_t>(CastFPtrTo<UINT_PTR>(_pFuncOrig) + lRelAddr + k_LEN_J + k_LEN_JO);
    }

    if (b0 == k_I_JUMP2 && _OldCode[1] == 0x25) {
        cMem::Copy(&lRelAddr, _OldCode + 2, sizeof(lRelAddr));
        return *(CastFPtrTo<FUNCPTR_t*>(CastFPtrTo<UINT_PTR>(_pFuncOrig) + lRelAddr + 2 + k_LEN_JO));
    }

    if (b0 == k_I_JUMP3 && _OldCode[1] == k_I_JUMP2 && _OldCode[2] == 0x25) {
        cMem::Copy(&lRelAddr, _OldCode + 3, sizeof(lRelAddr));
        return *(CastFPtrTo<FUNCPTR_t*>(CastFPtrTo<UINT_PTR>(_pFuncOrig) + lRelAddr + 3 + k_LEN_JO));
    }

    return nullptr;
}

FUNCPTR_t cHookJump::GetChainFunc() const {
    // Get a callable function from this.
    FUNCPTR_t p = GetChainFuncInt();
    if (p == nullptr) return _pFuncOrig;  // assume it is swapped back in.
    return p;
}

HRESULT cHookJump::SetProtectPages(bool isProtected) {
    //! Set/Remove code protection. so i can read/write to code space.
    //! ASSUME locked _Lock.
#ifdef _WIN32
    return cMemPageMgr::I().ProtectPages(cMemSpan(_pFuncOrig, k_LEN_A), isProtected);
#else
    return S_OK;
#endif
}

HRESULT cHookJump::InstallHook(FUNCPTR_t pFuncOrig, FUNCPTR_t pFuncNew, bool bSkipChainable) {
    //! Install my hook jump.
    //! @note X86 ONLY!! 32 or 64 bit.
    //! bSkipChainable = if chain-able code exists, we should skip over it. because we assume other callers might do this too.
    //! @todo i could insert a value in an address table if the jump uses that format ? jmp [XXX]

    const auto guard(_Lock.Lock());
    if (pFuncOrig == nullptr || pFuncNew == nullptr) {
        ASSERT(pFuncNew != nullptr);
        DEBUG_ERR(("InstallHook: nullptr."));
        return E_POINTER;
    }
    if (isHookInstalled()) {
        ASSERT(_pFuncOrig != nullptr);
        DEBUG_MSG(("InstallHook: already has JMP-implant."));
        return S_FALSE;
    }

    // DEBUG_TRACE(("InstallHook: pFuncOrig = %08x, pFuncNew = %08x", CastFPtrTo<UINT_PTR>(pFuncOrig), CastFPtrTo<UINT_PTR>(pFuncNew) ));
    _pFuncOrig = pFuncOrig;
    SetProtectPages(false);
    cMem::Copy(_OldCode, (void*)pFuncOrig, sizeof(_OldCode));  // save old code.

    if (bSkipChainable && isChainable()) {
        // just skip chain-able code until we reach non chain-able code. assume its not a loop.
        SetProtectPages(true);
        return cHookJump::InstallHook(GetChainFuncInt(), pFuncNew, true);
    }

    const INT_PTR lRelPtr = (CastFPtrTo<UINT_PTR>(pFuncNew) - CastFPtrTo<UINT_PTR>(pFuncOrig)) - sizeof(_Jump);
#ifdef USE_64BIT
    // Possible problem with 64 bit system. // assume this does not overflow 32 bits !!!NT_PTR
    if (lRelPtr <= cTypeLimit<INT32>::Min() || lRelPtr >= cTypeLimit<INT32>::Max()) {
        // The normal way to make 64 bit jumps is to put value in memory and then do like: jmp qword ptr [7FFF77CF3428h]   (6 bytes =? ff 25 f2 2c 06 00 )
        SetProtectPages(true);
        DEBUG_ERR(("InstallHook: 64 bit overflow."));
        return HRESULT_WIN32_C(ERROR_INVALID_HOOK_HANDLE);
    }
#endif
    const int lRelAddr = (int)lRelPtr;  // 32 bit.
    STATIC_ASSERT(sizeof(lRelAddr) == k_LEN_JO, lRelAddr);

    // DEBUG_TRACE(("InstallHook JMP %08x", lRelAddr));
    // create unconditional JMP to relative address is 5 bytes. X86/64 ONLY!!
    _Jump[0] = k_I_JUMP;
    cMem::Copy(_Jump + k_LEN_J, &lRelAddr, k_LEN_JO);

    if (cMem::IsEqual(_Jump, _OldCode, sizeof(_Jump))) {
        // We already injected this with some other cHookJump instance! This is bad. We are fighting ourselfs with duplicated code !! why?
        // We must unwind in proper order or it will fail.
        _Jump[0] = k_I_NULL;
        SetProtectPages(true);
        DEBUG_ERR(("InstallHook: duplicated hook."));
        return E_FAIL;
    }

    cMem::Copy((void*)pFuncOrig, _Jump, sizeof(_Jump));  // inject jump. we are now armed!

    // DEBUG_MSG(("InstallHook: JMP-hook planted."));
    return S_OK;
}

void cHookJump::RemoveHook() {
    const auto guard(_Lock.Lock());
    if (!isHookInstalled())  // was never set?
        return;
    ASSERT(_pFuncOrig != nullptr);
    GRAY_TRY {
        cMem::Copy((void*)_pFuncOrig, _OldCode, sizeof(_Jump));  // SwapOld(pFuncOrig)
        _Jump[0] = k_I_NULL;                                       // destroy my jump. (must reconstruct it)
        SetProtectPages(true);
    }
    GRAY_TRY_CATCHALL {
        // UNREFERENCED_PARAMETER(ex);
        DEBUG_ERR(("cHookJump::RemoveHook FAIL"));
    }
    GRAY_TRY_END;
}
}  // namespace Gray
#endif
