//
//! @file cBlob.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cBits.h"
#include "cHeap.h"
#include "cBlob.h"

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

void cBlob::FreeHeap() noexcept {
    DEBUG_CHECK(isHeap());
    if (cBits::HasMask(static_cast<BYTE>(_MemType), static_cast<BYTE>(MEMTYPE_t::_Secure))) {
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
    SUPER_t::SetSpan(cHeap::AllocPtr(nSize), nSize);
    if (!isValidPtr()) {  // nSize = 0 may be nullptr or not?
        SUPER_t::SetSpanNull();
        ASSERT(0);
        return false;  // FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
    }
    ASSERT(isValid());
    return true;
}

bool cBlob::AllocSpan(const void* pData, size_t nSize) {
    ASSERT(pData == nullptr || !isHeap() || !this->IsInternalPtr(pData));  // NOT from inside myself ! // Check before Alloc
    if (!AllocSize(nSize)) return false;                      // FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
    if (pData != nullptr) { // Init data or not?
        cMem::Copy(get_DataW(), pData, nSize);
    }
    return true;
}

bool cBlob::ReAllocSize(size_t nSize) {
    if (nSize <= 0) {
        SetBlobNull();
        return true;
    }
    if (isNull()) return AllocSize(nSize);
    const size_t nSizePrev = get_DataSize();
    if (!isHeap()) {
        // transition from static to heap.
        const auto pDataPrev = get_DataC();
        if (!AllocSize(nSize)) return false;
        cMem::Copy(get_DataW(), pDataPrev, cValT::Min(nSize, nSizePrev));
    } else if (nSize != nSizePrev) {
        SUPER_t::SetSpan(cHeap::ReAllocPtr(get_DataW(), nSize), nSize);
        if (!isValidPtr()) {
            ASSERT(0);
            return false;  // FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
        }
    }
    ASSERT(isValid());
    ASSERT(isHeap());
    return true;
}
bool cBlob::ReAllocSpan(const void* pData, size_t nSize) {
    // FAIL if pData overlaps this.
    if (pData == get_DataC())  // same pData so do nothing with data.
        pData = nullptr;
    ASSERT(pData == nullptr || !isHeap() || !this->IsInternalPtr(pData));  // NOT overlap myself ! // Check before Alloc
    if (!ReAllocSize(nSize)) return false;                                  // FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
    if (pData != nullptr) {
        cMem::Copy(get_DataW(), pData, nSize);
    }
    return true;
}

bool cBlob::ReAllocLazy(size_t iSizeNew) {
    if (iSizeNew > get_DataSize() && iSizeNew > get_AllocSize()) {
        if (!ReAllocSize(iSizeNew)) return false;
    }
    put_DataSize(iSizeNew);
    return true;
}

size_t cBlob::get_AllocSize() const noexcept {
    DEBUG_CHECK(!isCorrupt());
    if (isHeap()) return cHeap::GetSize(get_DataC());
    return get_DataSize();  // NOT Heap!
}

bool cBlob::SetBlob(const void* pDataSrc, size_t nSize, MEMTYPE_t memType) {
    if (get_DataC() == pDataSrc && get_DataSize() == nSize && _MemType == memType)  // no change!
        return true;
    if (memType >= MEMTYPE_t::_Heap) {  // i want a heap block. allocate and copy. isHeap.
        if (isHeap()) return ReAllocSpan(pDataSrc, nSize);
        return AllocSpan(pDataSrc, nSize);
    }
    if (nSize == 0 || pDataSrc == nullptr) {
        SetBlobNull();
        return true;
    }
    if (isHeap()) FreeHeap();     
    SUPER_t::SetSpanConst(pDataSrc, nSize);
    _MemType = memType;
    return true;
}
bool cBlob::SetBlobCopy(const cBlob& r) {
    return SetBlob(r.get_DataC(), r.get_DataSize(), r._MemType == MEMTYPE_t::_Temp ? MEMTYPE_t::_Heap : r._MemType);
}

cBlob::cBlob(const void* pDataSrc, size_t nSize, MEMTYPE_t memType) : cMemSpan(nullptr, 0), _MemType(MEMTYPE_t::_Null) {
    if (memType >= MEMTYPE_t::_Heap) {
        AllocSpan(pDataSrc, nSize);
    } else {
        SUPER_t::SetSpanConst(pDataSrc, nSize);
        _MemType = memType;
    }
}
}  // namespace Gray
