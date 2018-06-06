
// TestRTSPDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CTestRTSPDlg �Ի���
class CTestRTSPDlg : public CDialogEx
{
    // ����
public:
    CTestRTSPDlg(CWnd* pParent = NULL);	// ��׼���캯��

    // �Ի�������
    enum { IDD = IDD_TESTRTSP_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


    // ʵ��
protected:
    HICON m_hIcon;

    // ���ɵ���Ϣӳ�亯��
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

private:
    HANDLE m_hPlayer; // ������

    void Init();

public:
    CEdit m_UrlEdit;
    afx_msg void OnBnClickedButtonOpen();
    afx_msg void OnBnClickedButtonClose();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};
