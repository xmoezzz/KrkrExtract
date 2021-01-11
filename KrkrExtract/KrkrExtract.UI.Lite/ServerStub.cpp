#include "UIViewer.h"
#include <time.h>
#include <ctime>
#include <chrono>

using std::chrono::system_clock;

BOOL UIViewer::NotifyUIProgressBar(PCWSTR TaskName, ULONGLONG Current, ULONGLONG Total)
{
	WCHAR WindowsName[0x400];

	if ((TaskName == NULL || StrLengthW(TaskName) == 0) && Current == 0 && Total == 0) 
	{
		SendMessageW(m_hWndProgressBar, PBM_SETPOS, 0, 0);

		if (m_Taskbar)
		{
			m_Taskbar->SetProgressState(m_hWndMain, TBPF_NORMAL);
			m_Taskbar->SetProgressValue(m_hWndMain, 0, 0);
		}
	}

	if (Total != 0)
	{
		SendMessageW(m_hWndProgressBar, PBM_SETPOS, (ULONG)((double)Current / (double)Total) * 100.0, 0);
		wnsprintfW(WindowsName, countof(WindowsName), L"[%s] %lld/%lld", TaskName, Current, Total);
	}
	else {
		wnsprintfW(WindowsName, countof(WindowsName), L"[%s] ...", TaskName);
	}

	SetWindowTextW(m_hWndMain, WindowsName);

	if (m_Taskbar)
	{
		if (Total != 0)
		{
			m_Taskbar->SetProgressState(m_hWndMain, TBPF_NORMAL);
			m_Taskbar->SetProgressValue(m_hWndMain, Current, Total);
		}
		else
		{
			m_Taskbar->SetProgressValue(m_hWndMain, 0, 0);
			m_Taskbar->SetProgressState(m_hWndMain, TBPF_NOPROGRESS);
		}
	}

	return TRUE;
}

std::vector<std::wstring> SplitOutput(PCWSTR str, WCHAR separator, BOOL keepEmptyStrings = FALSE)
{
	std::vector<std::wstring> ret;
	PCWSTR strEnd = str + StrLengthW(str);

	for (PCWSTR splitEnd = str; splitEnd != strEnd; ++splitEnd)
	{
		if (*splitEnd == separator)
		{
			const ptrdiff_t splitLen = splitEnd - str;
			if (splitLen > 0 || keepEmptyStrings)
				ret.push_back(std::wstring(str, splitLen));
			str = splitEnd + 1;
		}
	}

	const ptrdiff_t splitLen = strEnd - str;
	if (splitLen > 0 || keepEmptyStrings)
		ret.push_back(std::wstring(str, splitLen));

	return ret;
}


BOOL UIViewer::NotifyUILogOutput(LogLevel Level, PCWSTR Command, BOOL IsCmd)
{
	std::vector<std::wstring> Outputs;
	WCHAR                     Line[0x200];
	WCHAR                     Time[0x100];
	system_clock::time_point  TimePoint;
	time_t                    TimeInfo;
	tm                        CurrentTime;

	TimePoint   = system_clock::now();
	TimeInfo    = system_clock::to_time_t(TimePoint);

	localtime_s(&CurrentTime, &TimeInfo);
	wcsftime(Time, countof(Time), L"%Y-%m-%d %H:%M:%S", &CurrentTime);

	if (!Command)
		return FALSE;

	Outputs = SplitOutput(Command, L'\n');

	const auto IsWindow10 = NtCurrentPeb()->OSMajorVersion == 10;

	//
	// ⚠✔️❌💠ℹ️
	//
	for (auto& Output : Outputs)
	{
		switch (Level)
		{
		case LogLevel::LOG_DEBUG:
			if (!IsCmd) {
				FormatStringW(Line, L"[%s %s]%s", IsWindow10 ? L"💠" : L"DEBUG" , Time, Output.c_str());
			}
			else {
				FormatStringW(Line, L"[Cmd : %s %s]%s", Time, IsWindow10 ? L"💠" : L"DEBUG", Output.c_str());
			}

			ListBox_AddString(m_hWndVirtualConsole, Line);
			PrintConsoleW(L"%s\n", Line);
			break;

		case LogLevel::LOG_ERROR:
			if (!IsCmd) {
				FormatStringW(Line, L"[%s %s]%s", IsWindow10 ? L"❌" : L"ERROR", Time, Output.c_str());
			}
			else {
				FormatStringW(Line, L"[Cmd : %s %s]%s", Time, IsWindow10 ? L"❌" : L"ERROR", Output.c_str());
			}

			ListBox_AddString(m_hWndVirtualConsole, Line);
			PrintConsoleW(L"%s\n", Line);
			break;

		case LogLevel::LOG_INFO:
			if (!IsCmd) {
				FormatStringW(Line, L"[%s %s]%s", IsWindow10 ? L"ℹ️" : L"INFO", Time, Output.c_str());
			}
			else {
				FormatStringW(Line, L"[Cmd : %s %s]%s", Time, IsWindow10 ? L"ℹ️" : L"INFO", Output.c_str());
			}

			ListBox_AddString(m_hWndVirtualConsole, Line);
			PrintConsoleW(L"%s\n", Line);
			break;

		case LogLevel::LOG_WARN:
			if (!IsCmd) {
				FormatStringW(Line, L"[%s %s]%s", IsWindow10 ? L"⚠" : L"WARN", Time, Output.c_str());
			}
			else {
				FormatStringW(Line, L"[Cmd : %s %s]%s", Time, IsWindow10 ? L"⚠" : L"WARN", Output.c_str());
			}

			ListBox_AddString(m_hWndVirtualConsole, Line);
			PrintConsoleW(L"%s\n", Line);
			break;

		case LogLevel::LOG_OK:
			if (!IsCmd) {
				FormatStringW(Line, L"[%s %s]%s", IsWindow10 ? L"✔️" : L"OK", Time, Output.c_str());
			}
			else {
				FormatStringW(Line, L"[Cmd : %s %s]%s", Time, IsWindow10 ? L"✔️" : L"OK", Output.c_str());
			}

			ListBox_AddString(m_hWndVirtualConsole, Line);
			PrintConsoleW(L"%s\n", Line);
			break;
		}
	}

	return TRUE;
}

BOOL UIViewer::NotifyUIReady()
{
	INT_PTR Success;

	Success = DoModel((HINSTANCE)m_CurrentModule, MAKEINTRESOURCEW(IDD_KrkrMainDlg));
	return Success == 0 ? TRUE : FALSE;
}


BOOL UIViewer::NotifyUITaskError()
{
	MessageBoxW(m_hWndMain, L"Unknown error", L"KrkrExtract", MB_OK | MB_ICONERROR);
	return TRUE;
}

BOOL UIViewer::NotifyUIMessageBox(PCWSTR Description, ULONG Flags, BOOL Locked)
{
	switch (Locked)
	{
	case FALSE:
		MessageBoxW(nullptr, Description, L"KrkrExtract", Flags);
		break;

	default:
		MessageBoxW(m_hWndMain, Description, L"KrkrExtract", Flags);
		break;
	}
	
	return TRUE;
}


BOOL UIViewer::NotifyUIExitFromRemoteProcess()
{
	return TRUE;
}

