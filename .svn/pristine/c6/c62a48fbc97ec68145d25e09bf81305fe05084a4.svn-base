//
//! @file CSingleton.h
//!	A singleton is a type of class of which only one single instance may exist.
//! This is commonly used for management classes used to control system-wide resources.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CSingleton_H
#define _INC_CSingleton_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CNonCopyable.h"
#include "CThreadLock.h"
#include "CUnitTestDecl.h"
#include "CTypeInfo.h"
#include "CHeapObject.h"
#include "CObject.h"
#include "CDebugAssert.h"

UNITTEST_PREDEF(CSingletonRegister)

namespace Gray
{
#ifdef _MSC_VER	
#pragma warning(disable:4355)	// disable the warning regarding 'this' pointers being used in base member initializer list. Singletons rely on this action
#endif

	template <class TYPE>
	class CSingletonStatic 
	{
		//! @class Gray::CSingletonStatic
		//! base class for a type that we want to make sure only one of these can exist at a time.
		//! @note TYPE = CSingletonStatic based class = this
		//! Externally created singleton. might be stack based, or abstract (e.g.CNTServiceImpl) but usually static allocated.
		//! @note Assume 1. gets constructed/destructed by the C Runtime, 2. Is inherently thread safe since its not created on demand.
		//! The BIG problem with this is that we cannot guarantee order of creation. So singletons that rely/construct on each other may be corrupt/uninitialized.

	protected:
		static TYPE* sm_pThe;	//!< pointer to the one and only object of this TYPE. ASSUME automatically init to = nullptr.
	private:
		NonCopyable_IMPL(CSingletonStatic);

	protected:
		CSingletonStatic(TYPE* pObject) noexcept
		{
			//! the singleton must be constructed with a reference to the controlled object
			//! typically this == pObject == sm_pThe
			DEBUG_ASSERT(pObject != nullptr, "CSingletonStatic");
			if (sm_pThe != nullptr)
			{
				// THIS SHOULD NOT HAPPEN! Find who else is creating this singleton!
				DEBUG_ASSERT(sm_pThe == nullptr, "CSingletonStatic");
				return;
			}
			sm_pThe = pObject;
			DEBUG_ASSERT(sm_pThe == this, "CSingletonStatic");
		}
		virtual ~CSingletonStatic() noexcept
		{
			//! the singleton accessor
			if (sm_pThe != nullptr)
			{
				DEBUG_ASSERT(sm_pThe == this, "~CSingletonStatic");
				sm_pThe = nullptr;
			}
		}

	public:
		// the singleton accessor
		static inline bool isSingleCreated() noexcept
		{
			return sm_pThe != nullptr;
		}
		static inline TYPE* get_SingleU() noexcept
		{
			//! @return sm_pThe but allow that it might be nullptr. Weird.
			return sm_pThe;
		}
		static inline TYPE* get_Single()
		{
			//! This is a complex or abstract or assumed static type that we cannot just create automatically on first usage.
			ASSERT(isSingleCreated());
			return sm_pThe;	// get_SingleU()
		}
		template <class TYPE2>
		static TYPE2* GRAYCALL get_SingleCast()	// ASSUME TYPE2 derived from TYPE?
		{
			// DYNPTR_CAST or CHECKPTR_CAST for up-cast.
			return CHECKPTR_CAST(TYPE2, get_Single());
		}
		static inline TYPE& I() noexcept
		{
			//! The singleton by reference.
			return *get_Single();
		}
	};

	template <class TYPE> TYPE* CSingletonStatic<TYPE>::sm_pThe = nullptr; // assume this is always set before any usage.

	class GRAYCORE_LINK CSingletonRegister : public CObject, public CHeapObject // IHeapObject
	{
		//! @class Gray::CSingletonRegister
		//! NON template base for CSingleton. MUST be IHeapObject
		//! Register this to allow for proper order of virtual destruction at C runtime destruct.
		//! Allows for ordered destruction of singletons if modules unload.
		//! @note Static singletons are not multi threaded anyhow. so don't worry about static init order for sm_LockSingle.

		friend class CSingletonManager;

