#pragma once

#include <my.h>
#include "ml.h"
#include <stdint.h>


#pragma pack(1)
struct PSBHDR 
{
	BYTE   Signature[4];
	WORD   Version;
	WORD   Encrypt;
	DWORD  OffsetEncrypt;
	DWORD  OffsetNames;
	DWORD  OffsetStrings;
	DWORD  OffsetStringsData;
	DWORD  OffsetChunkOffsets;
	DWORD  OffsetChunkLengths;
	DWORD  OffsetChunkData;
	DWORD  OffsetEntries;
};
#pragma pack()

class PsbJsonExporter;


enum PsbType
{
	TYPE_NONE = 0x0,
	TYPE_NULL = 0x1,
	TYPE_FALSE = 0x2,
	TYPE_TRUE = 0x3,

	TYPE_NUMBER_N0 = 0x4,
	TYPE_NUMBER_N1 = 0x5,
	TYPE_NUMBER_N2 = 0x6,
	TYPE_NUMBER_N3 = 0x7,
	TYPE_NUMBER_N4 = 0x8,
	TYPE_NUMBER_N5 = 0x9,
	TYPE_NUMBER_N6 = 0xA,
	TYPE_NUMBER_N7 = 0xB,
	TYPE_NUMBER_N8 = 0xC,

	TYPE_ARRAY_N1 = 0xD,
	TYPE_ARRAY_N2 = 0xE,
	TYPE_ARRAY_N3 = 0xF,
	TYPE_ARRAY_N4 = 0x10,
	TYPE_ARRAY_N5 = 0x11,
	TYPE_ARRAY_N6 = 0x12,
	TYPE_ARRAY_N7 = 0x13,
	TYPE_ARRAY_N8 = 0x14,

	TYPE_STRING_N1 = 0x15,
	TYPE_STRING_N2 = 0x16,
	TYPE_STRING_N3 = 0x17,
	TYPE_STRING_N4 = 0x18,

	TYPE_RESOURCE_N1 = 0x19,
	TYPE_RESOURCE_N2 = 0x1A,
	TYPE_RESOURCE_N3 = 0x1B,
	TYPE_RESOURCE_N4 = 0x1C,

	TYPE_FLOAT0 = 0x1D,
	TYPE_FLOAT = 0x1E,
	TYPE_DOUBLE = 0x1F,

	TYPE_COLLECTION = 0x20,
	TYPE_OBJECTS = 0x21,
};

class PsbValue 
{
public:
	PsbValue(class PsbJsonExporter& Psb, PsbType Type, PBYTE& Buff);
	PsbValue(class PsbJsonExporter& Psb, PBYTE&  Buff);

	virtual ~PsbValue();
	PsbType GetNodeType();

protected:
	class PsbJsonExporter& Psb;
	PsbType                Type;
};

class PsbNull : public PsbValue 
{
public:
	PsbNull(class PsbJsonExporter& Psb, PBYTE& Buff, PsbType Type);

private:
	PBYTE Buffer;
};

class PsbBool : public PsbValue
{
public:
	PsbBool(class PsbJsonExporter& Psb, PBYTE& Buff, PsbType Type);
	BOOLEAN GetBoolean();

private:
	PBYTE   Buffer;
	BOOLEAN Value;
};

class PsbResource : public PsbValue
{
public:
	PsbResource(class PsbJsonExporter& Psb, PBYTE& Buff, PsbType Type);
	PBYTE   GetBuffer();
	ULONG   GetLength();
	ULONG   GetIndex();

protected:
	ULONG ChunkIndex;
	PBYTE ChunkBuffer;
	ULONG ChunkLength;
};

class PsbNumber : public PsbValue
{
public:
	union PsbNumberValue
	{
		float  FloatValue;
		double DoubleValue;
		INT64  IntegerValue;
	};

	enum PsbNumberType
	{
		INTEGER,
		FLOAT,
		DOUBLE
	};

	PsbNumber(class PsbJsonExporter& _Psb, PBYTE& Buff, PsbType Type);

	float         GetFloat();
	double        GetDouble();
	INT64         GetInteger();
	PsbNumberType GetNumberType();

	static BOOLEAN IsNumberNode(PsbValue *_Value);

private:
	PsbNumberValue Value;
	PsbNumberType  NumberType;
	PBYTE          Buffer;
};

class PsbArray : public PsbValue 
{
public:
	PsbArray(class PsbJsonExporter& _Psb, PBYTE& Buff, PsbType _Type);

	ULONG Size();
	ULONG Get(ULONG Index);

	ULONG  DataLength;
	ULONG  EntryCount;
	ULONG  EntryLength;
	PBYTE  Buffer;
};

class PsbString : public PsbValue
{
public:
	PsbString(class PsbJsonExporter& Psb, PBYTE& Buff);
	ULONG  GetIndex();
	string GetString();

	PBYTE  Buffer;
};


class PsbObject : public PsbValue
{
public:
	PsbObject(class PsbJsonExporter& Psb, PBYTE& Buff);
	~PsbObject();

	ULONG  Size();
	string GetName(ULONG Index);
	PBYTE  GetData(ULONG Index);
	PBYTE  GetData(const string& Name);

	template<class T> void Unpack(T*& Out, const string& Name);


public:
	PsbArray*   Names;
	PsbArray*   Offsets;
	PBYTE       Buffer;
};


class PsbCollection : public PsbValue 
{
public:
	PsbCollection(class PsbJsonExporter& Psb, PBYTE& Buff);
	~PsbCollection();

	ULONG Size();
	PBYTE Get(DWORD Index);

	template<class T> VOID Unpack(T*& Out, DWORD Index);

public:
	PsbArray*   Offsets;
	PBYTE       Buffer;
};


class PsbJsonExporter
{
public:
	PsbJsonExporter(PBYTE Buff);
	~PsbJsonExporter();

	string     GetName(ULONG Index);
	BOOL       GetNumber(PBYTE Buff, PsbNumber::PsbNumberValue &Value, PsbNumber::PsbNumberType &ValueType);
	string     GetString(PBYTE Buff);
	ULONG      GetStringIndex(PBYTE Buff);
	PsbObject* GetObject();
	PBYTE      GetChunk(PBYTE Buff);
	ULONG      GetChunkLength(PBYTE Buff);
	PsbValue*  Unpack(unsigned char*& p);

	template<class T> void Unpack(T*& out, PBYTE& Buff);

public:
	ULONG      GetChunkIndex(PBYTE Buff);

	PBYTE       Buffer;
	PSBHDR*     Header;
	PsbArray*   Str1;
	PsbArray*   Str2;
	PsbArray*   Str3;
	PsbArray*   Strings;
	PCHAR       StringsData;
	PsbArray*   ChunkOffsets;
	PsbArray*   ChunkLengths;
	PBYTE       ChunkData;

	PsbObject*     Objects;
	PsbCollection* ExpireSuffixList;
	string         Extension;
};


template<class T> VOID PsbObject::Unpack(T*& Out, const string& Name)
{
	Out = NULL;
	auto Data = GetData(Name);

	if (Data) 
		Psb.Unpack(Out, Data);
}

template<class T> VOID PsbCollection::Unpack(T*& Out, ULONG Index)
{
	auto Data = Get(Index);

	Psb.Unpack(Out, Data);
}

template<class T> VOID PsbJsonExporter::Unpack(T*& Out, PBYTE& Buff) 
{
	Out = dynamic_cast<T*>(Unpack(Buff));
}

