
// TestRTSP.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CTestRTSPApp:
// �йش����ʵ�֣������ TestRTSP.cpp
//

class CTestRTSPApp : public CWinApp
{
public:
	CTestRTSPApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CTestRTSPApp theApp;