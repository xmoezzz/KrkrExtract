#pragma once
#include "my.h"

typedef struct
{
	PVOID Handle;
	union
	{
		ULONG_PTR Flags;
		struct
		{
			BOOLEAN Valid : 1;
		};
	};

} ML_HANDLE_TABLE_ENTRY, *PML_HANDLE_TABLE_ENTRY;

typedef struct _ML_HANDLE_TABLE
{
	union
	{
		struct _ML_HANDLE_TABLE *NextLevel;
		PML_HANDLE_TABLE_ENTRY   Entries;
	};

	ULONG_PTR Count;

} ML_HANDLE_TABLE, *PML_HANDLE_TABLE;

PML_HANDLE_TABLE
CreateHandleTable(
	VOID
);

PML_HANDLE_TABLE_ENTRY
CreateHandle(
	PML_HANDLE_TABLE    HandleTable,
	HANDLE              Handle
);

#define LOOKUP_HANDLE_CREATE_IF     (0x00000001u)

PML_HANDLE_TABLE_ENTRY
LookupHandleTable(
	PML_HANDLE_TABLE    HandleTable,
	HANDLE              Handle,
	ULONG_PTR           Flags = 0
);

VOID
RemoveHandle(
	PML_HANDLE_TABLE    HandleTable,
	HANDLE              Handle
);

typedef NTSTATUS(*HANDLE_TABLE_DESTROY_CALLBACK)(PML_HANDLE_TABLE_ENTRY Entry, ULONG_PTR Count, PVOID Context);

#define HandleTableDestroyM(Entry, Count, Context) [] (PML_HANDLE_TABLE_ENTRY Entry, ULONG_PTR Count, PVOID Context) -> NTSTATUS

VOID
DestroyHandleTable(
	PML_HANDLE_TABLE                HandleTable,
	HANDLE_TABLE_DESTROY_CALLBACK   DEF_VAL(Callback, NULL),
	PVOID                           DEF_VAL(Context, NULL)
);

#if CPP_DEFINED

class MlHandleTable : public MemoryAllocator
{
protected:
	PML_HANDLE_TABLE HandleTable;

public:
	MlHandleTable()
	{
		HandleTable = NULL;
	}

	~MlHandleTable()
	{
		Destroy();
	}

	PML_HANDLE_TABLE Create()
	{
		if (HandleTable != NULL)
			return HandleTable;

		HandleTable = ::CreateHandleTable();
		return HandleTable;
	}

	template<class HandleType>
	PML_HANDLE_TABLE_ENTRY Insert(HandleType Handle)
	{
		return ::CreateHandle(HandleTable, (HANDLE)Handle);
	}

	template<class HandleType>
	PML_HANDLE_TABLE_ENTRY Lookup(HandleType Handle, ULONG_PTR Flags = 0)
	{
		return ::LookupHandleTable(HandleTable, (HANDLE)Handle, Flags);
	}

	template<class HandleType>
	VOID Remove(HandleType Handle)
	{
		return ::RemoveHandle(HandleTable, (HANDLE)Handle);
	}

	VOID Destroy(HANDLE_TABLE_DESTROY_CALLBACK Callback = NULL, PVOID Context = NULL)
	{
		::DestroyHandleTable(HandleTable, Callback, Context);
		HandleTable = NULL;
	}
};

#endif // CPP_DEFINED

