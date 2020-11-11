//
//! @file cSecurityAttributes.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cSecurityAttributes.h"
#include "cLogMgr.h"

#if defined(_WIN32) && ! defined(UNDER_CE)
#include <sddl.h>
#include <AclAPI.h>

#ifdef __GNUC__

WINADVAPI BOOL WINAPI CreateWellKnownSid(
	IN WELL_KNOWN_SID_TYPE WellKnownSidType,
	IN PSID DomainSid,
	IN PSID pSid,
	OUT DWORD *cbSid
	);
WINADVAPI BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorW(
	IN  LPCWSTR StringSecurityDescriptor,
	IN  DWORD StringSDRevision,
	OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
	OUT PULONG  SecurityDescriptorSize OPTIONAL
	);
WINADVAPI BOOL WINAPI ConvertStringSecurityDescriptorToSecurityDescriptorA(
	IN  LPCSTR StringSecurityDescriptor,
	IN  DWORD StringSDRevision,
	OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
	OUT PULONG  SecurityDescriptorSize OPTIONAL
	);

WINADVAPI BOOL WINAPI ConvertSidToStringSidA( IN  PSID     Sid, OUT LPSTR  *StringSid );
WINADVAPI BOOL WINAPI ConvertSidToStringSidW( IN  PSID     Sid, OUT LPWSTR  *StringSid );
#ifdef UNICODE
#define ConvertSidToStringSid  ConvertSidToStringSidW
#else
#define ConvertSidToStringSid  ConvertSidToStringSidA
#endif // !UNICODE

WINADVAPI BOOL WINAPI ConvertStringSidToSidA( IN LPCSTR   StringSid, OUT PSID   *Sid );
WINADVAPI BOOL WINAPI ConvertStringSidToSidW( IN LPCWSTR   StringSid, OUT PSID   *Sid );
#ifdef UNICODE
#define ConvertStringSidToSid  ConvertStringSidToSidW
#else
#define ConvertStringSidToSid  ConvertStringSidToSidA
#endif // !UNICODE

#define WinBuiltinUsersSid ((WELL_KNOWN_SID_TYPE)1)

#endif	// __GNUC__

#ifndef LABEL_SECURITY_INFORMATION
#define LABEL_SECURITY_INFORMATION (0x00000010L)
#endif

namespace Gray
{
	cSecurityId::cSecurityId()
	{
		// Empty SID
	}

	cSecurityId::cSecurityId(WELL_KNOWN_SID_TYPE eWellKnownSidType)
	{
		// Variable Sized. samples use LocalAlloc() except for AllocateAndInitializeSid()
		DWORD dwSize = SECURITY_MAX_SID_SIZE;
		this->Alloc(dwSize);
		if (!::CreateWellKnownSid(eWellKnownSidType, nullptr, get_SID(), &dwSize))
		{
			this->Free();
			DEBUG_ERR(("cSecurityId CreateWellKnownSid"));
		}
		else
		{
			this->ReAlloc(dwSize);	// trim buffer to used size.
		}
	}
	cSecurityId::~cSecurityId()
	{
	}

#if 0
	cSecurityId::cSecurityId(DWORD dwSecondSubAuth)
	{
		// a SID with 2 sub authorities
		SecureZeroMemory(IdentifierAuthority.Value, sizeof(IdentifierAuthority.Value));
		Revision = SID_REVISION;
		SubAuthorityCount = 2;
		IdentifierAuthority.Value[5] = 5; // SECURITY_NT_AUTHORITY {0,0,0,0,5}
		SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
		DWORD m_dwSecondSubAuth = dwSecondSubAuth;
	}
#endif

	bool cSecurityId::SetSID(SID* pSID)
	{
		//! Copy SID.
		if (pSID == nullptr)
		{
			// Clear.
			this->Free();
			return true;
		}
		if (pSID == get_SID())
		{
			return true;	// same.
		}
		size_t nLengthSid = ::GetLengthSid(pSID);
		if (nLengthSid <= 0)
		{
			return false;
		}
		this->Alloc(pSID, nLengthSid);
		return true;
	}

	cString cSecurityId::GetStringSID() const
	{
		//! e.g. S-1-5-21-3686267286-921206174-156832652-1000
		LPTSTR pStringSid = nullptr;
		if (!::ConvertSidToStringSid(get_SID(), &pStringSid))
		{
			return "";
		}
		cString sSID = pStringSid;
		::LocalFree(pStringSid);
		return sSID;
	}

	bool cSecurityId::SetStringSID(const GChar_t* pszSID)
	{
		//! e.g. S-1-5-21-3686267286-921206174-156832652-1000
		//! _GT("S-1-1-0") = share to all users.
		if (!::ConvertStringSidToSid(pszSID, (PSID *)get_PPtrData()))
		{
			return false;
		}
		this->UpdateHandle();
		return true;
	}

