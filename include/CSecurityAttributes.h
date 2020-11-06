//
//! @file CSecurityAttributes.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CSecurityAttributes_H
#define _INC_CSecurityAttributes_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CWinHeap2.h"

#if defined(_WIN32) && ! defined(UNDER_CE)
#include <accctrl.h>

#ifdef __GNUC__
#define SDDL_REVISION_1 1
enum WELL_KNOWN_SID_TYPE
{
	WinNullSid = 0,
	WinWorldSid = 1,
	WinLocalSid = 2,
};
#define SID_MAX_SUB_AUTHORITIES 15
#define SECURITY_MAX_SID_SIZE  (sizeof(SID) - sizeof(DWORD) + (SID_MAX_SUB_AUTHORITIES * sizeof(DWORD)))
#endif
UNITTEST_PREDEF(CSecurityAttributes)

namespace Gray
{
	class GRAYCORE_LINK CSecurityId : private CWinLocalT < SID >
	{
		//! @class GrayLib::CSecurityId
		//! A users id; or id group. http://msdn.microsoft.com/en-us/library/aa379594%28v=VS.85%29.aspx
		//! ATL calls this CSid
		//! Variable Sized.
		//! some _WIN32 calls expect LocalFree() to be called for SID pointer returned.
		//! @note AllocateAndInitializeSid() would need to use FreeSid() NOT LocalFree(). so don't use it!
		//! The first character is always an S. It identifies it as a SID
		//! The second number is the SID version.
		//! The third number identifies the authority (5 is NT authority)
		//! The forth set of numbers is the domain identifier, up to 500
		//! The remainder is the account or group identifier. "HKEY_USERS\S-1-5-21-3686267286-921206174-156832652-1000"

		typedef CWinLocalT<SID> SUPER_t;
	public:
		CSecurityId();
		CSecurityId(WELL_KNOWN_SID_TYPE eWellKnownSidType);
		~CSecurityId();

		SID* get_SID() const
		{
			// like PSID and PISID // variable length?
			return SUPER_t::get_Data();
		}
		operator SID*()
		{
			return get_SID();
		}
		bool isValid() const
		{
			if (get_Data() == nullptr)
				return false;
			return ::IsValidSid(get_Data());
		}

		size_t get_Length() const
		{
			return ::GetLengthSid(get_SID());
		}

		bool SetSID(SID* pSID);
		cString GetStringSID() const;
		bool SetStringSID(const GChar_t* pszSID);

		HRESULT SetByUserName(const GChar_t* pszUserName);
	};

	class GRAYCORE_LINK CSecurityACL : private CWinLocalT < ACL >
	{
		//! @class GrayLib::CSecurityACL
		//! "Discretionary access-control list" (Dacl) or (Sacl)
		//! ATL calls this CAcl, CDacl and CSacl
		//! Variable Sized array of ACE. No real need to use  LocalAlloc(), LocalFree()
		//! @note This is a mess. AddAce uses this without knowing the size of the allocated array.

		typedef CWinLocalT<ACL> SUPER_t;
	public:
		CSecurityACL(SID* pSidFirst = nullptr, DWORD dwAccessMask = GENERIC_ALL);
		~CSecurityACL();

		ACL* get_ACL() const
		{
			return SUPER_t::get_Data();
		}
		operator ACL*()
		{
			return get_ACL();
		}
		bool isValid() const
		{
			if (get_ACL() == nullptr)
				return false;
			return(::IsValidAcl(get_ACL()));
		}
		int get_AceCount() const
		{
			if (get_ACL() == nullptr)
				return 0;
			return get_ACL()->AceCount;
		}

		bool AddAllowedAce(SID* pSid, DWORD dwAccessMask = GENERIC_ALL);
	};

	class GRAYCORE_LINK CSecurityDesc : public CWinLocalT < SECURITY_DESCRIPTOR >
	{
		//! @class GrayLib::CSecurityDesc
		//! Windows security descriptor is added to SECURITY_ATTRIBUTES
		//! Same as ATL CSecurityDesc
		//! some _WIN32 calls expect LocalFree() to be called for pointer returned.

	public:
		static const FILECHAR_t* k_szLowIntegrity;	// L"S:(ML;;NW;;;LW)";

	public:
		CSecurityDesc(ACL* pDacl = nullptr);
		CSecurityDesc(const FILECHAR_t* pszSaclName);
		~CSecurityDesc();

		bool InitSecurityDesc(const FILECHAR_t* pszSaclName);
		bool InitLowIntegrity();

