#include "FolderDialog.h"
#include <objbase.h>
#include <shobjidl.h>
#include <my.h>

//CFolderDialog* CFolderDialog::m_This = NULL;

CFolderDialog::CFolderDialog(HWND hWnd /* = NULL */)
{
	m_hWnd = hWnd;
	m_pszDefFileName = NULL;
}

int CFolderDialog::DoModule(LPCWSTR pszTitle /* = NULL */)
{
	HWND  hWnd;

	hWnd = GetFocus();
	if (hWnd)
		m_hWnd = hWnd;

	if (m_hWnd && IsWindowEnabled(m_hWnd))
		EnableWindow(m_hWnd, FALSE);

	if (LOBYTE(Nt_CurrentPeb()->OSMajorVersion) > 5)
	{
		m_iLength = FolderDialogNT6(m_szPath, countof(m_szPath), pszTitle);
	}
	else
	{
		m_iLength = FolderDialogNT5(m_szPath, countof(m_szPath), pszTitle);
	}

	if (m_hWnd)
	{
		EnableWindow(m_hWnd, TRUE);
		SetFocus(m_hWnd);
	}

	return m_iLength != 0;
}

UINT_PTR CFolderDialog::OFNHookProcWorker(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uiMsg)
	{
	case WM_INITDIALOG:
		OnInitDialog(hdlg, uiMsg, wParam, lParam);
		return TRUE;

	case WM_NOTIFY:
		OnNotify(hdlg, uiMsg, wParam, lParam);
		return TRUE;
	}

	return FALSE;
}

UINT_PTR CALLBACK CFolderDialog::OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	CFolderDialog *pThis;

	if (uiMsg == WM_INITDIALOG)
		SetWindowLongPtrW(hdlg, GWL_USERDATA, (LONG_PTR)((LPOPENFILENAMEW)lParam)->lCustData);

	pThis = (CFolderDialog *)GetWindowLongPtrW(hdlg, GWL_USERDATA);
	if (pThis == NULL)
		return FALSE;

	return pThis->OFNHookProcWorker(hdlg, uiMsg, wParam, lParam);
}

INT_PTR CFolderDialog::DialogProcWorker(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		if (wParam == MAKEWPARAM(IDOK, BN_CLICKED))
		{
			int iLength;
			LPWSTR lpPath;

			lpPath = m_pOFN->lpstrFile;
			SetWindowTextW(m_hWndCombo, L"");
			iLength = SendMessageW(m_hWndDlg, CDM_GETFILEPATH, m_pOFN->nMaxFile, (LPARAM)lpPath) - 1;
			if (lpPath[iLength] != '\\')
				*(LPDWORD)&lpPath[iLength] = '\\';
			EndDialog(hwndDlg, IDOK);
			return TRUE;
		}
		else if (wParam == MAKEWPARAM(IDCANCEL, BN_CLICKED))
		{
			EndDialog(m_hWndDlg, IDCANCEL);
			return TRUE;
		}
	}

	return CallWindowProcW(m_lpfnDialogProc, hwndDlg, uMsg, wParam, lParam);
}

INT_PTR CALLBACK CFolderDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CFolderDialog *pThis = (CFolderDialog *)GetWindowLongPtrW(hwndDlg, GWL_USERDATA);
	return pThis->DialogProcWorker(hwndDlg, uMsg, wParam, lParam);
}

HRESULT CFolderDialog::OnInitDialog(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(wParam);

	m_pOFN = (LPOPENFILENAMEW)lParam;
	m_hWndDlg = GetParent(hdlg);
	m_hWndList = GetDlgItem(m_hWndDlg, lst1);
	m_hWndCombo = GetDlgItem(m_hWndDlg, cmb13);
	m_lpfnDialogProc = (WNDPROC)SetWindowLongPtrW(m_hWndDlg, GWL_WNDPROC, (LONG_PTR)DialogProc);
	SetWindowLongPtrW(m_hWndDlg, GWL_USERDATA, (LONG_PTR)this);

	return ERROR_SUCCESS;
}

