
// scanner_clientDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "json.h"
#include "flatbed_scanner.h"
#include "czmq.h"
#include "zmq.h"
#include "zmq.h"

// Cscanner_clientDlg 对话框
class Cscanner_clientDlg : public CDialogEx
{
// 构造
public:
	Cscanner_clientDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_SCANNER_CLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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
