#include "WellknownPlugin.h"
#include "Helper.h"
#include "SafeMemory.h"
#include "Decoder.h"
#include "magic_enum.hpp"
#include <RpcDefine.h>

CTextPlugin::CTextPlugin() :
	m_Registered(FALSE),
	m_Reference(1)
{
}

CTextPlugin::~CTextPlugin()
{
}

HRESULT STDMETHODCALLTYPE CTextPlugin::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
{
	if (!ppvObject)
		return E_INVALIDARG;

	*ppvObject = NULL;
	if (riid == IID_IUnknown)
	{
		*ppvObject = (LPVOID)this;
		AddRef();
		return NOERROR;
	}

	return E_NOINTERFACE;
}


ULONG STDMETHODCALLTYPE CTextPlugin::AddRef()
{
	m_Reference++;
	return m_Reference;
}

ULONG STDMETHODCALLTYPE CTextPlugin::Release()
{
	m_Reference--;
	if (m_Reference == 0)
	{
		delete this;
	}

	return m_Reference;
}


HRESULT STDMETHODCALLTYPE CTextPlugin::Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count)
{
	if (!Extensions || !Count)
		return E_INVALIDARG;

	static PCWSTR ExtensionList[] =
	{
		L"KSD",
		L"KDT",
		L"TXT",
		L"KS",
		L"CSV",
		L"FUNC",
		L"STAND",
		L"ASD",
		L"INI",
		L"TJS",
		NULL
	};

	m_Handle  = Handle;
	m_Proxyer = Proxyer;
	*Extensions = ExtensionList;
	*Count = _countof(ExtensionList) - 1;
	return NOERROR;
}


HRESULT STDMETHODCALLTYPE CTextPlugin::Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream)
{
	NTSTATUS     Status;
	STATSTG      Stat;
	ULONG        ReadSize;
	WCHAR        FileName[MAX_PATH];
	KrkrTextMode TextMode;
	
	TextMode = (KrkrTextMode)m_Proxyer->GetTextFlag();
	
	switch (TextMode)
	{
	case KrkrTextMode::TEXT_RAW:
		return E_ABORT;

	case KrkrTextMode::TEXT_DECODE:
		break;

	default:
		m_Proxyer->TellServerLogOutput(
			LogLevel::LOG_WARN,
			L"CTextPlugin::Unpack : Unknown flag = %d",
			TextMode
		);

		return E_FAIL;
	}

	Stream->Stat(&Stat, STATFLAG_DEFAULT);
	RtlZeroMemory(FileName, countof(FileName) * sizeof(WCHAR));
	lstrcpyW(FileName, FilePath);

	LOOP_ONCE
	{
		Status = STATUS_SUCCESS;
		auto OriBuffer = AllocateMemorySafeP<BYTE>(Stat.cbSize.LowPart);
		if (!OriBuffer)
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		Stream->Read(OriBuffer.get(), Stat.cbSize.LowPart, &ReadSize);

		Status = STATUS_SUCCESS;
		if (DecodeText(m_Proxyer, FileName, OriBuffer.get(), Stat.cbSize.LowPart) < 0)
			return E_FAIL;
	}

	return NtStatusToHResult(Status);
}

