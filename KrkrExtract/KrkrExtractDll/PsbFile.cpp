#include "PsbFile.h"

PsbValue::PsbValue(class PsbJsonExporter& _Psb, PsbType _Type, PBYTE& Buff) :
	Psb(_Psb),
	Type(_Type)
{
}

PsbValue::PsbValue(class PsbJsonExporter& _Psb, PBYTE& Buff) : 
	Psb(_Psb)
{
}

PsbType PsbValue::GetNodeType()
{
	return Type;
}


PsbValue::~PsbValue()
{
}

PsbNull::PsbNull(class PsbJsonExporter&  _Psb, PBYTE& Buff, PsbType _Type) : 
	PsbValue(_Psb, _Type, Buff)
{
	Buffer = Buff;
}


PsbBool::PsbBool(class PsbJsonExporter&  _Psb, PBYTE& Buff, PsbType _Type) : 
	PsbValue(_Psb, _Type, Buff)
{
	switch (_Type)
	{
	case TYPE_FALSE:
		Value = FALSE;
		break;

	default:
	case TYPE_TRUE:
		Value = TRUE;
		break;
	}

	Buffer = Buff;
}


BOOLEAN PsbBool::GetBoolean()
{
	return Value;
}

PsbResource::PsbResource(class PsbJsonExporter&  _Psb, PBYTE& Buff, PsbType _Type) :
	PsbValue(_Psb, _Type, Buff), 
	ChunkIndex(-1)
{
	ChunkIndex  = Psb.GetChunkIndex(Buff);
	ChunkBuffer = Psb.GetChunk(Buff);
	ChunkLength = Psb.GetChunkLength(Buff);
}

ULONG PsbResource::GetIndex()
{
	return ChunkIndex;
}

PBYTE PsbResource::GetBuffer()
{
	return ChunkBuffer;
}

ULONG PsbResource::GetLength()
{
	return ChunkLength;
}


PsbNumber::PsbNumber(class PsbJsonExporter& _Psb, PBYTE& Buff, PsbType _Type) :
	PsbValue(_Psb, _Type, Buff)
{
	Buffer = Buff;
	_Psb.GetNumber(Buff, Value, NumberType);
}

INT64 PsbNumber::GetInteger()
{
	return Value.IntegerValue;
}

float PsbNumber::GetFloat()
{
	return Value.FloatValue;
}

double PsbNumber::GetDouble()
{
	return Value.DoubleValue;
}

BOOLEAN PsbNumber::IsNumberNode(PsbValue *_Value)
{
	switch (_Value->GetNodeType())
	{
	case TYPE_NUMBER_N0:
	case TYPE_NUMBER_N1:
	case TYPE_NUMBER_N2:
	case TYPE_NUMBER_N3:
	case TYPE_NUMBER_N4:
		return TRUE;

	default:
		return FALSE;
	}
}


PsbArray::PsbArray(class PsbJsonExporter& _Psb, PBYTE& Buff, PsbType _Type) :
	PsbValue(_Psb, _Type, Buff),
	DataLength(0)
{
	ULONG n = *Buff++ - 0xC;

	DataLength += 1;
	ULONG Value = 0;

	for (ULONG i = 0; i < n; i++)
	{
		Value |= *Buff++ << (i * 8);
		DataLength++;
	}

	EntryCount  = Value;
	EntryLength = *Buff++ - 0xC;
	DataLength += 1;
	Buffer      = Buff;

	DataLength += (EntryCount * EntryLength);
	Buff += EntryCount * EntryLength;
}

ULONG PsbArray::Size()
{
	return EntryCount;
}

ULONG PsbArray::Get(ULONG Index)
{
	ULONG Value = 0;

	auto p = Buffer + Index * EntryLength;

	for (ULONG i = 0; i < EntryLength; i++)
		Value |= *p++ << (i * 8);

	return Value;
}


PsbString::PsbString(PsbJsonExporter& _Psb, PBYTE& Buff) : 
	PsbValue(_Psb, TYPE_STRING_N1, Buff),
	Buffer(--Buff)
{
}

ULONG PsbString::GetIndex()
{
	return Psb.GetStringIndex(Buffer);
}

string PsbString::GetString()
{
	return Psb.GetString(Buffer);
}

