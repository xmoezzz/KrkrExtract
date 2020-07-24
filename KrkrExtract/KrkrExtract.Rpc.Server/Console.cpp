#include "Console.h"


ULONG_PTR NtPrintConsole(PCWSTR Format, ...)
{
	BOOL        Success;
	ULONG       Length;
	WCHAR       Buffer[0xF00 / 2];
	va_list     Args;

	va_start(Args, Format);
	Length = _vsnwprintf_s(Buffer, countof(Buffer) - 1, Format, Args);
	if (Length == -1)
		return Length;

	Success = WriteConsoleW(
		GetStdHandle(STD_OUTPUT_HANDLE),
		Buffer,
		Length,
		&Length,
		NULL
	);

	return Success ? Length : 0;
}
