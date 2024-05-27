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
    if (isHeap()) return cHeap::IsValidHeap(GetTPtrC());
    return cMem::IsValidApp(GetTPtrC());
}

bool cBlob::isCorrupt() const noexcept {
    if (isNull()) return false;
    if (isHeap()) return cHeap::IsCorruptHeap(GetTPtrC());
    return cMem::IsCorruptApp(GetTPtrC(), get_SizeBytes());
}
size_t cBlob::get_AllocSize() const noexcept {
    DEBUG_CHECK(!isCorrupt());
    if (isHeap()) return cHeap::GetSize(GetTPtrC());
    return get_SizeBytes();  // NOT Heap!
}

void cBlob::FreeHeap() noexcept {
    DEBUG_CHECK(isHeap());
    if (cBits::HasAny(static_cast<BYTE>(_MemType), static_cast<BYTE>(MEMTYPE_t::_Secret))) {
        SetZeros();
    }
    // cHeapAlign ?
    cHeap::FreePtr(GetTPtrW());
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
    } else if (nSize != spanPrev.get_SizeBytes()) {
        SUPER_t::SetSpan2(cHeap::ReAllocPtr(GetTPtrW(), nSize), nSize);
        if (!isValidPtr()) {
            ASSERT(0);
            return false;  // FAILED E_OUTOFMEMORY
        }
    }
    ASSERT(isValid());
    ASSERT(isHeap());
    return true;
}
bool cBlob::ReAllocLazy(size_t nSizeNew) {
    if (nSizeNew > get_SizeBytes() && nSizeNew > get_AllocSize()) {
        if (!ReAllocSize(nSizeNew)) return false;
    }
    put_SizeBytes(nSizeNew);
    return true;
}

bool cBlob::SetCopyAlloc(const cMemSpan& m) {
    ASSERT(m.isNull() || !isHeap() || !this->IsInternalPtr(m));  // NOT from inside myself ! // Check before Alloc
    if (!AllocSize(m.get_SizeBytes())) return false;              // FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
    this->SetCopySpan(m);
    return true;
}
bool cBlob::SetCopyReAlloc(const cMemSpan& m) {
    // FAIL if pData overlaps this.
    ASSERT(m.isNull() || !isHeap() || !this->IsInternalPtr(m) || m == GetTPtrC());  // NOT overlap myself ! // Check before Alloc
    if (!ReAllocSize(m.get_SizeBytes())) return false;            // FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
    this->SetCopySpan(m);
    return true;
}

bool cBlob::SetBlob(const cMemSpan& r, MEMTYPE_t memType) {
    if (this->IsSameSpam(r) && _MemType == memType) return true;  // no change!        
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