PsbObject::PsbObject(PsbJsonExporter& _Psb, PBYTE& Buff) : 
	PsbValue(_Psb, TYPE_OBJECTS, Buff),
	Buffer(Buff)
{
	Names   = new PsbArray(_Psb, Buff, (PsbType)Buff[0]);
	Offsets = new PsbArray(_Psb, Buff, (PsbType)Buff[0]);
}

PsbObject::~PsbObject()
{
	delete Offsets;
	delete Names;
}

ULONG PsbObject::Size()
{
	return Names->Size();
}

string PsbObject::GetName(ULONG Index)
{
	return Psb.GetName(Names->Get(Index));
}

PBYTE PsbObject::GetData(ULONG Index)
{
	return Buffer + Offsets->Get(Index);
}

PBYTE PsbObject::GetData(const string& Name)
{
	for (ULONG i = 0; i < Names->Size(); i++)
	{
		if (GetName(i) == Name) 
			return GetData(i);
	}

	return NULL;
}



PsbCollection::PsbCollection(PsbJsonExporter& _Psb, PBYTE& Buff) : 
	PsbValue(_Psb, TYPE_COLLECTION, Buff)
{
	Offsets = new PsbArray(_Psb, Buff, (PsbType)Buff[0]);
	Buffer = Buff;
}

PsbCollection::~PsbCollection() 
{
	delete Offsets;
}

ULONG PsbCollection::Size()
{
	return Offsets->Size();
}

PBYTE PsbCollection::Get(ULONG Index)
{
	return Buffer + Offsets->Get(Index);
}


PsbJsonExporter::PsbJsonExporter(PBYTE Buff) :
	Buffer(Buff)
{
	Header = (PSBHDR*)Buffer;
	auto p = Buffer + Header->OffsetNames;

	Str1 = new PsbArray(*this, p, (PsbType)p[0]);
	Str2 = new PsbArray(*this, p, (PsbType)p[0]);
	Str3 = new PsbArray(*this, p, (PsbType)p[0]);

	p = Buff + Header->OffsetStrings;
	Strings = new PsbArray(*this, p, (PsbType)p[0]);

	StringsData = (char*)(Buffer + Header->OffsetStringsData);

	p = Buffer + Header->OffsetChunkOffsets;
	ChunkOffsets = new PsbArray(*this, p, (PsbType)p[0]);

	p = Buffer+ Header->OffsetChunkLengths;
	ChunkLengths = new PsbArray(*this, p, (PsbType)p[0]);

	ChunkData = Buffer + Header->OffsetChunkData;

	p = Buff + Header->OffsetEntries;
	Unpack(Objects, p);

	ExpireSuffixList = NULL;

	if (Objects)
	{
		Objects->Unpack(ExpireSuffixList, "expire_suffix_list");
		if (ExpireSuffixList)
			Extension = GetString(ExpireSuffixList->Get(0));
	}
}

PsbJsonExporter::~PsbJsonExporter() 
{
	if (ExpireSuffixList) delete ExpireSuffixList;
	if (Objects)          delete Objects;
	if (ChunkLengths)     delete ChunkLengths;
	if (ChunkOffsets)     delete ChunkOffsets;
	if (Strings)          delete Strings;
	if (Str3)             delete Str3;
	if (Str2)             delete Str2;
	if (Str1)             delete Str1;

	ExpireSuffixList = NULL;
	Objects          = NULL;
	ChunkLengths     = NULL;
	ChunkOffsets     = NULL;
	Str1             = NULL;
	Str2             = NULL;
	Str3             = NULL;
	Strings          = NULL;
}

string PsbJsonExporter::GetName(ULONG Index)
{
	string   accum;
	uint32_t a = Str3->Get(Index);
	uint32_t b = Str2->Get(a);

	LOOP_FOREVER
	{
		uint32_t c = Str2->Get(b);
		uint32_t d = Str1->Get(c);
		uint32_t e = b - d;

		b = c;
		accum = (char)e + accum;

		if (!b) break;
	};
	return accum;
}

