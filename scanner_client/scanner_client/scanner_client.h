
// scanner_client.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Cscanner_clientApp: 
// �йش����ʵ�֣������ scanner_client.cpp
//

class Cscanner_clientApp : public CWinApp
{
public:
	Cscanner_clientApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Cscanner_clientApp theApp;