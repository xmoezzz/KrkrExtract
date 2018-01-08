#include "Utility.h"

#define IS_ATOM(_class) ((((ULONG)(_class)) & 0xFFFF0000) == 0)

VOID FreeString(PVOID String)
{
	FreeMemoryP(String);
}

PSTR WCharToMByte(PCWSTR Unicode, ULONG_PTR Length)
{
	PSTR AnsiString;

	if (Length == -1)
		Length = StrLengthW(Unicode);

	++Length;
	Length *= sizeof(WCHAR);

	AnsiString = (PSTR)AllocateMemoryP(Length);
	if (AnsiString == nullptr)
		return nullptr;

	RtlUnicodeToMultiByteN(AnsiString, Length, nullptr, Unicode, Length);

	return AnsiString;
}

PWSTR MByteToWChar(PCSTR AnsiString, ULONG_PTR Length)
{
	PWSTR Unicode;

	if (Length == -1)
		Length = StrLengthA(AnsiString);

	++Length;

	Unicode = (PWSTR)AllocateMemoryP(Length * sizeof(WCHAR));
	if (Unicode == nullptr)
		return nullptr;

	RtlMultiByteToUnicodeN(Unicode, Length * sizeof(WCHAR), nullptr, AnsiString, Length);

	return Unicode;
}

PSTR ClassWCharToMByte(PCWSTR Unicode)
{
	return IS_ATOM(Unicode) ? (PSTR)Unicode : WCharToMByte(Unicode);
}

PWSTR ClassMByteToWChar(PCSTR AnsiString)
{
	return IS_ATOM(AnsiString) ? (PWSTR)AnsiString : MByteToWChar(AnsiString);
}

PSTR TitleWCharToMByte(PCWSTR Unicode)
{
	if (Unicode[0] == 0xFFFF)
	{
		PSTR ResourceId = (PSTR)AllocateMemoryP(4);
		if (ResourceId == nullptr)
			return nullptr;

		ResourceId[0] = -1;
		*(PUSHORT)&ResourceId[1] = Unicode[1];
		ResourceId[3] = 0;

		return ResourceId;
	}

	return WCharToMByte(Unicode);
}

PWSTR TitleMByteToWChar(PCSTR AnsiString)
{
	if (AnsiString[0] == -1)
	{
		PWSTR ResourceId = (PWSTR)AllocateMemoryP(6);
		if (ResourceId == nullptr)
			return nullptr;

		ResourceId[0] = 0xFFFF;
		ResourceId[1] = *(PUSHORT)&AnsiString[1];
		ResourceId[2] = 0;

		return ResourceId;
	}

	return MByteToWChar(AnsiString);
}

BOOL FreeClass(PVOID ClassName)
{
	return IS_ATOM(ClassName) ? TRUE : FreeMemoryP(ClassName);
}

VOID InitEmptyLargeString(PLARGE_UNICODE_STRING String)
{
	ZeroMemory(String, sizeof(*String));
}

VOID InitStringFromLargeString(PANSI_STRING AnsiString, PLARGE_UNICODE_STRING LargeString)
{
	AnsiString->Length = LargeString->Length;
	AnsiString->MaximumLength = LargeString->MaximumLength;
	AnsiString->Buffer = LargeString->AnsiBuffer;
}

VOID InitStringFromLargeString(PUNICODE_STRING UnicodeString, PLARGE_UNICODE_STRING LargeString)
{
	UnicodeString->Length = LargeString->Length;
	UnicodeString->MaximumLength = LargeString->MaximumLength;
	UnicodeString->Buffer = LargeString->UnicodeBuffer;
}

PLARGE_UNICODE_STRING LargeStringDuplicate(PLARGE_UNICODE_STRING LargeString, PLARGE_UNICODE_STRING Destination)
{
	union
	{
		ANSI_STRING     Ansi;
		UNICODE_STRING  Unicode;
	};

	union
	{
		ANSI_STRING     NewAnsi;
		UNICODE_STRING  NewUnicode;
	};

	if (LargeString->Ansi)
		return nullptr;

	InitStringFromLargeString(&Unicode, LargeString);
	if (NT_FAILED(RtlDuplicateUnicodeString(RTL_DUPSTR_ADD_NULL, &Unicode, &NewUnicode)))
		return nullptr;

	Destination->Ansi = FALSE;
	Destination->Length = NewUnicode.Length;
	Destination->MaximumLength = NewUnicode.MaximumLength;
	Destination->UnicodeBuffer = NewUnicode.Buffer;

	return Destination;
}

