//
//! @file cWinHeap.inl
//! macro template for CWinLocalHandle, CWinLocalV, CWinLocalT<> etc
//! @note requires proper #define set up. DON'T include this directly!
//! "#define WINHEAPN(), WINHEAPM(), WINHEAPH, WINHEAPF()"
//

namespace Gray
{
	class WINHEAPN(Handle) : public cMemBlock
	{
		//! CWinLocalHandle or CWinGlobalHandle
		//! Wrap the HANDLE_t heap/memory object for lock/unlock of an instance. (HLOCAL or HGLOBAL)
		//! manage lock and unlock. yes i know lock/unlock doesn't do anything in _WIN32
		//! Does NOT free on destruct. just unlock.
		//! m_pData = Locked pointer. GlobalHandle(m_pData)==m_hData

	public:
		typedef WINHEAPH HANDLE_t;
		WINHEAPN(Handle)(HANDLE_t hData = HANDLE_NULL, void* pData = nullptr, size_t nSize = 0)
			: m_hData(hData)
			, cMemBlock(pData, nSize)	// size may not be known?
		{
			//! Attach existing handle to this class.
		}
		~WINHEAPN(Handle)()
		{
			Unlock();
		}
		bool isAlloc() const
		{
			return m_hData != HANDLE_NULL;
		}
		HANDLE_t get_Handle() const noexcept
		{
			return m_hData;
		}
		void UpdateHandle()
		{
			//! If i have the m_pData, make sure the m_hData matches.
			//! Lock() does the equiv for m_pData.
			if (m_pData != nullptr)
			{
				m_hData = WINHEAPF(Handle)(m_pData);
			}
		}
		void AttachHandle(HANDLE_t hData, void* pData = nullptr)
		{
			//! Attach existing handle to this class. put_Handle()
			// ASSERT( m_hData == HANDLE_NULL );
			m_hData = hData;
			m_pData = pData;
		}

		SIZE_T GetSize() const
		{
			return WINHEAPF(Size)(m_hData);
		}
		UINT GetFlags() const
		{
			return WINHEAPF(Flags)(m_hData);
		}
		LPVOID Lock()
		{
			//! this actually does nothing on _WIN32 systems? only used for WIN16
			if (m_hData == HANDLE_NULL)
				return nullptr;
			if (m_pData == nullptr)
			{
#ifdef UNDER_CE
				m_pData = (void*)(m_hData);
#else
				m_pData = WINHEAPF(Lock)(m_hData);
#endif
#ifdef _DEBUG
				if (m_pData == nullptr)
				{
					HRESULT hRes = HResult::GetLastDef();
					DEBUG_ERR(("GlobalLock ERR='%s' for h=0%x", LOGERR(hRes), m_hData));
				}
#endif
			}
			return get_Data();
		}
		void Unlock()
		{
			if (m_pData != nullptr)
			{
				ASSERT(m_hData != HANDLE_NULL);
#ifndef UNDER_CE
				WINHEAPF(Unlock)(m_hData);
#endif
				m_pData = nullptr;
			}
		}
		HANDLE_t AllocHandle(size_t dwSize, DWORD dwFlags = WINHEAPM(MOVEABLE))
		{
			//! Allocate a non locked handle.
			//! We may want an unlocked handle for some reason.
			//! dwFlags = GMEM_MOVEABLE | GMEM_ZEROINIT = WINHEAPM(MOVEABLE)|WINHEAPM(ZEROINIT)
			Unlock();
			if (m_hData != HANDLE_NULL)
				m_hData = WINHEAPF(ReAlloc)(m_hData, (SIZE_T)dwSize, dwFlags);
			else
				m_hData = WINHEAPF(Alloc)(dwFlags, (SIZE_T)dwSize);
			return m_hData;
		}
		void* ReAlloc(size_t dwSize, DWORD dwFlags = WINHEAPM(FIXED))
		{
			Unlock();
			m_hData = WINHEAPF(ReAlloc)(m_hData, (SIZE_T)dwSize, dwFlags);
			return Lock();
		}
		void* Alloc(size_t dwSize, DWORD dwFlags = WINHEAPM(FIXED))
		{
			//! Allocate and lock the handle.
			//! dwFlags = GMEM_MOVEABLE | GMEM_ZEROINIT 
			AllocHandle(dwSize, dwFlags);
			return Lock();
		}
		void* Alloc(const void* pSrc, size_t dwSize, DWORD dwFlags = WINHEAPM(FIXED))
		{
			void* pDst = Alloc(dwSize, dwFlags);
			if (pDst != nullptr)
			{
				::memcpy(pDst, pSrc, dwSize);
			}
			return pDst;
		}
		void Free()
		{
			//!< Unlock and free. NOT done automatically on destruct of this class.
			if (m_hData != HANDLE_NULL)
			{
				Unlock();	// only unlock if needed.
				FreeHandle();
			}
		}
		HANDLE_t DetachHandle()
		{
			HANDLE_t hTmp = m_hData;
			m_hData = HANDLE_NULL;
			m_pData = nullptr;
			return hTmp;
		}

