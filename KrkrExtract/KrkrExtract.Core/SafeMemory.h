#pragma once

#include <my.h>
#include <memory>



template <class T> inline std::shared_ptr<T> AllocateMemorySafe(SIZE_T Size)
{
	return std::shared_ptr<T>(
		(T*)AllocateMemory(Size),
		[](T* Ptr)
	{
		if (Ptr) {
			FreeMemory(Ptr);
		}
	});
}

template <class T> inline std::shared_ptr<T> AllocateMemorySafeP(SIZE_T Size)
{
	return std::shared_ptr<T>(
		(T*)AllocateMemoryP(Size),
		[](T* Ptr)
	{
		if (Ptr) {
			FreeMemoryP(Ptr);
		}
	});
}

template <class T> inline std::shared_ptr<T> CopyStringSafe(T* Buffer)
{
	ULONG Length;

	switch (sizeof(T))
	{
	case 1:
		Length = StrLengthA((PCSTR)Buffer);
		break;

	case 2:
		Length = StrLengthW((PCWSTR)Buffer);
		break;

	default:
		return nullptr;
	}

	if (Length == 0)
		return nullptr;

	auto NewBuffer = AllocateMemorySafe<T>((Length + 1) * sizeof(T));
	if (!NewBuffer)
		return nullptr;

	switch (sizeof(T))
	{
	case 1: StrCopyA((PSTR) NewBuffer.get(), (PCSTR) Buffer); break;
	case 2: StrCopyW((PWSTR)NewBuffer.get(), (PCWSTR)Buffer); break;
	}
	
	return NewBuffer;
}

template <class T> inline std::shared_ptr<T> CopyStringSafeP(T* Buffer)
{
	ULONG Length;

	switch (sizeof(T))
	{
	case 1:
		Length = StrLengthA((PCSTR)Buffer);
		break;

	case 2:
		Length = StrLengthW((PCWSTR)Buffer);
		break;

	default:
		return nullptr;
	}

	if (Length == 0)
		return nullptr;

	auto NewBuffer = AllocateMemorySafeP<T>((Length + 1) * sizeof(T));
	if (!NewBuffer)
		return nullptr;

	switch (sizeof(T))
	{
	case 1: StrCopyA((PSTR) NewBuffer.get(), (PCSTR) Buffer); break;
	case 2: StrCopyW((PWSTR)NewBuffer.get(), (PCWSTR)Buffer); break;
	}

	return NewBuffer;
}

template <class T> inline std::shared_ptr<T> CopyMemorySafe(T* Buffer, SIZE_T Size)
{
	auto NewBuffer = AllocateMemorySafe(Size);
	if (!NewBuffer)
		return nullptr;

	RtlCopyMemory(NewBuffer.get(), Buffer, Size);
	return NewBuffer;
}

template <class T> inline std::shared_ptr<T> CopyMemorySafeP(T* Buffer, SIZE_T Size)
{
	auto NewBuffer = AllocateMemorySafeP(Size);
	if (!NewBuffer)
		return nullptr;

	RtlCopyMemory(NewBuffer.get(), Buffer, Size);
	return NewBuffer;
}

inline std::shared_ptr<PORT_MESSAGE> AllocateAlpcMessage(PPORT_MESSAGE PortMessage, ULONG MessageType, SIZE_T MessageSize, LPVOID Message)
{
	auto AlpcMessage = AllocateMemorySafeP<PORT_MESSAGE>(MessageSize + sizeof(PORT_MESSAGE) + sizeof(MessageType));
	if (!AlpcMessage)
		return AlpcMessage;

	RtlCopyMemory(AlpcMessage.get(), PortMessage, sizeof(PORT_MESSAGE));
	RtlCopyMemory((PBYTE)AlpcMessage.get() + sizeof(PORT_MESSAGE), &MessageType, sizeof(UINT32));
	RtlCopyMemory((PBYTE)AlpcMessage.get() + sizeof(PORT_MESSAGE) + sizeof(UINT32), Message, MessageSize);

	return AlpcMessage;
}



