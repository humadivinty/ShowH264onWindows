
// TestRTSPDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "TestRTSP.h"
#include "TestRTSPDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestRTSPDlg �Ի���



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
END_MESSAGE_MAP()


// CTestRTSPDlg ��Ϣ�������

BOOL CTestRTSPDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
    //  ִ�д˲���
    SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
    SetIcon(m_hIcon, FALSE);		// ����Сͼ��

    // TODO: �ڴ���Ӷ���ĳ�ʼ������
    Init();

    return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CTestRTSPDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // ���ڻ��Ƶ��豸������

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // ʹͼ���ڹ����������о���
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // ����ͼ��
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
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

// ���λس���ESC
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

// ��
void CTestRTSPDlg::OnBnClickedButtonOpen()
{
    OnBnClickedButtonClose();

    GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_CLOSE)->EnableWindow(TRUE);

    CWnd *pWnd = GetDlgItem(IDC_STATIC); // ȡ�ÿؼ���ָ��  
    HWND hwnd = pWnd->GetSafeHwnd(); // ȡ�ÿؼ��ľ��
    CString strUrl;
    m_UrlEdit.GetWindowText(strUrl);
    m_hPlayer = H264_Play(hwnd, strUrl);
}

// �ر�
void CTestRTSPDlg::OnBnClickedButtonClose()
{
    GetDlgItem(IDC_BUTTON_OPEN)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_CLOSE)->EnableWindow(FALSE);
 
    if (m_hPlayer != NULL) {
        H264_Destroy(m_hPlayer);
        m_hPlayer = NULL;
    }
}
