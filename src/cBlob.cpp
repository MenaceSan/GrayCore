//! @file cBlob.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cBits.h"
#include "cBlob.h"
#include "cHeap.h"

namespace Gray {
bool cBlob::isValidRead() const noexcept {
    if (isNull()) return false;
    if (isHeap()) return cHeap::IsValidHeap(get_DataC());
    return cMem::IsValidApp(get_DataC());
}

bool cBlob::isCorrupt() const noexcept {
    if (isNull()) return false;
    if (isHeap()) return cHeap::IsCorruptHeap(get_DataC());
    return cMem::IsCorruptApp(get_DataC(), get_DataSize());
}
size_t cBlob::get_AllocSize() const noexcept {
    DEBUG_CHECK(!isCorrupt());
    if (isHeap()) return cHeap::GetSize(get_DataC());
    return get_DataSize();  // NOT Heap!
}

void cBlob::FreeHeap() noexcept {
    DEBUG_CHECK(isHeap());
    if (cBits::HasAny(static_cast<BYTE>(_MemType), static_cast<BYTE>(MEMTYPE_t::_Secure))) {
        SetZeros();
    }
    // cHeapAlign ?
    cHeap::FreePtr(get_DataW());
}

void cBlob::SetBlobNull() noexcept {
    if (isNull()) return;
    if (isHeap()) FreeHeap();
    _MemType = MEMTYPE_t::_Null;
    SUPER_t::SetSpanNull();
}

bool cBlob::AllocSize(size_t nSize) {
    if (nSize == 0) {
        SetBlobNull();
        return true;
    }
    if (isHeap()) FreeHeap();

    _MemType = MEMTYPE_t::_Heap;
    SUPER_t::SetSpan2(cHeap::AllocPtr(nSize), nSize);
    if (!isValidPtr()) {  // nSize = 0 may be nullptr or not?
        SUPER_t::SetSpanNull();
        ASSERT(0);
        return false;  // FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
    }
    ASSERT(isValid());
    return true;
}

bool cBlob::ReAllocSize(size_t nSize) {
    if (nSize <= 0) {
        SetBlobNull();
        return true;
    }
    if (isNull()) return AllocSize(nSize);
    const cMemSpan spanPrev = *this;
    if (!isHeap()) {
        // transition from static to heap.
        if (!AllocSize(nSize)) return false;
        this->SetCopySpan(spanPrev);
    } else if (nSize != spanPrev.get_DataSize()) {
        SUPER_t::SetSpan2(cHeap::ReAllocPtr(get_DataW(), nSize), nSize);
        if (!isValidPtr()) {
            ASSERT(0);
            return false;  // FAILED E_OUTOFMEMORY
        }
    }
    ASSERT(isValid());
    ASSERT(isHeap());
    return true;
}
bool cBlob::ReAllocLazy(size_t iSizeNew) {
    if (iSizeNew > get_DataSize() && iSizeNew > get_AllocSize()) {
        if (!ReAllocSize(iSizeNew)) return false;
    }
    put_DataSize(iSizeNew);
    return true;
}

bool cBlob::SetCopyAlloc(const cMemSpan& m) {
    ASSERT(m.isNull() || !isHeap() || !this->IsInternalPtr(m));  // NOT from inside myself ! // Check before Alloc
    if (!AllocSize(m.get_DataSize())) return false;              // FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
    this->SetCopySpan(m);
    return true;
}
bool cBlob::SetCopyReAlloc(const cMemSpan& m) {
    // FAIL if pData overlaps this.
    ASSERT(m.isNull() || !isHeap() || !this->IsInternalPtr(m) || m == get_DataC());  // NOT overlap myself ! // Check before Alloc
    if (!ReAllocSize(m.get_DataSize())) return false;            // FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
    this->SetCopySpan(m);
    return true;
}

bool cBlob::SetBlob(const cMemSpan& r, MEMTYPE_t memType) {
    if (this->IsSameSpan(r) && _MemType == memType)  // no change!
        return true;
    if (memType >= MEMTYPE_t::_Heap) {  // i want a heap block. allocate and copy. isHeap.
        if (isHeap()) return SetCopyReAlloc(r);
        return SetCopyAlloc(r);
    }
    // Not Heap.
    if (r.isEmpty() || r.isNull()) {
        SetBlobNull();
        return true;
    }
    if (isHeap()) FreeHeap();
    SUPER_t::SetSpan(r);
    _MemType = memType;
    return true;
}
bool cBlob::SetBlobCopy(const cBlob& r) {
    return SetBlob(r, r._MemType == MEMTYPE_t::_Temp ? MEMTYPE_t::_Heap : r._MemType);
}

cBlob::cBlob(const cMemSpan& m, MEMTYPE_t memType) : _MemType(MEMTYPE_t::_Null) {
    if (memType >= MEMTYPE_t::_Heap) {
        SetCopyAlloc(m);
    } else {
        SUPER_t::SetSpan(m);
        _MemType = memType;
    }
}
}  // namespace Gray
