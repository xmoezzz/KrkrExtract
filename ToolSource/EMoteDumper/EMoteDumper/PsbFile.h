#pragma once

#include <my.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>


namespace PsbJson
{

using std::string;
using std::wstring;
using std::vector;
using std::map;

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
	PsbValue(class PsbJsonExporter& Psb, PsbType Type, PByte& Buff);
	PsbValue(class PsbJsonExporter& Psb, PByte&  Buff);

	virtual ~PsbValue();
	PsbType GetNodeType();

protected:
	class PsbJsonExporter& Psb;
	PsbType                Type;
};

class PsbNull : public PsbValue
{
public:
	PsbNull(class PsbJsonExporter& Psb, PByte& Buff, PsbType Type);

private:
	PByte Buffer;
};

class PsbBool : public PsbValue
{
public:
	PsbBool(class PsbJsonExporter& Psb, PByte& Buff, PsbType Type);
	BOOLEAN GetBoolean();

private:
	PByte   Buffer;
	BOOLEAN Value;
};

class PsbResource : public PsbValue
{
public:
	PsbResource(class PsbJsonExporter& Psb, PByte& Buff, PsbType Type);
	PByte   GetBuffer();
	ULONG   GetLength();
	ULONG   GetIndex();

protected:
	ULONG ChunkIndex;
	PByte ChunkBuffer;
	ULONG ChunkLength;
};

class PsbNumber : public PsbValue
{
public:
	union PsbNumberValue
	{
		Float  FloatValue;
		Double DoubleValue;
		Int64  IntegerValue;
	};

	enum PsbNumberType
	{
		INTEGER,
		FLOAT,
		DOUBLE
	};

	PsbNumber(class PsbJsonExporter& _Psb, PByte& Buff, PsbType Type);

	Float         GetFloat();
	Double        GetDouble();
	Int64         GetInteger();
	PsbNumberType GetNumberType();

	static BOOLEAN IsNumberNode(PsbValue *_Value);

private:
	PsbNumberValue Value;
	PsbNumberType  NumberType;
	PByte          Buffer;
};

class PsbArray : public PsbValue
{
public:
	PsbArray(class PsbJsonExporter& _Psb, PByte& Buff, PsbType _Type);

	ULONG Size();
	ULONG Get(ULONG Index);

	ULONG  DataLength;
	ULONG  EntryCount;
	ULONG  EntryLength;
	PByte  Buffer;
};

class PsbString : public PsbValue
{
public:
	PsbString(class PsbJsonExporter& Psb, PByte& Buff);
	ULONG  GetIndex();
	string GetString();

	PByte  Buffer;
};


class PsbObject : public PsbValue
{
public:
	PsbObject(class PsbJsonExporter& Psb, PByte& Buff);
	~PsbObject();

	ULONG  Size();
	string GetName(ULONG Index);
	PByte  GetData(ULONG Index);
	PByte  GetData(const string& Name);

	template<class T> void Unpack(T*& Out, const string& Name);


public:
	PsbArray*   Names;
	PsbArray*   Offsets;
	PByte       Buffer;
};


class PsbCollection : public PsbValue
{
public:
	PsbCollection(class PsbJsonExporter& Psb, PByte& Buff);
	~PsbCollection();

	ULONG Size();
	PByte Get(DWORD Index);

	template<class T> Void Unpack(T*& Out, DWORD Index);

public:
	PsbArray*   Offsets;
	PByte       Buffer;
};


class PsbJsonExporter
{
public:
	PsbJsonExporter(PByte Buff);
	~PsbJsonExporter();

	string     GetName(ULONG Index);
	BOOL       GetNumber(PByte Buff, PsbNumber::PsbNumberValue &Value, PsbNumber::PsbNumberType &ValueType);
	string     GetString(PByte Buff);
	ULONG      GetStringIndex(PByte Buff);
	PsbObject* GetObject();
	PByte      GetChunk(PByte Buff);
	ULONG      GetChunkLength(PByte Buff);
	PsbValue*  Unpack(unsigned char*& p);

	template<class T> void Unpack(T*& out, PByte& Buff);

public:
	ULONG      GetChunkIndex(PByte Buff);

	PByte       Buffer;
	PSBHDR*     Header;
	PsbArray*   Str1;
	PsbArray*   Str2;
	PsbArray*   Str3;
	PsbArray*   Strings;
	PCHAR       StringsData;
	PsbArray*   ChunkOffsets;
	PsbArray*   ChunkLengths;
	PByte       ChunkData;

	PsbObject*     Objects;
	PsbCollection* ExpireSuffixList;
	string         Extension;
};


template<class T> Void PsbObject::Unpack(T*& Out, const string& Name)
{
	Out = NULL;
	auto Data = GetData(Name);

	if (Data)
		Psb.Unpack(Out, Data);
}

template<class T> Void PsbCollection::Unpack(T*& Out, ULONG Index)
{
	auto Data = Get(Index);

	Psb.Unpack(Out, Data);
}

template<class T> Void PsbJsonExporter::Unpack(T*& Out, PByte& Buff)
{
	Out = dynamic_cast<T*>(Unpack(Buff));
}


////export
NTSTATUS WINAPI DecompilePsbJson(PBYTE Buffer, ULONG Length, LPCWSTR BasePath, LPCWSTR FileName);
};
