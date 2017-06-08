// maindlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINDLG_H__EC6FDD0B_3DE4_4250_9D87_C96E6677A927__INCLUDED_)
#define AFX_MAINDLG_H__EC6FDD0B_3DE4_4250_9D87_C96E6677A927__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"

#define MakePtr(cast, base, offset) (cast)((DWORD_PTR)(base) + (DWORD_PTR)(offset))

BOOL WINAPI MyMessageBeep ( IN UINT uType);
HRESULT WriteProtectedMemory(LPVOID pDest, LPCVOID pSrc, DWORD dwSize);

class CMainDlg : public CDialogImpl<CMainDlg>
{
	UINT m_uType;

public:
	enum { IDD = IDD_MAINDLG };

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDCANCEL, OnClose)
		COMMAND_ID_HANDLER(IDC_METHOD0, OnMethod0)
		COMMAND_ID_HANDLER(IDC_METHOD1, OnMethod1)
		COMMAND_ID_HANDLER(IDC_METHOD2, OnMethod2)
		COMMAND_ID_HANDLER(IDC_METHOD3, OnMethod3)
		COMMAND_ID_HANDLER(IDC_METHOD4, OnMethod4)
		COMMAND_ID_HANDLER(IDC_METHOD5, OnMethod5)
		COMMAND_RANGE_HANDLER(IDC_BEEP, IDC_OK, OnType)
	END_MSG_MAP()

	CMainDlg() :
		m_uType(-1)
	{
	}

	LRESULT OnMethod0(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		return ::MessageBeep(m_uType);
	}

	LRESULT OnMethod1(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMethod2(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMethod3(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMethod4(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnMethod5(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnType(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		switch(wID)
		{
		case IDC_BEEP:
			m_uType = -1;
			break;
		case IDC_ASTERISK:
			m_uType = MB_ICONASTERISK;
			break;
		case IDC_EXCLAMATION:
			m_uType = MB_ICONEXCLAMATION;
			break;
		case IDC_HAND:
			m_uType = MB_ICONHAND;
			break;
		case IDC_QUESTION:
			m_uType = MB_ICONQUESTION;
			break;
		case IDC_OK:
			m_uType = MB_OK;
			break;
		}
		return 0;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// center the dialog on the screen
		CenterWindow();

		// set icons
		HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
		SetIcon(hIcon, TRUE);
		HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		SetIcon(hIconSmall, FALSE);

		CheckDlgButton(IDC_BEEP, BST_CHECKED);
		return TRUE;
	}

	LRESULT OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINDLG_H__EC6FDD0B_3DE4_4250_9D87_C96E6677A927__INCLUDED_)