		operator SECURITY_DESCRIPTOR*()
		{
			return get_Data();
		}
		static bool GRAYCALL IsValid(SECURITY_DESCRIPTOR* pSD)
		{
			if (pSD == nullptr) 	// null is valid.
				return true;
			return ::IsValidSecurityDescriptor(pSD);
		}
		bool isValid() const
		{
			return IsValid(get_Data());
		}
		ACL* GetSacl(BOOL* pbSaclPresent = nullptr, BOOL* pbSaclDefaulted = nullptr) const
		{
			//! Get attached CSecurityACL. No need to free this pointer.
			ACL* pSacl = nullptr;
			if (!::GetSecurityDescriptorSacl(get_Data(), pbSaclPresent, &pSacl, pbSaclDefaulted))
			{
				return nullptr;
			}
			return(pSacl);
		}
		BOOL SetSacl(ACL* pSacl, bool bSaclPresent = true, bool bSaclDefaulted = false)
		{
			return ::SetSecurityDescriptorSacl(get_Data(), bSaclPresent, pSacl, bSaclDefaulted);
		}
#if 0
		BOOL SetSaclRules(size_t nCount, EXPLICIT_ACCESS* pRules)
		{
			ACL* pSacl = nullptr;
			LSTATUS iRet = ::SetEntriesInAcl(nCount, pRules, nullptr, &pSacl);
			if (iRet != NO_ERROR)
				return false;
			return SetSacl(pSacl);
		}
#endif

		ACL* GetDacl(BOOL* pbDaclPresent = nullptr, BOOL* pbDaclDefaulted = nullptr) const
		{
			//! Get attached CSecurityACL. No need to free this pointer.
			ACL* pDacl = nullptr;
			if (!::GetSecurityDescriptorDacl(get_Data(), pbDaclPresent, &pDacl, pbDaclDefaulted))
			{
				return nullptr;
			}
			return(pDacl);
		}
		BOOL SetDacl(ACL* pDacl, bool bDaclPresent = true, bool bDaclDefaulted = false)
		{
			return ::SetSecurityDescriptorDacl(get_Data(), bDaclPresent, pDacl, bDaclDefaulted);
		}
		BOOL SetOwner(PSID pOwner = nullptr, BOOL bOwnerDefaulted = true)
		{
			// default = clear.
			return ::SetSecurityDescriptorOwner(get_Data(), pOwner, bOwnerDefaulted);
		}

		bool AttachToObject(HANDLE hObject, SE_OBJECT_TYPE type = SE_KERNEL_OBJECT) const;
	};

	class GRAYCORE_LINK CSecurityAttributes : public SECURITY_ATTRIBUTES
	{
		//! @class GrayLib::CSecurityAttributes
		//! Windows security attributes. for CreateFile etc.
		//! Same as ATL CSecurityAttributes
		//! Holds: lpSecurityDescriptor = SECURITY_DESCRIPTOR*

	public:
		CSecurityDesc m_sd;	//!< attached SECURITY_DESCRIPTOR.
	protected:
		void UpdateSecurityDescriptor();

	public:
		CSecurityAttributes(bool bInheritHandle = false, ACL* pDacl = nullptr);
		CSecurityAttributes(bool bInheritHandle, const FILECHAR_t* pszSaclName);
		~CSecurityAttributes(void);

		operator SECURITY_ATTRIBUTES*()
		{
			return static_cast<SECURITY_ATTRIBUTES*>(this);
		}
		bool isValid() const;

		UNITTEST_FRIEND(CSecurityAttributes);
	};

	class GRAYCORE_LINK CSecurityAttribsLowIntegrity : public CSecurityAttributes
	{
		//! @class GrayLib::CSecurityAttribsLowIntegrity
		//! Windows bullshit to indicate that i can speak to untrusted apps. mail channel can be opened by other apps. etc.
	public:
		CSecurityAttribsLowIntegrity(bool bInheritHandle = false)
		: CSecurityAttributes(bInheritHandle, CSecurityDesc::k_szLowIntegrity)
		{
		}
		~CSecurityAttribsLowIntegrity()
		{
		}
	};

	class GRAYCORE_LINK CSecurityAttribsWKS : public CSecurityAttributes
	{
		//! @class GrayLib::CSecurityAttribsWKS
		//! Default Windows security. "Well Known SID" type. e.g. WinLocalSid
		//! Consolidate all the crap into a single object.
	public:
		CSecurityAttribsWKS(WELL_KNOWN_SID_TYPE eWellKnownSidType = WinLocalSid, DWORD dwAccess = GENERIC_ALL, bool bInheritHandle = true)
		: CSecurityAttributes(bInheritHandle)
		, m_sid(eWellKnownSidType)
		, m_dacl(m_sid, dwAccess)
		{
			m_sd.SetDacl(m_dacl);
			UpdateSecurityDescriptor();
		}
		~CSecurityAttribsWKS()
		{
		}
	public:
		CSecurityId m_sid;	//!< for WELL_KNOWN_SID_TYPE
		CSecurityACL m_dacl;
	};
};
#endif	// _WIN32
#endif	// CSecurityAttributes