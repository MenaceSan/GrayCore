//! @file cHookJump.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cHookJump_H
#define _INC_cHookJump_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cMem.h"
#include "cThreadLock.h"
#include "FuncPtr.h"  // FUNCPTR_t

#if USE_INTEL
namespace Gray {
/// <summary>
/// Create/Define a relative jump to hook/replace an existing API call. Jump to new code is injected into old function start.
/// @note This is NOT for use in hooking an interface or a vtable. those don't require a jump instruction.
/// @note This ASSUMEs Intel x86 type CPU. 32 or 64 bit instructions. The CPU type must be defined as _M_IX86 or _M_X64
/// https://www.felixcloutier.com/x86/jmp
/// </summary>
class GRAYCORE_LINK cHookJump {
 public:
    static const BYTE k_I_NULL = 0x00;  /// Not a valid instruction.

    static const BYTE k_I_JUMP = 0xe9;                 /// X86 32 bit relative jump instruction (same on 64 bit system). NOTE: "48 ff 25" can act the same in 64 bit code. 3 byte jump prefix. or "ff 25" for 32 bit code.
    static const size_t k_LEN_J = 1;                   /// size_t of the jump instruction. 0xe9 = k_JUMP_I
    static const size_t k_LEN_JO = 4;                  /// size_t of the relative 32 bit jump offset. NOT same as sizeof(FUNCPTR_t) or INT_PTR
    static const size_t k_LEN_A = sizeof(size_t) * 2;  /// size_t k_LEN_D page aligned to 8 or 16 (for 32 bit code or 64 bit code)

    friend class cHookSwapLock;

    static const BYTE k_I_JUMP2 = 0xff;  // 0x25 + 4 bytes offset.
    static const BYTE k_I_JUMP3 = 0x48;  // 0xff 0x25

    friend class cHookLock;

 protected:
    FUNCPTR_t _pFuncOrig;            /// Pointer to the original/old function. The one i will replace. Inject code here.
    BYTE _OldCode[k_LEN_A];          /// What was at _pFuncOrig previously. Take more than i actually need to account for isChainable() tests.
    BYTE _Jump[k_LEN_J + k_LEN_JO];  /// What do i want to replace _pFuncOrig with. k_I_JUMP to pFuncNew
    mutable cThreadLockableX _Lock;  /// prevent multiple threads from using this at the same time.

 protected:
    bool SwapOld() noexcept {
        //! put back saved code fragment. temporary to call previous version of the function.
        //! ASSUME use of cHookLock _Lock, and SetProtectPages
        if (!isHookValid()) return false;
        cMem::Copy((void*)_pFuncOrig, _OldCode, sizeof(_Jump));
        return true;
    }
    void SwapReset() noexcept {
        //! put back original JMP instruction again
        //! ASSUME use of cHookLock _Lock, and SetProtectPages
        if (!isHookInstalled() || _pFuncOrig == nullptr)  // hook has since been destroyed!
            return;
        cMem::Copy((void*)_pFuncOrig, _Jump, sizeof(_Jump));
    }

    HRESULT SetProtectPages(bool isProtected);
    FUNCPTR_t GetChainFuncInt() const;

 public:
    cHookJump() noexcept : _pFuncOrig(nullptr) {
        _OldCode[0] = k_I_NULL;
        _Jump[0] = k_I_NULL;
        static_assert(k_LEN_A == 8 || k_LEN_A == 16, "k_LEN_A");
    }
    ~cHookJump() {
        RemoveHook();
    }

    bool isHookInstalled() const noexcept {
        return _Jump[0] == k_I_JUMP;
    }
    bool isHookValid() const noexcept {
        //! @note sometimes DLLs' can reload themselves and destroy our hook behind our backs.
        if (!isHookInstalled() || _pFuncOrig == nullptr) return false;
        // ASSUME SetProtectPages()
        if (!cMem::IsEqual((const void*)_pFuncOrig, _Jump, sizeof(_Jump))) return false;  // NOT set !!
        return true;
    }

    bool isChainable() const noexcept;
    FUNCPTR_t GetChainFunc() const;

    HRESULT InstallHook(FUNCPTR_t pFuncOrig, FUNCPTR_t pFuncNew, bool bSkipChainable = false);
    void RemoveHook();
};

template <class TYPE = FUNCPTR_t>
struct cHookJumpT : public cHookJump {
    HRESULT InstallHook(TYPE pFuncOrig, TYPE pFuncNew, bool bSkipChainable = false) {
        return cHookJump::InstallHook((FUNCPTR_t)pFuncOrig, (FUNCPTR_t)pFuncNew, bSkipChainable);
    }
    TYPE GetChainFunc() const {
        return reinterpret_cast<TYPE>(cHookJump::GetChainFunc());
    }
};

/// <summary>
/// Stack based temporary lock for cHookJump. swap original call back so it may be used inside hook.
/// </summary>
class GRAYCORE_LINK cHookLock : public cLockerT<cThreadLockableX> {
    typedef cLockerT<cThreadLockableX> SUPER_t;

    cHookJump& _rJump;  /// The code we are locking for use.
    bool _IsSwapOld;     /// has Old swapped back in. Must be locked. NOT isChainable

 public:
    cHookLock(cHookJump& rJump, bool swap = true)
        : SUPER_t(rJump._Lock.Lock()),  // MUST lock while we do this. single thread.
          _rJump(rJump) {
        _IsSwapOld = swap ? _rJump.SwapOld() : false;
    }
    ~cHookLock() noexcept {
        if (_IsSwapOld) {  // did i use the swap?
            _rJump.SwapReset();
        }
    }
};

/// <summary>
/// Stack based temporary lock for cHookJump. Will chain if possible (isChainable()) else swap original call back so it may be used inside hook.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
struct cHookChain : public cHookLock {
    TYPE _pFuncChain;  /// chained version of _pFuncOrig. or fallback to _pFuncOrig. like: FARPROC/FUNCPTR_t

    cHookChain(cHookJump& rJump) : cHookLock(rJump, !rJump.isChainable()) {
        _pFuncChain = reinterpret_cast<TYPE>(rJump.GetChainFunc());
    }
};
}  // namespace Gray
#endif
#endif  // _INC_cHookJump_H
