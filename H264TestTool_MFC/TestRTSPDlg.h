
// TestRTSPDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CTestRTSPDlg 对话框
class CTestRTSPDlg : public CDialogEx
{
    // 构造
public:
    CTestRTSPDlg(CWnd* pParent = NULL);	// 标准构造函数

    // 对话框数据
    enum { IDD = IDD_TESTRTSP_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


    // 实现
protected:
    HICON m_hIcon;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

private:
    HANDLE m_hPlayer; // 播放器

    void Init();

public:
    CEdit m_UrlEdit;
    afx_msg void OnBnClickedButtonOpen();
    afx_msg void OnBnClickedButtonClose();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};
