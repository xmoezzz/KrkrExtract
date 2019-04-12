#pragma once

NTSYSAPI
NTSTATUS
NTAPI
RtlConnectToSm(
    _In_ PUNICODE_STRING ApiPortName,
    _In_ HANDLE ApiPortHandle,
    _In_ DWORD ProcessImageType,
    _Out_ PHANDLE SmssConnection
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSendMsgToSm(
    _In_ HANDLE ApiPortHandle,
    _In_ PPORT_MESSAGE MessageData
    );
 
