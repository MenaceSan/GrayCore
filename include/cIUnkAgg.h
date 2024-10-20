//! @file cIUnkAgg.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cIUnkAgg_H
#define _INC_cIUnkAgg_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cRefPtr.h"

namespace Gray {
/// <summary>
/// Abstract Base for an aggregated interface class.
/// This is the base for a class that is a static or member. has no ref count of its own.
/// This makes interfaces look like multiple inheritance when it really is not. (why not just use multiple inheritance ?)
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE = ::IUnknown>
class DECLSPEC_NOVTABLE cIUnkAggBase {
 protected:
    TYPE* _pIAggOuter;  /// the outer object interface.

 public:
    cIUnkAggBase(TYPE* pIAggOuter) : _pIAggOuter(pIAggOuter) {
        ASSERT_NN(pIAggOuter);
    }
    virtual ~cIUnkAggBase() {}
    // Support IUnknown functions.
    HRESULT QueryInterface(const IID& riid, void** ppv) {
        if (ppv == nullptr || _pIAggOuter == nullptr) {
            *ppv = nullptr;
            return E_POINTER;
        }
        return _pIAggOuter->QueryInterface(riid, ppv);
    }
    ULONG AddRef() {
        ASSERT_NN(_pIAggOuter);
        return _pIAggOuter->AddRef();
    }
    ULONG Release() {
        ASSERT_NN(_pIAggOuter);
        return _pIAggOuter->Release();
    }
};

/// <summary>
/// Abstract Base class for an interface that allows aggregation from some IUnknown parent. (or not)
/// </summary>
class DECLSPEC_NOVTABLE cIUnkAgg : public cRefBase, public cIUnkAggBase<IUnknown> {
 public:
    cIUnkAgg(::IUnknown* pIAggOuter) : cIUnkAggBase((pIAggOuter == nullptr) ? this : pIAggOuter) {}
    // Support IUknown functions.
    STDMETHODIMP_(ULONG) AddRef() override {
        if (_pIAggOuter == this) {
            IncRefCount();
            return CastN(ULONG, get_RefCount());
        }
        ASSERT_NN(_pIAggOuter);
        return _pIAggOuter->AddRef();
    }
    STDMETHODIMP_(ULONG) Release() override {
        if (_pIAggOuter == this) {
            DecRefCount();
            return CastN(ULONG, get_RefCount());
        }
        ASSERT_NN(_pIAggOuter);
        return _pIAggOuter->Release();
    }
    STDMETHODIMP QueryInterface(const IID& riid, void** ppv) override {
        if (ppv == nullptr) return E_POINTER;
        if (_pIAggOuter == nullptr) {
            *ppv = nullptr;
            return E_POINTER;
        }
        if (_pIAggOuter == this) {
            if (riid == __uuidof(IUnknown)) {
                *ppv = this;
                IncRefCount();
                return S_OK;
            }
            return E_NOINTERFACE;
        }
        return _pIAggOuter->QueryInterface(riid, ppv);
    }
};
}  // namespace Gray
#endif  // _INC_cIUnkAgg_H
