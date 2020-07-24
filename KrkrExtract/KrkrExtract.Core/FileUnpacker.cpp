#include "WellknownPlugin.h"
#include "KrkrExtract.h"
#include "Helper.h"

CFilePlugin::CFilePlugin() :
	m_Registered(FALSE),
	m_Reference(1)
{
}

CFilePlugin::~CFilePlugin()
{
	m_Proxyer = nullptr;
}


HRESULT STDMETHODCALLTYPE CFilePlugin::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
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

ULONG STDMETHODCALLTYPE CFilePlugin::AddRef()
{
	m_Reference++;
	return m_Reference;
}

ULONG STDMETHODCALLTYPE CFilePlugin::Release()
{
	m_Reference--;
	if (m_Reference == 0)
	{
		delete this;
	}

	return m_Reference;
}

HRESULT STDMETHODCALLTYPE CFilePlugin::Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count)
{
	if (!Extensions || !Count || !Proxyer)
		return E_INVALIDARG;

	static PCWSTR ExtensionList[] =
	{
		L"*",
		NULL
	};

	m_Handle  = Handle;
	m_Proxyer = Proxyer;
	*Extensions = ExtensionList;
	*Count = _countof(ExtensionList) - 1;
	return NOERROR;
}

HRESULT STDMETHODCALLTYPE CFilePlugin::Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream)
{
	NTSTATUS         Status;
	STATSTG          Stat;
	NtFileDisk       File;
	LARGE_INTEGER    Tranferred, WriteSize, TempSize, Offset;
	ULONG            ReadSize;
	KrkrExtractCore* Handle;

	static BYTE Buffer[1024 * 64];

	Offset.QuadPart = 0;
	Stream->Seek(Offset, FILE_BEGIN, NULL);
	Stream->Stat(&Stat, STATFLAG_DEFAULT);
	Tranferred.QuadPart = 0;

	Status = File.Create(FilePath);
	if (NT_FAILED(Status))
	{
		m_Proxyer->TellServerLogOutput(
			LogLevel::LOG_WARN, 
			L"File : Failed to create file [%s]%lx\n",
			FilePath,
			Status);

		return NtStatusToHResult(Status);
	}

	while (Tranferred.QuadPart < (LONG64)Stat.cbSize.QuadPart)
	{
		Stream->Read(&Buffer, sizeof(Buffer), &ReadSize);
		Tranferred.QuadPart += ReadSize;
		TempSize.QuadPart = 0;
		while (TempSize.QuadPart < ReadSize)
		{
			File.Write(Buffer, ReadSize, &WriteSize);
			TempSize.QuadPart += WriteSize.QuadPart;
		}
	}

	File.Close();
	return NtStatusToHResult(Status);
}