	protected:
#ifndef UNDER_CE
		HMODULE m_hModuleLoaded;	//!< What modules loaded this ? So singletons can be destroyed if DLL/SO unloads.
#endif
	public:
		static CThreadLockFast sm_LockSingle; //!< common lock for all CSingleton.
	protected:
		CSingletonRegister(const TYPEINFO_t& rAddrCode);
		virtual ~CSingletonRegister();
		void RegisterSingleton();
	public:
		static void GRAYCALL ReleaseModule(HMODULE hMod);
		UNITTEST_FRIEND(CSingletonRegister);
	};

	template <class TYPE>
	class CSingleton : public CSingletonStatic<TYPE>, public CSingletonRegister
	{
		//! @class Gray::CSingleton
		//! abstract base class for singleton created lazy/on demand if it does not yet exist or maybe static. see CSingletonSmart<> to destroy on non use.
		//! Thread safe.
		//! ASSUME CSingletonRegister will handle destruct order on app close or module unload.
		//! ASSUME TYPE is based on CSingleton and IHeapObject.
		//! @note If this is really static beware of the "first use" race condition. static init will ASSERT if dynamic is called first.
		//! @note This can get created at C static init time if used inside some other static. But later is OK too of course.
		//!  It's Safe being constructed INSIDE another C runtime init constructor. (order irrelevant) ASSUME m_pThe = nullptr at init.
		//!  http://www.cs.wustl.edu/~schmidt/editorial-3.html

		typedef CSingletonStatic<TYPE> SUPER_t;

	protected:
		CSingleton(TYPE* pObject, const TYPEINFO_t& rAddrCode = typeid(TYPE))
			: CSingletonStatic<TYPE>(pObject)
			, CSingletonRegister(rAddrCode)	// track the module for the codes implementation.
		{
			//! typically this == pObject is type of CSingleton
			//! @arg rAddrCode = typeid(XXX) but GCC doesn't like it as part of template?
		}
		virtual ~CSingleton()
		{
			//! I am being destroyed.
			//! sm_pThe is set to nullptr in ~CSingletonStatic
		}

	public:
		static TYPE* GRAYCALL get_Single()
		{
			//! get (or create) a pointer to the singleton object.
			//! @note This ensures proper creation order for singletons (Statics) that ref each other!
			//! @todo use CreateObject from CObjectCreator Like the MFC CreateObject() and "CRuntime?"

			if (!SUPER_t::isSingleCreated())
			{
				// Double Check Lock for multi threaded
				CThreadGuardFast threadguard(CSingletonRegister::sm_LockSingle);	// thread sync critical section.
				if (!SUPER_t::isSingleCreated())
				{
					ASSERT(!TYPE::isSingleCreated());	// SUPER_t::sm_pThe
					new TYPE(); // assume it calls its constructor properly and sets sm_pThe
					ASSERT(TYPE::isSingleCreated());	// SUPER_t::sm_pThe
					SUPER_t::sm_pThe->RegisterSingleton();	// Register only if i know it is dynamic. Not static.
				}
			}
			return SUPER_t::sm_pThe;
		}

		template <class TYPE2>
		static TYPE2* GRAYCALL get_SingleT()	// ASSUME TYPE2 derived from TYPE
		{
			//! get (or create) a pointer to the derived singleton TYPE2 object. 
			//! ASSUME TYPE2 derived from TYPE.
			//! @note This can create a race condition. This decides the true TYPE of the object.

			if (!SUPER_t::isSingleCreated())
			{
				// Double Check Lock for multi threaded
				CThreadGuardFast threadguard(CSingletonRegister::sm_LockSingle);	// thread sync critical section.
				if (!SUPER_t::isSingleCreated())
				{
					ASSERT(!TYPE2::isSingleCreated());	// SUPER_t::sm_pThe
					TYPE2* p = new TYPE2();
					ASSERT(p == SUPER_t::sm_pThe);
					ASSERT(TYPE2::isSingleCreated());	// SUPER_t::sm_pThe
					SUPER_t::sm_pThe->RegisterSingleton();	// Register only if i know it is dynamic. Not static.
				}
			}
			return CHECKPTR_CAST(TYPE2, SUPER_t::sm_pThe);
		}

		static TYPE& GRAYCALL I()
		{
			//! The singleton by reference.
			return *get_Single();
		}
	};
}

#endif	// _INC_CSingleton_H
