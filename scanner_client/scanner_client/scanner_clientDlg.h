
// scanner_clientDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "json.h"
#include "flatbed_scanner.h"
#include "czmq.h"
#include "zmq.h"
#include "zmq.h"

// Cscanner_clientDlg �Ի���
class Cscanner_clientDlg : public CDialogEx
{
// ����
public:
	Cscanner_clientDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_SCANNER_CLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedButton1();
  CListBox messageInfo;
  afx_msg void OnBnClickedCloseCom();
  afx_msg void OnBnClickedEnable();
  afx_msg void OnBnClickedDisable();
  afx_msg void OnBnClickedGetBarcode();
  afx_msg void OnBnClickedReset();

  CTime m_time1;
  CString timeSystem;

};
