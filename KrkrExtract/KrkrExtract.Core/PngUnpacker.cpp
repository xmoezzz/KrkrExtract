#include "WellknownPlugin.h"
#include "Helper.h"
#include "SafeMemory.h"
#include "Decoder.h"
#include "magic_enum.hpp"
#include <RpcDefine.h>


CPngPlugin::CPngPlugin() :
	m_Registered(FALSE),
	m_Reference(1)
{
}

CPngPlugin::~CPngPlugin()
{
	m_Proxyer = nullptr;
}

HRESULT STDMETHODCALLTYPE CPngPlugin::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
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


ULONG STDMETHODCALLTYPE CPngPlugin::AddRef()
{
	m_Reference++;
	return m_Reference;
}

ULONG STDMETHODCALLTYPE CPngPlugin::Release()
{
	m_Reference--;
	if (m_Reference == 0)
	{
		delete this;
	}

	return m_Reference;
}


HRESULT STDMETHODCALLTYPE CPngPlugin::Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count)
{
	if (!Extensions || !Count || !Proxyer)
		return E_INVALIDARG;

	static PCWSTR ExtensionList[] =
	{
		L"PNG",
		NULL
	};

	m_Handle  = Handle;
	m_Proxyer = Proxyer;
	*Extensions = ExtensionList;
	*Count = _countof(ExtensionList) - 1;
	return NOERROR;
}


HRESULT STDMETHODCALLTYPE CPngPlugin::Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream)
{
	NTSTATUS         Status;
	NtFileDisk       File;
	KrkrPngMode      PngMode;
	
	PngMode = (KrkrPngMode)m_Proxyer->GetPngFlag();
	if (PngMode == KrkrPngMode::PNG_RAW)
		return E_ABORT;

	switch (PngMode)
	{
	case KrkrPngMode::PNG_RAW:
	case KrkrPngMode::PNG_SYS:
		break;

	default:
		m_Proxyer->TellServerLogOutput(
			LogLevel::LOG_WARN,
			L"CPngPlugin::Unpack : Unknown flag = %d",
			PngMode
		);

		return E_FAIL;
	}

	switch (PngMode)
	{
	case KrkrPngMode::PNG_SYS:
		Status = SavePng(GetPackageName(FilePath).c_str(), FormatPathFull(FilePath).c_str());
		return NtStatusToHResult(Status);
	}

	//
	// should never reach here
	//

	return E_ABORT;
}

