//! @file cWinHeap.inl
//! macro template for cWinLocalHandle, cWinLocalV, cWinLocalT<> etc
//! @note requires proper #define set up. DON'T include this directly!
//! "#define WINHEAPN(), WINHEAPM(), WINHEAPH, WINHEAPF()"

namespace Gray {
/// <summary>
/// implementation of cWinLocalHandle or cWinGlobalHandle
/// Wrap the HANDLE_t heap/memory object for lock/unlock of an instance. (HLOCAL or HGLOBAL)
/// manage lock and unlock. yes i know lock/unlock doesn't do anything in _WIN32
/// Does NOT free on destruct. just unlock.
/// _pData = Locked pointer. GlobalHandle(_pData)==_hData
/// </summary>
class WINHEAPN(Handle) : public cMemSpan {
    typedef cMemSpan SUPER_t;

 public:
    typedef WINHEAPH HANDLE_t;

 protected:
    HANDLE_t _hData;

 public:
    WINHEAPN(Handle)
    (HANDLE_t hData = cOSHandle::kNULL, void* pData = nullptr, size_t nSize = 0) noexcept
        : _hData(hData),
          SUPER_t(PtrCast<BYTE>(pData), nSize)  // size may not be known? Already Locked?
    {
        //! Attach existing handle to this class.
    }
    ~WINHEAPN(Handle)() {
        Unlock();
    }
    bool isAlloc() const noexcept {
        return _hData != cOSHandle::kNULL;
    }
    HANDLE_t get_Handle() const noexcept {
        return _hData;
    }
    void UpdateHandle(void* p) noexcept {
        //! If i have the _pData, make sure the _hData matches.
        if (GetTPtrC() == p) return;
        Free();
        SetSpan2(p, get_SizeBytes());
        if (isValidPtr()) {
            _hData = WINHEAPF(Handle)(p);
        }
    }

    /// <summary>
    /// Attach existing handle (and/or pointer) to this class. put_Handle()
    /// </summary>
    void AttachHandle(HANDLE_t hData, size_t nSize, void* pData = nullptr) noexcept {
        // ASSERT( _hData == cOSHandle::kNULL );
        _hData = hData;
        SetSpan2(pData, nSize);
    }

    /// <summary>
    /// Get the allocated size. not same as requested size ?
    /// </summary>
    /// <returns></returns>
    SIZE_T GetSize() const noexcept {
        return WINHEAPF(Size)(_hData);
    }
    UINT GetFlags() const noexcept {
        return WINHEAPF(Flags)(_hData);
    }
    LPVOID Lock() noexcept {
        //! this actually does nothing on _WIN32 systems? only used for WIN16
        if (_hData == cOSHandle::kNULL) return nullptr;
        if (!isValidPtr()) {
#ifdef UNDER_CE
            SetSpan((void*)(_hData), get_SizeBytes());
#else
            SetSpan2(WINHEAPF(Lock)(_hData), get_SizeBytes());
#endif
#ifdef _DEBUG
            if (!isValidPtr()) {
                const HRESULT hRes = HResult::GetLastDef();
                DEBUG_ERR(("Heap Lock ERR='%s' for h=0%x", LOGERR(hRes), _hData));
            }
#endif
        }
        return GetTPtrW();
    }
    void Unlock() {
        if (isValidPtr()) {
            ASSERT(_hData != cOSHandle::kNULL);
#ifndef UNDER_CE
            WINHEAPF(Unlock)(_hData);
#endif
            SetSpanConst(nullptr, get_SizeBytes());
        }
    }
    /// <summary>
    /// Allocate a non locked handle.
    /// We may want an unlocked handle for some reason.
    /// </summary>
    /// <param name="dwSize"></param>
    /// <param name="dwFlags">GMEM_MOVEABLE | GMEM_ZEROINIT = WINHEAPM(MOVEABLE)|WINHEAPM(ZEROINIT)</param>
    /// <returns></returns>
    HANDLE_t AllocHandle(size_t dwSize, DWORD dwFlags = WINHEAPM(MOVEABLE)) {
        Unlock();
        if (_hData != cOSHandle::kNULL)
            _hData = WINHEAPF(ReAlloc)(_hData, (SIZE_T)dwSize, dwFlags);
        else
            _hData = WINHEAPF(Alloc)(dwFlags, (SIZE_T)dwSize);
        put_SizeBytes(dwSize);
        return _hData;
    }
    void* ReAlloc(size_t dwSize, DWORD dwFlags = WINHEAPM(FIXED)) {
        Unlock();
        _hData = WINHEAPF(ReAlloc)(_hData, (SIZE_T)dwSize, dwFlags);
        put_SizeBytes(dwSize);
        return Lock();
    }
    /// <summary>
    /// Allocate and lock the handle.
    /// </summary>
    /// <param name="dwSize"></param>
    /// <param name="dwFlags">GMEM_MOVEABLE | GMEM_ZEROINIT</param>
    /// <returns></returns>
    void* AllocPtr2(size_t dwSize, DWORD dwFlags = WINHEAPM(FIXED)) {
        AllocHandle(dwSize, dwFlags);
        return Lock();
    }
    void* AllocSpan(const cMemSpan& src, DWORD dwFlags = WINHEAPM(FIXED)) {
        void* pDst = AllocPtr2(src.get_SizeBytes(), dwFlags);
        if (src.isNull()) {
            src.CopyTo(pDst);
        }
        return pDst;
    }
    void Free() {
        /// Unlock and free. NOT done automatically on destruct of this class.
        if (_hData != cOSHandle::kNULL) {
            Unlock();  // only unlock if needed.
            FreeHandle();
        }
    }
    HANDLE_t DetachHandle() noexcept {
        // Assume unlocked?
        const HANDLE_t hTmp = _hData;
        _hData = cOSHandle::kNULL;
        SetSpanNull();
        return hTmp;
    }

