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
 
cAtomManager::cAtomManager() : cSingleton<cAtomManager>(this, typeid(cAtomManager)) {}

cAtomRef cAtomManager::FindAtomStr(const ATOMCHAR_t* pszText) const {
    if (StrT::IsNullOrEmpty(pszText)) return cAtomRef();
    const auto guard(m_Lock.Lock());
    cAtomRef pDef(m_aName.FindArgForKey(pszText));
    if (!pDef.isValidPtr()) return cAtomRef();
    return cAtomRef(pDef);
}

cAtomRef cAtomManager::FindAtomHashCode(ATOMCODE_t idAtom) const {
    if (idAtom == k_HASHCODE_CLEAR) return cAtomRef();
    const auto guard(m_Lock.Lock());
    cAtomRef pDef(m_aHash.FindArgForKey(idAtom));
    if (!pDef.isValidPtr()) return cAtomRef();
    return cAtomRef(pDef);
}

bool cAtomManager::RemoveAtom(DATA_t* pDef) {
    // below kRefsBase
    if (pDef == nullptr) return false;
    const auto guard(m_Lock.Lock());
    bool bRetRemove = m_aHash.DeleteArg(pDef);
    ASSERT(bRetRemove);
    bRetRemove = m_aName.DeleteArg(pDef);
    ASSERT(bRetRemove);
#ifdef _DEBUG
    const REFCOUNT_t iRefCount = pDef->get_RefCount();
    ASSERT(iRefCount == 1);
#endif
    return bRetRemove;
}

cAtomRef cAtomManager::CreateAtom(const cHashIterator& index, COMPARE_t iCompareRes, DATA_t* pData) {
    //! Insertion sort.
    m_aName.InsertAt(index, iCompareRes, pData);
    // const ITERATE_t nHashSize = m_aHash.GetSize();	// previous size.
    const ITERATE_t nHashRet = m_aHash.Add(pData);
    ASSERT(nHashRet >= 0);
    // ASSERT(m_aHash.GetSize() > nHashSize);	// Hash collision?!
    UNREFERENCED_PARAMETER(nHashRet);  // release mode.
    return cAtomRef(pData);
}

cAtomRef cAtomManager::FindorCreateAtom(const cSpan<ATOMCHAR_t>& src) noexcept {
    if (StrT::IsNullOrEmpty<ATOMCHAR_t>(src)) return cAtomRef();
    const auto guard(m_Lock.Lock());
    COMPARE_t iCompareRes;
    const cHashIterator index = m_aName.FindINearKey(src, iCompareRes);
    if (iCompareRes == COMPARE_Equal) return cAtomRef(m_aName.GetAtHash(index));  // already here.
    return CreateAtom(index, iCompareRes, DATA_t::CreateStringSpan(src));
}
cAtomRef cAtomManager::FindorCreateAtom(const cStringA& sName) noexcept {
    if (sName.IsEmpty()) return cAtomRef();
    const auto guard(m_Lock.Lock());
    COMPARE_t iCompareRes;
    const cHashIterator index = m_aName.FindINearKey(sName, iCompareRes);
    if (iCompareRes == COMPARE_Equal) return cAtomRef(m_aName.GetAtHash(index));  // already here.
    return CreateAtom(index, iCompareRes, const_cast<cStringA&>(sName).get_Head());
}

void cAtomManager::SetAtomStatic(DATA_t* pDef) {
    m_aStatic.AddSort(pDef, 2);
}

HRESULT cAtomManager::DebugDumpFile(ITextWriter& o) const {
    const auto guard(m_Lock.Lock());

    // Order by name
    FOREACH_HASH_TABLE(m_aName, i) {
        auto pDef = m_aName.GetAtHash(i);
        o.Printf("%s" FILE_EOL, StrArg<char>(pDef->get_CPtr()));
    }

    // Order by hash
    FOREACH_HASH_TABLE(m_aHash, k) {
        auto pDef = m_aHash.GetAtHash(k);
        o.Printf("%x = '%s'" FILE_EOL, pDef->get_HashCode(), StrArg<char>(pDef->get_CPtr()));
    }
    return S_OK;
}

bool cAtomManager::DebugTest() const {
    // Is the atom manager ok ?
    return m_aName.isHashSorted();
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

StrLen_t GRAYCALL cAtomRef::MakeSymName(OUT ATOMCHAR_t* pszTagRet, const ATOMCHAR_t* pszExp, bool bAllowDots) {
    if (pszExp == nullptr) return 0;
    ASSERT_NN(pszTagRet);

    ATOMCHAR_t ch = pszExp[0];
    StrLen_t i = 0;
    if (!bAllowDots) {                // JSON allows leading numbers and dots.
        if (!StrChar::IsCSymF(ch)) {  // first char of symbol is special.
            pszTagRet[0] = '\0';    // Insert leading '_' to cover the invalid first char?
            return 0;  // Can't fix this.
        }
        pszTagRet[0] = ch;  // first is special.
        i++;
    }

    for (; pszExp[i] != '\0'; i++) {
        ch = pszExp[i];
        if (!bAllowDots || ch != '.') {
            if (!StrChar::IsCSym(ch)) break;
        }
        if (i >= k_LEN_MAX_CSYM) {
            pszTagRet[k_LEN_MAX_CSYM - 1] = '\0';
            return 0;
        }

        pszTagRet[i] = ch;
    }

    pszTagRet[i] = '\0';
    return i;
}
}  // namespace Gray
