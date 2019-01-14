//
//! @file CStreamProgress.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CStreamProgress_H
#define _INC_CStreamProgress_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "COSHandle.h"

namespace Gray
{
	template< typename TYPE = STREAM_POS_t >
	class CStreamProgressT
	{
		//! @class Gray::CStreamProgressT
		//! How much of some total has been processed?

	public:
		TYPE m_nAmount;		//!< How far the stream has progressed toward m_nTotal.
		TYPE m_nTotal;		//!< Total size of the stream. 0 = i have no idea how big the total is.

	public:
		CStreamProgressT(TYPE nAmount = 0, TYPE nTotal = 0)
		: m_nAmount(nAmount)
		, m_nTotal(nTotal)
		{
		}
		bool isComplete() const
		{
			if (m_nTotal == 0) // no idea.
				return true;
			if (m_nAmount >= m_nTotal)
				return true;
			return false;
		}
		float get_PercentFloat() const
		{
			//! Get percent of total.
			//! @return 0.0f to 1.0f
			if (m_nTotal == 0) // no idea.
				return 0;
			return (float)(((double)m_nAmount) / ((double)m_nTotal));
		}
		int get_PercentInt() const
		{
			//! @return From 0 to 100. MULDIV
			if (m_nTotal == 0) // no idea.
				return 0;
			return (int)((m_nAmount*100) / m_nTotal);
		}
		bool isValidPercent() const
		{
			if (m_nTotal <= 0)	// no idea.
				return false;
			return(m_nAmount <= m_nTotal);
		}
		void InitZero()
		{
			m_nAmount = 0;
			m_nTotal = 0;	// 0 = i have no idea how big the total is.
		}
	};

	typedef CStreamProgressT<STREAM_POS_t> CStreamProgress;

	class GRAYCORE_LINK CStreamProgressF
	{
		//! @class Gray::CStreamProgressF
		//! We are descending into nested tasks we have not fully measured.
		//! i.e. enumerating subdirectories i have not yet counted.
		//! TODO This can be a used as a time throbber. the task time is just an estimate. we should never actually reach it.

		friend class CStreamProgressChunk;
	private:
		// Stats
		float m_nTotal;			//!< Estimated Value of the directory we are processing. (1.0=total of all files)
		float m_nAmount;	//!< Current progress 0 to 1.0 (m_nTotal)

	public:
		CStreamProgressF()
		: m_nTotal(1.0f)
		, m_nAmount(0.0f)
		{}

		void InitPercent()
		{
			m_nTotal = 1.0f;	// of everything.
			m_nAmount = 0.0f;
		}
#if 0
		void put_PercentComplete( float fComplete = 1.0f )
		{
			m_nAmount = fComplete;	// indicate we are done.
		}
#endif
		float get_PercentComplete() const
		{
			return m_nAmount;	// return value <= 1.0f
		}
		float get_PercentChunk() const
		{
			return m_nTotal;
		}
	};

	class GRAYCORE_LINK CStreamProgressChunk
	{
		//! @class Gray::CStreamProgressChunk
		//! Track nested work load. Processing a tree.

	private:
		CStreamProgressF& m_Prog;
		CStreamProgressF m_ProgPrev;	//!< m_Prog at start.
		int m_iChunk;	//!< What chunk are we on ?
		int m_iChunks;	//!< how many chunks is this supposed to be ?

	public:
		CStreamProgressChunk(CStreamProgressF& prog, int iSubChunks, int iParentChunks = 1)
		: m_Prog(prog)
		, m_iChunk(0)
		, m_iChunks(iSubChunks)
		{
			//! Start a sub-chunk of the task. expect iChunks in this task. IncChunk() will be called.
			//! iParentChunks = the number of m_nTotal we represent.
			//! ASSUME caller will IncChunk(iParentChunks) after this is destructed.
			m_ProgPrev = m_Prog;
			ASSERT(m_iChunks >= 0);
			if (m_iChunks == 0)
			{
				prog.m_nTotal = 0;
			}
			else
			{
				prog.m_nTotal = (((float)iParentChunks) * prog.m_nTotal) / ((float)m_iChunks);	// MULDIV
			}
		}
		~CStreamProgressChunk()
		{
			//! complete the task.
			if (m_ProgPrev.m_nTotal >= 1.0f)
			{
				m_Prog.m_nTotal = m_Prog.m_nAmount = 1.0f;	// i have no parent. we are done.
			}
			else
			{
				m_Prog = m_ProgPrev;	// back out my changes and assume IncChunk() will be called from my parent.
			}
		}
		void IncChunk(int iChunks = 1)
		{
			//! We are making some progress at the current task.
			//! ASSERT(prog.m_nTotal);
			m_iChunk += iChunks;
			if (m_iChunk > m_iChunks)
			{
				// this really shouldn't happen!
				m_iChunk = m_iChunks;
			}
			m_Prog.m_nAmount += iChunks * m_Prog.m_nTotal;
		}
	};

	struct GRAYCORE_LINK DECLSPEC_NOVTABLE IStreamProgressCallback
	{
		//! @struct Gray::IStreamProgressCallback
		//! Abstract Base class. Get call backs indicating the overall progress of some action.
		//! Similar to .NET System.IProgress<T>
		//! This can be used as ICancellable with CThreadState. The caller may decide to cancel the function via the onProgressCallback return.

		IGNORE_WARN_ABSTRACT(IStreamProgressCallback);

		virtual HRESULT _stdcall onProgressCallback(const CStreamProgress& progress)
		{
			//! Some synchronous process is notifying us how far it has gone.
			//! @return
			//!  S_OK = just keep going
			//!  FAILED(hRes) = stop the action. e.g. HRESULT_WIN32_C(ERROR_CANCELED) = stop
			UNREFERENCED_REFERENCE(progress);
			return S_OK;	// S_OK=just keep going.
		}
	};
};

#endif
