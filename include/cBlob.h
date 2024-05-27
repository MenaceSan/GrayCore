//! @file cBlob.h
//! wrap a dynamically allocated (un-typed) blob/block of heap memory.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cBlob_H
#define _INC_cBlob_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cSpan.h"

namespace Gray {
/// <summary>
/// How is some memory blob managed? writable? heap free on destruct? size aligned?
/// </summary>
enum class MEMTYPE_t : BYTE {
    _Null = 0,     /// nullptr allocation.
    _StaticConst,  /// Static const. No free, No write. Pointer lives forever.
    _Static,       /// Static allocation of memory. No free but i might write to it. Pointer lives forever.
    _Temp,         /// Externally/unknown allocated. probably Stack based? Cannot preserve pointer.

    /// <summary>
    /// A cHeap allocated blob. Allow write, cHeap::FreePtr() on destruct.
    /// heap allocated size might be more than cMemSpan m_nSize in __linux__ or Lazy allocations.
    /// ASSUME pointer aligned to k_SizeAlignDef = 8 bytes on 32 bit systems, 16 bytes on 64 bit systems.
    /// </summary>
    _Heap,

    // TODO BitMask added to indicate alignment required.
    _A16 = 0x11,   /// 16 byte cHeapAlign. Used only for 32 bit since same as default for 64 bit.
    _A32 = 0x12,   /// 32 byte cHeapAlign.
    _A64 = 0x13,   /// 64 byte cHeapAlign.
    _A128 = 0x14,  /// 128 byte cHeapAlign.

    _Secret = 0x20,  /// Zero on Free
};

/// <summary>
/// A cMemSpan managed according to MEMTYPE_t. Maybe heap, or static, etc.
/// </summary>
class GRAYCORE_LINK cBlob : public cMemSpan {
    typedef cBlob THIS_t;
    typedef cMemSpan SUPER_t;

    MEMTYPE_t _MemType = MEMTYPE_t::_Null;  /// What type of memory is cMemSpan stored in? should we manage this cMemSpan?

    // Don't allow public access to some cMemSpan methods.
    void SetSpanNull() = delete;
    void SetSpanConst(const void* pData, size_t nSize) = delete;
    void SetSpan2(void* pData, size_t nSize) = delete;
    void SetSpan(const SUPER_t& a) = delete;
    void SetSkipBytes(size_t nSize) = delete;

    /// <summary>
    /// Internal function does not clear values!
    /// </summary>
    void FreeHeap() noexcept;

 public:
    static const THIS_t k_EmptyBlob;

    constexpr cBlob() noexcept : SUPER_t(nullptr, 0), _MemType(MEMTYPE_t::_Null) {}
    constexpr cBlob(const cMemSpan& m, bool isStaticWritable) noexcept : SUPER_t(m), _MemType(isStaticWritable ? MEMTYPE_t::_Static : MEMTYPE_t::_StaticConst) {}

    /// <summary>
    /// move constructor.
    /// </summary>
    cBlob(THIS_t&& ref) noexcept : SUPER_t(ref), _MemType(ref._MemType) {
        ref.DetachBlob();
    }
    /// <summary>
    /// copy constructor
    /// </summary>
    cBlob(const THIS_t& ref) : cBlob() {
        SetBlobCopy(ref);
    }
    /// <summary>
    /// Heap Allocate with initial size. uninitialized data.
    /// </summary>
    explicit cBlob(size_t nSize) : cBlob() {
        AllocSize(nSize);
    }
    /// <summary>
    /// Heap Allocate then Copy pDataCopy data into this.
    /// </summary>
    cBlob(const cMemSpan& s, MEMTYPE_t memType);

    ~cBlob() {
        DEBUG_CHECK(isValid());
        if (isHeap()) {
            FreeHeap();
        }
    }

    /// Can this grow?
    inline constexpr bool isHeap() const noexcept {
        return _MemType >= MEMTYPE_t::_Heap;
    }

