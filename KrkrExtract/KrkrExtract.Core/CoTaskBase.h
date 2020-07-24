#pragma once

#include "NativeThread.h"
#include "ml.h"

class CoTaskBase : public NativeThread
{
public:
	wstring GetLastMessage() const
	{
		return m_LastMessage;
	}

	BOOL GetLastStatus() const
	{
		return m_LastStatus;
	}

	void SetLastMessage(BOOL Status, PCWSTR Message)
	{
		m_LastMessage = Message;
		m_LastStatus = Status;
	}

	void SetLastMessage(BOOL Status, const wstring& Message)
	{
		m_LastMessage = Message;
		m_LastStatus = Status;
	}

	virtual ~CoTaskBase() {};

protected:
	std::wstring                          m_LastMessage;
	BOOL                                  m_LastStatus = TRUE;
};

