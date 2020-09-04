#include "TaskAutoUnlocker.h"
#include "KrkrExtract.h"

TaskAutoUnlocker::~TaskAutoUnlocker()
{
	KrkrExtractCore* Handle;

	Handle = (KrkrExtractCore*)m_Proxyer->GetPrivatePointer();
	if (m_Instance) 
	{
		Handle->TellServerProgressBar(NULL, 0, 0);
		Handle->TellServerTaskEndAndEnableUI(
			m_Instance->GetLastStatus(), 
			m_Instance->GetLastMessage().length() ? m_Instance->GetLastMessage().c_str() : NULL
		);
	}

	m_Instance = nullptr;
	m_Proxyer  = nullptr;
}