 protected:
    HANDLE_t FreeHandleLast() {
        return WINHEAPF(Free)(_hData);
    }
    void FreeHandle() {
        HANDLE_t hFail = FreeHandleLast();
#ifdef _DEBUG
        if (hFail) {
            const HRESULT hRes = HResult::GetLastDef();
            DEBUG_ERR(("GlobalFree ERR='%s'", LOGERR(hRes)));
        }
#endif
        _hData = cOSHandle::kNULL;
    }
};

/// <summary>
/// implementation of cWinLocalV or cWinGlobalV. Void pointer.
/// Wrap the HANDLE_t heap/memory allocation object.
/// Similar to MFC CGlobalHeap or CWin32Heap
/// Similar to cBlob
/// Free on destruct
/// </summary>
class WINHEAPN(V) : public WINHEAPN(Handle) {
    typedef WINHEAPN(Handle) SUPER_t;

 public:
    WINHEAPN(V)(HANDLE_t hData = cOSHandle::kNULL) : SUPER_t(hData) {}
    ~WINHEAPN(V)() {
        if (_hData != cOSHandle::kNULL) {
            Unlock();
            FreeHandleLast();
        }
    }
    void AttachHandle(HANDLE_t hData, size_t nSize, void* pData = nullptr) {
        if (_hData != cOSHandle::kNULL) {
            if (hData == _hData) return;
            Unlock();
            FreeHandleLast();
        }
        SUPER_t::AttachHandle(hData, nSize, pData);
    }
    void AttachPtr(void* pData) {
        AttachHandle(WINHEAPF(Handle)(pData), 0, pData);  // unknown size?
    }
    HANDLE_t* get_PPtrHandle() {
        //! @note Make sure you call Lock() after this.
        Free();
        return &_hData;
    }
    static LPVOID GRAYCALL AllocPtrX(size_t nSize, DWORD nFlags = 0) {
        //! like _WIN32 GlobalAllocPtr( UINT, SIZE_T ) or GlobalAlloc
        //! nFlags = GMEM_MOVEABLE | GMEM_ZEROINIT
        return WINHEAPF(Lock)(WINHEAPF(Alloc)(nFlags, nSize));
    }
    static void GRAYCALL FreePtr(void* pData) {
        //! like _WIN32 GlobalFreePtr()
        //! @note Yes i know Unlock does nothing in modern OS
        if (pData == nullptr) return;
        HANDLE_t hData = WINHEAPF(Handle)(pData);
        WINHEAPF(Unlock)(hData);
        HANDLE_t hFail = WINHEAPF(Free)(hData);
        UNREFERENCED_PARAMETER(hFail);
    }
};

/// <summary>
/// implementation of cWinGlobalT or cWinLocalT
/// A type cast windows heap pointer/handle pair. like cMemSpanT
/// </summary>
/// <typeparam name="_TYPE"></typeparam>
template <class _TYPE>
struct WINHEAPN(T) : public WINHEAPN(V) {
    typedef WINHEAPN(V) SUPER_t;
    WINHEAPN(T)(HANDLE_t hData = cOSHandle::kNULL) : SUPER_t(hData) {}

    _TYPE* operator->() noexcept {
        return GetTPtrW<_TYPE>();
    }
    static _TYPE* GRAYCALL AllocPtrX(size_t nSize, DWORD nFlags = 0) {
        return (_TYPE*)SUPER_t::AllocPtrX(nSize, nFlags);
    }
};
}  // namespace Gray
