
// scanner_clientDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "scanner_client.h"
#include "scanner_clientDlg.h"
#include "afxdialogex.h"
#include "mdcliapi.h"

#define CLIENT_ADDR "tcp://localhost:5555" //"tcp://localhost:5555"

mdcli_t *session;
mdcli_asyn_t *session_asyn;

void *controller;

zmq_thread_fn(asyn_recv);
zmq_thread_fn(subscrip_recv);

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Cscanner_clientDlg 对话框



Cscanner_clientDlg::Cscanner_clientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Cscanner_clientDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cscanner_clientDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_LIST1, messageInfo);
}

BEGIN_MESSAGE_MAP(Cscanner_clientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
  ON_BN_CLICKED(IDC_BUTTON1, &Cscanner_clientDlg::OnBnClickedButton1)
  ON_BN_CLICKED(IDC_CLOSE_COM, &Cscanner_clientDlg::OnBnClickedCloseCom)
  ON_BN_CLICKED(IDC_ENABLE, &Cscanner_clientDlg::OnBnClickedEnable)
  ON_BN_CLICKED(IDC_DISABLE, &Cscanner_clientDlg::OnBnClickedDisable)
  ON_BN_CLICKED(IDC_GET_BARCODE, &Cscanner_clientDlg::OnBnClickedGetBarcode)
  ON_BN_CLICKED(IDC_RESET, &Cscanner_clientDlg::OnBnClickedReset)
END_MESSAGE_MAP()


// Cscanner_clientDlg 消息处理程序

BOOL Cscanner_clientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码
  session_asyn = mdcli_asyn_new(CLIENT_ADDR, 0);
  zmq_threadstart(asyn_recv, this);

  //订阅发布=================
  void *context = zmq_ctx_new();
  controller = zmq_socket(context, ZMQ_SUB);
  zmq_connect(controller, "tcp://localhost:5559");
  zsock_set_subscribe(controller, "A");
  zmq_threadstart(subscrip_recv, this);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void Cscanner_clientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Cscanner_clientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR Cscanner_clientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void timeInfo(Cscanner_clientDlg *mThis, char *str)
{
  CString strInfo;
  mThis->m_time1 = CTime::GetCurrentTime();
  mThis->timeSystem = mThis->m_time1.Format(_T("%Y-%m-%d %H:%M:%S"));
  strInfo.Format(_T("%s: %s"), mThis->timeSystem, str);
  mThis->messageInfo.AddString(strInfo);
}

//===========异步接收线程============//
void asyn_recv(void *argc)
{
  Cscanner_clientDlg *mThis = (Cscanner_clientDlg *)argc;
  while (true) {
    zmsg_t *reply = mdcli_asyn_recv(session_asyn);
    if (reply) {
      zframe_t *data = zmsg_last(reply);
      char *strMessage = zframe_strdup(data);
      if (strMessage == NULL) {
        continue;
      }
      timeInfo(mThis, strMessage);
      free(strMessage);
      zmq_ctx_destroy(&reply);
      continue;
    }
    else
      continue;
  }
}

//===========订阅接收线程============//
void subscrip_recv(void *argc)
{
  Cscanner_clientDlg *mThis = (Cscanner_clientDlg *)argc;
  while (true) {
    zmsg_t *msg = zmsg_recv(controller);
    zframe_t *category = zmsg_last(msg);
    char *data = zframe_strdup(category);
    if (data == NULL) {
      continue;
    }
    timeInfo(mThis, data);
    free(data);
  }
}

void send_server(std::string str, Cscanner_clientDlg *mThis)
{

  zmsg_t *request = zmsg_new();
  zmsg_pushstr(request,str.c_str());
 
  int ret = mdcli_asyn_send(session_asyn, "echo", &request);
  if (ret != 0) {
    timeInfo(mThis, "异步发送失败\n");
    return;
  }
  
#if 0
  zmsg_t *request = zmsg_new();
  zmsg_pushstr(request, str.c_str());
  session = mdcli_new(CLIENT_ADDR, 0);
  zmsg_t *reply = mdcli_send(session, "echo", &request);
  if (reply == NULL) {
    timeInfo(mThis, "reply is NULL");
    return;
  }
  zframe_t *data = zmsg_last(reply);
  char *strMessage = zframe_strdup(data);
  timeInfo(mThis, strMessage);
  free(strMessage);
#endif
}

void Cscanner_clientDlg::OnBnClickedButton1()
{
  Json::Value root; 
  Json::FastWriter write;

  root["cmd"] = "open";
  root["device"] = "com1";
  std::string str = write.write(root);
  send_server(str, this);
}


void Cscanner_clientDlg::OnBnClickedCloseCom()
{
  Json::Value root;
  Json::FastWriter write;

  root["cmd"] = "close";
  std::string str = write.write(root);
  send_server(str, this);
}


void Cscanner_clientDlg::OnBnClickedEnable()
{
  Json::Value root;
  Json::FastWriter write;

  root["cmd"] = "enable";
  std::string str = write.write(root);
  send_server(str, this);
}


void Cscanner_clientDlg::OnBnClickedDisable()
{
  Json::Value root;
  Json::FastWriter write;

  root["cmd"] = "disable";
  std::string str = write.write(root);
  send_server(str, this);
}


void Cscanner_clientDlg::OnBnClickedGetBarcode()
{
  Json::Value root;
  Json::FastWriter write;

  root["cmd"] = "get_barcode";
  std::string str = write.write(root);
  send_server(str, this);
}


void Cscanner_clientDlg::OnBnClickedReset()
{
  Json::Value root;
  Json::FastWriter write;

  root["cmd"] = "reset";
  std::string str = write.write(root);
  send_server(str, this);
}
