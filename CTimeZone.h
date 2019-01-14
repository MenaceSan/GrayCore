//
//! @file CTimeZone.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CTimeZone_H
#define _INC_CTimeZone_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CTimeUnits.h"
#include "CUnitTestDecl.h"
#include "StrConst.h"

//UNITTEST_PREDEF(CTimeZoneMgr);

namespace Gray
{
	enum TZ_DSTRULE_TYPE
	{
		//! @enum Gray::TZ_DSTRULE_TYPE
		//! Enumerate the DST rules that might exist in the world.
		//! Try to be historically accurate. May or may not use DST.
		//! http://www.worldtimezone.com/daylight.html
		TZ_DSTRULE_NONE = 0,		//!< don't use DST at all. UTC.
		TZ_DSTRULE_AMERICAN,		//!< use the American rules for DST.
	};

	struct CTimeZone
	{
		//! @struct Gray::CTimeZone
		//! TimeZone/DST offset rules for a political region by name.
		//! Similar to _WIN32 GetTimeZoneInformation(TIME_ZONE_INFORMATION)

		const GChar_t* m_pszTimeZoneName;	//!< Short Name. EST, PST, etc.
		const GChar_t* m_pszTimeZoneDesc;	//!< Long name and description.

		TZ_TYPE m_nTimeZoneOffset;		//!< offset from UTC/GMT in minutes. Pure geography, NOT DST.
		TZ_DSTRULE_TYPE m_eDSTRule;		//!< does it have/use a DST calculation? needs CTimeUnits.
	};

	class GRAYCORE_LINK CTimeZoneMgr // : public CSingleton<CTimeZoneMgr>
	{
		//! @class Gray::CTimeZoneMgr
		//! Manage the collection of time zones. We need to make this configurable since it may change over time.
		//! @todo Manage dynamic list of TZ from file or db.

	private:
		static bool sm_bInitTimeZoneSet;			//!< Have i called tzset() ? So I know this computers local time zone.

	public:
		static const CTimeZone k_TimeZones[];	//!< Fixed/Default array of world time zones. terminated by name = nullptr;

	public:
		static TZ_TYPE GRAYCALL GetLocalTimeZoneOffset();

		static const CTimeZone* GRAYCALL FindTimeZone(TZ_TYPE nTimeZoneOffset);
		static const CTimeZone* GRAYCALL FindTimeZone(const GChar_t* pszName);
		static const CTimeZone* GRAYCALL FindTimeZoneHead(const GChar_t* pszName);

		//UNITTEST_FRIEND(CTimeZoneMgr);
	};


}

#endif
