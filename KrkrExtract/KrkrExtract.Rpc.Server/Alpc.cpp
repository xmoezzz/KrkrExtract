#include "ALPC.h"

std::pair<ULONG, size_t> ATTRIBUTE_BY_FLAGS[] = {
  {ALPC_MESSAGE_SECURITY_ATTRIBUTE, sizeof(ALPC_SECURITY_ATTR)},
  {ALPC_MESSAGE_VIEW_ATTRIBUTE, sizeof(ALPC_DATA_VIEW_ATTR)},
  {ALPC_MESSAGE_CONTEXT_ATTRIBUTE, sizeof(ALPC_CONTEXT_ATTR)},
  {ALPC_MESSAGE_HANDLE_ATTRIBUTE, sizeof(ALPC_HANDLE_ATTR)},
  {ALPC_MESSAGE_TOKEN_ATTRIBUTE, sizeof(ALPC_TOKEN_ATTR)},
  {ALPC_MESSAGE_DIRECT_ATTRIBUTE, sizeof(ALPC_DIRECT_ATTR)},
  {ALPC_MESSAGE_WORK_ON_BEHALF_ATTRIBUTE, sizeof(ALPC_WORK_ON_BEHALF_ATTR)},
};

void* ALPCMessageAttribute::get(ULONG Attribute) {
    if (!allocated(Attribute))
        return nullptr;

    ULONG_PTR offset = sizeof(ALPC_MESSAGE_ATTRIBUTES);
    for (auto pair : ATTRIBUTE_BY_FLAGS) {
        if (pair.first == Attribute) {
            return reinterpret_cast<void*>((char*)m_attributes + offset);
        }
        if (allocated(pair.first))
            offset += pair.second;
    }
    return nullptr;
}

ALPCServer* ALPCServer::create(PCWSTR Name, ULONG MsgLen) {
    OBJECT_ATTRIBUTES obj_attrs;
    ALPC_PORT_ATTRIBUTES port_attrs;
    UNICODE_STRING port_name;
    NTSTATUS status;
    HANDLE port;

    RtlInitUnicodeString(&port_name, Name);
    InitializeObjectAttributes(&obj_attrs, &port_name, 0, nullptr, nullptr);

    RtlSecureZeroMemory(&port_attrs, sizeof(ALPC_PORT_ATTRIBUTES));
    port_attrs.MaxMessageLength = MsgLen;
    port_attrs.DupObjectTypes = 0xffffffff;

    status = NtAlpcCreatePort(&port, &obj_attrs, &port_attrs);
    if (!NT_SUCCESS(status)) {
        return nullptr;
    }
    return new (std::nothrow) ALPCServer(port, MsgLen);
}

bool ALPCServer::accept(ALPCMessage& message) {
    HANDLE dataPort;
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ProcessHandle;

    status = NtAlpcAcceptConnectPort(&dataPort, port(), 0, nullptr, nullptr, nullptr, message.buffer(), nullptr, TRUE);
    if (NT_FAILED(status)) {
        return false;
    }

    RtlZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

    status = NtOpenProcess(&ProcessHandle,
        PROCESS_QUERY_INFORMATION,
        &ObjectAttributes,
        &message.buffer()->ClientId
    );

    if (NT_FAILED(status)) {
        CloseHandle(dataPort);
        return false;
    }
    
    m_processHandleToDataPort[dataPort] = ProcessHandle;
    m_pidToProcessHandle[(ULONG)message.buffer()->ClientId.UniqueProcess] = ProcessHandle;
    return true;
}

bool ALPCServer::handleClose(ALPCMessage& message) {
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ProcessHandle;
    
    RtlZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

    status = NtOpenProcess(&ProcessHandle,
        PROCESS_QUERY_INFORMATION,
        &ObjectAttributes,
        &message.buffer()->ClientId
    );

    auto ptr = m_pidToProcessHandle.find((ULONG)message.buffer()->ClientId.UniqueProcess);
    if (ptr == m_pidToProcessHandle.end()) {
        PrintConsoleW(L"ALPCServer::handleClose : unknown pid %d\n", message.buffer()->ClientId.UniqueProcess);
        return false;
    }

    auto handle = m_pidToProcessHandle[(ULONG)message.buffer()->ClientId.UniqueProcess];
    CloseHandle(handle);

    return true;
}

ALPCClient* ALPCClient::connect(PCWSTR Name, ULONG MsgLen) {
    UNICODE_STRING port_name;
    NTSTATUS status;
    HANDLE port;
    ALPC_PORT_ATTRIBUTES port_attr;

    RtlInitUnicodeString(&port_name, Name);

    RtlSecureZeroMemory(&port_attr, sizeof(ALPC_PORT_ATTRIBUTES));
    port_attr.MaxMessageLength = MsgLen;
    port_attr.DupObjectTypes = 0xFFFFFFFF;
    port_attr.SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    port_attr.SecurityQos.ImpersonationLevel = SecurityImpersonation;
    port_attr.SecurityQos.ContextTrackingMode = 0;
    port_attr.SecurityQos.EffectiveOnly = 0;

    status = NtAlpcConnectPort(&port, &port_name, nullptr, &port_attr, ALPC_MSGFLG_SYNC_REQUEST, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (!NT_SUCCESS(status)) {
        return nullptr;
    }
    return new ALPCClient(port);
}

bool ALPCTransportBase::send(ALPCMessage& message, ULONG flags) {
    NTSTATUS status;

    status = NtAlpcSendWaitReceivePort(port(), flags, message.buffer(), message.attributes(), nullptr, nullptr, nullptr, nullptr);
    if (!NT_SUCCESS(status)) {
        return false;
    }
    return true;
}

bool ALPCTransportBase::recv(ALPCMessage& message, ULONG flags) {
    NTSTATUS status;
    SIZE_T size = message.total_size();

    status = NtAlpcSendWaitReceivePort(port(), flags, nullptr, nullptr, message.buffer(), &size, message.attributes(), nullptr);
    if (!NT_SUCCESS(status)) {
        return false;
    }
    return true;
}

bool ALPCTransportBase::sendrecv(ALPCMessage& send_message, ALPCMessage& recv_message, ULONG flags) {
    NTSTATUS status;
    SIZE_T size = recv_message.total_size();

    status = NtAlpcSendWaitReceivePort(port(), flags, send_message.buffer(), send_message.attributes(), recv_message.buffer(), &size, recv_message.attributes(), nullptr);
    if (!NT_SUCCESS(status)) {
        PrintConsoleW(L"ALPCTransportBase::sendrecv failedd : %x\n", status);
        return false;
    }
    return true;
}