PLARGE_UNICODE_STRING LargeStringAnsiToUnicode(PLARGE_UNICODE_STRING LargeAnsiString, PLARGE_UNICODE_STRING LargeUnicodeString)
{
	if (LargeAnsiString == nullptr)
		return nullptr;

	if (!LargeAnsiString->Ansi)
		return LargeStringDuplicate(LargeAnsiString, LargeUnicodeString);

	ANSI_STRING     AnsiString;
	UNICODE_STRING  UnicodeString;

	AnsiString.Buffer = LargeAnsiString->AnsiBuffer;
	AnsiString.Length = LargeAnsiString->Length;
	AnsiString.MaximumLength = LargeAnsiString->MaximumLength;

	if (NT_FAILED(RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE)))
		return nullptr;

	LargeUnicodeString->Ansi = FALSE;
	LargeUnicodeString->Length = UnicodeString.Length;
	LargeUnicodeString->MaximumLength = UnicodeString.MaximumLength;
	LargeUnicodeString->Buffer = (ULONG64)UnicodeString.Buffer;

	return LargeUnicodeString;
}

VOID FreeLargeString(PLARGE_UNICODE_STRING LargeString)
{
	union
	{
		ANSI_STRING     Ansi;
		UNICODE_STRING  Unicode;
	};

	if (LargeString->Buffer == 0)
		return;

	if (LargeString->Ansi)
	{
		Ansi.Length = LargeString->Length;
		Ansi.MaximumLength = LargeString->MaximumLength;
		Ansi.Buffer = LargeString->AnsiBuffer;

		RtlFreeAnsiString(&Ansi);
	}
	else
	{
		Unicode.Length = LargeString->Length;
		Unicode.MaximumLength = LargeString->MaximumLength;
		Unicode.Buffer = LargeString->UnicodeBuffer;

		RtlFreeUnicodeString(&Unicode);
	}
}

PLARGE_UNICODE_STRING CaptureAnsiWindowName(PLARGE_UNICODE_STRING WindowName, PLARGE_UNICODE_STRING UnicodeWindowName)
{
	InitEmptyLargeString(UnicodeWindowName);

	if (WindowName == nullptr || WindowName->Buffer == 0)
		return nullptr;

	if (WindowName->UnicodeBuffer[0] == 0xFFFF)
	{
		WCHAR                   Buffer[0x10];
		ULONG_PTR               Length;
		LARGE_UNICODE_STRING    TitleAsResourceId;

		TitleAsResourceId.Ansi = FALSE;

		Length = WindowName->Ansi ? 3 : 4;

		CopyMemory(Buffer, WindowName->AnsiBuffer + WindowName->Ansi, Length);

		TitleAsResourceId.Length = Length;
		TitleAsResourceId.MaximumLength = Length;
		TitleAsResourceId.Buffer = (ULONG64)Buffer;

		return LargeStringDuplicate(&TitleAsResourceId, UnicodeWindowName);
	}

	return LargeStringAnsiToUnicode(WindowName, UnicodeWindowName);
}

BOOL IsSystemCall(PVOID Routine)
{
	PBYTE   Buffer;
	BOOL    HasMovEax;
	BOOL    HasCall;
	BOOL    HasRet;

	Buffer = (PBYTE)Routine;
	HasMovEax = FALSE;
	HasCall = FALSE;
	HasRet = FALSE;

	for (ULONG_PTR Count = 6; Count != 0; --Count)
	{
		switch (Buffer[0])
		{
		case 0xFF:
			if (Buffer[1] == 0x15 || Buffer[1] == 0x25)
				return FALSE;

			goto CHECK_CALL;


		case 0xB8:
			if ((HasMovEax | HasCall | HasRet) != FALSE)
				return FALSE;

			HasMovEax = TRUE;
			break;

		case 0x64:
			if (*(PUSHORT)&Buffer[1] != 0x15FF)
				break;

		case CALL:
		CHECK_CALL:
			if (HasMovEax == FALSE)
				return FALSE;

			if ((HasCall | HasRet) != FALSE)
				return FALSE;

			HasCall = TRUE;
			break;

		case 0xC2:
		case 0xC3:
			if (HasRet != FALSE)
				return FALSE;

			if ((HasMovEax & HasCall) == FALSE)
				return FALSE;

			HasRet = TRUE;
			Count = 1;
			continue;
		}

		Buffer += GetOpCodeSize(Buffer);
	}

	return HasMovEax & HasCall & HasRet;
}
