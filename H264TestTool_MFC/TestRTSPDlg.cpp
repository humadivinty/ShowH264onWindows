
// TestRTSPDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "TestRTSP.h"
#include "TestRTSPDlg.h"
#include "afxdialogex.h"
#include <iostream>
#include <fstream>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MAX_IMG_BUF_SIZE (20*1024*1024)

bool SaveFile(const char* fileName, void* buf, size_t bufSize);
// CTestRTSPDlg 对话框



CTestRTSPDlg::CTestRTSPDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(CTestRTSPDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestRTSPDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT1, m_UrlEdit);
}

BEGIN_MESSAGE_MAP(CTestRTSPDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_OPEN, &CTestRTSPDlg::OnBnClickedButtonOpen)
    ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CTestRTSPDlg::OnBnClickedButtonClose)
	ON_BN_CLICKED(IDC_BUTTON_Capture, &CTestRTSPDlg::OnBnClickedButtonCapture)
END_MESSAGE_MAP()


// CTestRTSPDlg 消息处理程序

BOOL CTestRTSPDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
    //  执行此操作
    SetIcon(m_hIcon, TRUE);			// 设置大图标
    SetIcon(m_hIcon, FALSE);		// 设置小图标

    // TODO: 在此添加额外的初始化代码
    Init();
	m_pImgBuf = new unsigned char[MAX_IMG_BUF_SIZE];

    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTestRTSPDlg::OnPaint()
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
HCURSOR CTestRTSPDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CTestRTSPDlg::Init()
{
    m_hPlayer = NULL;
    m_UrlEdit.SetWindowText("rtsp://172.18.222.1/h264ESVideoTest");

    GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_CLOSE)->EnableWindow(FALSE);
}

// 屏蔽回车和ESC
BOOL CTestRTSPDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        if (pMsg->wParam == VK_RETURN
            || pMsg->wParam == VK_ESCAPE)
        {
            return TRUE;
        }
    }
    return CDialog::PreTranslateMessage(pMsg);
}

// 打开
void CTestRTSPDlg::OnBnClickedButtonOpen()
{
    OnBnClickedButtonClose();

    GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_CLOSE)->EnableWindow(TRUE);

    CWnd *pWnd = GetDlgItem(IDC_STATIC); // 取得控件的指针  
    HWND hwnd = pWnd->GetSafeHwnd(); // 取得控件的句柄
    CString strUrl;
    m_UrlEdit.GetWindowText(strUrl);
    m_hPlayer = H264_Play(hwnd, strUrl);
}

// 关闭
void CTestRTSPDlg::OnBnClickedButtonClose()
{
    GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_CLOSE)->EnableWindow(FALSE);
 
    if (m_hPlayer != NULL) {
        H264_Destroy(m_hPlayer);
        m_hPlayer = NULL;
    }
}


void CTestRTSPDlg::OnBnClickedButtonCapture()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strLog;
	if (m_hPlayer != NULL)
	{
		int iBufSize = MAX_IMG_BUF_SIZE;
		if (m_pImgBuf)
		{
			memset(m_pImgBuf, 0, iBufSize);
		}		
		int iWidth = 0;
		int iHeight = 0;
		bool bRet = H264_GetOneBmpImg(m_hPlayer, m_pImgBuf, iBufSize, iWidth, iHeight);

		if (bRet)
		{
			SaveFile("Captrue.bmp", m_pImgBuf, iBufSize);
		}

		strLog.Format("H264_GetOneBmpImg = %d, image length= %d, width= %d, height= %d  ", bRet, iBufSize, iWidth, iHeight);
	}
	else
	{
		strLog.Format("H264 handle is NULL  ");
	}
	MessageBox(strLog);	
}

bool SaveFile(const char* fileName, void* buf, size_t bufSize)
{
	if (NULL == fileName
		|| NULL == buf
		|| 0 >= bufSize)
	{
		return false;
	}
	std::string filename(fileName);
	std::fstream fs(filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	if (!fs.is_open( ))
	{
		std::cout << "failed to open " << filename << '\n';
		return false;
	}
	else
	{
		fs.write(reinterpret_cast<char*>(buf), bufSize);
		fs.close();
		return true;
	}
}