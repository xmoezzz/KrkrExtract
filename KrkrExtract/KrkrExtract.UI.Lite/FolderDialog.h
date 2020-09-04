#ifndef _FOLDERDIALOG_H_
#define _FOLDERDIALOG_H_

#include <my.h>

typedef struct
{
	UINT uMsg;
	WPARAM wParam;
	LPARAM lParam;
} TCtrlMsg;

class CFolderDialog
{
public:
	CFolderDialog(HWND hWnd = NULL);

public:
	int     DoModule(LPCWSTR pszTitle = NULL);
	int     GetPathName(WCHAR *szOutPath, DWORD dwSize);
	LPWSTR  GetPathName();
	VOID    SetDefaultFileName(LPCWSTR pszFileName);

protected:
	Void ClientToScreenRect(HWND hWnd, LPRECT lpRect);
	Void ScreenToClientRect(HWND hWnd, LPRECT lpRect);
	int  FolderDialogNT5(WCHAR *szOutPath, DWORD dwSize, LPCWSTR pszTitle = NULL);
	int  FolderDialogNT6(WCHAR *szOutPath, DWORD dwSize, LPCWSTR pszTitle = NULL);

	// message handlers
	HRESULT  OnInitDialog(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HRESULT  OnNotify(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR  DialogProcWorker(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	UINT_PTR OFNHookProcWorker(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

	static INT_PTR  CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

protected:
	int   m_iLength;
	HWND  m_hWnd, m_hWndDlg, m_hWndList, m_hWndCombo;
	LPOPENFILENAMEW m_pOFN;
	WNDPROC m_lpfnDialogProc;
	WCHAR m_szPath[MAX_PATH];
	LPCWSTR m_pszDefFileName;

	//    static CFolderDialog *m_This;
};

#endif /* _FOLDERDIALOG_H_ */

