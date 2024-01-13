//
//! @file cWinHeap.inl
//! macro template for cWinLocalHandle, cWinLocalV, cWinLocalT<> etc
//! @note requires proper #define set up. DON'T include this directly!
//! "#define WINHEAPN(), WINHEAPM(), WINHEAPH, WINHEAPF()"
//! 
namespace Gray {
/// <summary>
/// implementation of cWinLocalHandle or cWinGlobalHandle
/// Wrap the HANDLE_t heap/memory object for lock/unlock of an instance. (HLOCAL or HGLOBAL)
/// manage lock and unlock. yes i know lock/unlock doesn't do anything in _WIN32
/// Does NOT free on destruct. just unlock.
/// m_pData = Locked pointer. GlobalHandle(m_pData)==m_hData
/// </summary>
class WINHEAPN(Handle) : public cMemSpan {
    typedef cMemSpan SUPER_t;

 public:
    typedef WINHEAPH HANDLE_t;

 protected:
    HANDLE_t m_hData;

 public:
    WINHEAPN(Handle)
    (HANDLE_t hData = HANDLE_NULL, void* pData = nullptr, size_t nSize = 0) noexcept
        : m_hData(hData),
          SUPER_t(PtrCast<BYTE>(pData), nSize)  // size may not be known? Already Locked?
    {
        //! Attach existing handle to this class.
    }
    ~WINHEAPN(Handle)() {
        Unlock();
    }
    bool isAlloc() const noexcept {
        return m_hData != HANDLE_NULL;
    }
    HANDLE_t get_Handle() const noexcept {
        return m_hData;
    }
    void UpdateHandle(void* p) noexcept {
        //! If i have the m_pData, make sure the m_hData matches.
        if (get_DataC() == p) return;
        Free();
        SetSpan(p, get_DataSize());
        if (isValidPtr()) {
            m_hData = WINHEAPF(Handle)(p);
        }
    }

    /// <summary>
    /// Attach existing handle (and/or pointer) to this class. put_Handle()
    /// </summary>
    void AttachHandle(HANDLE_t hData, size_t nSize, void* pData = nullptr) noexcept {
        // ASSERT( m_hData == HANDLE_NULL );
        m_hData = hData;
        SetSpan(pData, nSize);
    }

    /// <summary>
    /// Get the allocated size. not same as m_nSize ?
    /// </summary>
    /// <returns></returns>
    SIZE_T GetSize() const noexcept {
        return WINHEAPF(Size)(m_hData);
    }
    UINT GetFlags() const noexcept {
        return WINHEAPF(Flags)(m_hData);
    }
    LPVOID Lock() noexcept {
        //! this actually does nothing on _WIN32 systems? only used for WIN16
        if (m_hData == HANDLE_NULL) return nullptr;
        if (!isValidPtr()) {
#ifdef UNDER_CE
            SetSpan((void*)(m_hData), get_DataSize());
#else
            SetSpan(WINHEAPF(Lock)(m_hData), get_DataSize());
#endif
#ifdef _DEBUG
            if (!isValidPtr()) {
                const HRESULT hRes = HResult::GetLastDef();
                DEBUG_ERR(("Heap Lock ERR='%s' for h=0%x", LOGERR(hRes), m_hData));
            }
#endif
        }
        return get_DataW();
    }
    void Unlock() {
        if (isValidPtr()) {
            ASSERT(m_hData != HANDLE_NULL);
#ifndef UNDER_CE
            WINHEAPF(Unlock)(m_hData);
#endif
            SetSpanConst(nullptr, get_DataSize());
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
        if (m_hData != HANDLE_NULL)
            m_hData = WINHEAPF(ReAlloc)(m_hData, (SIZE_T)dwSize, dwFlags);
        else
            m_hData = WINHEAPF(Alloc)(dwFlags, (SIZE_T)dwSize);
        put_DataSize(dwSize);
        return m_hData;
    }
    void* ReAlloc(size_t dwSize, DWORD dwFlags = WINHEAPM(FIXED)) {
        Unlock();
        m_hData = WINHEAPF(ReAlloc)(m_hData, (SIZE_T)dwSize, dwFlags);
        put_DataSize(dwSize);
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
    void* AllocPtr3(const void* pSrc, size_t dwSize, DWORD dwFlags = WINHEAPM(FIXED)) {
        void* pDst = AllocPtr2(dwSize, dwFlags);
        if (pDst != nullptr) {
            cMem::Copy(pDst, pSrc, dwSize);
        }
        return pDst;
    }
    void Free() {
        /// Unlock and free. NOT done automatically on destruct of this class.
        if (m_hData != HANDLE_NULL) {
            Unlock();  // only unlock if needed.
            FreeHandle();
        }
    }
    HANDLE_t DetachHandle() noexcept {
        // Assume unlocked?
        const HANDLE_t hTmp = m_hData;
        m_hData = HANDLE_NULL;
        SetSpanNull();
        return hTmp;
    }

 protected:
    HANDLE_t FreeHandleLast() {
        return WINHEAPF(Free)(m_hData);
    }
    void FreeHandle() {
        HANDLE_t hFail = FreeHandleLast();
#ifdef _DEBUG
        if (hFail) {
            const HRESULT hRes = HResult::GetLastDef();
            DEBUG_ERR(("GlobalFree ERR='%s'", LOGERR(hRes)));
        }
#endif
        m_hData = HANDLE_NULL;
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
    WINHEAPN(V)(HANDLE_t hData = HANDLE_NULL) : SUPER_t(hData) {}
    ~WINHEAPN(V)() {
        if (m_hData != HANDLE_NULL) {
            Unlock();
            FreeHandleLast();
        }
    }
    void AttachHandle(HANDLE_t hData, size_t nSize, void* pData = nullptr) {
        if (m_hData != HANDLE_NULL) {
            if (hData == m_hData) return;
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
        return &m_hData;
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
    WINHEAPN(T)(HANDLE_t hData = HANDLE_NULL) : SUPER_t(hData) {}

    _TYPE* operator->() noexcept {
        return get_DataW<_TYPE>();
    }
    static _TYPE* GRAYCALL AllocPtrX(size_t nSize, DWORD nFlags = 0) {
        return (_TYPE*)SUPER_t::AllocPtrX(nSize, nFlags);
    }
};
}  // namespace Gray
