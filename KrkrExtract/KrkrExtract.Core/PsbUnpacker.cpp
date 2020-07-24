#include "WellknownPlugin.h"
#include "Helper.h"
#include "magic_enum.hpp"
#include "Decoder.h"
#include <RpcDefine.h>



CPsbPlugin::CPsbPlugin() :
	m_Registered(FALSE),
	m_Reference(1)
{
}

CPsbPlugin::~CPsbPlugin()
{
	m_Proxyer = nullptr;
}


HRESULT STDMETHODCALLTYPE CPsbPlugin::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
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

ULONG STDMETHODCALLTYPE CPsbPlugin::AddRef()
{
	m_Reference++;
	return m_Reference;
}

ULONG STDMETHODCALLTYPE CPsbPlugin::Release()
{
	m_Reference--;
	if (m_Reference == 0)
	{
		delete this;
	}

	return m_Reference;
}

HRESULT STDMETHODCALLTYPE CPsbPlugin::Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count)
{
	if (!Extensions || !Count || !Proxyer)
		return E_INVALIDARG;

	static PCWSTR ExtensionList[] =
	{
		L"PSB",
		L"SCN",
		L"MTN",
		L"PIMG",
		NULL
	};

	m_Handle  = Handle;
	m_Proxyer = Proxyer;
	*Extensions = ExtensionList;
	*Count = _countof(ExtensionList) - 1;
	return NOERROR;
}

HRESULT STDMETHODCALLTYPE CPsbPlugin::Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream)
{
	NTSTATUS      Status;
	NtFileDisk    File;
	LARGE_INTEGER Offset;
	STATSTG       Stat;
	BOOL          MustAbort;
	ULONG         Binds, OldFlags, Flags;


	Binds           = 0;
	MustAbort       = FALSE;
	Offset.QuadPart = 0;
	Stream->Seek(Offset, SEEK_SET, NULL);
	Stream->Stat(&Stat, STATFLAG_DEFAULT);

	Binds = (ULONG)KrkrPsbMode::PSB_RAW |
		(ULONG)KrkrPsbMode::PSB_TEXT    |
		(ULONG)KrkrPsbMode::PSB_DECOM   |
		(ULONG)KrkrPsbMode::PSB_IMAGE   |
		(ULONG)KrkrPsbMode::PSB_ANM     |
		(ULONG)KrkrPsbMode::PSB_JSON;
	
	Flags    = (ULONG)m_Proxyer->GetPsbFlag();
	OldFlags = (ULONG)m_Proxyer->GetPsbFlag();
	CLEAR_FLAG(OldFlags, Binds);
	if (Flags != (ULONG)KrkrPsbMode::PSB_ALL && OldFlags != 0)
		return E_FAIL;

	if (FLAG_ON(Flags, (ULONG)KrkrPsbMode::PSB_RAW) || Flags == (ULONG)KrkrPsbMode::PSB_ALL)
		MustAbort = TRUE;

	if (FLAG_ON(Flags, (ULONG)KrkrPsbMode::PSB_TEXT)  || 
		FLAG_ON(Flags, (ULONG)KrkrPsbMode::PSB_DECOM) || 
		FLAG_ON(Flags, (ULONG)KrkrPsbMode::PSB_IMAGE) ||
		FLAG_ON(Flags, (ULONG)KrkrPsbMode::PSB_ANM))
	{
		Status = DumpPsbTjs2(
			m_Proxyer,
			Stream, 
			FLAG_ON(Flags, (ULONG)KrkrPsbMode::PSB_TEXT),
			FLAG_ON(Flags, (ULONG)KrkrPsbMode::PSB_IMAGE) || FLAG_ON(Flags, (ULONG)KrkrPsbMode::PSB_ANM),
			FLAG_ON(Flags, (ULONG)KrkrPsbMode::PSB_DECOM),
			GetFileNameExtension(wstring(FilePath)),
			FilePath
		);
	}

	if (FLAG_ON(Flags, (ULONG)KrkrPsbMode::PSB_JSON))
	{
		Status = DecompilePsbJson(
			m_Proxyer,
			Stream, 
			GetFileBasePath(wstring(FilePath)).c_str(), 
			ReplaceFileNameExtension(GetPackageName(wstring(FilePath)), L"").c_str()
		);
	}

	if (MustAbort)
		return E_ABORT;

	return NtStatusToHResult(Status);
}