	HRESULT cSecurityId::SetByUserName(const GChar_t* pszUserName)
	{
		//! Find a user by name. set to SID for this user.
		//! @return SID_NAME_USE (the user account type)

		if (pszUserName == nullptr)
			return E_INVALIDARG;

		DWORD dwDomainSize = 0;
		DWORD dwSidSize = 0;
		SID_NAME_USE snu;

		// Call to get size info for alloc
		BOOL bRet = _GTN(::LookupAccountName)(nullptr, pszUserName, nullptr, &dwSidSize, nullptr, &dwDomainSize, &snu);

		HRESULT hRes = HResult::GetLast();
		if (hRes != HRESULT_WIN32_C(ERROR_INSUFFICIENT_BUFFER))
		{
			return HResult::GetDef(hRes);
		}

		cString sDomain;
		Alloc(dwSidSize);
		bRet = _GTN(::LookupAccountName)(nullptr, pszUserName, get_SID(), &dwSidSize, sDomain.GetBuffer(dwDomainSize), &dwDomainSize, &snu);
		sDomain.ReleaseBuffer();

		if (!bRet)
		{
			this->Free();
			return HResult::GetLastDef(E_OUTOFMEMORY);
		}

		this->UpdateHandle();
		return (HRESULT)snu;
	}

	//****************************************

	cSecurityACL::cSecurityACL(SID* pSidFirst, DWORD dwAccessMask)
	{
		//! declared access list.
		//! Variable Sized. samples use LocalAlloc()

		DWORD nACLSizeEst = sizeof(ACL) + ((sizeof(ACCESS_ALLOWED_ACE) + sizeof(SID)) * 4); // estimate size needed?
		this->Alloc(nACLSizeEst);	// LocalAlloc
		ASSERT(get_ACL() != nullptr);
		ASSERT(!isValid());
		if (!::InitializeAcl(get_ACL(), nACLSizeEst, ACL_REVISION))
		{
			this->Free();
			DEBUG_ERR(("cSecurityACL InitializeAcl"));
		}
		ASSERT(isValid());
		if (pSidFirst)
		{
			AddAllowedAce(pSidFirst, dwAccessMask);
		}
	}

	cSecurityACL::~cSecurityACL()
	{
	}

	bool cSecurityACL::AddAllowedAce(SID* pSid, DWORD dwAccessMask)
	{
		//! same as ATL CDacl::AddAllowedAce
		//! do not use the EX version - ACE inheritance is not required.
		ASSERT(isValid());

		// Grow the m_pACL array ??
		if (!::AddAccessAllowedAce(get_ACL(), ACL_REVISION, dwAccessMask, pSid))
		{
			this->Free();	// kill it.
			DEBUG_ERR(("cSecurityACL AddAccessAllowedAce"));
			return false;
		}

		return true;
	}

	//****************************************

	const FILECHAR_t* cSecurityDesc::k_szLowIntegrity = _FN("S:(ML;;NW;;;LW)");

	cSecurityDesc::cSecurityDesc(ACL* pDacl)
	{
		//! @note pDacl can be nullptr
		Alloc(sizeof(SECURITY_DESCRIPTOR));

		if (!::InitializeSecurityDescriptor(get_Data(), SECURITY_DESCRIPTOR_REVISION))
		{
			Free();
			DEBUG_ERR(("cSecurityDesc InitializeSecurityDescriptor "));
			return;
		}

		// Set the initial Dacl to the security descriptor. can be nullptr.
		if (!SetDacl(pDacl, true, false))
		{
			Free();
			DEBUG_ERR(("cSecurityDesc SetSecurityDescriptorDacl "));
			return;
		}

		ASSERT(isValid());
	}

	cSecurityDesc::cSecurityDesc(const FILECHAR_t* pszSaclName)
	{
		InitSecurityDesc(pszSaclName);
	}

	cSecurityDesc::~cSecurityDesc()
	{
	}

	bool cSecurityDesc::InitSecurityDesc(const FILECHAR_t* pszSaclName)
	{
		//! Create a _WIN32 security descriptor from a string.
		//! pwStr = L"S:(ML;;NW;;;LW)" = k_szLowIntegrity.
		//! @note Must ::LocalFree(pSD) this when done.
		// ASSUME this->Free() -> LocalFree() is called in get_PPtrData

		ULONG lSecurityDescriptorSize = 0;
		if (!_FNF(::ConvertStringSecurityDescriptorToSecurityDescriptor)(
			pszSaclName, SDDL_REVISION_1, (PSECURITY_DESCRIPTOR*)get_PPtrData(), &lSecurityDescriptorSize))
		{
			// 0x8007070c = ERROR_INVALID_DATATYPE = "The specified data type is invalid.".
			// HRESULT hRes = HResult::GetLast();
			ASSERT(!isValid());
			return false;
		}
		this->UpdateHandle();
		ASSERT(isValid());
		return true;
	}

