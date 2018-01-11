#pragma once

#include <my.h>
#include <map>


class IMarshalObject : public IUnknown
{
public:
	virtual IStream*        NTAPI Marshal() = 0;
	virtual IMarshalObject* NTAPI UnMarshal(IStream* Stream) = 0;
};

class CMarshalMap : public IMarshalObject
{
public:
	CMarshalMap() :
		ReferCount(1)
	{
		RtlZeroMemory(DisasmLine, sizeof(DisasmLine));
		InsLength  = 0;
		Offset     = 0;
		PrevLength = 0;
	}

	~CMarshalMap()
	{
		Release();
	}


	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return E_NOTIMPL;
	}

	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return E_NOTIMPL;
	}

	ULONG STDMETHODCALLTYPE Release(void)
	{
		return E_NOTIMPL;
	}


	IStream* NTAPI Marshal()
	{
		IStream*      Stream;
		HRESULT       Result;
		ULONG         ChunkLength;
		STATSTG       Stat;
		LARGE_INTEGER MoveOffset;

		Result = CreateStreamOnHGlobal(NULL, TRUE, &Stream);
		if (FAILED(Result))
			return NULL;

		ChunkLength = 0;
		Stream->Write(&ChunkLength, sizeof(ChunkLength), NULL);
		Stream->Write(&Offset,     sizeof(Offset),     NULL);
		Stream->Write(&InsLength,  sizeof(InsLength),  NULL);
		Stream->Write(&PrevLength, sizeof(PrevLength), NULL);
		ChunkLength = StrLengthA(DisasmLine) + 1;
		Stream->Write(&ChunkLength, sizeof(ChunkLength), NULL);
		Stream->Write(DisasmLine, ChunkLength, NULL);
		Stream->Stat(&Stat, STATFLAG_DEFAULT);

		MoveOffset.QuadPart = 0;
		Stream->Seek(MoveOffset, FILE_BEGIN, NULL);
		Stream->Write(&Stat.cbSize.LowPart, sizeof(Stat.cbSize.LowPart), NULL);

		return Stream;
	}

	IMarshalObject* NTAPI UnMarshal(IStream* Stream)
	{
		ULONG         ChunkLength;
		STATSTG       Stat;
		LARGE_INTEGER MoveOffset;

		if (Stream == NULL)
			return NULL;

		Stream->Read(&ChunkLength, sizeof(ChunkLength), NULL);
		Stream->Stat(&Stat, STATFLAG_DEFAULT);
		if (Stat.cbSize.LowPart != ChunkLength)
			return NULL;

		Stream->Read(&Offset,      sizeof(Offset),      NULL);
		Stream->Read(&InsLength,   sizeof(InsLength),   NULL);
		Stream->Read(&PrevLength , sizeof(PrevLength),  NULL);
		Stream->Read(&ChunkLength, sizeof(ChunkLength), NULL);

		//check : crc32 
		RtlZeroMemory(DisasmLine, sizeof(DisasmLine));
		Stream->Write(DisasmLine, ChunkLength, NULL);

		MoveOffset.QuadPart = 0;
		Stream->Seek(MoveOffset, FILE_BEGIN, NULL);

		return this;
	}

	CMarshalMap& operator = (const CMarshalMap& o)
	{
		RtlCopyMemory(DisasmLine, o.DisasmLine, sizeof(DisasmLine));
		InsLength  = o.InsLength;
		Offset     = o.Offset;
		PrevLength = o.PrevLength;
		return *this;
	}

	CHAR  DisasmLine[100];
	ULONG InsLength;
	ULONG PrevLength;
	ULONG Offset;

private:
	ULONG ReferCount;
};

typedef struct _PE_DATABASE
{
	ULONG      Magic;
	ULONG      Crc32;
	ULONG64    EntryCount;
}PE_DATABASE, *PPE_DATABASE;


