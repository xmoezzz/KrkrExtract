#include "WellknownPlugin.h"
#include "Helper.h"
#include "magic_enum.hpp"
#include "Decoder.h"
#include <RpcDefine.h>

CPbdPlugin::CPbdPlugin() :
	m_Registered(FALSE),
	m_Reference(1)
{
}

CPbdPlugin::~CPbdPlugin()
{
	m_Proxyer = nullptr;
}


HRESULT STDMETHODCALLTYPE CPbdPlugin::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
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

ULONG STDMETHODCALLTYPE CPbdPlugin::AddRef()
{
	m_Reference++;
	return m_Reference;
}

ULONG STDMETHODCALLTYPE CPbdPlugin::Release()
{
	m_Reference--;
	if (m_Reference == 0)
	{
		delete this;
	}

	return m_Reference;
}

HRESULT STDMETHODCALLTYPE CPbdPlugin::Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count)
{
	if (!Extensions || !Count || !Proxyer)
		return E_INVALIDARG;

	static PCWSTR ExtensionList[] =
	{
		L"PBD",
		NULL
	};

	m_Handle  = Handle;
	m_Proxyer = nullptr;
	*Extensions = ExtensionList;
	*Count = _countof(ExtensionList) - 1;
	return NOERROR;
}

HRESULT STDMETHODCALLTYPE CPbdPlugin::Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream)
{
	NTSTATUS    Status;
	KrkrPbdMode PbdMode;
	wstring     FileName(FilePath);

	PbdMode = (KrkrPbdMode)m_Proxyer->GetPbdFlag();

	switch (PbdMode)
	{
	case KrkrPbdMode::PBD_RAW:
		return E_ABORT;

	case KrkrPbdMode::PBD_JSON:
		break;

	default:
		m_Proxyer->TellServerLogOutput(
			LogLevel::LOG_WARN,
			L"CPbdPlugin::Unpack : Unknown flag = %d",
			PbdMode
		);

		return E_FAIL;
	}

	ReplaceFileNameExtension(FileName, L".json");
	Status = SavePbd(GetPackageName(wstring(FilePath)).c_str(), FileName.c_str());
	
	return NtStatusToHResult(Status);
}

