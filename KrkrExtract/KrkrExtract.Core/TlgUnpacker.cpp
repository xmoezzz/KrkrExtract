#include "WellknownPlugin.h"
#include "Decoder.h"
#include "magic_enum.hpp"
#include "SafeMemory.h"
#include "Helper.h"
#include "KrkrExtract.h"
#include <RpcDefine.h>

CTlgPlugin::CTlgPlugin() :
	m_Registered(FALSE),
	m_Reference(1)
{
}

CTlgPlugin::~CTlgPlugin()
{
	m_Proxyer = nullptr;
}

HRESULT STDMETHODCALLTYPE CTlgPlugin::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject)
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


ULONG STDMETHODCALLTYPE CTlgPlugin::AddRef()
{
	m_Reference++;
	return m_Reference;
}

ULONG STDMETHODCALLTYPE CTlgPlugin::Release()
{
	m_Reference--;
	if (m_Reference == 0)
	{
		delete this;
	}

	return m_Reference;
}


HRESULT STDMETHODCALLTYPE CTlgPlugin::Bind(_In_ KrkrClientProxyer* Proxyer, _In_ HANDLE Handle, _Out_ PCWSTR** Extensions, _Out_ ULONG* Count)
{
	if (!Extensions || !Count)
		return E_INVALIDARG;

	static PCWSTR ExtensionList[] =
	{
		L"TLG",
		NULL
	};

	m_Handle  = Handle;
	m_Proxyer = Proxyer;
	*Extensions = ExtensionList;
	*Count = _countof(ExtensionList) - 1;
	return NOERROR;
}


NTSTATUS DecodeTlgToBitmap(std::shared_ptr<BYTE> Buffer, ULONG Size, std::shared_ptr<BYTE>& BitmapBuffer, ULONG& BitmapSize)
{
	PBYTE    ActBuffer;
	ULONG    Signature;
	NTSTATUS Status;

	BitmapSize = 0;
	ActBuffer  = Buffer.get();

	if (Size < 6)
		return STATUS_BUFFER_TOO_SMALL;

	Status = STATUS_SUCCESS;
	
Label_Redo:
	Signature = *((PULONG)ActBuffer);
	switch (Signature)
	{
	case KRKR2_TLG0_TAG:
		if (Size <= 0xF)
			return STATUS_BUFFER_TOO_SMALL;

		if (RtlCompareMemory(ActBuffer, KRKR2_TLG0_MAGIC, CONST_STRLEN(KRKR2_TLG0_MAGIC)) != CONST_STRLEN(KRKR2_TLG0_MAGIC))
			return STATUS_UNSUCCESSFUL;

		ActBuffer += 0xF;
		Size -= 0xF;
		goto Label_Redo;

	case KRKR2_TLG5_TAG:
		if (RtlCompareMemory(ActBuffer, KRKR2_TLG5_MAGIC, CONST_STRLEN(KRKR2_TLG5_MAGIC)) != CONST_STRLEN(KRKR2_TLG5_MAGIC))
			return STATUS_UNSUCCESSFUL;

		Status = DecodeTLG5(ActBuffer, Size, BitmapBuffer, BitmapSize);
		break;

	case KRKR2_TLG6_TAG:
		if (RtlCompareMemory(ActBuffer, KRKR2_TLG6_MAGIC, CONST_STRLEN(KRKR2_TLG6_MAGIC)) != CONST_STRLEN(KRKR2_TLG6_MAGIC))
			return STATUS_UNSUCCESSFUL;

		Status = DecodeTLG6(ActBuffer, Size, BitmapBuffer, BitmapSize);
		break;
	}
	
	return Status;
}


