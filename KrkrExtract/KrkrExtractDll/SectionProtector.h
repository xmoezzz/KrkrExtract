#pragma once
#ifndef _SECTIONPROTECTOR_H_00185e71_a85a_4b7a_bc62_08ac6375404c_
#define _SECTIONPROTECTOR_H_00185e71_a85a_4b7a_bc62_08ac6375404c_

#include <my.h>

#if SUPPORT_VA_ARGS_MACRO

//#define PROTECT_SECTION_(Type, Ptr, ...) for (SectionProtector<Type> _cc(Ptr, __VA_ARGS__); _cc ; )


#define PROTECT_SECTION_WORKER(Type, Ptr, Name, ...) \
            for (SectionProtector<Type> _cc(Ptr, __VA_ARGS__); _cc.__Condition; _cc.__Condition = FALSE)


#define PROTECT_SECTION__(Type, Ptr, Name, ...) PROTECT_SECTION_WORKER(Type, Ptr, Name, __VA_ARGS__)
#define PROTECT_SECTION_(Type, Ptr, ...) PROTECT_SECTION__(Type, Ptr, MAKE_UNIQUE_NAME(__LINE__), __VA_ARGS__)
#define PROTECT_SECTION(LockPtr, ...) PROTECT_SECTION_(TYPE_OF(LockPtr), LockPtr, __VA_ARGS__)

#else // no va args

#define PROTECT_SECTION_(Type, Ptr) for (SectionProtector<Type> _cc(Ptr); _cc.__Number != 0 ; --_cc.__Number)
#define PROTECT_SECTION(LockPtr) PROTECT_SECTION_(TYPE_OF(LockPtr), LockPtr)

#endif // SUPPORT_VA_ARGS_MACRO

#define PROTECT_SECTION_INLINE ForceInline

namespace SectionProtectorTypes
{
	enum
	{
		SharedLock,
		ExclusiveLock,
	};

};

class SectionProtectorBase
{
public:
	BOOL __Condition;

	PROTECT_SECTION_INLINE SectionProtectorBase()
	{
		__Condition = TRUE;
	}
};

template<class LockType>
class SectionProtector : public SectionProtectorBase
{
private:
	SectionProtector(LockType *Lock) {}
};


#if ML_KERNEL_MODE

/************************************************************************
KernelMode
************************************************************************/

template <>
class SectionProtector<PKSPIN_LOCK> : public SectionProtectorBase
{
public:
	KIRQL Irql, Irqlx;
	PKSPIN_LOCK SpinLock;

	PROTECT_SECTION_INLINE SectionProtector(PKSPIN_LOCK SpinLock)
	{
		Irqlx = KeGetCurrentIrql();
		if (Irqlx > DISPATCH_LEVEL)
			return;

		KeAcquireSpinLock(SpinLock, &Irql);
		this->SpinLock = SpinLock;
	}

	PROTECT_SECTION_INLINE ~SectionProtector()
	{
		if (Irqlx > DISPATCH_LEVEL)
			return;

		KeReleaseSpinLock(SpinLock, Irql);
	}
};

template <>
class SectionProtector<PERESOURCE> : public SectionProtectorBase
{
public:
	KIRQL Irql;
	PERESOURCE Resource;

	PROTECT_SECTION_INLINE SectionProtector(PERESOURCE Resource, BOOL Shared = SectionProtectorTypes::SharedLock, BOOL Wait = TRUE)
	{
		Irql = KeGetCurrentIrql();
		if (Irql > APC_LEVEL)
			return;

		KeEnterCriticalRegion();
		(Shared == SectionProtectorTypes::SharedLock) ? ExAcquireResourceSharedLite(Resource, Wait) : ExAcquireResourceExclusiveLite(Resource, Wait);
		this->Resource = Resource;
	}

	PROTECT_SECTION_INLINE ~SectionProtector()
	{
		if (Irql > APC_LEVEL)
			return;

		ExReleaseResourceLite(Resource);
		KeLeaveCriticalRegion();
	}
};

#else // r3

template<>
class SectionProtector<PRTL_CRITICAL_SECTION> : public SectionProtectorBase
{
public:
	PRTL_CRITICAL_SECTION CriticalSection;

	SectionProtector(PRTL_CRITICAL_SECTION CriticalSection)
	{
		this->CriticalSection = CriticalSection;
		RtlEnterCriticalSection(CriticalSection);
	}

	~SectionProtector()
	{
		RtlLeaveCriticalSection(this->CriticalSection);
	}
};

template <>
class SectionProtector<PRTL_RESOURCE> : public SectionProtectorBase
{
public:
	PRTL_RESOURCE Resource;

	PROTECT_SECTION_INLINE SectionProtector(PRTL_RESOURCE Resource, BOOL Shared = TRUE, BOOL Wait = TRUE)
	{
		Shared ? RtlAcquireResourceShared(Resource, Wait) : RtlAcquireResourceExclusive(Resource, Wait);
		this->Resource = Resource;
	}

	PROTECT_SECTION_INLINE ~SectionProtector()
	{
		RtlReleaseResource(Resource);
	}
};

template<>
class SectionProtector<HANDLE> : public SectionProtectorBase
{
public:
	HANDLE m_Event;

	SectionProtector(HANDLE Event, ULONG_PTR Timeout = INFINITE, BOOL Altertable = FALSE)
	{
		LARGE_INTEGER TimeOut;

		m_Event = Event;

		FormatTimeOut(&TimeOut, Timeout);
		NtWaitForSingleObject(Event, Altertable, &TimeOut);
	}

	~SectionProtector()
	{
		NtSetEvent(m_Event, NULL);
	}
};

#endif // rx

#endif // _SECTIONPROTECTOR_H_00185e71_a85a_4b7a_bc62_08ac6375404c_
