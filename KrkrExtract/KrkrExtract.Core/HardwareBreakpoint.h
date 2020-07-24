#pragma once

#include <my.h>
#include "magic_enum.hpp"

enum class HwBreakPointStatus : ULONG
{
	SlotEmpty = 0,
	Slot0IsBusy = 1,
	Slot1IsBusy = 2,
	Slot2IsBusy = 4,
	Slot3IsBusy = 8
};

class HardwareBreakpoint
{
public:
	HardwareBreakpoint() { m_Index = -1; }
	~HardwareBreakpoint() { Clear(); }

	enum class Condition : ULONG
	{ 
		Write     = DR7_RW_WRITE,
		ReadWrite = DR7_RW_ACCESS,
		Execute   = DR7_RW_EXECUTE
	};

	enum class BreakPointLength : ULONG
	{
		OneByte   = DR7_LEN_1_BYTE,
		TwoBytes  = DR7_LEN_2_BYTE,
		FourBytes = DR7_LEN_4_BYTE,
#if defined(ML_AMD6)
		EightBytes = DR7_LEN_8_BYTE
#endif
	};

	static NTSTATUS FindAvailableRegister(INT& Index)
	{
		NTSTATUS         Status;
		CONTEXT          Context;
		PDR7_INFO        Dr7;

		Index = -1;
		Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
		Status = NtGetContextThread(NtCurrentThread(), &Context);
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"HardwareBreakpoint::FindAvailableRegister : NtGetContextThread failed %08x\n", Status);
			return Status;
		}

		Dr7 = (PDR7_INFO)&Context.Dr7;
		if (Dr7->G3 == 0 && Dr7->L3 == 0) {
			Index = 3;
		}
		else if (Dr7->G2 == 0 && Dr7->L2 == 0) {
			Index = 2;
		}
		else if (Dr7->G1 == 0 && Dr7->L1 == 0) {
			Index = 1;
		}
		else if (Dr7->G0 == 0 && Dr7->L0 == 0) {
			Index = 0;
		}

		if (Index >= 0) {
			return STATUS_SUCCESS;
		}

		PrintConsoleW(L"HardwareBreakpoint::FindAvailableRegister : No debug register avaliable\n");
		return STATUS_UNSUCCESSFUL;
	}

	NTSTATUS Set(PVOID Address, SIZE_T Length, Condition When, INT Index)
	{
		NTSTATUS         Status;
		CONTEXT          Context;
		BreakPointLength BreakLength;
		PDR7_INFO        Dr7;

		if (m_Index >= 0) {
			PrintConsoleW(L"HardwareBreakpoint::Set : Breakpoint is already set. (%d)\n", m_Index);
			return STATUS_UNSUCCESSFUL;
		}

		switch (Length)
		{
		case 1: 
			BreakLength = BreakPointLength::OneByte;
			break;

		case 2: 
			BreakLength = BreakPointLength::TwoBytes;
			break;

		case 4: 
			BreakLength = BreakPointLength::FourBytes;
			break;

#if defined(ML_AMD64)
		case 8:
			BreakLength = BreakPointLength::EightBytes;
#endif

		default:
			PrintConsoleW(L"HardwareBreakpoint::Set : Unsupported length %d\n", Length);
			return STATUS_NOT_SUPPORTED;
		}

		Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
		Status = NtGetContextThread(NtCurrentThread(), &Context);
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"HardwareBreakpoint::Set : NtGetContextThread failed %08x\n", Status);
			return Status;
		}

		Dr7 = (PDR7_INFO)&Context.Dr7;
		switch (Index)
		{
		case 0:
			if (Dr7->G0 != 0 || Dr7->L0 != 0) {
				PrintConsoleW(L"HardwareBreakpoint::Set : Debug register Dr%d is busy\n", Index);
				return STATUS_UNSUCCESSFUL;
			}
			Dr7->RW0 = (BYTE)When;
			Dr7->L0 = 1;
			Dr7->G0 = 0;
			Dr7->LEN0 = (BYTE)BreakLength;
			Context.Dr0 = (ULONG_PTR)Address;
			break;

		case 1:
			if (Dr7->G1 != 0 || Dr7->L1 != 0) {
				PrintConsoleW(L"HardwareBreakpoint::Set : Debug register Dr%d is busy\n", Index);
				return STATUS_UNSUCCESSFUL;
			}
			Dr7->RW1 = (BYTE)When;
			Dr7->L1 = 1;
			Dr7->G0 = 0;
			Dr7->LEN1 = (BYTE)BreakLength;
			Context.Dr1 = (ULONG_PTR)Address;
			break;

		case 2:
			if (Dr7->G2 != 0 || Dr7->L2 != 0) {
				PrintConsoleW(L"HardwareBreakpoint::Set : Debug register Dr%d is busy\n", Index);
				return STATUS_UNSUCCESSFUL;
			}
			Dr7->RW2 = (BYTE)When;
			Dr7->L2 = 1;
			Dr7->G2 = 0;
			Dr7->LEN2 = (BYTE)BreakLength;
			Context.Dr2 = (ULONG_PTR)Address;
			break;

		case 3:
			if (Dr7->G3 != 0 || Dr7->L3 != 0) {
				PrintConsoleW(L"HardwareBreakpoint::Set : Debug register Dr%d is busy\n", Index);
				return STATUS_UNSUCCESSFUL;
			}
			Dr7->RW3 = (BYTE)When;
			Dr7->L3 = 1;
			Dr7->G3 = 0;
			Dr7->LEN3 = (BYTE)BreakLength;
			Context.Dr3 = (ULONG_PTR)Address;
			break;

		default:
			PrintConsoleW(L"HardwareBreakpoint::Set : Unknown debug register %d\n", Index);
			return STATUS_UNSUCCESSFUL;
		}

		Status = NtSetContextThread(NtCurrentThread(), &Context);
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"HardwareBreakpoint::Set : NtSetContextThread failed %08x\n", Status);
		}

		m_Address = (ULONG_PTR)Address;
		m_Index   = Index;

		return Status;
	}

	static NTSTATUS ClearAll(PCONTEXT Context, HwBreakPointStatus CurrentBpStatus)
	{
		PDR7_INFO        Dr7;

		if (!Context) {
			PrintConsoleW(L"HardwareBreakpoint::Clear : Context %p\n", Context);
			return STATUS_INVALID_PARAMETER;
		}

		if (CurrentBpStatus == HwBreakPointStatus::SlotEmpty)
			return STATUS_SUCCESS;

		Dr7 = (PDR7_INFO)&Context->Dr7;

		using namespace magic_enum::bitwise_operators;
		if (FLAG_ON(CurrentBpStatus, HwBreakPointStatus::Slot0IsBusy))
		{
			Context->Dr0 = 0;
			Dr7->LEN0 = 0;
			Dr7->G0 = Dr7->L0 = 0;
		}

		if (FLAG_ON(CurrentBpStatus, HwBreakPointStatus::Slot1IsBusy))
		{
			Context->Dr1 = 0;
			Dr7->LEN1 = 0;
			Dr7->G1 = Dr7->L1 = 0;
		}
		
		if (FLAG_ON(CurrentBpStatus, HwBreakPointStatus::Slot2IsBusy))
		{
			Context->Dr2 = 0;
			Dr7->LEN2 = 0;
			Dr7->G2 = Dr7->L2 = 0;
		}

		if (FLAG_ON(CurrentBpStatus, HwBreakPointStatus::Slot3IsBusy))
		{
			Context->Dr3 = 0;
			Dr7->LEN3 = 0;
			Dr7->G3 = Dr7->L3 = 0;
		}

		return STATUS_SUCCESS;
	}

	ForceInline VOID Invaild()
	{
		m_Index = -1;
	}

	ForceInline BOOL IsBusy()
	{
		return m_Index != -1;
	}

	ForceInline ULONG_PTR GetAddress()
	{
		return m_Address;
	}

	NTSTATUS Clear()
	{
		NTSTATUS       Status;
		CONTEXT        Context;
		PDR7_INFO      Dr7;

		if (m_Index < 0)
			return STATUS_SUCCESS;

		Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
		Status = NtGetContextThread(NtCurrentThread(), &Context);
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"HardwareBreakpoint::Clear : NtGetContextThread failed %08x\n", Status);
			return Status;
		}

		Dr7 = (PDR7_INFO)&Context.Dr7;

		switch (m_Index)
		{
		case 0:
			Dr7->G0 = Dr7->L0 = 0;
			Dr7->LEN0 = 0;
			Context.Dr0 = 0;
			break;

		case 1:
			Dr7->G1 = Dr7->L1 = 0;
			Dr7->LEN1 = 0;
			Context.Dr1 = 0;
			break;

		case 2:
			Dr7->G2 = Dr7->L2 = 0;
			Dr7->LEN2 = 0;
			Context.Dr2 = 0;
			break;

		case 3:
			Dr7->G3 = Dr7->L3 = 0;
			Dr7->LEN3 = 0;
			Context.Dr3 = 0;
			break;
		}

		Status = NtSetContextThread(NtCurrentThread(), &Context);
		if (NT_FAILED(Status)) {
			PrintConsoleW(L"HardwareBreakpoint::Clear : NtSetContextThread failed %08x\n", Status);
			return Status;
		}

		m_Index = -1;
		return Status;
	}

protected:
	INT       m_Index   = -1;
	ULONG_PTR m_Address = 0;
};

