#pragma once

// Filter manager

#define FLT_PORT_CONNECT 0x0001
#define FLT_PORT_ALL_ACCESS (FLT_PORT_CONNECT | STANDARD_RIGHTS_ALL)

// VDM

typedef enum _VDMSERVICECLASS
{
    VdmStartExecution,
    VdmQueueInterrupt,
    VdmDelayInterrupt,
    VdmInitialize,
    VdmFeatures,
    VdmSetInt21Handler,
    VdmQueryDir,
    VdmPrinterDirectIoOpen,
    VdmPrinterDirectIoClose,
    VdmPrinterInitialize,
    VdmSetLdtEntries,
    VdmSetProcessLdtInfo,
    VdmAdlibEmulation,
    VdmPMCliControl,
    VdmQueryVdmProcess
} VDMSERVICECLASS, *PVDMSERVICECLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtVdmControl(
    _In_ VDMSERVICECLASS Service,
    _Inout_ PVOID ServiceData
    );

// WMI/ETW

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceEvent(
    _In_ HANDLE TraceHandle,
    _In_ ULONG Flags,
    _In_ ULONG FieldSize,
    _In_ PVOID Fields
    );


/*
		None = 0,
		String = 1,
		ExpandString = 2,
		Binary = 3,
		Dword = 4,
		DwordBigEndian = 5,
		Link = 6,
		MultiString = 7,
		ResourceList = 8,
		FullResourceDescriptor = 9,
		ResourceRequirementsList = 10,
		Qword = 11
*/


#if (NTDDI_VERSION >= NTDDI_VISTA)
// private
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTraceControl(
    _In_ ULONG FunctionCode,
    _In_reads_bytes_opt_(InBufferLen) PVOID InBuffer,
    _In_ ULONG InBufferLen,
    _Out_writes_bytes_opt_(OutBufferLen) PVOID OutBuffer,
    _In_ ULONG OutBufferLen,
    _Out_ PULONG ReturnLength
    );
#endif


typedef ULONG REGISTRY_VALUE_TYPE;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryLicenseValue(
	_In_ PUNICODE_STRING Name,
	_Out_ REGISTRY_VALUE_TYPE* Type,
	_Out_writes_bytes_opt_(Length) PVOID Buffer,
	_In_ ULONG Length,
	_Out_ PULONG ReturnLength
);


 