HRESULT CFolderDialog::OnNotify(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int idCtrl;
	LPOFNOTIFYEX pNotify;

	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(hdlg);

	idCtrl = wParam;
	pNotify = (LPOFNOTIFYEX)lParam;

	switch (pNotify->hdr.code)
	{
	case CDN_SELCHANGE:
	{
		int   iLength;
		WCHAR szPath[MAX_PATH];

		iLength = SendMessageW(m_hWndDlg, CDM_GETFILEPATH, countof(szPath), (LPARAM)szPath);
		if (iLength < 0)
			break;

		while (szPath[--iLength] != '\\');
		SetWindowTextW(m_hWndCombo, szPath + iLength + 1);
	}
	break;

	case CDN_INITDONE:
	{
		HDC   hDC;
		POINT pt;
		RECT  rc1, rc2;
		SIZE  size;
		HWND  hWndCtrl;
		union { HWND hWndOK, hWndCancel, hWndCombo, hWndLabel; } UhWnd;
		TCtrlMsg CtrlMsg[] =
		{
			{ CDM_SETCONTROLTEXT, stc3, (LPARAM)L"Folder:"        },
			{ CDM_SETCONTROLTEXT, IDOK, (LPARAM)L"&Select Folder" },
			{ CDM_HIDECONTROL,    cmb1, 0                         },
			{ CDM_HIDECONTROL,    stc2, 0                         },
		};

		for (UINT i = 0; i != countof(CtrlMsg); ++i)
		{
			SendMessageW(m_hWndDlg, CtrlMsg[i].uMsg, CtrlMsg[i].wParam, CtrlMsg[i].lParam);
		}

		hDC = GetDC(m_hWndDlg);

		// move the OK button to the left of the Cancel Button
		UhWnd.hWndCancel = GetDlgItem(m_hWndDlg, IDCANCEL);
		GetWindowRect(UhWnd.hWndCancel, &rc1);
		UhWnd.hWndOK = GetDlgItem(m_hWndDlg, IDOK);
		GetWindowRect(UhWnd.hWndOK, &rc2);
		GetTextExtentPoint32W(hDC, (LPCWSTR)CtrlMsg[1].lParam, lstrlenW((LPCWSTR)CtrlMsg[1].lParam), &size);
		rc2.right = rc2.left + size.cx;
		pt.x = rc1.left - 10 - (rc2.right - rc2.left);
		pt.y = rc1.top;
		ScreenToClient(m_hWndDlg, &pt);
		SetWindowPos(UhWnd.hWndOK, NULL, pt.x, pt.y, rc2.right - rc2.left, rc2.bottom - rc2.top, 0);

		// stretch the cmb13 to fill the blank
		UhWnd.hWndCombo = GetDlgItem(m_hWndDlg, cmb13);
		hWndCtrl = GetDlgItem(m_hWndDlg, stc3);
		GetWindowRect(UhWnd.hWndCombo, &rc2);
		pt.x = rc1.right - rc2.left;
		pt.y = rc2.bottom - rc2.top;
		GetWindowRect(hWndCtrl, &rc1);

		GetTextExtentPoint32W(hDC, (LPCWSTR)CtrlMsg[0].lParam, lstrlenW((LPCWSTR)CtrlMsg[0].lParam), &size);
		rc1.right = rc1.left + size.cx;
		pt.x += rc2.left - rc1.right - 10;
		ScreenToClientRect(m_hWndDlg, &rc1);
		ScreenToClientRect(m_hWndDlg, &rc2);

		SetWindowPos(hWndCtrl, NULL, 0, 0, rc1.right - rc1.left, rc1.bottom - rc1.top, SWP_NOMOVE);
		SetWindowPos(UhWnd.hWndCombo, NULL, rc1.right, rc2.top, pt.x, pt.y, 0);

		ReleaseDC(m_hWndDlg, hDC);

		SetFocus(m_hWndList);
	}
	break;
	}

	return ERROR_SUCCESS;
}

int CFolderDialog::FolderDialogNT5(WCHAR *szOutPath, DWORD dwSize, LPCWSTR pszTitle /* = NULL */)
{
	//    WCHAR szPath[MAX_PATH];
	OPENFILENAMEW ofn;

	//    GetExecuteDirectoryW(szPath, countof(szPath));

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrTitle = pszTitle;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrFilter = L"/\0/\0";
	ofn.lpstrFile = szOutPath;
	ofn.nMaxFile = dwSize;
	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_ENABLESIZING | OFN_ENABLEHOOK | OFN_ENABLEINCLUDENOTIFY | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpfnHook = OFNHookProc;
	ofn.lCustData = (LPARAM)this;

	return GetOpenFileNameW(&ofn) ? lstrlenW(szOutPath) + 1 : 0;
}

int CFolderDialog::FolderDialogNT6(WCHAR *szOutPath, DWORD dwSize, LPCWSTR pszTitle /* = NULL */)
{
	if (FAILED(CoInitialize(0)))
		return -1;

	int iLength;
	WCHAR *ppszFile;
	HRESULT hr;
	IShellItem *pShellItem;
	IFileDialog *pfd;

	pfd = NULL;
	pShellItem = NULL;
	iLength = 0;

	LOOP_ONCE
	{
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, 0x17, IID_PPV_ARGS(&pfd));
		if (FAILED(hr))
			break;

		if (pszTitle != NULL)
			pfd->SetTitle(pszTitle);

		if (m_pszDefFileName != NULL)
			pfd->SetFileName(m_pszDefFileName);
		pfd->SetOptions(FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST | FOS_PICKFOLDERS);
		pfd->Show(m_hWnd);

		hr = pfd->GetResult(&pShellItem);
		if (FAILED(hr))
			break;

		hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &ppszFile);
		if (FAILED(hr))
			break;

		iLength = lstrlenW(ppszFile) + 1;
		iLength = min(iLength, (int)dwSize);
		lstrcpynW(szOutPath, ppszFile, iLength);

		CoTaskMemFree(ppszFile);
	}

	if (pShellItem) pShellItem->Release();
	if (pfd) pfd->Release();

	CoUninitialize();

	return iLength;
}

VOID CFolderDialog::SetDefaultFileName(LPCWSTR pszFileName)
{
	m_pszDefFileName = pszFileName;
}

int CFolderDialog::GetPathName(WCHAR *szOutPath, DWORD dwSize)
{
	if (m_iLength != -1)
	{
		CopyMemory(szOutPath, m_szPath, min((int)dwSize, m_iLength) * sizeof(WCHAR));
	}
	return m_iLength;
}

LPWSTR CFolderDialog::GetPathName()
{
	return m_szPath;
}

Void CFolderDialog::ClientToScreenRect(HWND hWnd, LPRECT lpRect)
{
	ClientToScreen(hWnd, (LPPOINT)&lpRect->left);
	ClientToScreen(hWnd, (LPPOINT)&lpRect->right);
}

Void CFolderDialog::ScreenToClientRect(HWND hWnd, LPRECT lpRect)
{
	ScreenToClient(hWnd, (LPPOINT)&lpRect->left);
	ScreenToClient(hWnd, (LPPOINT)&lpRect->right);
}
