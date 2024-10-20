//! @file cAtom.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cAtom.h"
#include "cAtomManager.h"
#include "cCodeProfiler.h"
#include "cFile.h"
#include "cThreadLock.h"

namespace Gray {
cSingleton_IMPL(cAtomManager);

cAtomManager::cAtomManager() : cSingleton<cAtomManager>(this) {}

cAtomRef cAtomManager::FindAtomStr(const ATOMCHAR_t* pszText) const {
    if (StrT::IsNullOrEmpty(pszText)) return cAtomRef();
    const auto guard(_Lock.Lock());
    cAtomRef pDef(_aNames.FindArgForKey(pszText));
    if (!pDef.isValidPtr()) return cAtomRef();
    return cAtomRef(pDef);
}

cAtomRef cAtomManager::FindAtomHashCode(ATOMCODE_t idAtom) const {
    if (idAtom == k_HASHCODE_CLEAR) return cAtomRef();
    const auto guard(_Lock.Lock());
    cAtomRef pDef(_aHashes.FindArgForKey(idAtom));
    if (!pDef.isValidPtr()) return cAtomRef();
    return cAtomRef(pDef);
}

bool cAtomManager::RemoveAtom(DATA_t* pDef) {
    // below kRefsBase
    if (pDef == nullptr) return false;
    const auto guard(_Lock.Lock());
    bool bRetRemove = _aHashes.DeleteArg(pDef);
    ASSERT(bRetRemove);
    bRetRemove = _aNames.DeleteArg(pDef);
    ASSERT(bRetRemove);
#ifdef _DEBUG
    const REFCOUNT_t iRefCount = pDef->get_RefCount();
    ASSERT(iRefCount == 1);
#endif
    return bRetRemove;
}

cAtomRef cAtomManager::CreateAtom(const cHashIterator& index, COMPARE_t iCompareRes, DATA_t* pData) {
    //! Insertion sort. ASSUME _Lock.
    _aNames.InsertAt(index, iCompareRes, pData);
    // const ITERATE_t nHashSize = _aHashes.GetSize();	// previous size.
    const ITERATE_t nHashRet = _aHashes.Add(pData);
    UNREFERENCED_PARAMETER(nHashRet);  // release mode.
    ASSERT(nHashRet >= 0);
// ASSERT(_aHashes.GetSize() > nHashSize);	// Hash collision?!
#if 0 // def _DEBUG
    if (pData->CompareNoCase("Root") == 0) {
        DEBUG_MSG(("Root"));
    }
#endif
    return cAtomRef(pData);
}

cAtomRef cAtomManager::FindorCreateAtom(const cSpan<ATOMCHAR_t>& src) noexcept {
    if (StrT::IsNullOrEmpty<ATOMCHAR_t>(src)) return cAtomRef();
    const auto guard(_Lock.Lock());
    COMPARE_t iCompareRes;
    const cHashIterator index = _aNames.FindINearKey(src, iCompareRes);
    if (iCompareRes == COMPARE_Equal) return cAtomRef(_aNames.GetAtHash(index));  // already here.
    return CreateAtom(index, iCompareRes, DATA_t::CreateStringSpan(src));
}
cAtomRef cAtomManager::FindorCreateAtom(const cStringA& sName) noexcept {
    if (sName.IsEmpty()) return cAtomRef();
    const auto guard(_Lock.Lock());
    COMPARE_t iCompareRes;
    const cHashIterator index = _aNames.FindINearKey(sName, iCompareRes);
    if (iCompareRes == COMPARE_Equal) return cAtomRef(_aNames.GetAtHash(index));  // already here.
    return CreateAtom(index, iCompareRes, const_cast<cStringA&>(sName).get_Head());
}

void cAtomManager::SetAtomStatic(DATA_t* pDef) {
    _aStatics.AddSort(pDef, 2);
}

HRESULT cAtomManager::DebugDumpFile(ITextWriter& o) const {
    const auto guard(_Lock.Lock());

    // Order by name
    FOREACH_HASH_TABLE(_aNames, i) {
        auto pDef = _aNames.GetAtHash(i);
        o.Printf("%s" FILE_EOL, StrArg<char>(pDef->get_CPtr()));
    }

    // Order by hash
    FOREACH_HASH_TABLE(_aHashes, k) {
        auto pDef = _aHashes.GetAtHash(k);
        o.Printf("%x = '%s'" FILE_EOL, pDef->get_HashCode(), StrArg<char>(pDef->get_CPtr()));
    }
    return S_OK;
}

bool cAtomManager::DebugTestPoint() const {
    // Is the atom manager ok ?
    return _aNames.isHashSorted();
}

//*********************************

void cAtomRef::SetEmptyA() {
    //! Free this ref count. Delete if there are no more refs
    CODEPROFILEFUNC();
    if (IsEmpty()) return;  // already empty. never free the empty atom.
    cAtomManager& rAM = cAtomManager::I();

    // Remove the cAtomRef from the tables if last use.
    const REFCOUNT_t iRefCount = get_RefCount();
    if (iRefCount <= cAtomManager::kRefsBase) {
        rAM.RemoveAtom(get_Ptr());  // Remove me from the cAtomManager tables.
    }

    put_Ptr(nullptr);
}

size_t cAtomRef::CountHeapStats(OUT ITERATE_t& iAllocCount) const {
    //! return SUPER_t::CountHeapStats(iAllocCount);
    CODEPROFILEFUNC();
    if (IsEmpty()) return 0;  // already empty
    const REFCOUNT_t iRefCount = get_RefCount();
    ASSERT(iRefCount >= 2);
    return get_Ptr()->GetHeapStatsThis(iAllocCount) / (iRefCount - 1);
}

cAtomRef GRAYCALL cAtomRef::FindAtomStr(const ATOMCHAR_t* pszText) {
    CODEPROFILEFUNC();
    return cAtomManager::I().FindAtomStr(pszText);
}
cAtomRef GRAYCALL cAtomRef::FindAtomHashCode(ATOMCODE_t idAtom) {
    CODEPROFILEFUNC();
    return cAtomManager::I().FindAtomHashCode(idAtom);
}

void cAtomRef::SetAtomStatic() {
    cAtomManager::I().SetAtomStatic(get_Ptr());
}

cAtomRef GRAYCALL cAtomRef::FindorCreateAtom(const cSpan<ATOMCHAR_t>& src) noexcept {
    return cAtomManager::I().FindorCreateAtom(src);
}

cAtomRef GRAYCALL cAtomRef::FindorCreateAtom(const STR_t& sText) noexcept {
    return cAtomManager::I().FindorCreateAtom(sText);
}

#ifdef _DEBUG
HRESULT GRAYCALL cAtomRef::DebugDumpFile(const FILECHAR_t* pszFilePath) {
    CODEPROFILEFUNC();
    cFile file;
    const HRESULT hRes = file.OpenX(pszFilePath, OF_CREATE | OF_WRITE);
    if (FAILED(hRes)) return hRes;
    return cAtomManager::I().DebugDumpFile(file);
}
#endif

StrLen_t GRAYCALL cAtomRef::MakeSymName(cSpan<ATOMCHAR_t> symName, const ATOMCHAR_t* pszExp, bool bAllowDots) {
    if (pszExp == nullptr) return 0;

    ATOMCHAR_t* pszSymOut = symName.get_PtrW();
    ATOMCHAR_t ch = pszExp[0];
    StrLen_t i = 0;
    if (!bAllowDots) {                // JSON allows leading numbers and dots.
        if (!StrChar::IsCSymF(ch)) {  // first char of symbol is special.
            pszSymOut[0] = '\0';      // Insert leading '_' to cover the invalid first char?
            return 0;                 // Can't fix this.
        }
        pszSymOut[0] = ch;  // first is special.
        i++;
    }

    for (; pszExp[i] != '\0'; i++) {
        ch = pszExp[i];
        if (!bAllowDots || ch != '.') {
            if (!StrChar::IsCSym(ch)) break;
        }
        if (i >= k_LEN_MAX_CSYM) {
            pszSymOut[k_LEN_MAX_CSYM - 1] = '\0';
            return 0;
        }

        pszSymOut[i] = ch;
    }

    pszSymOut[i] = '\0';
    return i;
}
}  // namespace Gray
