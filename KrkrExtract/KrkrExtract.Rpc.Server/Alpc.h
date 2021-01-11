#pragma once

#include <my.h>
#include <stdexcept>
#include <exception>
#include <utility>
#include <string>
#include <vector>
#include <map>

class ALPCMessageAttribute {
public:
    ALPCMessageAttribute(ULONG Attributes) {
        ULONG retval;

        if (AlpcInitializeMessageAttribute(Attributes, nullptr, 0, &retval) != 0xC0000023) {  // buffer too small
            throw std::runtime_error("Failed to get attribute size.");
        }
        m_attributes = reinterpret_cast<PALPC_MESSAGE_ATTRIBUTES>(AllocateMemory(retval));
        if (!NT_SUCCESS(AlpcInitializeMessageAttribute(Attributes, m_attributes, retval, &retval))) {
            throw std::runtime_error("Failed to initialize attribute.");
        }
    }

    ~ALPCMessageAttribute() {
        if (m_attributes) FreeMemory(m_attributes);
        m_attributes = nullptr;
    }

    static ALPCMessageAttribute* from(PALPC_MESSAGE_ATTRIBUTES Attributes) {
        return new ALPCMessageAttribute(Attributes);
    }

    bool valid(ULONG Attribute) { return m_attributes->ValidAttributes & Attribute; }
    bool allocated(ULONG Attribute) { return m_attributes->AllocatedAttributes & Attribute; }
    PALPC_MESSAGE_ATTRIBUTES buffer() { return m_attributes; }

    void enable(ULONG Attribute) {
        if (allocated(Attribute))
            m_attributes->ValidAttributes |= Attribute;
    }

    void disable(ULONG Attribute) {
        if (allocated(Attribute) && valid(Attribute))
            m_attributes->ValidAttributes -= Attribute;
    }

    void clear() {
        m_attributes->ValidAttributes = 0;
    }

    template<typename T>
    T* get_pointer(ULONG Attribute) {
        return reinterpret_cast<T*>(get(Attribute));
    }

private:
    ALPCMessageAttribute(PALPC_MESSAGE_ATTRIBUTES Attributes) : m_attributes(Attributes) { }

    void* get(ULONG Attribute);

private:
    PALPC_MESSAGE_ATTRIBUTES m_attributes;
};

class ALPCMessage {
public:
    ALPCMessage(DWORD size) : m_size(size) {
        m_message = reinterpret_cast<PORT_MESSAGE*>(AllocateMemory(size + sizeof(PORT_MESSAGE)));
        if (!m_message) {
            throw std::runtime_error("Failed allocating message struct.");
        }

#pragma warning(disable:4244)
#pragma warning(disable:4267)
        m_message->u1.s1.DataLength = size;
        m_message->u1.s1.TotalLength = size + sizeof(PORT_MESSAGE);
#pragma warning(default:4244)
#pragma warning(default:4267)
        m_attribute = nullptr;
    }
    ALPCMessage(void* message) : m_message(reinterpret_cast<PORT_MESSAGE*>(message)) {
        m_attribute = nullptr;
    }

    ~ALPCMessage() {
        if (m_attribute)
            delete m_attribute;

        if (m_message)
            FreeMemory(m_message);

        m_message = nullptr;
        m_attribute = nullptr;
    }

    PPORT_MESSAGE buffer() { return m_message; }
    ULONG size() { return m_message->u1.s1.DataLength; }
    ULONG total_size() { return m_message->u1.s1.TotalLength; }
    PVOID data() { return reinterpret_cast<PVOID>((char*)m_message + sizeof(PORT_MESSAGE)); }
    CSHORT type() { return m_message->u2.s2.Type & 0xFFF; }

    void clear() {
        RtlZeroMemory(m_message, m_size + sizeof(PORT_MESSAGE));
#pragma warning(disable:4244)
#pragma warning(disable:4267)
        m_message->u1.s1.DataLength = m_size;
        m_message->u1.s1.TotalLength = m_size + sizeof(PORT_MESSAGE);
#pragma warning(default:4244)
#pragma warning(default:4267)

        if (m_attribute) {
            m_attribute->clear();
        }
    }

    PALPC_MESSAGE_ATTRIBUTES attributes() {
        if (!m_attribute)
            return nullptr;
        return m_attribute->buffer();
    }
    ALPCMessageAttribute* attrib() { return m_attribute; }
    void with_attribute(ALPCMessageAttribute* Attribute) {
        m_attribute = Attribute;
    }

private:
    ULONG m_size;
    PPORT_MESSAGE m_message;
    ALPCMessageAttribute* m_attribute;
};

class ALPCTransportBase {
public:
    HANDLE port() { return m_port; }
    void port(HANDLE Port) { m_port = Port; }

    virtual bool send(ALPCMessage& message, ULONG flags = 0);
    virtual bool recv(ALPCMessage& message, ULONG flags = 0);
    virtual bool sendrecv(ALPCMessage& send_message, ALPCMessage& recv_message, ULONG flags = 0);

private:
    HANDLE m_port;
};

class ALPCClient : public ALPCTransportBase {
public:
    static ALPCClient* connect(PCWSTR Name, ULONG MsgLen);

    ~ALPCClient() {
        NtAlpcDisconnectPort(port(), 0);
        CloseHandle(port());
    }

private:
    ALPCClient(HANDLE Port) { port(Port); }
};

class ALPCServer : public ALPCTransportBase {
public:
    static ALPCServer* create(PCWSTR Name, ULONG MsgLen);

    bool accept(ALPCMessage& message);
    bool handleClose(ALPCMessage& message);

    ~ALPCServer() {
        for (auto& [ph, dp] : m_processHandleToDataPort) {
            CloseHandle(dp);
            CloseHandle(ph);
        }
        NtAlpcDisconnectPort(port(), 0);
        CloseHandle(port());
    }

private:
    ALPCServer(HANDLE Port, ULONG MsgLen) : m_msgLen(MsgLen) { port(Port); }

    std::map<HANDLE, HANDLE> m_processHandleToDataPort;
    std::map<ULONG, HANDLE> m_pidToProcessHandle;

    ULONG m_msgLen;
};


