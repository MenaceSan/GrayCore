//
//! @file cOSHandleSet.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cOSHandleSet_H
#define _INC_cOSHandleSet_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cOSHandle.h"
#include "cArray.h"

namespace Gray
{
	class GRAYCORE_LINK cOSHandleSet
	{
		//! @class Gray::cOSHandleSet
		//! A collection of cOSHandle
		//! Wait on any of a set of OS handles to be signaled.
		//! Similar to CNetSocketSet and ::select() especially for __linux__
		//! Similar to _WIN32 WaitForMultipleObjects(). MAX = MAXIMUM_WAIT_OBJECTS or FD_SETSIZE

	public:
		static const int k_nHandeMax =	//! not always the same as CNetSocketSet::k_nSocketSetSize
#ifdef _WIN32
		MAXIMUM_WAIT_OBJECTS
#else
		FD_SETSIZE
#endif
		;

	private:
#ifdef _WIN32
		cArrayVal<HANDLE> m_fds;	//!< Just an array of OS handles. like fd_set. but dynamic.
#elif defined(__linux__)
		HANDLE m_hHandleMax;		//!< Largest handle we have. <= FD_SETSIZE
		fd_set m_fds;				//!< array of FD_SETSIZE possible HANDLE(s). NOTE: sizeof(m_fds) varies/fixed with FD_SETSIZE
#else
#error NOOS
#endif

	private:
		void InitHandles() noexcept
		{
#ifdef __linux__
			m_hHandleMax = 0;
			FD_ZERO(&m_fds);
#endif
		}

	public:
		cOSHandleSet(void) noexcept
		{
			InitHandles();
		}
		cOSHandleSet(HANDLE h)
		{
			// a handle set with a single handle.
#ifdef __linux__			 
			m_hHandleMax = h;
			FD_ZERO(&fds);
			FD_SET(h, &m_fds);
#else
			InitHandles();
			AddHandle(h);
#endif
		}
		cOSHandleSet(const cOSHandleSet& nss)
		{
			InitHandles();
			SetCopy(nss);
		}
		~cOSHandleSet()
		{
		}

		void operator = (const cOSHandleSet& nss)
		{
			SetCopy(nss);
		}
		void SetCopy(const cOSHandleSet& nss)
		{
#ifdef __linux__
			m_hHandleMax = nss.m_hHandleMax;
			// FD_COPY(pfds,&m_fds);
			cMem::Copy(&m_fds,&nss.m_fds,sizeof(m_fds));
#else
			m_fds = nss.m_fds;
#endif
		}

		bool AddHandle(HANDLE h)
		{
			//! Add OSHandle to the set.
			//! @note can't add more than k_nHandeMax.
			//! FD_SETSIZE on __linux__. MAXIMUM_WAIT_OBJECTS on _WIN32.

			if (h == INVALID_HANDLE_VALUE)
				return false;
#ifdef __linux__
			if ( h > m_hHandleMax )
				m_hHandleMax = h;
			FD_SET(h,&m_fds);
#else
			// Only add up to
			if (h == HANDLE_NULL)	// cOSHandle
				return false;
			if (m_fds.GetSize() >= MAXIMUM_WAIT_OBJECTS)
				return false;
			m_fds.Add(h);
#endif
			return true;
		}
		void RemoveHandle(HANDLE h)
		{
			if (h == INVALID_HANDLE_VALUE)
				return;
#ifdef __linux__
			FD_CLR(h,&m_fds);
#else
			m_fds.RemoveArg(h);
#endif
		}
		void ClearHandles()
		{
#ifdef __linux__
			InitHandles();
#else
			m_fds.RemoveAll();
#endif
		}

		HRESULT WaitForObjects(TIMESYSD_t dwMilliseconds, bool bWaitForAll = false);
	};
};
#endif
