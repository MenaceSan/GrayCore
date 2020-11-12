//
//! @file cSystemHelper.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cSystemHelper_H
#define _INC_cSystemHelper_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cSystemInfo.h"
#include "cFilePath.h"

namespace Gray
{
	class GRAYCORE_LINK cSystemHelper : public cSingleton < cSystemHelper >
	{
		//! @class Gray::cSystemHelper
		//! The system as a whole. (as far as we can detect) not just the current running app/process or user login.
		//! The detected system params may be effected by system virtualization.

		friend class cSingleton < cSystemHelper >;

	protected:
		cStringF m_sSystemName;		//!< Cached (or overridden for debug purposes) system name.

	public:
		cSystemInfo& m_Info;	 

	public:
		cSystemHelper();

		cString get_OSInfoStr() const;
		cStringF get_SystemName();		//!< The node name of the machine.

		static cStringF GRAYCALL get_SystemDir();
	};
}

#endif
