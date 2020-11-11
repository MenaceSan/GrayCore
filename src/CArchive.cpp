//
//! @file cArchive.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cArchive.h"

namespace Gray
{
	HRESULT cArchive::Serialize(void* pData, size_t nSize)
	{
		//! @return <0 = error HRESULT_WIN32_C(ERROR_IO_INCOMPLETE)
		HRESULT hRes;
		if (IsStoring())
		{
			hRes = ref_Out().WriteT(pData, nSize);
		}
		else
		{
			hRes = ref_Inp().ReadT(pData, nSize);
		}
		return hRes;
	}

	HRESULT cArchive::SerializeSize(size_t& nSize)
	{
		//! Write a compressed size. high bit of byte is reserved to say there is more to come.
		//! bytes stored low to high (of course)
		//! MFC calls this "Count"
		if (IsStoring())
		{
			return ref_Out().WriteSize(nSize);
		}
		else
		{
			return ref_Inp().ReadSize(nSize);
		}
	}
}

//*************************************************************************
#if USE_UNITTESTS
#include "cUnitTest.h"
#include "cString.h"
#include "cStreamQueue.h"
#include "cLogMgr.h"

namespace Gray
{
	class cUnitTestArchive1
	{
		// A class with a bunch of types to serialize.
	public:
		size_t m_nSize;
		int m_i1;
		int m_i2;
		UINT m_u1;
		UINT64 m_u2;
		cStringA m_s1;
		cStringW m_s2;
		cString m_s3;
		// cArrayVal

	public:
		cUnitTestArchive1()
			: m_nSize(654321)
			, m_i1(1)
			, m_i2(22)
			, m_u1(333)
			, m_u2(444444)
			, m_s1("test")
			, m_s2(L"Junk Test String")
			, m_s3(cUnitTestCur::k_sTextBlob.get_CPtr())
		{
		}

		void SetZero()
		{
			m_nSize = 0;
			m_i1 = 0;
			m_i2 = 0;
			m_u1 = 0;
			m_u2 = 0;
			m_s1.SetErase();
			m_s2.SetErase(); 
			m_s3.SetErase();
		}
	
		HRESULT Serialize(cArchive& a)
		{
			// Read or write all members of this structure.
			HRESULT hRes;
			hRes = a.SerializeSize(m_nSize);
			hRes = a.Serialize(m_i1);
			hRes = a.Serialize(m_i2);
			hRes = a.Serialize(m_u1);
			hRes = a.Serialize(m_u2);
			hRes = m_s1.Serialize(a);
			hRes = m_s2.Serialize(a);
			hRes = m_s3.Serialize(a);
			return hRes;
		}
		bool IsEqual(const cUnitTestArchive1& x) const
		{
			if (x.m_nSize != m_nSize)
				return false;
			if (x.m_i1 != m_i1)
				return false;
			if (x.m_i2 != m_i2)
				return false;
			if (x.m_u1 != m_u1)
				return false;
			if (x.m_u2 != m_u2)
				return false;
			if (x.m_s1 != m_s1)
				return false;
			if (x.m_s2 != m_s2)
				return false;
			if (x.m_s3 != m_s3)
				return false;
			return true;
		}
	};
};

UNITTEST_CLASS(cArchive)
{
	UNITTEST_METHOD(cArchive)
	{
		cStreamQueue q1;
		cArchive awrite(q1, true);	// write

		cUnitTestArchive1 t1;
		UNITTEST_TRUE(t1.IsEqual(t1));
		HRESULT hRes = t1.Serialize(awrite);
		UNITTEST_TRUE(SUCCEEDED(hRes));

		size_t nQSize = q1.get_ReadQty();
		const void* pQData = q1.get_ReadPtr();

		cArchive aread(q1, false); // read it back.

		cUnitTestArchive1 t2;
		t2.SetZero();
		UNITTEST_TRUE(!t1.IsEqual(t2));

		hRes = t2.Serialize(aread);
		UNITTEST_TRUE(SUCCEEDED(hRes));
		UNITTEST_TRUE(t1.IsEqual(t2));

		cStreamQueue q2;
		cArchive awrite2(q2, true);	// write it again
		hRes = t2.Serialize(awrite2);
		UNITTEST_TRUE(SUCCEEDED(hRes));

		// it is the same?
		UNITTEST_TRUE(q2.get_ReadQty() == (int)nQSize);
		const void* pQData2 = q2.get_ReadPtr();
		UNITTEST_TRUE(!cMem::Compare(pQData2, pQData, nQSize));

	}
};
UNITTEST_REGISTER(cArchive, UNITTEST_LEVEL_Core);
#endif
