
// chatClientDlg.h : 头文件
//

#pragma once
#include "string"
#include "list"
#include "afxwin.h"
#include "fstream"
using namespace std;

enum Message_Type
{
	Message_Min,
	Message_Login,
	Message_LoginOut,
	Message_Chat,
	Message_ChatToMe,
	Message_ChatToOther,
	Message_File,
	Message_Max
};

struct Fileinfo{
	CString FileName;
	unsigned int FileSize;
};

// CchatClientDlg 对话框
class CchatClientDlg : public CDialogEx
{
// 构造
public:
	CchatClientDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_CHATCLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	bool Connect(SOCKET sock);
	bool ConnectServer();
	static void Receive(void* p);

	void SendToServer(Message_Type msgType,wstring str= L"");
	void RecvMessage(wstring bufStr);
	void UpdateMsg();

	list<wstring> m_listUser;

	list<wstring> m_listMsg;
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
	CString m_FilePath;
	CString m_FileName;
	CString m_FileSize;
	Fileinfo m_fileinfo;
public:
	CListBox m_listUserCtrl;
	afx_msg void OnBnClickedSendmsg();
	CEdit m_ChatText;
	afx_msg void OnBnClickedSendfile();
};