    /// <summary>
    /// static does not necessarily mean (const) read only. static can be writable.
    /// If the blobs are in truly read only memory. CPU memory protection will just throw its own exception if we try to modify it..
    /// </summary>
    /// <returns></returns>
    inline constexpr bool isReadOnly() const noexcept {
        return _MemType == MEMTYPE_t::_StaticConst;
    }

    /// <summary>
    /// Is this valid to use for read?
    /// Must NOT be nullptr!
    /// Has the memory been corrupted ?
    /// @note this should only ever be used in debug code. Use only in an ASSERT.
    /// </summary>
    bool isValidRead() const noexcept;

    /// <summary>
    /// Is this a corrupt heap pointer? nullptr is OK.
    /// @note this should only ever be used in debug code. Use only in an ASSERT.
    /// </summary>
    bool isCorrupt() const noexcept;

    /// Free memory if needed
    void SetBlobNull() noexcept;

    /// <summary>
    /// Get a writable TYPE pointer but NOT if read-only!
    /// </summary>
    template <typename TYPE = BYTE>
    inline TYPE* GetTPtrW() noexcept {
        DEBUG_CHECK(!isReadOnly());
        return SUPER_t::GetTPtrW<TYPE>();
    }

    /// <summary>
    /// Copy from Src into me/this.
    /// </summary>
    bool SetBlob(const cMemSpan& r, MEMTYPE_t memType);
    bool SetBlobCopy(const cBlob& r);

    /// <summary>
    /// Someone has copied this buffer. Clear without free.
    /// </summary>
    void DetachBlob() noexcept {
        _MemType = MEMTYPE_t::_Null;
        SUPER_t::SetSpanNull();
    }

    /// copy assignment operator. Allocate a new copy.
    THIS_t& operator=(const THIS_t& ref) {
        SetBlobCopy(ref);
        return *this;
    }

    /// move assignment operator
    THIS_t& operator=(THIS_t&& ref) noexcept {
        SUPER_t::SetSpan(ref);
        _MemType = ref._MemType;
        ref.DetachBlob();
        return *this;
    }

    /// <summary>
    /// Special version of get_Size() to measure the true heap allocation size.
    /// @note Not always the size of the allocation request in __linux__ or Lazy.
    /// </summary>
    /// <returns>The actual size of the allocation in bytes. May be greater than I requested? get_Size()</returns>
    size_t get_AllocSize() const noexcept;
    size_t CountHeapStats(OUT ITERATE_t& iAllocCount) const noexcept {
        //! sizeof all children alloc(s). not size of *this
        if (!isValidPtr() || !isHeap()) return 0;
        iAllocCount++;  // count total allocations used.
        return get_AllocSize();
    }

    /// <summary>
    /// Allocate a NEW memory blob of size. assume m_pData points to uninitialized data.
    /// @note cHeap::AllocPtr(0) != nullptr ! maybe ?
    /// Some really old/odd code relies on AllocPtr(0) having/returning a real pointer? not well defined.
    /// @note why not prefer using ReAlloc() ?
    /// </summary>
    bool AllocSize(size_t nSize);

    /// <summary>
    /// If already allocated re-use the current blob if possible. else alloc new.
    /// copy existing data to new blob if move is needed. preserve data.
    /// </summary>
    bool ReAllocSize(size_t nSize);

    /// <summary>
    /// Do not shrink the buffer size. only grow. but record the size i asked for.
    /// A heap blob that is faster to reallocate.
    /// Optimize reallocate to smaller size by leaving the allocation alone. Lazy realloc in the case of shrink.
    /// </summary>
    bool ReAllocLazy(size_t nSizeNew);

    /// <summary>
    /// Allocate then copy something into it.
    /// @note why not prefer using ReAlloc() ?
    /// </summary>
    bool SetCopyAlloc(const cMemSpan& m);

    /// <summary>
    /// realloc then copy stuff into it.
    /// </summary>
    bool SetCopyReAlloc(const cMemSpan& m);
};
}  // namespace Gray
#endif
