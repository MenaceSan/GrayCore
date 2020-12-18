//
//! @file cSingleton.h
//!	A singleton is a type of class of which only one single instance may exist.
//! This is commonly used for management classes used to control system-wide resources.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cSingleton_H
#define _INC_cSingleton_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cNonCopyable.h"
#include "cThreadLock.h"
#include "cUnitTestDecl.h"
#include "cTypeInfo.h"
#include "cHeapObject.h"
#include "cObject.h"
#include "cDebugAssert.h"

namespace Gray
{
	UNITTEST2_PREDEF(cSingleton);

#ifdef _MSC_VER	
#pragma warning(disable:4355)	// disable the warning regarding 'this' pointers being used in base member initializer list. Singletons rely on this action
#endif

	template <class TYPE>
	class cSingletonStatic 
	{
		//! @class Gray::cSingletonStatic
		//! base class for a type that we want to make sure only one of these can exist at a time.
		//! @note TYPE = cSingletonStatic based class = this
		//! Externally created singleton. might be stack based, or abstract (e.g.cNTServiceImpl) but usually static allocated.
		//! @note Assume 1. gets constructed/destructed by the C Runtime, 2. Is inherently thread safe since its not created on demand.
		//! The BIG problem with this is that we cannot guarantee order of creation/destruction. So singletons that rely/construct on each other may be corrupt/uninitialized.

	protected:
		static TYPE* sm_pThe;	//!< pointer to the one and only object of this TYPE. ASSUME automatically init to = nullptr.
	private:
		NonCopyable_IMPL(cSingletonStatic);

	protected:
		cSingletonStatic(TYPE* pObject) noexcept
		{
			//! the singleton must be constructed with a reference to the controlled object
			//! typically this == pObject == sm_pThe
			DEBUG_ASSERT(pObject != nullptr, "cSingletonStatic");
			if (sm_pThe != nullptr)
			{
				// THIS SHOULD NOT HAPPEN! Find who else is creating this singleton!
				DEBUG_ASSERT(sm_pThe == nullptr, "cSingletonStatic");
				return;
			}
			sm_pThe = pObject;
			DEBUG_ASSERT(sm_pThe == this, "cSingletonStatic");
		}
		virtual ~cSingletonStatic() noexcept
		{
			//! the singleton accessor
			if (sm_pThe != nullptr)
			{
				DEBUG_ASSERT(sm_pThe == this, "~cSingletonStatic");
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
			// DYNPTR_CAST or CHECKPTR_CAST for up-cast to some other type.
			return CHECKPTR_CAST(TYPE2, get_Single());
		}
		static inline TYPE& I() noexcept
		{
			//! The singleton by reference.
			return *get_Single();
		}
	};

	template <class TYPE> TYPE* cSingletonStatic<TYPE>::sm_pThe = nullptr; // assume this is always set before any usage.

	class GRAYCORE_LINK cSingletonRegister : public CObject, public cHeapObject // IHeapObject
	{
		//! @class Gray::cSingletonRegister
		//! NON template base for cSingleton. MUST be IHeapObject
		//! Register this to allow for proper order of virtual destruction at C runtime destruct.
		//! Allows for ordered destruction of singletons if modules unload.
		//! @note Static singletons are not multi threaded anyhow. so don't worry about static init order for sm_LockSingle.

		friend class cSingletonManager;

	protected:
#ifndef UNDER_CE
		HMODULE m_hModuleLoaded;	//!< What modules loaded this ? So singletons can be destroyed if DLL/SO unloads.
#endif
	public:
		static cThreadLockFast sm_LockSingle; //!< common lock for all cSingleton.
	protected:
		cSingletonRegister(const TYPEINFO_t& rAddrCode) noexcept;
		virtual ~cSingletonRegister();
		void RegisterSingleton();
	public:
		static void GRAYCALL ReleaseModule(HMODULE hMod);
		UNITTEST2_FRIEND(cSingleton);
	};

	template <class TYPE>
	class cSingleton : public cSingletonStatic<TYPE>, public cSingletonRegister
	{
		//! @class Gray::cSingleton
		//! abstract base class for singleton created lazy/on demand if it does not yet exist or maybe static. see cSingletonSmart<> to destroy on non use.
		//! Thread safe.
		//! ASSUME cSingletonRegister will handle destruct order on app close or module unload.
		//! ASSUME TYPE is based on cSingleton and IHeapObject.
		//! @note If this is really static beware of the "first use" race condition. static init will ASSERT if dynamic is called first.
		//! @note This can get created at C static init time if used inside some other static. But later is OK too of course.
		//!  It's Safe being constructed INSIDE another C runtime init constructor. (order irrelevant) ASSUME m_pThe = nullptr at init.
		//!  http://www.cs.wustl.edu/~schmidt/editorial-3.html

		typedef cSingletonStatic<TYPE> SUPER_t;

	protected:
		cSingleton(TYPE* pObject, const TYPEINFO_t& rAddrCode = typeid(TYPE)) noexcept
			: cSingletonStatic<TYPE>(pObject)
			, cSingletonRegister(rAddrCode)	// track the module for the codes implementation.
		{
			//! typically this == pObject is type of cSingleton
			//! @arg rAddrCode = typeid(XXX) but GCC doesn't like it as part of template?
		}
		virtual ~cSingleton()
		{
			//! I am being destroyed.
			//! sm_pThe is set to nullptr in ~cSingletonStatic
		}

	public:
		static TYPE* GRAYCALL get_Single()
		{
			//! get (or create) a pointer to the singleton object.
			//! @note This ensures proper creation order for singletons (Statics) that ref each other!
			//! @todo use CreateObject from cObjectFactory Like the MFC CreateObject() and "CRuntime?"

			if (!SUPER_t::isSingleCreated())
			{
				// Double Check Lock for multi threaded
				cThreadGuardFast threadguard(cSingletonRegister::sm_LockSingle);	// thread sync critical section.
				if (!SUPER_t::isSingleCreated())
				{
					DEBUG_CHECK(!TYPE::isSingleCreated());	// SUPER_t::sm_pThe
					new TYPE(); // assume it calls its constructor properly and sets sm_pThe
					DEBUG_CHECK(TYPE::isSingleCreated());	// SUPER_t::sm_pThe
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
				cThreadGuardFast threadguard(cSingletonRegister::sm_LockSingle);	// thread sync critical section.
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

#endif	// _INC_cSingleton_H
