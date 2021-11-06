#include "UIViewer.h"

UIViewer::UIViewer()
{
	m_MinSize.cx = 0;
	m_MinSize.cy = 0;
	m_LastWindowSize = m_MinSize;
}

BOOL UIViewer::ActiveUIViewer(PVOID DllModule, ClientStub* Client, ServerStub** Server)
{
	HRESULT Success;

	if (InterlockedCompareExchange((PULONG)&m_ViewerServer, 0xffffffff, 0) != 0)
		return TRUE;

	if (!DllModule || !m_CurrentModule || DllModule == m_CurrentModule)
		return FALSE;

	if (Client == NULL || Server == NULL)
		return FALSE;

	m_ViewerServer     = new UIViewerServer(this);
	m_ClientCallback   = Client;
	m_KrkrModule       = DllModule;

	Success = CoInitialize(NULL);
	if (FAILED(Success)) {
		PrintConsoleW(L"UIViewer::ActiveUIViewer : CoInitialize failed, %08x\n", Success);
	}
	
	Success = CoCreateInstance(
		CLSID_TaskbarList, 
		NULL, 
		CLSCTX_INPROC_SERVER,
		IID_ITaskbarList3, 
		reinterpret_cast<void**>(&m_Taskbar)
	);

	if (SUCCEEDED(Success))
	{
		Success = m_Taskbar->HrInit();
		if (FAILED(Success))
		{
			m_Taskbar->Release();
			m_Taskbar = nullptr;
		}
	}

	using GetKrkrExtractVersionFunc = PCWSTR(NTAPI*) ();
	auto GetKrkrExtractVersion = (GetKrkrExtractVersionFunc)Nt_GetProcAddress(m_KrkrModule, "GetKrkrExtractVersion");
	if (!GetKrkrExtractVersion) {
		m_Version = L"Unknown";
	}
	else {
		m_Version = GetKrkrExtractVersion();
	}

	*Server = m_ViewerServer;
	return TRUE;
}


BOOL UIViewer::NotifyDllLoad(PVOID Module)
{
	if (!Module)
		return FALSE;

	m_CurrentModule = Module;
	return TRUE;
}



