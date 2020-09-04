#include "HandleTable.h"

#define LOW_LEVEL_MASK      (0x000003FFu)
#define MID_LEVEL_MASK      (0x000FFC00u)
#define HIGH_LEVEL_MASK     (0x00F00000u)
#define LOW_LEVEL_ENTRIES   0x400
#define MID_LEVEL_ENTRIES   0x400
#define HIGH_LEVEL_ENTRIES  0x10

#define GET_LOW_INDEX(Handle)       (PtrAnd(LOW_LEVEL_MASK, Handle))
#define GET_MID_INDEX(Handle)       (PtrAnd(MID_LEVEL_MASK, Handle) >> 10)
#define GET_HIGH_INDEX(Handle)      (PtrAnd(HIGH_LEVEL_MASK, Handle) >> 20)

PML_HANDLE_TABLE CreateHandleTable()
{
	PML_HANDLE_TABLE HandleTable = new ML_HANDLE_TABLE[HIGH_LEVEL_ENTRIES];

	if (HandleTable != NULL)
		ZeroMemory(HandleTable, sizeof(*HandleTable) * HIGH_LEVEL_ENTRIES);

	return HandleTable;
}

PML_HANDLE_TABLE_ENTRY CreateHandle(PML_HANDLE_TABLE HandleTable, HANDLE Handle)
{
	ULONG_PTR               LowIndex, MidIndex, HighIndex;
	PML_HANDLE_TABLE        MidLevel, HighLevel;
	PML_HANDLE_TABLE_ENTRY  LowLevel;

	if (HandleTable == NULL)
		return NULL;

	HighIndex = GET_HIGH_INDEX(Handle);

	HighLevel = HandleTable;
	MidLevel = HighLevel[HighIndex].NextLevel;
	if (MidLevel == NULL)
	{
		MidLevel = new ML_HANDLE_TABLE[MID_LEVEL_ENTRIES];
		if (MidLevel == NULL)
			return NULL;

		HighLevel[HighIndex].NextLevel = MidLevel;
		ZeroMemory(MidLevel, sizeof(*MidLevel) * MID_LEVEL_ENTRIES);
	}

	MidIndex = GET_MID_INDEX(Handle);
	MidLevel = MidLevel + MidIndex;
	LowLevel = MidLevel->Entries;
	if (LowLevel == NULL)
	{
		LowLevel = new ML_HANDLE_TABLE_ENTRY[MID_LEVEL_ENTRIES];
		if (LowLevel == NULL)
			return NULL;

		MidLevel->Entries = LowLevel;
		ZeroMemory(LowLevel, sizeof(*LowLevel) * LOW_LEVEL_ENTRIES);
	}

	LowIndex = GET_LOW_INDEX(Handle);

	LowLevel += LowIndex;
	MidLevel->Count += !LowLevel->Valid;
	LowLevel->Valid = TRUE;

	return LowLevel;
}

#define LOOKUP_HANDLE_MID_LEVEL     (0x80000000u)

PML_HANDLE_TABLE_ENTRY LookupHandleTableInternal(PML_HANDLE_TABLE HandleTable, HANDLE Handle, ULONG_PTR Flags)
{
	ULONG_PTR               LowIndex, MidIndex, HighIndex;
	PML_HANDLE_TABLE        MidLevel, HighLevel;
	PML_HANDLE_TABLE_ENTRY  LowLevel;

	if (HandleTable == NULL)
		return NULL;

	HighIndex = GET_HIGH_INDEX(Handle);

	HighLevel = HandleTable;
	MidLevel = HighLevel[HighIndex].NextLevel;
	if (MidLevel == NULL)
		return NULL;

	MidIndex = GET_MID_INDEX(Handle);
	MidLevel = MidLevel + MidIndex;
	if (FLAG_ON(Flags, LOOKUP_HANDLE_MID_LEVEL))
		return (PML_HANDLE_TABLE_ENTRY)MidLevel;

	LowLevel = MidLevel->Entries;
	if (LowLevel == NULL)
		return NULL;

	LowIndex = GET_LOW_INDEX(Handle);
	LowLevel += LowIndex;

	if (!LowLevel->Valid)
	{
		if (!FLAG_ON(Flags, LOOKUP_HANDLE_CREATE_IF))
			return NULL;

		LowLevel->Valid = TRUE;
		++MidLevel->Count;
	}

	return LowLevel;
}

PML_HANDLE_TABLE_ENTRY LookupHandleTable(PML_HANDLE_TABLE HandleTable, HANDLE Handle, ULONG_PTR Flags /* = 0 */)
{
	PML_HANDLE_TABLE_ENTRY Entry = LookupHandleTableInternal(HandleTable, Handle, Flags);

	if (Entry == NULL && FLAG_ON(Flags, LOOKUP_HANDLE_CREATE_IF))
		Entry = CreateHandle(HandleTable, Handle);

	return Entry;
}

VOID RemoveHandle(PML_HANDLE_TABLE HandleTable, HANDLE Handle)
{
	PML_HANDLE_TABLE MidLevel;
	PML_HANDLE_TABLE_ENTRY LowLevel;

	if (HandleTable == NULL)
		return;

	MidLevel = (PML_HANDLE_TABLE)LookupHandleTableInternal(HandleTable, Handle, LOOKUP_HANDLE_MID_LEVEL);
	if (MidLevel == NULL)
		return;

	if (MidLevel->Entries == NULL)
		return;

	LowLevel = &MidLevel->Entries[GET_LOW_INDEX(Handle)];

	if (!LowLevel->Valid)
		return;

	LowLevel->Valid = FALSE;
	LowLevel->Handle = NULL;

	if (--MidLevel->Count == 0)
	{
		SafeDeleteT(MidLevel->Entries);
	}
}

VOID
DestroyHandleTable(
	PML_HANDLE_TABLE                HandleTable,
	HANDLE_TABLE_DESTROY_CALLBACK   Callback /* = NULL */,
	PVOID                           Context /* = NULL */
)
{
	PML_HANDLE_TABLE MidLevel, HighLevel;

	if (HandleTable == NULL)
		return;

	HighLevel = HandleTable;
	for (ULONG_PTR HighCount = HIGH_LEVEL_ENTRIES; HighCount; ++HighLevel, --HighCount)
	{
		MidLevel = HighLevel->NextLevel;
		if (MidLevel == NULL)
			continue;

		for (ULONG_PTR MidCount = MID_LEVEL_ENTRIES; MidCount; ++MidLevel, --MidCount)
		{
			if (MidLevel->Entries == NULL)
				continue;

			if (Callback != NULL)
				Callback(MidLevel->Entries, LOW_LEVEL_ENTRIES, Context);

			delete MidLevel->Entries;
		}

		delete HighLevel->NextLevel;
	}

	delete HandleTable;
}