	protected:
		HANDLE_t FreeHandleLast()
		{
			return WINHEAPF(Free)(m_hData);
		}
		void FreeHandle()
		{
			HANDLE_t hFail = FreeHandleLast();
#ifdef _DEBUG
			if (hFail)
			{
				HRESULT hRes = HResult::GetLastDef();
				DEBUG_ERR(("GlobalFree ERR='%s'", LOGERR(hRes)));
			}
#endif
			m_hData = HANDLE_NULL;
		}

	protected:
		HANDLE_t m_hData;
	};

	class WINHEAPN(V) : public WINHEAPN(Handle)
	{
		//! CWinLocalV or CWinGlobalV
		//! Wrap the HANDLE_t heap/memory allocation object.
		//! Similar to MFC CGlobalHeap or CWin32Heap
		//! Similar to cHeapBlock
		//! Free on destruct

		typedef WINHEAPN(Handle) SUPER_t;
	public:
		WINHEAPN(V)(HANDLE_t hData = HANDLE_NULL)
			: WINHEAPN(Handle)(hData)
		{}
		~WINHEAPN(V)()
		{
			if (m_hData != HANDLE_NULL)
			{
				Unlock();
				FreeHandleLast();
			}
		}
		void AttachHandle(HANDLE_t hData, void* pData = nullptr)
		{
			if (m_hData != HANDLE_NULL)
			{
				if (hData == m_hData)
					return;
				Unlock();
				FreeHandleLast();
			}
			SUPER_t::AttachHandle(hData, pData);
		}
		void AttachPtr(void* pData)
		{
			AttachHandle(WINHEAPF(Handle)(pData), pData);
		}
		void** get_PPtrData()
		{
			//! Get a pointer to an empty pointer.
			//! @note Make sure you call UpdateHandle() after this.
			Free();
			return &m_pData;
		}
		HANDLE_t* get_PPtrHandle()
		{
			//! @note Make sure you call Lock() after this.
			Free();
			return &m_hData;
		}
		static LPVOID GRAYCALL AllocPtr(size_t nSize, DWORD nFlags = 0)
		{
			//! like _WIN32 GlobalAllocPtr( UINT, SIZE_T ) or GlobalAlloc
			//! nFlags = GMEM_MOVEABLE | GMEM_ZEROINIT 
			return WINHEAPF(Lock)(WINHEAPF(Alloc)(nFlags, nSize));
		}
		static void GRAYCALL FreePtr(void* pData)
		{
			//! like _WIN32 GlobalFreePtr()
			//! @note Yes i know Unlock does nothing in modern OS
			if (pData == nullptr)
				return;
			HANDLE_t hData = WINHEAPF(Handle)(pData);
			WINHEAPF(Unlock)(hData);
			HANDLE_t hFail = WINHEAPF(Free)(hData);
			UNREFERENCED_PARAMETER(hFail);
		}
	};

	template <class _TYPE>
	struct WINHEAPN(T) : public WINHEAPN(V)
	{
		//! A type cast windows heap pointer/handle pair. like cMemBlockT<>
		//! CWinGlobalT<> or CWinLocalT<>
		typedef WINHEAPN(V) SUPER_t;

		_TYPE* get_Data() const
		{
			return (_TYPE*)m_pData;
		}
		void AttachPtr(_TYPE * pData)
		{
			SUPER_t::AttachPtr(pData);
		}
		_TYPE** get_PPtrData()
		{
			//! @note Make sure you call UpdateHandle() after this.
			return (_TYPE**)SUPER_t::get_PPtrData();
		}
		_TYPE* operator ->() const
		{
			return (_TYPE*)m_pData;
		}
	};
} 