HRESULT STDMETHODCALLTYPE CTlgPlugin::Unpack(_In_ PCWSTR FilePath, _In_ IStream* Stream)
{
	NTSTATUS         Status;
	STATSTG          Stat;
	ULONG            ReadSize, BitmapSize;
	LARGE_INTEGER    Offset;
	PBYTE            OutBuffer;
	NtFileDisk       File;
	KrkrTlgMode      TlgMode;
	KrkrExtractCore* Handle;
	wstring          CurrentFileName;
	std::shared_ptr<BYTE> BitmapBuffer;
	

	TlgMode = (KrkrTlgMode)m_Proxyer->GetTlgFlag();

	switch (TlgMode)
	{
	case KrkrTlgMode::TLG_RAW:
		return E_ABORT;

	case KrkrTlgMode::TLG_BUILDIN:
	case KrkrTlgMode::TLG_SYS:
	case KrkrTlgMode::TLG_PNG:
	case KrkrTlgMode::TLG_JPG:
		break;

	default:
		m_Proxyer->TellServerLogOutput(
			LogLevel::LOG_WARN,
			L"CTlgPlugin::Unpack : Unknown flag = %d",
			TlgMode
		);

		return E_FAIL;
	}

	Offset.QuadPart = 0;
	Stream->Seek(Offset, SEEK_SET, NULL);
	Stream->Stat(&Stat, STATFLAG_DEFAULT);

	auto Buffer = AllocateMemorySafeP<BYTE>(Stat.cbSize.LowPart);
	if (!Buffer)
	{
		m_Proxyer->TellServerLogOutput(LogLevel::LOG_WARN, L"Tlg : failed to allocate memory for tlg Decoder");
		return E_OUTOFMEMORY;
	}

	Status = STATUS_SUCCESS;
	Stream->Read(Buffer.get(), Stat.cbSize.LowPart, &ReadSize);

	switch (TlgMode)
	{
	case KrkrTlgMode::TLG_SYS:
		CurrentFileName = wstring(FilePath);
		ReplaceFileNameExtension(CurrentFileName, L".png");
		SavePng(GetPackageName(wstring(FilePath)).c_str(), FormatPathFull(CurrentFileName.c_str()).c_str());
		return NtStatusToHResult(Status);


	case KrkrTlgMode::TLG_BUILDIN:
		CurrentFileName = wstring(FilePath);
		BitmapSize      = 0;
		ReplaceFileNameExtension(CurrentFileName, L".bmp");
		Status = DecodeTlgToBitmap(Buffer, Stat.cbSize.LowPart, BitmapBuffer, BitmapSize);
		if (NT_FAILED(Status))
			return NtStatusToHResult(Status);

		Status = File.Create(CurrentFileName.c_str());
		if (NT_FAILED(Status))
			NtStatusToHResult(Status);

		File.Write(BitmapBuffer.get(), BitmapSize);
		File.Close();
		return NO_ERROR;
		
	case KrkrTlgMode::TLG_PNG:
		CurrentFileName = wstring(FilePath);
		BitmapSize = 0;
		ReplaceFileNameExtension(CurrentFileName, L".png");
		Status = DecodeTlgToBitmap(Buffer, Stat.cbSize.LowPart, BitmapBuffer, BitmapSize);
		if (NT_FAILED(Status))
			return NtStatusToHResult(Status);

		Status = Bmp2PNG(BitmapBuffer.get(), BitmapSize, CurrentFileName.c_str());
		if (NT_FAILED(Status))
		{
			ReplaceFileNameExtension(CurrentFileName, L".bmp");
			Status = File.Create(CurrentFileName.c_str());
			if (NT_FAILED(Status))
				NtStatusToHResult(Status);

			File.Write(BitmapBuffer.get(), BitmapSize);
			File.Close();
		}

		return NtStatusToHResult(Status);

	case KrkrTlgMode::TLG_JPG:
		CurrentFileName = wstring(FilePath);
		BitmapSize = 0;
		ReplaceFileNameExtension(CurrentFileName, L".jpg");
		Status = DecodeTlgToBitmap(Buffer, Stat.cbSize.LowPart, BitmapBuffer, BitmapSize);
		if (NT_FAILED(Status))
			return NtStatusToHResult(Status);

		Status = Bmp2JPG(BitmapBuffer.get(), BitmapSize, CurrentFileName.c_str());
		if (NT_FAILED(Status))
		{
			ReplaceFileNameExtension(CurrentFileName, L".bmp");
			Status = File.Create(CurrentFileName.c_str());
			if (NT_FAILED(Status))
				NtStatusToHResult(Status);

			File.Write(BitmapBuffer.get(), BitmapSize);
			File.Close();
		}

		return NtStatusToHResult(Status);

	default:
		m_Proxyer->TellServerLogOutput(
			LogLevel::LOG_WARN,
			L"CTlgPlugin::Unpack : Unknown flag = %d",
			TlgMode
		);

		return E_FAIL;
	}

	//
	// should never reach here
	//

	return E_ABORT;
}