	bool cSecurityDesc::InitLowIntegrity()
	{
		//! set to "low integrity". k_szLowIntegrity.
		//! LOW_INTEGRITY_SDDL_SACL_W = L"S:(ML;;NW;;;LW)";

		return InitSecurityDesc(k_szLowIntegrity);
	}

	bool cSecurityDesc::AttachToObject(HANDLE hObject, SE_OBJECT_TYPE eType) const
	{
		//! Attach this security descriptor to some system hObject.
		//! eType = SE_KERNEL_OBJECT

		BOOL fSaclPresent = false;
		BOOL fSaclDefaulted = false;
		ACL* pSacl = this->GetSacl(&fSaclPresent, &fSaclDefaulted);
		if (pSacl == nullptr)
			return false;

		LSTATUS dwErr = ::SetSecurityInfo(
			hObject, eType, LABEL_SECURITY_INFORMATION,
			nullptr, nullptr, nullptr, pSacl);
		if (NO_ERROR != dwErr)
		{
			return false;
		}

		return true;
	}

	//*******************************

	cSecurityAttributes::cSecurityAttributes(bool bInheritHandle, ACL* pDacl)
		: m_sd(pDacl)
	{
		cMem::Zero(static_cast<SECURITY_ATTRIBUTES*>(this), sizeof(SECURITY_ATTRIBUTES));
		this->nLength = sizeof(SECURITY_ATTRIBUTES);
		this->bInheritHandle = bInheritHandle;
		UpdateSecurityDescriptor();
	}

	cSecurityAttributes::cSecurityAttributes(bool bInheritHandle, const FILECHAR_t* pszSaclName)
		: m_sd(pszSaclName)
	{
		cMem::Zero(static_cast<SECURITY_ATTRIBUTES*>(this), sizeof(SECURITY_ATTRIBUTES));
		this->nLength = sizeof(SECURITY_ATTRIBUTES);
		this->bInheritHandle = bInheritHandle;
		UpdateSecurityDescriptor();
	}

	cSecurityAttributes::~cSecurityAttributes(void)
	{
	}

	bool cSecurityAttributes::isValid() const
	{
		if (this->nLength <= 0)
			return false;
		return cSecurityDesc::IsValid((SECURITY_DESCRIPTOR*)(this->lpSecurityDescriptor));
	}

	void cSecurityAttributes::UpdateSecurityDescriptor()
	{
		// update
		SECURITY_DESCRIPTOR* pSD = m_sd.get_Data();
		this->lpSecurityDescriptor = pSD;
	}
}

//****************************************************************************

#if USE_UNITTESTS
#include "cOSUser.h"
#include "CFile.h"
#include "cUnitTest.h"

UNITTEST_CLASS(cSecurityAttributes)
{
	UNITTEST_METHOD(cSecurityAttributes)
	{
		// cSecurityId sidUsers(DOMAIN_ALIAS_RID_USERS);
		// cSecurityId sidAdmins(DOMAIN_ALIAS_RID_ADMINS);

		cSecurityId sidUser;
		cOSUserToken token;
		HRESULT hRes = token.GetSID(sidUser);
		UNITTEST_TRUE(SUCCEEDED(hRes));
		sm_pLog->addInfoF("sidUser='%s'", LOGSTR(sidUser.GetStringSID()));

		cSecurityId sidEveryone(WinLocalSid);
		sm_pLog->addInfoF("WinLocalSid='%s'", LOGSTR(sidEveryone.GetStringSID()));

		cSecurityACL dacl1(sidEveryone, GENERIC_ALL);
		cSecurityAttributes sa1(false, dacl1);

		cSecurityAttribsWKS saNull(WinNullSid);	// anyone.
		cSecurityAttribsWKS sa2(WinBuiltinUsersSid);	// WinBuiltinPowerUsersSid

		// TODO: Create a file and assign it this privilege.
		cStringF sPathOut = cFilePath::CombineFilePathX(get_TestOutDir(), _FN(GRAY_NAMES) _FN("cSecurityAttributes") _FN(MIME_EXT_txt));
		hRes = cFile::DeletePath(sPathOut);

		cFile fileTest;
		hRes = fileTest.OpenCreate(sPathOut, OF_WRITE | OF_CREATE, sa1);
		UNITTEST_TRUE(SUCCEEDED(hRes));
		hRes = fileTest.WriteX("WinLocalSid\n", 5);
		UNITTEST_TRUE(SUCCEEDED(hRes));

		cSecurityAttributes sa3;
	}
};
UNITTEST_REGISTER(cSecurityAttributes, UNITTEST_LEVEL_Lib);
#endif

#endif // _WIN32