BOOL PsbJsonExporter::GetNumber(PBYTE Buff, PsbNumber::PsbNumberValue &Value, PsbNumber::PsbNumberType &NumberType)
{
	static const DWORD TYPE_TO_KIND[] =
	{
		0, 1, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 10, 11, 12
	};

	BYTE    Type = *Buff++;
	INT64   ParseValue;

	switch (TYPE_TO_KIND[Type])
	{
	case 1:
		Value.IntegerValue = 0;
		break;

	case 2:
		Value.IntegerValue = 1;
		break;

	case 3:
	case 4:
	{
		ULONG n = Type - 4;

		ParseValue = 0;
		for (ULONG i = 0; i < n; i++) 
			ParseValue |= *Buff++ << (i * 8);

		INT64 mask = (INT64)1 << ((n * 8) - 1);
		if (ParseValue & mask) 
		{
			for (int i = n * 8; i < sizeof(ParseValue) * 8; i++)
				ParseValue |= (int64_t)1 << i;
		}

		Value.IntegerValue = ParseValue;
		NumberType = PsbNumber::PsbNumberType::INTEGER;
	}
	break;

	case 9:
		if (Type == 0x1E) 
		{
			Value.FloatValue = *(float*)Buff;
			NumberType = PsbNumber::PsbNumberType::FLOAT;
		}
		else if (Type == 0x1D) 
		{
			Value.FloatValue = 0.0f;
			NumberType = PsbNumber::PsbNumberType::FLOAT;
		}
		break;

	case 10:
		if (Type == 0x1F) 
		{
			Value.DoubleValue = *(double*)Buff;
			NumberType = PsbNumber::PsbNumberType::DOUBLE;
			Buff += 8;
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

string PsbJsonExporter::GetString(PBYTE p) 
{
	ULONG n = *p++ - 0x14;
	ULONG v = 0;

	for (ULONG i = 0; i < n; i++)
		v |= *p++ << (i * 8);

	return StringsData + Strings->Get(v);
}

ULONG PsbJsonExporter::GetStringIndex(PBYTE p)
{
	ULONG n = *p++ - 0x14;
	ULONG v = 0;

	for (ULONG i = 0; i < n; i++) 
		v |= *p++ << (i * 8);

	return v;
}

PsbObject* PsbJsonExporter::GetObject()
{
	return Objects;
}

PBYTE PsbJsonExporter::GetChunk(PBYTE p)
{
	return ChunkData + ChunkOffsets->Get(GetChunkIndex(p));
}

ULONG PsbJsonExporter::GetChunkLength(PBYTE p)
{
	return ChunkLengths->Get(GetChunkIndex(p));
}

PsbValue* PsbJsonExporter::Unpack(PBYTE& p)
{
	BYTE Type = *p++;

	switch (Type)
	{
	case TYPE_NULL:
		return new PsbNull(*this, p, (PsbType)Type);
	case TYPE_FALSE:
	case TYPE_TRUE:
		return new PsbBool(*this, p, (PsbType)Type);

	case TYPE_NUMBER_N0:
	case TYPE_NUMBER_N1:
	case TYPE_NUMBER_N2:
	case TYPE_NUMBER_N3:
	case TYPE_NUMBER_N4:
	case TYPE_NUMBER_N5:
	case TYPE_NUMBER_N6:
	case TYPE_NUMBER_N7:
	case TYPE_NUMBER_N8:
	case TYPE_FLOAT:
	case TYPE_FLOAT0:
	case TYPE_DOUBLE:
		return new PsbNumber(*this, --p, (PsbType)Type);

	case TYPE_ARRAY_N1:
	case TYPE_ARRAY_N2:
	case TYPE_ARRAY_N3:
	case TYPE_ARRAY_N4:
	case TYPE_ARRAY_N5:
	case TYPE_ARRAY_N6:
	case TYPE_ARRAY_N7:
	case TYPE_ARRAY_N8:
		return new PsbArray(*this, --p, (PsbType)Type);

	case TYPE_COLLECTION:
		return new PsbCollection(*this, p);

	case TYPE_OBJECTS:
		return new PsbObject(*this, p);

	case TYPE_RESOURCE_N1:
	case TYPE_RESOURCE_N2:
	case TYPE_RESOURCE_N3:
	case TYPE_RESOURCE_N4:
		return new PsbResource(*this, --p, TYPE_RESOURCE_N1);

	case TYPE_STRING_N1:
	case TYPE_STRING_N2:
	case TYPE_STRING_N3:
	case TYPE_STRING_N4:
		return new PsbString(*this, p);

	default:
		p--;
		return NULL;
	}

	return NULL;
}

ULONG PsbJsonExporter::GetChunkIndex(PBYTE Buff)
{
	ULONG n = *Buff++ - 0x18;
	ULONG Value = 0;

	for (ULONG i = 0; i < n; i++)
		Value |= *Buff++ << (i * 8);

	return Value;
}
