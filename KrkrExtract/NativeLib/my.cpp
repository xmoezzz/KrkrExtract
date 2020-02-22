#include "my.h"
#include "phnt_ntdef.h"
#include "ntrtl.h"
#include "ntldr.h"

#pragma warning(disable : 4996)

ULONG_PTR FASTCALL SizeOfProc32(PVOID Proc)
{
	ULONG_PTR   Length;
	PBYTE       pOpcode;
	ULONG_PTR   Result = 0;

	do
	{
		Length = LdeGetOpCodeSize32(Proc, (PVOID *)&pOpcode);
		Result += Length;
		if ((Length == 1) && (*pOpcode == 0xC3))
			break;

		Proc = (PVOID)((ULONG_PTR)Proc + Length);
	} while (Length);

	return Result;
}

ULONG_PTR FASTCALL SizeOfProc64(PVOID Proc)
{
	ULONG_PTR   Length;
	PBYTE       pOpcode;
	ULONG_PTR   Result = 0;

	do
	{
		Length = LdeGetOpCodeSize64(Proc, (PVOID *)&pOpcode);
		Result += Length;
		if ((Length == 1) && (*pOpcode == 0xC3))
			break;

		Proc = (PVOID)((ULONG_PTR)Proc + Length);
	} while (Length);

	return Result;
}

#define OP_X64_NONE           0x00
#define OP_X64_MODRM          0x01
#define OP_X64_DATA_I8        0x02
#define OP_X64_DATA_I16       0x04
#define OP_X64_DATA_I32       0x08
#define OP_X64_DATA_PRE66_67  0x10
#define OP_X64_WORD           0x20
#define OP_X64_REL32          0x40

ForceInline PUCHAR GetOpCodeFlags()
{
	static UCHAR OpcodeFlags[256] =
	{
		OP_X64_MODRM,                      // 00
		OP_X64_MODRM,                      // 01
		OP_X64_MODRM,                      // 02
		OP_X64_MODRM,                      // 03
		OP_X64_DATA_I8,                    // 04
		OP_X64_DATA_PRE66_67,              // 05
		OP_X64_NONE,                       // 06
		OP_X64_NONE,                       // 07
		OP_X64_MODRM,                      // 08
		OP_X64_MODRM,                      // 09
		OP_X64_MODRM,                      // 0A
		OP_X64_MODRM,                      // 0B
		OP_X64_DATA_I8,                    // 0C
		OP_X64_DATA_PRE66_67,              // 0D
		OP_X64_NONE,                       // 0E
		OP_X64_NONE,                       // 0F
		OP_X64_MODRM,                      // 10
		OP_X64_MODRM,                      // 11
		OP_X64_MODRM,                      // 12
		OP_X64_MODRM,                      // 13
		OP_X64_DATA_I8,                    // 14
		OP_X64_DATA_PRE66_67,              // 15
		OP_X64_NONE,                       // 16
		OP_X64_NONE,                       // 17
		OP_X64_MODRM,                      // 18
		OP_X64_MODRM,                      // 19
		OP_X64_MODRM,                      // 1A
		OP_X64_MODRM,                      // 1B
		OP_X64_DATA_I8,                    // 1C
		OP_X64_DATA_PRE66_67,              // 1D
		OP_X64_NONE,                       // 1E
		OP_X64_NONE,                       // 1F
		OP_X64_MODRM,                      // 20
		OP_X64_MODRM,                      // 21
		OP_X64_MODRM,                      // 22
		OP_X64_MODRM,                      // 23
		OP_X64_DATA_I8,                    // 24
		OP_X64_DATA_PRE66_67,              // 25
		OP_X64_NONE,                       // 26
		OP_X64_NONE,                       // 27
		OP_X64_MODRM,                      // 28
		OP_X64_MODRM,                      // 29
		OP_X64_MODRM,                      // 2A
		OP_X64_MODRM,                      // 2B
		OP_X64_DATA_I8,                    // 2C
		OP_X64_DATA_PRE66_67,              // 2D
		OP_X64_NONE,                       // 2E
		OP_X64_NONE,                       // 2F
		OP_X64_MODRM,                      // 30
		OP_X64_MODRM,                      // 31
		OP_X64_MODRM,                      // 32
		OP_X64_MODRM,                      // 33
		OP_X64_DATA_I8,                    // 34
		OP_X64_DATA_PRE66_67,              // 35
		OP_X64_NONE,                       // 36
		OP_X64_NONE,                       // 37
		OP_X64_MODRM,                      // 38
		OP_X64_MODRM,                      // 39
		OP_X64_MODRM,                      // 3A
		OP_X64_MODRM,                      // 3B
		OP_X64_DATA_I8,                    // 3C
		OP_X64_DATA_PRE66_67,              // 3D
		OP_X64_NONE,                       // 3E
		OP_X64_NONE,                       // 3F
		OP_X64_NONE,                       // 40
		OP_X64_NONE,                       // 41
		OP_X64_NONE,                       // 42
		OP_X64_NONE,                       // 43
		OP_X64_NONE,                       // 44
		OP_X64_NONE,                       // 45
		OP_X64_NONE,                       // 46
		OP_X64_NONE,                       // 47
		OP_X64_NONE,                       // 48
		OP_X64_NONE,                       // 49
		OP_X64_NONE,                       // 4A
		OP_X64_NONE,                       // 4B
		OP_X64_NONE,                       // 4C
		OP_X64_NONE,                       // 4D
		OP_X64_NONE,                       // 4E
		OP_X64_NONE,                       // 4F
		OP_X64_NONE,                       // 50
		OP_X64_NONE,                       // 51
		OP_X64_NONE,                       // 52
		OP_X64_NONE,                       // 53
		OP_X64_NONE,                       // 54
		OP_X64_NONE,                       // 55
		OP_X64_NONE,                       // 56
		OP_X64_NONE,                       // 57
		OP_X64_NONE,                       // 58
		OP_X64_NONE,                       // 59
		OP_X64_NONE,                       // 5A
		OP_X64_NONE,                       // 5B
		OP_X64_NONE,                       // 5C
		OP_X64_NONE,                       // 5D
		OP_X64_NONE,                       // 5E
		OP_X64_NONE,                       // 5F
		OP_X64_NONE,                       // 60
		OP_X64_NONE,                       // 61
		OP_X64_MODRM,                      // 62
		OP_X64_MODRM,                      // 63
		OP_X64_NONE,                       // 64
		OP_X64_NONE,                       // 65
		OP_X64_NONE,                       // 66
		OP_X64_NONE,                       // 67
		OP_X64_DATA_PRE66_67,              // 68
		OP_X64_MODRM | OP_X64_DATA_PRE66_67,   // 69
		OP_X64_DATA_I8,                    // 6A
		OP_X64_MODRM | OP_X64_DATA_I8,         // 6B
		OP_X64_NONE,                       // 6C
		OP_X64_NONE,                       // 6D
		OP_X64_NONE,                       // 6E
		OP_X64_NONE,                       // 6F
		OP_X64_DATA_I8,                    // 70
		OP_X64_DATA_I8,                    // 71
		OP_X64_DATA_I8,                    // 72
		OP_X64_DATA_I8,                    // 73
		OP_X64_DATA_I8,                    // 74
		OP_X64_DATA_I8,                    // 75
		OP_X64_DATA_I8,                    // 76
		OP_X64_DATA_I8,                    // 77
		OP_X64_DATA_I8,                    // 78
		OP_X64_DATA_I8,                    // 79
		OP_X64_DATA_I8,                    // 7A
		OP_X64_DATA_I8,                    // 7B
		OP_X64_DATA_I8,                    // 7C
		OP_X64_DATA_I8,                    // 7D
		OP_X64_DATA_I8,                    // 7E
		OP_X64_DATA_I8,                    // 7F
		OP_X64_MODRM | OP_X64_DATA_I8,         // 80
		OP_X64_MODRM | OP_X64_DATA_PRE66_67,   // 81
		OP_X64_MODRM | OP_X64_DATA_I8,         // 82
		OP_X64_MODRM | OP_X64_DATA_I8,         // 83
		OP_X64_MODRM,                      // 84
		OP_X64_MODRM,                      // 85
		OP_X64_MODRM,                      // 86
		OP_X64_MODRM,                      // 87
		OP_X64_MODRM,                      // 88
		OP_X64_MODRM,                      // 89
		OP_X64_MODRM,                      // 8A
		OP_X64_MODRM,                      // 8B
		OP_X64_MODRM,                      // 8C
		OP_X64_MODRM,                      // 8D
		OP_X64_MODRM,                      // 8E
		OP_X64_MODRM,                      // 8F
		OP_X64_NONE,                       // 90
		OP_X64_NONE,                       // 91
		OP_X64_NONE,                       // 92
		OP_X64_NONE,                       // 93
		OP_X64_NONE,                       // 94
		OP_X64_NONE,                       // 95
		OP_X64_NONE,                       // 96
		OP_X64_NONE,                       // 97
		OP_X64_NONE,                       // 98
		OP_X64_NONE,                       // 99
		OP_X64_DATA_I16 | OP_X64_DATA_PRE66_67,// 9A
		OP_X64_NONE,                       // 9B
		OP_X64_NONE,                       // 9C
		OP_X64_NONE,                       // 9D
		OP_X64_NONE,                       // 9E
		OP_X64_NONE,                       // 9F
		OP_X64_DATA_PRE66_67,              // A0
		OP_X64_DATA_PRE66_67,              // A1
		OP_X64_DATA_PRE66_67,              // A2
		OP_X64_DATA_PRE66_67,              // A3
		OP_X64_NONE,                       // A4
		OP_X64_NONE,                       // A5
		OP_X64_NONE,                       // A6
		OP_X64_NONE,                       // A7
		OP_X64_DATA_I8,                    // A8
		OP_X64_DATA_PRE66_67,              // A9
		OP_X64_NONE,                       // AA
		OP_X64_NONE,                       // AB
		OP_X64_NONE,                       // AC
		OP_X64_NONE,                       // AD
		OP_X64_NONE,                       // AE
		OP_X64_NONE,                       // AF
		OP_X64_DATA_I8,                    // B0
		OP_X64_DATA_I8,                    // B1
		OP_X64_DATA_I8,                    // B2
		OP_X64_DATA_I8,                    // B3
		OP_X64_DATA_I8,                    // B4
		OP_X64_DATA_I8,                    // B5
		OP_X64_DATA_I8,                    // B6
		OP_X64_DATA_I8,                    // B7
		OP_X64_DATA_PRE66_67,              // B8
		OP_X64_DATA_PRE66_67,              // B9
		OP_X64_DATA_PRE66_67,              // BA
		OP_X64_DATA_PRE66_67,              // BB
		OP_X64_DATA_PRE66_67,              // BC
		OP_X64_DATA_PRE66_67,              // BD
		OP_X64_DATA_PRE66_67,              // BE
		OP_X64_DATA_PRE66_67,              // BF
		OP_X64_MODRM | OP_X64_DATA_I8,         // C0
		OP_X64_MODRM | OP_X64_DATA_I8,         // C1
		OP_X64_DATA_I16,                   // C2
		OP_X64_NONE,                       // C3
		OP_X64_MODRM,                      // C4
		OP_X64_MODRM,                      // C5
		OP_X64_MODRM | OP_X64_DATA_I8,       // C6
		OP_X64_MODRM | OP_X64_DATA_PRE66_67, // C7
		OP_X64_DATA_I8 | OP_X64_DATA_I16,      // C8
		OP_X64_NONE,                       // C9
		OP_X64_DATA_I16,                   // CA
		OP_X64_NONE,                       // CB
		OP_X64_NONE,                       // CC
		OP_X64_DATA_I8,                    // CD
		OP_X64_NONE,                       // CE
		OP_X64_NONE,                       // CF
		OP_X64_MODRM,                      // D0
		OP_X64_MODRM,                      // D1
		OP_X64_MODRM,                      // D2
		OP_X64_MODRM,                      // D3
		OP_X64_DATA_I8,                    // D4
		OP_X64_DATA_I8,                    // D5
		OP_X64_NONE,                       // D6
		OP_X64_NONE,                       // D7
		OP_X64_WORD,                       // D8
		OP_X64_WORD,                       // D9
		OP_X64_WORD,                       // DA
		OP_X64_WORD,                       // DB
		OP_X64_WORD,                       // DC
		OP_X64_WORD,                       // DD
		OP_X64_WORD,                       // DE
		OP_X64_WORD,                       // DF
		OP_X64_DATA_I8,                    // E0
		OP_X64_DATA_I8,                    // E1
		OP_X64_DATA_I8,                    // E2
		OP_X64_DATA_I8,                    // E3
		OP_X64_DATA_I8,                    // E4
		OP_X64_DATA_I8,                    // E5
		OP_X64_DATA_I8,                    // E6
		OP_X64_DATA_I8,                    // E7
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // E8
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // E9
		OP_X64_DATA_I16 | OP_X64_DATA_PRE66_67,// EA
		OP_X64_DATA_I8,                    // EB
		OP_X64_NONE,                       // EC
		OP_X64_NONE,                       // ED
		OP_X64_NONE,                       // EE
		OP_X64_NONE,                       // EF
		OP_X64_NONE,                       // F0
		OP_X64_NONE,                       // F1
		OP_X64_NONE,                       // F2
		OP_X64_NONE,                       // F3
		OP_X64_NONE,                       // F4
		OP_X64_NONE,                       // F5
		OP_X64_MODRM,                      // F6
		OP_X64_MODRM,                      // F7
		OP_X64_NONE,                       // F8
		OP_X64_NONE,                       // F9
		OP_X64_NONE,                       // FA
		OP_X64_NONE,                       // FB
		OP_X64_NONE,                       // FC
		OP_X64_NONE,                       // FD
		OP_X64_MODRM,                      // FE
		OP_X64_MODRM | OP_X64_REL32            // FF
	};

	return OpcodeFlags;
}


ForceInline PUCHAR GetOpCodeFlagsExt()
{
	static UCHAR OpcodeFlagsExt[256] =
	{
		OP_X64_MODRM,                      // 00
		OP_X64_MODRM,                      // 01
		OP_X64_MODRM,                      // 02
		OP_X64_MODRM,                      // 03
		OP_X64_NONE,                       // 04
		OP_X64_NONE,                       // 05
		OP_X64_NONE,                       // 06
		OP_X64_NONE,                       // 07
		OP_X64_NONE,                       // 08
		OP_X64_NONE,                       // 09
		OP_X64_NONE,                       // 0A
		OP_X64_NONE,                       // 0B
		OP_X64_NONE,                       // 0C
		OP_X64_MODRM,                      // 0D
		OP_X64_NONE,                       // 0E
		OP_X64_MODRM | OP_X64_DATA_I8,         // 0F
		OP_X64_MODRM,                      // 10
		OP_X64_MODRM,                      // 11
		OP_X64_MODRM,                      // 12
		OP_X64_MODRM,                      // 13
		OP_X64_MODRM,                      // 14
		OP_X64_MODRM,                      // 15
		OP_X64_MODRM,                      // 16
		OP_X64_MODRM,                      // 17
		OP_X64_MODRM,                      // 18
		OP_X64_NONE,                       // 19
		OP_X64_NONE,                       // 1A
		OP_X64_NONE,                       // 1B
		OP_X64_NONE,                       // 1C
		OP_X64_NONE,                       // 1D
		OP_X64_NONE,                       // 1E
		OP_X64_NONE,                       // 1F
		OP_X64_MODRM,                      // 20
		OP_X64_MODRM,                      // 21
		OP_X64_MODRM,                      // 22
		OP_X64_MODRM,                      // 23
		OP_X64_MODRM,                      // 24
		OP_X64_NONE,                       // 25
		OP_X64_MODRM,                      // 26
		OP_X64_NONE,                       // 27
		OP_X64_MODRM,                      // 28
		OP_X64_MODRM,                      // 29
		OP_X64_MODRM,                      // 2A
		OP_X64_MODRM,                      // 2B
		OP_X64_MODRM,                      // 2C
		OP_X64_MODRM,                      // 2D
		OP_X64_MODRM,                      // 2E
		OP_X64_MODRM,                      // 2F
		OP_X64_NONE,                       // 30
		OP_X64_NONE,                       // 31
		OP_X64_NONE,                       // 32
		OP_X64_NONE,                       // 33
		OP_X64_NONE,                       // 34
		OP_X64_NONE,                       // 35
		OP_X64_NONE,                       // 36
		OP_X64_NONE,                       // 37
		OP_X64_NONE,                       // 38
		OP_X64_NONE,                       // 39
		OP_X64_NONE,                       // 3A
		OP_X64_NONE,                       // 3B
		OP_X64_NONE,                       // 3C
		OP_X64_NONE,                       // 3D
		OP_X64_NONE,                       // 3E
		OP_X64_NONE,                       // 3F
		OP_X64_MODRM,                      // 40
		OP_X64_MODRM,                      // 41
		OP_X64_MODRM,                      // 42
		OP_X64_MODRM,                      // 43
		OP_X64_MODRM,                      // 44
		OP_X64_MODRM,                      // 45
		OP_X64_MODRM,                      // 46
		OP_X64_MODRM,                      // 47
		OP_X64_MODRM,                      // 48
		OP_X64_MODRM,                      // 49
		OP_X64_MODRM,                      // 4A
		OP_X64_MODRM,                      // 4B
		OP_X64_MODRM,                      // 4C
		OP_X64_MODRM,                      // 4D
		OP_X64_MODRM,                      // 4E
		OP_X64_MODRM,                      // 4F
		OP_X64_MODRM,                      // 50
		OP_X64_MODRM,                      // 51
		OP_X64_MODRM,                      // 52
		OP_X64_MODRM,                      // 53
		OP_X64_MODRM,                      // 54
		OP_X64_MODRM,                      // 55
		OP_X64_MODRM,                      // 56
		OP_X64_MODRM,                      // 57
		OP_X64_MODRM,                      // 58
		OP_X64_MODRM,                      // 59
		OP_X64_MODRM,                      // 5A
		OP_X64_MODRM,                      // 5B
		OP_X64_MODRM,                      // 5C
		OP_X64_MODRM,                      // 5D
		OP_X64_MODRM,                      // 5E
		OP_X64_MODRM,                      // 5F
		OP_X64_MODRM,                      // 60
		OP_X64_MODRM,                      // 61
		OP_X64_MODRM,                      // 62
		OP_X64_MODRM,                      // 63
		OP_X64_MODRM,                      // 64
		OP_X64_MODRM,                      // 65
		OP_X64_MODRM,                      // 66
		OP_X64_MODRM,                      // 67
		OP_X64_MODRM,                      // 68
		OP_X64_MODRM,                      // 69
		OP_X64_MODRM,                      // 6A
		OP_X64_MODRM,                      // 6B
		OP_X64_MODRM,                      // 6C
		OP_X64_MODRM,                      // 6D
		OP_X64_MODRM,                      // 6E
		OP_X64_MODRM,                      // 6F
		OP_X64_MODRM | OP_X64_DATA_I8,         // 70
		OP_X64_MODRM | OP_X64_DATA_I8,         // 71
		OP_X64_MODRM | OP_X64_DATA_I8,         // 72
		OP_X64_MODRM | OP_X64_DATA_I8,         // 73
		OP_X64_MODRM,                      // 74
		OP_X64_MODRM,                      // 75
		OP_X64_MODRM,                      // 76
		OP_X64_NONE,                       // 77
		OP_X64_NONE,                       // 78
		OP_X64_NONE,                       // 79
		OP_X64_NONE,                       // 7A
		OP_X64_NONE,                       // 7B
		OP_X64_MODRM,                      // 7C
		OP_X64_MODRM,                      // 7D
		OP_X64_MODRM,                      // 7E
		OP_X64_MODRM,                      // 7F
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 80
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 81
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 82
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 83
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 84
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 85
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 86
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 87
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 88
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 89
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8A
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8B
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8C
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8D
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8E
		OP_X64_DATA_PRE66_67 | OP_X64_REL32,   // 8F
		OP_X64_MODRM,                      // 90
		OP_X64_MODRM,                      // 91
		OP_X64_MODRM,                      // 92
		OP_X64_MODRM,                      // 93
		OP_X64_MODRM,                      // 94
		OP_X64_MODRM,                      // 95
		OP_X64_MODRM,                      // 96
		OP_X64_MODRM,                      // 97
		OP_X64_MODRM,                      // 98
		OP_X64_MODRM,                      // 99
		OP_X64_MODRM,                      // 9A
		OP_X64_MODRM,                      // 9B
		OP_X64_MODRM,                      // 9C
		OP_X64_MODRM,                      // 9D
		OP_X64_MODRM,                      // 9E
		OP_X64_MODRM,                      // 9F
		OP_X64_NONE,                       // A0
		OP_X64_NONE,                       // A1
		OP_X64_NONE,                       // A2
		OP_X64_MODRM,                      // A3
		OP_X64_MODRM | OP_X64_DATA_I8,         // A4
		OP_X64_MODRM,                      // A5
		OP_X64_NONE,                       // A6
		OP_X64_NONE,                       // A7
		OP_X64_NONE,                       // A8
		OP_X64_NONE,                       // A9
		OP_X64_NONE,                       // AA
		OP_X64_MODRM,                      // AB
		OP_X64_MODRM | OP_X64_DATA_I8,         // AC
		OP_X64_MODRM,                      // AD
		OP_X64_MODRM,                      // AE
		OP_X64_MODRM,                      // AF
		OP_X64_MODRM,                      // B0
		OP_X64_MODRM,                      // B1
		OP_X64_MODRM,                      // B2
		OP_X64_MODRM,                      // B3
		OP_X64_MODRM,                      // B4
		OP_X64_MODRM,                      // B5
		OP_X64_MODRM,                      // B6
		OP_X64_MODRM,                      // B7
		OP_X64_NONE,                       // B8
		OP_X64_NONE,                       // B9
		OP_X64_MODRM | OP_X64_DATA_I8,         // BA
		OP_X64_MODRM,                      // BB
		OP_X64_MODRM,                      // BC
		OP_X64_MODRM,                      // BD
		OP_X64_MODRM,                      // BE
		OP_X64_MODRM,                      // BF
		OP_X64_MODRM,                      // C0
		OP_X64_MODRM,                      // C1
		OP_X64_MODRM | OP_X64_DATA_I8,         // C2
		OP_X64_MODRM,                      // C3
		OP_X64_MODRM | OP_X64_DATA_I8,         // C4
		OP_X64_MODRM | OP_X64_DATA_I8,         // C5
		OP_X64_MODRM | OP_X64_DATA_I8,         // C6
		OP_X64_MODRM,                      // C7
		OP_X64_NONE,                       // C8
		OP_X64_NONE,                       // C9
		OP_X64_NONE,                       // CA
		OP_X64_NONE,                       // CB
		OP_X64_NONE,                       // CC
		OP_X64_NONE,                       // CD
		OP_X64_NONE,                       // CE
		OP_X64_NONE,                       // CF
		OP_X64_MODRM,                      // D0
		OP_X64_MODRM,                      // D1
		OP_X64_MODRM,                      // D2
		OP_X64_MODRM,                      // D3
		OP_X64_MODRM,                      // D4
		OP_X64_MODRM,                      // D5
		OP_X64_MODRM,                      // D6
		OP_X64_MODRM,                      // D7
		OP_X64_MODRM,                      // D8
		OP_X64_MODRM,                      // D9
		OP_X64_MODRM,                      // DA
		OP_X64_MODRM,                      // DB
		OP_X64_MODRM,                      // DC
		OP_X64_MODRM,                      // DD
		OP_X64_MODRM,                      // DE
		OP_X64_MODRM,                      // DF
		OP_X64_MODRM,                      // E0
		OP_X64_MODRM,                      // E1
		OP_X64_MODRM,                      // E2
		OP_X64_MODRM,                      // E3
		OP_X64_MODRM,                      // E4
		OP_X64_MODRM,                      // E5
		OP_X64_MODRM,                      // E6
		OP_X64_MODRM,                      // E7
		OP_X64_MODRM,                      // E8
		OP_X64_MODRM,                      // E9
		OP_X64_MODRM,                      // EA
		OP_X64_MODRM,                      // EB
		OP_X64_MODRM,                      // EC
		OP_X64_MODRM,                      // ED
		OP_X64_MODRM,                      // EE
		OP_X64_MODRM,                      // EF
		OP_X64_MODRM,                      // F0
		OP_X64_MODRM,                      // F1
		OP_X64_MODRM,                      // F2
		OP_X64_MODRM,                      // F3
		OP_X64_MODRM,                      // F4
		OP_X64_MODRM,                      // F5
		OP_X64_MODRM,                      // F6
		OP_X64_MODRM,                      // F7
		OP_X64_MODRM,                      // F8
		OP_X64_MODRM,                      // F9
		OP_X64_MODRM,                      // FA
		OP_X64_MODRM,                      // FB
		OP_X64_MODRM,                      // FC
		OP_X64_MODRM,                      // FD
		OP_X64_MODRM,                      // FE
		OP_X64_NONE                        // FF
	};

	return OpcodeFlagsExt;
}


ULONG_PTR FASTCALL LdeGetOpCodeSize64(PVOID Code, PVOID *OpCodePtr)
{
	// OpCode High 5 bits as index, (1 << low 3bits) as flag

	static UCHAR PrefixTable[0x20] =
	{
		0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x00
	};

	PUCHAR      Ptr;
	BOOL        PFX66, PFX67;
	BOOL        SibPresent;
	BOOL        Rex;
	ULONG_PTR   Flags;
	ULONG_PTR   Mod, RM, Reg;
	ULONG_PTR   OffsetSize, Imm64;
	ULONG_PTR   Opcode;

	Imm64 = 0;
	OffsetSize = 0;
	PFX66 = FALSE;
	PFX67 = FALSE;
	Rex = FALSE;
	Ptr = (PUCHAR)Code;

	//    while ( (*cPtr == 0x2E) || (*cPtr == 0x3E) || (*cPtr == 0x36) ||
	//            (*cPtr == 0x26) || (*cPtr == 0x64) || (*cPtr == 0x65) ||
	//            (*cPtr == 0xF0) || (*cPtr == 0xF2) || (*cPtr == 0xF3) ||
	//            (*cPtr == 0x66) || (*cPtr == 0x67) )
	while (PrefixTable[*Ptr >> 3] & (1 << (*Ptr & 7)))
	{
		PFX66 = *Ptr == 0x66;
		PFX67 = *Ptr == 0x67;
		Ptr++;
		if (Ptr > (PUCHAR)Code + 16)
			return 0;
	}

	// 0x40 ~ 0x4F
	if (((*Ptr) >> 4) == 0x4)
	{
		Rex = *Ptr & 0x0F;
		if (Rex)
			++Ptr;
	}

	Opcode = *Ptr;
	if (OpCodePtr)
		*OpCodePtr = Ptr;

	if (*Ptr == 0x0F)
	{
		Ptr++;
		Flags = GetOpCodeFlagsExt()[*Ptr];
	}
	else
	{
		ULONG_PTR tmp;

		Flags = GetOpCodeFlags()[Opcode];

		// if (Opcode >= 0xA0 && Opcode <= 0xA3)
		tmp = Opcode >> 2;
		if (tmp == 0x28)
		{
			PFX66 = PFX67;
		}
		else if (Rex && (tmp >> 1) == 0x17)     // 0xB8 ~ 0xBF  mov r64, imm64
		{
			Imm64 = 4;
		}
	}

	Ptr++;
	Ptr += FLAG_ON(Flags, OP_X64_WORD);

	if (Flags & OP_X64_MODRM)
	{
		Mod = *Ptr >> 6;
		Reg = (*Ptr & 0x38) >> 3;
		RM = *Ptr & 7;
		Ptr++;

		if ((Opcode == 0xF6) && !Reg)
			SET_FLAG(Flags, OP_X64_DATA_I8);

		if ((Opcode == 0xF7) && !Reg)
			SET_FLAG(Flags, OP_X64_DATA_PRE66_67);

		SibPresent = !PFX67 & (RM == 4);
		switch (Mod)
		{
		case 0:
			if (PFX67 && (RM == 6))
				OffsetSize = 2;
			if (!PFX67 && (RM == 5))
				OffsetSize = 4;
			break;

		case 1:
			OffsetSize = 1;
			break;

		case 2:
			OffsetSize = PFX67 ? 2 : 4;
			break;

		case 3:
			SibPresent = FALSE;
			break;
		}

		if (SibPresent)
		{
			if (((*Ptr & 7) == 5) && ((!Mod) || (Mod == 2)))
				OffsetSize = 4;

			Ptr++;
		}

		Ptr += OffsetSize;
	}

	Ptr += FLAG_ON(Flags, OP_X64_DATA_I8);
	Ptr += FLAG_ON(Flags, OP_X64_DATA_I16) ? 2 : 0;
	Ptr += FLAG_ON(Flags, OP_X64_DATA_I32) ? 4 : 0;
	Ptr += FLAG_ON(Flags, OP_X64_DATA_PRE66_67) ? (PFX66 ? 2 : 4) : 0;
	Ptr += (Rex & 9) ? Imm64 : 0;   // 0x48 || 0x49

	return (ULONG_PTR)Ptr - (ULONG_PTR)Code;
}




#define OP_X86_NONE           0x000
#define OP_X86_DATA_I8        0x001
#define OP_X86_DATA_I16       0x002
#define OP_X86_DATA_I32       0x004
#define OP_X86_MODRM          0x008
#define OP_X86_DATA_PRE66_67  0x010
#define OP_X86_PREFIX         0x020
#define OP_X86_REL32          0x040
#define OP_X86_REL8           0x080

/* extended opcode flags (by analyzer) */
#define OP_X86_EXTENDED       0x100

ForceInline PBYTE GetPackedTable()
{
	static BYTE PackedTable[] =
	{
		0x80, 0x84, 0x80, 0x84, 0x80, 0x84, 0x80, 0x84,
		0x80, 0x88, 0x80, 0x88, 0x80, 0x88, 0x80, 0x88,
		0x8c, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b,
		0x90, 0x94, 0x98, 0x8b, 0x9c, 0x9c, 0x9c, 0x9c,
		0xa0, 0x80, 0x80, 0x80, 0x8b, 0x8b, 0xa4, 0x8b,
		0xa8, 0x8b, 0x84, 0x8b, 0xac, 0xac, 0xa8, 0xa8,
		0xb0, 0xb4, 0xb8, 0xbc, 0x80, 0xc0, 0x80, 0x80,
		0x9c, 0xac, 0xc4, 0x8b, 0xc8, 0x90, 0x8b, 0x90,
		0x80, 0x8b, 0x8b, 0xcc, 0x80, 0x80, 0xd0, 0x8b,
		0x80, 0xd4, 0x80, 0x80, 0x8b, 0x8b, 0x8b, 0x8b,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0xd8, 0xdc, 0x8b, 0x80,
		0xe0, 0xe0, 0xe0, 0xe0, 0x80, 0x80, 0x80, 0x80,
		0x8f, 0xcf, 0x8f, 0xdb, 0x80, 0x80, 0xe4, 0x80,
		0xe8, 0xd9, 0x8b, 0x8b, 0x80, 0x80, 0x80, 0x80,
		0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xdc,
		0x08, 0x08, 0x08, 0x08, 0x01, 0x10, 0x00, 0x00,
		0x01, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x08, 0x08, 0x20, 0x20, 0x20, 0x20,
		0x10, 0x18, 0x01, 0x09, 0x81, 0x81, 0x81, 0x81,
		0x09, 0x18, 0x09, 0x09, 0x00, 0x00, 0x12, 0x00,
		0x10, 0x10, 0x10, 0x10, 0x01, 0x01, 0x01, 0x01,
		0x09, 0x09, 0x02, 0x00, 0x08, 0x08, 0x09, 0x18,
		0x03, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00,
		0x01, 0x01, 0x00, 0x00, 0x50, 0x50, 0x12, 0x81,
		0x20, 0x00, 0x20, 0x20, 0x00, 0x08, 0x00, 0x09,
		0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00,
		0x09, 0x09, 0x09, 0x09, 0x08, 0x08, 0x08, 0x00,
		0x50, 0x50, 0x50, 0x50, 0x00, 0x00, 0x09, 0x08,
		0x08, 0x08, 0x09, 0x08
	};

	return PackedTable;
}

ForceInline ULONG_PTR GetOpCodeFlags32(ULONG_PTR OpCode)
{
	return GetPackedTable()[GetPackedTable()[OpCode / 4] + (OpCode % 4)];
}

ULONG_PTR GetOpCodeExtendedFlags32(unsigned char *addr)
{
	ULONG_PTR opcode;

	opcode = addr[0];
	if (opcode == 0x0F)
	{
		opcode = addr[1] | OP_X86_EXTENDED;
	}

	return GetOpCodeFlags32(opcode) | (opcode & OP_X86_EXTENDED);
}

ULONG_PTR FASTCALL LdeGetOpCodeSize32(PVOID Code, PVOID *OpCodePtr)
{
	unsigned char i_mod, i_rm, i_reg;
	ULONG_PTR op1, op2, flags;
	ULONG_PTR pfx66, pfx67;
	ULONG_PTR osize, oflen;
	PBYTE code;

	pfx66 = 0;
	pfx67 = 0;
	osize = 0;
	oflen = 0;

	code = (PBYTE)Code;
	op1 = *code;

	/* skip preffixes */
	while (GetOpCodeFlags32(op1) & OP_X86_PREFIX)
	{
		switch (op1)
		{
		case 0x66:
			pfx66 = 1;
			break;

		case 0x67:
			pfx67 = 1;
			break;
		}

		osize++;
		op1 = *++code;
	}

	/* get opcode size and flags */
	if (OpCodePtr != NULL)
		*OpCodePtr = code;

	++code;
	osize++;

	if (op1 == 0x0F)
	{
		op2 = (*code | OP_X86_EXTENDED);
		code++;
		osize++;
	}
	else
	{
		op2 = op1;

		/* pfx66 = pfx67 for opcodes A0 - A3 */
		if (op2 >= 0xA0 && op2 <= 0xA3)
		{
			pfx66 = pfx67;
		}
	}

	flags = GetOpCodeFlags32(op2);

	/* process MODRM byte */
	if (flags & OP_X86_MODRM)
	{
		i_mod = (*code >> 6);
		i_reg = (*code & 0x38) >> 3;
		i_rm = (*code & 7);
		code++;
		osize++;

		/* in F6 and F7 opcodes, immediate value present if i_reg == 0 */
		if (op1 == 0xF6 && i_reg == 0)
		{
			flags |= OP_X86_DATA_I8;
		}
		if (op1 == 0xF7 && i_reg == 0)
		{
			flags |= OP_X86_DATA_PRE66_67;
		}

		switch (i_mod)
		{
		case 0:
			if (pfx67)
			{
				if (i_rm == 6)
					oflen = 2;
			}
			else
			{
				if (i_rm == 5)
					oflen = 4;
			}
			break;

		case 1:
			oflen = 1;
			break;

		case 2:
			if (pfx67)
				oflen = 2;
			else
				oflen = 4;
			break;
		}

		/* process SIB byte */
		if (pfx67 == 0 && i_rm == 4 && i_mod != 3)
		{
			if ((*code & 7) == 5 && (i_mod != 1))
			{
				oflen = 4;
			}

			oflen++;
		}

		osize += oflen;
	}

	/* process offset */
	if (flags & OP_X86_DATA_PRE66_67)
	{
		osize += 4 - (pfx66 << 1);
	}
	/* process immediate value */
	osize += (flags & 7);

	return osize;
}



PVOID FASTCALL GetRoutineAddress(PVOID ModuleBase, LPCSTR RoutineName)
{
	PVOID       ProcAddress;
	LONG        Ordinal;
	NTSTATUS    Status;
	ANSI_STRING ProcString, *ProcName;

	if ((ULONG_PTR)RoutineName > 0xFFFF)
	{
		RtlInitAnsiString(&ProcString, (PSTR)RoutineName);
		ProcName = &ProcString;
		Ordinal = 0;
	}
	else
	{
		ProcName = NULL;
		Ordinal = (LONG)(LONG_PTR)RoutineName;
	}

	Status = LdrGetProcedureAddress(ModuleBase, ProcName, (USHORT)Ordinal, &ProcAddress);
	if (!NT_SUCCESS(Status))
		return NULL;

	return ProcAddress == (PVOID)ModuleBase ? NULL : (PVOID)ProcAddress;
}



PLDR_MODULE FindLdrModuleByName(PUNICODE_STRING ModuleName)
{
	PLDR_MODULE Ldr;
	PLIST_ENTRY LdrLink, NextLink;
	PVOID       LoaderLockCookie;
	NTSTATUS    Status;

	Status = LdrLockLoaderLock(0, NULL, &LoaderLockCookie);

	LdrLink = &Nt_CurrentPeb()->Ldr->InLoadOrderModuleList;
	NextLink = LdrLink->Flink;

	if (ModuleName == NULL)
	{
		if (NT_SUCCESS(Status))
			LdrUnlockLoaderLock(0, LoaderLockCookie);

		return FIELD_BASE(NextLink, LDR_MODULE, InLoadOrderLinks);
	}

	while (NextLink != LdrLink)
	{
		Ldr = FIELD_BASE(NextLink, LDR_MODULE, InLoadOrderLinks);

		if (RtlEqualUnicodeString(ModuleName, &Ldr->BaseDllName, TRUE))
		{
			if (NT_SUCCESS(Status))
				LdrUnlockLoaderLock(0, LoaderLockCookie);

			return Ldr;
		}

		NextLink = NextLink->Flink;
	}

	if (NT_SUCCESS(Status))
		LdrUnlockLoaderLock(0, LoaderLockCookie);

	return NULL;
}

PVOID Nt_GetModuleHandle(LPCWSTR lpModuleName)
{
	UNICODE_STRING  ModuleFileName;
	PLDR_MODULE     Module;

	RtlInitUnicodeString(&ModuleFileName, (PWSTR)lpModuleName);
	Module = FindLdrModuleByName(&ModuleFileName);
	return Module == NULL ? NULL : Module->DllBase;
}

PVOID FASTCALL Nt_GetProcAddress(PVOID ModuleBase, LPCSTR lpProcName)
{
	return GetRoutineAddress(ModuleBase, lpProcName);
}


NTSTATUS
Nt_CreateThread(
	PVOID                   StartAddress,
	PVOID                   StartParameter,
	BOOL                    CreateSuspended,
	HANDLE                  ProcessHandle,
	PHANDLE                 ThreadHandle,
	PCLIENT_ID              ClientID,
	ULONG                   StackZeroBits,
	ULONG                   StackReserved,
	ULONG                   StackCommit,
	PSECURITY_DESCRIPTOR    SecurityDescriptor
)
{
	NTSTATUS    Status;
	CLIENT_ID   ThreadId;
	HANDLE      NewThreadHandle;

	Status = RtlCreateUserThread(
		ProcessHandle,
		SecurityDescriptor,
		(BOOLEAN)CreateSuspended,
		StackZeroBits,
		StackReserved,
		StackCommit,
		(PUSER_THREAD_START_ROUTINE)StartAddress,
		StartParameter,
		&NewThreadHandle,
		&ThreadId
	);
	if (!NT_SUCCESS(Status))
		return Status;

	if (ClientID != NULL)
		*ClientID = ThreadId;

	if (ThreadHandle != NULL)
	{
		*ThreadHandle = NewThreadHandle;
	}
	else
	{
		NtClose(NewThreadHandle);
	}

	return Status;
}



PLDR_MODULE FindLdrModuleByHandle(PVOID BaseAddress)
{
	PLDR_MODULE Ldr;
	PLIST_ENTRY LdrLink, NextLink;

	if (BaseAddress != NULL)
	{
		NTSTATUS Status;

		Status = LdrFindEntryForAddress(BaseAddress, &Ldr);
		return NT_SUCCESS(Status) ? Ldr : NULL;
	}

	LdrLink = &Nt_CurrentPeb()->Ldr->InLoadOrderModuleList;
	NextLink = LdrLink->Flink;

	return FIELD_BASE(NextLink, LDR_MODULE, InLoadOrderLinks);
}


PVOID GetImageBaseAddress(PVOID ImageAddress)
{
	PLDR_MODULE LdrModule;

	LdrModule = FindLdrModuleByHandle(ImageAddress);

	return LdrModule == nullptr ? nullptr : LdrModule->DllBase;
}


PLDR_MODULE Nt_FindLdrModuleByHandle(PVOID BaseAddress)
{
	return FindLdrModuleByHandle(BaseAddress);
}


ULONG
InternalCopyUnicodeString(
	PUNICODE_STRING Unicode,
	PWCHAR          Buffer,
	ULONG_PTR       BufferCount,
	BOOL            IsDirectory = FALSE
)
{
	ULONG_PTR Length;

	Length = MIN(Unicode->Length, BufferCount * sizeof(WCHAR));
	CopyMemory(Buffer, Unicode->Buffer, Length);
	Length /= sizeof(WCHAR);

	if (IsDirectory && Length < BufferCount && Buffer[Length - 1] != '\\')
		Buffer[Length++] = '\\';

	if (Length < BufferCount)
		Buffer[Length] = 0;

	return Length;
}

ULONG_PTR Nt_GetModulePath(PVOID ModuleBase, PWSTR Path, ULONG_PTR BufferCount)
{
	PLDR_MODULE LdrModule;
	ULONG_PTR   PathLength;

	LdrModule = Nt_FindLdrModuleByHandle(ModuleBase);
	PathLength = (LdrModule->FullDllName.Length - LdrModule->BaseDllName.Length) / sizeof(WCHAR);

	InternalCopyUnicodeString(&LdrModule->FullDllName, Path, PathLength);
	if (PathLength < BufferCount)
		Path[PathLength] = 0;

	return PathLength;
}

ULONG Nt_GetExeDirectory(PWCHAR Path, ULONG BufferCount)
{
	return Nt_GetModulePath(NULL, Path, BufferCount);
}


ForceInline ULONG HashAPI(PCCHAR pszName)
{
	ULONG Hash = 0;

	while (*(PBYTE)pszName)
	{
		Hash = _rotl(Hash, 0x0D) ^ *(PBYTE)pszName++;
	}

	return Hash;
}


BOOL ValidateDataDirectory(PIMAGE_DATA_DIRECTORY DataDirectory, ULONG_PTR SizeOfImage)
{
	if (DataDirectory->Size == 0)
		return FALSE;

	if ((ULONG64)DataDirectory->VirtualAddress + DataDirectory->Size > ULONG_MAX)
		return FALSE;

	return DataDirectory->VirtualAddress <= SizeOfImage && DataDirectory->VirtualAddress + DataDirectory->Size <= SizeOfImage;
}


BOOL IsValidImage(PVOID ImageBase, ULONG_PTR Flags)
{
	PVOID                       Base, End, EndOfImage;
	ULONG_PTR                   Size, SizeOfImage;
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS           NtHeader;
	PIMAGE_NT_HEADERS64         NtHeader64;
	PIMAGE_DATA_DIRECTORY       DataDirectory, Directory;
	PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return FALSE;

	NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)ImageBase + DosHeader->e_lfanew);
	if (NtHeader->Signature != IMAGE_NT_SIGNATURE)
		return FALSE;

	if (Flags == 0)
		return TRUE;

	switch (NtHeader->OptionalHeader.Magic)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		DataDirectory = NtHeader->OptionalHeader.DataDirectory;
		SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		NtHeader64 = (PIMAGE_NT_HEADERS64)NtHeader;
		DataDirectory = NtHeader64->OptionalHeader.DataDirectory;
		SizeOfImage = NtHeader64->OptionalHeader.SizeOfImage;
		break;

	default:
		return FALSE;
	}

	if (FLAG_ON(Flags, IMAGE_VALID_IMPORT_ADDRESS_TABLE))
	{
		if (!ValidateDataDirectory(&DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT], SizeOfImage))
			return FALSE;

		ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)ImageBase + DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		if (ImportDescriptor->Name == 0)
			return FALSE;
	}

	if (FLAG_ON(Flags, IMAGE_VALID_EXPORT_ADDRESS_TABLE))
	{
		PIMAGE_EXPORT_DIRECTORY ExportDirectory;

		if (!ValidateDataDirectory(&DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT], SizeOfImage))
			return FALSE;

		ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((ULONG_PTR)ImageBase + DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		if (ExportDirectory->AddressOfFunctions >= SizeOfImage)
			return FALSE;
	}

	if (FLAG_ON(Flags, IMAGE_VALID_RELOC))
	{
		if (!ValidateDataDirectory(&DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC], SizeOfImage))
			return FALSE;
	}

	if (FLAG_ON(Flags, IMAGE_VALID_RESOURCE))
	{
		if (!ValidateDataDirectory(&DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE], SizeOfImage))
			return FALSE;
	}

	return TRUE;
}



PVOID EATLookupRoutineByHashNoFix64(PVOID ImageBase, ULONG_PTR Hash)
{
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS64         NtHeaders;
	PIMAGE_EXPORT_DIRECTORY     ExportDirectory;
	ULONG_PTR                   NumberOfNames;
	PULONG                      AddressOfFuntions;
	PULONG                      AddressOfNames;
	PUSHORT                     AddressOfNameOrdinals;

	if (Hash != 0 && !IsValidImage(ImageBase, IMAGE_VALID_EXPORT_ADDRESS_TABLE))
		return nullptr;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	NtHeaders = (PIMAGE_NT_HEADERS64)((ULONG_PTR)ImageBase + DosHeader->e_lfanew);

	if (Hash == 0)
	{
		return &NtHeaders->OptionalHeader.AddressOfEntryPoint;
	}

	ExportDirectory = nullptr;
	ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((ULONG_PTR)ExportDirectory + (PBYTE)ImageBase);
	ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((ULONG_PTR)ExportDirectory + NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	NumberOfNames     = ExportDirectory->NumberOfNames;
	AddressOfFuntions = (PULONG)ImageBase + (ULONG_PTR)ExportDirectory->AddressOfFunctions;
	AddressOfNames    = (PULONG)ImageBase + (ULONG_PTR)ExportDirectory->AddressOfNames;
	AddressOfNameOrdinals = (PUSHORT)ImageBase + (ULONG_PTR)ExportDirectory->AddressOfNameOrdinals;

	do
	{
		if (!(HashAPI((PCCHAR)((PCSTR)ImageBase + *AddressOfNames)) ^ Hash))
		{
			return AddressOfFuntions + *AddressOfNameOrdinals;
		}

		++AddressOfNameOrdinals;
		++AddressOfNames;
	} while (--NumberOfNames);

	return nullptr;
}


PVOID EATLookupRoutineByHashPNoFix64(PVOID ImageBase, ULONG_PTR Hash)
{
	PVOID Pointer;

	Pointer = EATLookupRoutineByHashNoFix64(ImageBase, Hash);
	if (Pointer != nullptr)
		Pointer = (PBYTE)(ULONG_PTR)*(PULONG)Pointer + ((ULONG_PTR)ImageBase);

	return Pointer;
}

typedef struct
{
	PVOID                       ImageBase;
	PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;
	PVOID                       EndOfImage;
	PVOID                       EndOfTable;
	ULONG_PTR                   SizeOfImage;
	ULONG_PTR                   SizeOfTable;
	ULONG_PTR                   NtHeadersVersion;

} WALK_IMPORT_TABLE_INTERNAL_DATA, *PWALK_IMPORT_TABLE_INTERNAL_DATA;


typedef struct
{
	PVOID                       ImageBase;
	PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;

	union
	{
		PIMAGE_THUNK_DATA       ThunkData;
		PIMAGE_THUNK_DATA32     ThunkData32;
		PIMAGE_THUNK_DATA64     ThunkData64;
	};

	PCSTR                       DllName;
	PCSTR                       FunctionName;
	ULONG_PTR                   Ordinal;
	PVOID                       Context;

} WALK_IMPORT_TABLE_DATA, *PWALK_IMPORT_TABLE_DATA;


template<class CallbackRoutine, class CallbackContext, class PIMAGE_THUNK_DATA_TYPE>
inline NTSTATUS WalkImportTableInternal(PWALK_IMPORT_TABLE_INTERNAL_DATA InternalData, CallbackRoutine Callback, CallbackContext Context)
{
	NTSTATUS                    Status;
	PIMAGE_THUNK_DATA_TYPE      OriginalThunk, FirstThunk;
	PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;

	ImportDescriptor = InternalData->ImportDescriptor;

	for (; ImportDescriptor->Name != NULL && ImportDescriptor->FirstThunk != NULL; ++ImportDescriptor)
	{
		LONG_PTR DllName;

		if (ImportDescriptor->FirstThunk > InternalData->SizeOfImage)
			continue;

		if (*(PULONG_PTR)PtrAdd(InternalData->ImageBase, ImportDescriptor->FirstThunk) == NULL)
			continue;

		OriginalThunk = (PIMAGE_THUNK_DATA_TYPE)InternalData->ImageBase;
		if (ImportDescriptor->OriginalFirstThunk != NULL)
		{
			OriginalThunk = PtrAdd(OriginalThunk, ImportDescriptor->OriginalFirstThunk);
		}
		else
		{
			OriginalThunk = PtrAdd(OriginalThunk, ImportDescriptor->FirstThunk);
		}

		if (OriginalThunk >= InternalData->EndOfImage)
			continue;

		DllName = PtrAdd((LONG_PTR)InternalData->ImageBase, ImportDescriptor->Name);
		if ((PVOID)DllName >= InternalData->EndOfImage)
			continue;

		FirstThunk = (PIMAGE_THUNK_DATA_TYPE)PtrAdd(InternalData->ImageBase, ImportDescriptor->FirstThunk);
		while (OriginalThunk->u1.AddressOfData != NULL)
		{
			LONG_PTR    FunctionName;
			ULONG_PTR   Ordinal;

			FunctionName = (LONG_PTR)OriginalThunk->u1.AddressOfData;
			if (FunctionName < 0)
			{
				Ordinal = (USHORT)FunctionName;
				FunctionName = NULL;
			}
			else
			{
				Ordinal = IMAGE_INVALID_ORDINAL;
				FunctionName += (LONG_PTR)PtrAdd(InternalData->ImageBase, 2);
			}

			WALK_IMPORT_TABLE_DATA Data;

			Data.ImageBase = InternalData->ImageBase;
			Data.ImportDescriptor = ImportDescriptor;
			Data.ThunkData = (PIMAGE_THUNK_DATA)FirstThunk;
			Data.DllName = (PCSTR)DllName;
			Data.Ordinal = Ordinal;
			Data.FunctionName = (PCSTR)FunctionName;
			Data.Context = (PVOID)(ULONG_PTR)Context;

			Status = Callback(&Data);
			if (Status == STATUS_VALIDATE_CONTINUE)
				break;

			FAIL_RETURN(Status);

			++OriginalThunk;
			++FirstThunk;
		}
	}

	return STATUS_SUCCESS;
}


template<class CallbackRoutine, class CallbackContext>
inline NTSTATUS WalkImportTableT(PVOID ImageBase, CallbackRoutine Callback, CallbackContext Context = nullptr)
{
	ULONG_PTR                   NtHeadersVersion;
	NTSTATUS                    Status;
	PIMAGE_NT_HEADERS32         NtHeaders32;
	PIMAGE_NT_HEADERS64         NtHeaders64;

	WALK_IMPORT_TABLE_INTERNAL_DATA InternalData;

	if (!IsValidImage(ImageBase, IMAGE_VALID_IMPORT_ADDRESS_TABLE))
		return STATUS_INVALID_IMAGE_FORMAT;

	NtHeaders32 = (PIMAGE_NT_HEADERS32)ImageNtHeaders(ImageBase, &NtHeadersVersion);
	NtHeaders64 = (PIMAGE_NT_HEADERS64)NtHeaders32;

	InternalData.ImageBase = ImageBase;
	InternalData.ImportDescriptor = nullptr;

	switch (NtHeadersVersion)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		InternalData.ImportDescriptor = PtrAdd(InternalData.ImportDescriptor, NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		InternalData.SizeOfTable = NtHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
		InternalData.SizeOfImage = NtHeaders32->OptionalHeader.SizeOfImage;
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		InternalData.ImportDescriptor = PtrAdd(InternalData.ImportDescriptor, NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		InternalData.SizeOfTable = NtHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
		InternalData.SizeOfImage = NtHeaders64->OptionalHeader.SizeOfImage;
		break;

	default:
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	InternalData.ImportDescriptor = PtrAdd(InternalData.ImportDescriptor, ImageBase);
	InternalData.EndOfImage = PtrAdd(ImageBase, InternalData.SizeOfImage);
	InternalData.EndOfTable = PtrAdd(InternalData.ImportDescriptor, InternalData.SizeOfTable);

	switch (NtHeadersVersion)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		return WalkImportTableInternal<CallbackRoutine, CallbackContext, PIMAGE_THUNK_DATA32>(&InternalData, Callback, Context);

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		return WalkImportTableInternal<CallbackRoutine, CallbackContext, PIMAGE_THUNK_DATA64>(&InternalData, Callback, Context);
	}

	return STATUS_INVALID_IMAGE_FORMAT;
}


typedef NTSTATUS(*WalkImportTableCallback)(PWALK_IMPORT_TABLE_DATA Data);

#define WalkIATCallbackM(Data) [&] (PWALK_IMPORT_TABLE_DATA Data) -> NTSTATUS


PVOID LookupImportTable(PVOID ImageBase, PCSTR DllName, ULONG Hash)
{
	PCSTR TargetDllName;
	PVOID IATVA;

	IATVA = IMAGE_INVALID_VA;
	TargetDllName = nullptr;

	WalkImportTableT(ImageBase,
		WalkIATCallbackM(Data)
	{
		if (DllName != nullptr)
		{
			if (TargetDllName != nullptr)
			{
				if (TargetDllName != Data->DllName)
					return STATUS_NOT_FOUND;
			}
			else if (lstrcmpiA(Data->DllName, DllName) != 0)
			{
				return STATUS_VALIDATE_CONTINUE;
			}
			else
			{
				TargetDllName = Data->DllName;
			}
		}

		if (Data->FunctionName == nullptr)
			return STATUS_SUCCESS;

		if (HashAPI((PCCHAR)Data->FunctionName) == Hash)
		{
			IATVA = Data->ThunkData;
			return STATUS_NO_MORE_MATCHES;
		}

		return STATUS_SUCCESS;
	},
		0
		);

	return IATVA;
}

PVOID LookupImportTable(PVOID ImageBase, PCSTR DllName, PCSTR RoutineName)
{
	return LookupImportTable(ImageBase, DllName, HashAPI((PCCHAR)RoutineName));
}


PVOID
LoadDll(
	PCWSTR ModuleFileName
)
{
	PVOID           ModuleBase;
	NTSTATUS        Status;
	UNICODE_STRING  ModuleFile;

	RtlInitUnicodeString(&ModuleFile, (PWSTR)ModuleFileName);

	Status = LdrLoadDll(NULL, NULL, &ModuleFile, &ModuleBase);

	return NT_SUCCESS(Status) ? ModuleBase : NULL;
}

NTSTATUS
UnloadDll(
	PVOID DllHandle
)
{
	return LdrUnloadDll(DllHandle);
}


PVOID Nt_LoadLibrary(PWSTR ModuleFileName)
{
	return LoadDll(ModuleFileName);
}


NtFileDisk::NtFileDisk()
{
	m_FileHandle = nullptr;
}

NtFileDisk::NtFileDisk(const NtFileDisk &file)
{
	*this = file;
}

NtFileDisk::operator HANDLE() const
{
	return m_FileHandle;
}

NtFileDisk& NtFileDisk::operator=(const NtFileDisk &file)
{
	if (this == &file)
		return *this;

	Close();

	NtDuplicateObject(
		NtCurrentProcess(),
		file,
		NtCurrentProcess(),
		&m_FileHandle,
		0,
		0,
		DUPLICATE_SAME_ACCESS
	);

	return *this;
}

NtFileDisk& NtFileDisk::operator=(HANDLE Handle)
{
	if (m_FileHandle == Handle)
		return *this;

	Close();

	NtDuplicateObject(
		NtCurrentProcess(),
		Handle,
		NtCurrentProcess(),
		&m_FileHandle,
		0,
		0,
		DUPLICATE_SAME_ACCESS
	);

	return *this;
}

NTSTATUS NtFileDisk::QueryFullNtPath(PCWSTR FileName, PUNICODE_STRING NtFilePath, ULONG_PTR Flags)
{
	WCHAR ExpandBuffer[MAX_PATH];
	UNICODE_STRING Path, Expand;

	static UNICODE_STRING DosDevicesPrefix = RTL_CONSTANT_STRING((PWSTR)L"\\??\\");

	if (FLAG_ON(Flags, NFD_EXPAND_ENVVAR))
	{
		RtlInitUnicodeString(&Path, (PWSTR)FileName);

		RtlInitEmptyString(&Expand, ExpandBuffer, sizeof(ExpandBuffer));
		RtlExpandEnvironmentStrings_U(nullptr, &Path, &Expand, nullptr);
	}
	else
	{
		RtlInitUnicodeString(&Expand, (PWSTR)FileName);
	}

	if (
		Expand.Length >= DosDevicesPrefix.Length &&
		RtlCompareMemory(Expand.Buffer, DosDevicesPrefix.Buffer, DosDevicesPrefix.Length) == DosDevicesPrefix.Length
		)
	{
		return RtlDuplicateUnicodeString(RTL_DUPSTR_ADD_NULL, &Expand, NtFilePath);
	}

	return RtlDosPathNameToNtPathName_U(Expand.Buffer, NtFilePath, nullptr, nullptr) ? STATUS_SUCCESS : STATUS_OBJECT_PATH_NOT_FOUND;
}

NTSTATUS
NtFileDisk::
CreateFileInternalWithFullPath(
	PHANDLE         FileHandle,
	PUNICODE_STRING FileName,
	ULONG_PTR       Flags,
	ULONG_PTR       ShareMode,
	ULONG_PTR       Access,
	ULONG_PTR       CreationDisposition,
	ULONG_PTR       Attributes,
	ULONG_PTR       CreateOptions /* = 0 */
)
{
	NTSTATUS            Status;
	OBJECT_ATTRIBUTES   ObjectAttributes;
	IO_STATUS_BLOCK     IoStatus;

	if (!FLAG_ON(Attributes, 0x40000000))
	{
		CreateOptions |= FILE_SYNCHRONOUS_IO_NONALERT;
	}

	CLEAR_FLAG(Attributes, 0x40000000);
	InitializeObjectAttributes(&ObjectAttributes, FileName, OBJ_CASE_INSENSITIVE, nullptr, nullptr);

	Status = NtCreateFile(
		FileHandle,
		Access | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
		&ObjectAttributes,
		&IoStatus,
		nullptr,
		Attributes,
		ShareMode,
		CreationDisposition,
		CreateOptions,
		nullptr,
		0
	);

	return Status;
}

NTSTATUS
NtFileDisk::
CreateFileInternal(
	PHANDLE     FileHandle,
	LPCWSTR     FileName,
	ULONG_PTR   Flags,
	ULONG_PTR   ShareMode,
	ULONG_PTR   Access,
	ULONG_PTR   CreationDisposition,
	ULONG_PTR   Attributes,
	ULONG_PTR   CreateOptions /* = 0 */
)
{
	NTSTATUS            Status;
	UNICODE_STRING      NtFilePath;

	if (FLAG_ON(Flags, NFD_NOT_RESOLVE_PATH))
	{
		RtlInitUnicodeString(&NtFilePath, (PWSTR)FileName);
	}
	else
	{
		Status = QueryFullNtPath(FileName, &NtFilePath, Flags);
		if (!NT_SUCCESS(Status))
			return Status;
	}

	Status = CreateFileInternalWithFullPath(
		FileHandle,
		&NtFilePath,
		Flags,
		ShareMode,
		Access,
		CreationDisposition,
		Attributes,
		CreateOptions
	);

	if (!FLAG_ON(Flags, NFD_NOT_RESOLVE_PATH))
		RtlFreeUnicodeString(&NtFilePath);

	return Status;
}

NTSTATUS NtFileDisk::GetSizeInternal(HANDLE FileHandle, PLARGE_INTEGER pFileSize)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;
	FILE_STANDARD_INFORMATION FileStandard;

	if (pFileSize == nullptr)
		return STATUS_INVALID_PARAMETER;

	pFileSize->QuadPart = 0;

	Status = NtQueryInformationFile(
		FileHandle,
		&IoStatus,
		&FileStandard,
		sizeof(FileStandard),
		FileStandardInformation
	);

	if (!NT_SUCCESS(Status))
		return Status;

	pFileSize->QuadPart = FileStandard.EndOfFile.QuadPart;

	return Status;
}

NTSTATUS
NtFileDisk::
SeekInternal(
	HANDLE          FileHandle,
	LARGE_INTEGER   Offset,
	ULONG_PTR           MoveMethod,
	PLARGE_INTEGER  NewPosition /* = NULL */
)
{
	NTSTATUS                    Status;
	IO_STATUS_BLOCK             IoStatus;
	FILE_POSITION_INFORMATION   FilePosition;
	FILE_STANDARD_INFORMATION   FileStandard;

	switch (MoveMethod)
	{
	case FILE_CURRENT:
		NtQueryInformationFile(
			FileHandle,
			&IoStatus,
			&FilePosition,
			sizeof(FilePosition),
			FilePositionInformation);

		FilePosition.CurrentByteOffset.QuadPart += Offset.QuadPart;
		break;

	case FILE_END:
		NtQueryInformationFile(
			FileHandle,
			&IoStatus,
			&FileStandard,
			sizeof(FileStandard),
			FileStandardInformation);

		FilePosition.CurrentByteOffset.QuadPart = FileStandard.EndOfFile.QuadPart + Offset.QuadPart;
		break;

	case FILE_BEGIN:
		FilePosition.CurrentByteOffset.QuadPart = Offset.QuadPart;
		break;

	default:
		return STATUS_INVALID_PARAMETER_2;
	}

	if (FilePosition.CurrentByteOffset.QuadPart < 0)
		return STATUS_INVALID_PARAMETER_1;

	Status = NtSetInformationFile(
		FileHandle,
		&IoStatus,
		&FilePosition,
		sizeof(FILE_POSITION_INFORMATION),
		FilePositionInformation);

	if (!NT_SUCCESS(Status))
		return Status;

	if (NewPosition != nullptr)
	{
		NewPosition->QuadPart = FilePosition.CurrentByteOffset.QuadPart;
	}

	return Status;
}

NTSTATUS
NtFileDisk::
ReadInternal(
	HANDLE          FileHandle,
	PVOID           Buffer,
	ULONG_PTR           Size,
	PLARGE_INTEGER  BytesRead /* = NULL */,
	PLARGE_INTEGER  Offset     /* = NULL */
)
{
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;

	if (BytesRead != nullptr)
		BytesRead->QuadPart = 0;

	Status = NtReadFile(
		FileHandle,
		nullptr,
		nullptr,
		nullptr,
		&IoStatus,
		Buffer,
		Size,
		Offset,
		nullptr
	);

	if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
		return Status;

	if (BytesRead != nullptr)
		BytesRead->QuadPart = IoStatus.Information;

	return Status;
}

NTSTATUS
NtFileDisk::
WriteInternal(
	HANDLE          FileHandle,
	PVOID           Buffer,
	ULONG_PTR           Size,
	PLARGE_INTEGER  pBytesWritten  /* = NULL */,
	PLARGE_INTEGER  Offset         /* = NULL */
)
{
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;

	Status = NtWriteFile(
		FileHandle,
		nullptr,
		nullptr,
		nullptr,
		&IoStatus,
		Buffer,
		Size,
		Offset,
		nullptr
	);

	if (!NT_SUCCESS(Status))
		return Status;

	if (pBytesWritten != nullptr)
		pBytesWritten->QuadPart = IoStatus.Information;

	return Status;
}

NTSTATUS NtFileDisk::DeleteInternal(HANDLE FileHandle)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK  IoStatus;
	FILE_DISPOSITION_INFORMATION FileInformation;

	FileInformation.DeleteFile = TRUE;
	Status = NtSetInformationFile(
		FileHandle,
		&IoStatus,
		&FileInformation,
		sizeof(FileInformation),
		FileDispositionInformation);

	return Status;
}

NTSTATUS NtFileDisk::SetEndOfFileInternal(HANDLE FileHandle, LARGE_INTEGER EndPosition)
{
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;

	union
	{
		FILE_END_OF_FILE_INFORMATION    EndOfFile;
		FILE_ALLOCATION_INFORMATION     FileAllocation;
	};

	EndOfFile.EndOfFile.QuadPart = EndPosition.QuadPart;

	Status = NtSetInformationFile(
		FileHandle,
		&IoStatus,
		&EndOfFile,
		sizeof(EndOfFile),
		FileEndOfFileInformation
	);

	if (!NT_SUCCESS(Status))
		return Status;

	FileAllocation.AllocationSize.QuadPart = EndPosition.QuadPart;
	Status = NtSetInformationFile(
		FileHandle,
		&IoStatus,
		&FileAllocation,
		sizeof(FileAllocation),
		FileAllocationInformation
	);

	return Status;
}

NTSTATUS
NtFileDisk::
QuerySymbolicTargetInternal(
	HANDLE                  FileHandle,
	PREPARSE_DATA_BUFFER    ReparseBuffer,
	ULONG_PTR               BufferSize,
	PULONG_PTR              BytesReturned /* = NULL */
)
{
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;

	Status = NtDeviceIoControlFile(
		FileHandle,
		nullptr,
		nullptr,
		nullptr,
		&IoStatus,
		FSCTL_GET_REPARSE_POINT,
		nullptr,
		0,
		ReparseBuffer,
		(ULONG_PTR)BufferSize
	);

	if (BytesReturned != nullptr)
		*BytesReturned = IoStatus.Information;

	return Status;
}

NTSTATUS
NtFileDisk::
QuerySymbolicTargetInternal(
	HANDLE                  FileHandle,
	PWSTR                   PrintNameBuffer,
	PULONG_PTR              PrintNameBufferCount,
	PWSTR                   SubstituteNameBuffer       /* = NULL */,
	PULONG_PTR              SubstituteNameBufferCount  /* = NULL */
)
{
	PWSTR                   PrintName, SubstituteName;
	ULONG_PTR               PrintLength, SubstituteLength;
	ULONG_PTR               ReparseBufferSize, Returned;
	NTSTATUS                Status;
	PREPARSE_DATA_BUFFER    ReparseBuffer;

	ReparseBufferSize = sizeof(*ReparseBuffer) + MAX_PATH * sizeof(WCHAR);
	ReparseBuffer = (PREPARSE_DATA_BUFFER)AllocStack(ReparseBufferSize);

	LOOP_FOREVER
	{
		Status = QuerySymbolicTargetInternal(FileHandle, ReparseBuffer, ReparseBufferSize, &Returned);
		if (NT_SUCCESS(Status))
			break;
		if (Status != STATUS_BUFFER_TOO_SMALL)
			return Status;

		ReparseBufferSize += MAX_PATH * sizeof(WCHAR);
		ReparseBuffer = (PREPARSE_DATA_BUFFER)AllocStack(ReparseBufferSize);
	}

		if (PrintNameBuffer == nullptr || PrintNameBufferCount == nullptr)
		{
			if (PrintNameBufferCount != nullptr)
			{
				*PrintNameBufferCount = ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameLength / sizeof(WCHAR);
			}
		}
		else
		{
			PrintName = ReparseBuffer->SymbolicLinkReparseBuffer.PathBuffer + ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(WCHAR);
			PrintLength = ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameLength;

			PrintLength = MIN(PrintLength, *PrintNameBufferCount);
			CopyMemory(PrintNameBuffer, PrintName, PrintLength);
			PrintLength /= sizeof(WCHAR);
			if (PrintLength < *PrintNameBufferCount)
				PrintNameBuffer[PrintLength] = 0;

			*PrintNameBufferCount = PrintLength;
		}

	if (SubstituteNameBuffer == nullptr || SubstituteNameBufferCount == nullptr)
	{
		if (SubstituteNameBufferCount != nullptr)
		{
			*SubstituteNameBufferCount = ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
		}
	}
	else
	{
		SubstituteName = ReparseBuffer->SymbolicLinkReparseBuffer.PathBuffer + ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR);
		SubstituteLength = ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength;

		SubstituteLength = MIN(SubstituteLength, *SubstituteNameBufferCount);
		CopyMemory(SubstituteNameBuffer, SubstituteName, SubstituteLength);
		SubstituteLength /= sizeof(WCHAR);
		if (SubstituteLength < *SubstituteNameBufferCount)
			SubstituteNameBuffer[SubstituteLength] = 0;

		*SubstituteNameBufferCount = SubstituteLength;
	}

	return Status;
}

NTSTATUS NtFileDisk::Close()
{
	NTSTATUS Status;

	if (m_FileHandle == nullptr)
		return STATUS_SUCCESS;

	Status = NtClose(m_FileHandle);
	if (!NT_SUCCESS(Status))
		return Status;

	m_FileHandle = nullptr;

	return Status;
}

NTSTATUS
NtFileDisk::
Create(
	LPCWSTR FileName,
	ULONG_PTR   Flags               /* = 0 */,
	ULONG_PTR   ShareMode           /* = FILE_SHARE_READ */,
	ULONG_PTR   Access              /* = GENERIC_WRITE */,
	ULONG_PTR   CreationDisposition /* = FILE_OVERWRITE_IF */,
	ULONG_PTR   Attributes          /* = FILE_ATTRIBUTE_NORMAL */,
	ULONG_PTR   CreateOptions       /* = 0 */
)
{
	NTSTATUS Status;

	Close();

	Status = CreateFileInternal(&m_FileHandle, FileName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	if (!NT_SUCCESS(Status))
		return Status;

	if (CreationDisposition != FILE_OVERWRITE_IF)
	{
		Status = GetSizeInternal(m_FileHandle, &m_FileSize);
		if (!NT_SUCCESS(Status))
		{
			Close();
			return Status;
		}
	}
	else
	{
		m_FileSize.QuadPart = 0;
	}

	m_Position.QuadPart = 0;

	return Status;
}

NTSTATUS
NtFileDisk::
OpenDevice(
	PCWSTR      FileName,
	ULONG_PTR   Flags               /* = 0 */,
	ULONG_PTR   ShareMode           /* = FILE_SHARE_READ */,
	ULONG_PTR   Access              /* = GENERIC_READ */,
	ULONG_PTR   CreationDisposition /* = FILE_OPEN */,
	ULONG_PTR   Attributes          /* = FILE_ATTRIBUTE_NORMAL */,
	ULONG_PTR   CreateOptions       /* = 0 */
)
{
	NTSTATUS        Status;
	UNICODE_STRING  DeviceName;

	Close();

	RtlInitUnicodeString(&DeviceName, (PWSTR)FileName);

	Status = CreateFileInternalWithFullPath(&m_FileHandle, &DeviceName, Flags, ShareMode, Access, CreationDisposition, Attributes, CreateOptions);
	if (!NT_SUCCESS(Status))
		return Status;

	return Status;
}

NTSTATUS
NtFileDisk::
DeviceIoControl(
	ULONG_PTR   IoControlCode,
	PVOID       InputBuffer,
	ULONG_PTR   InputBufferLength,
	PVOID       OutputBuffer,
	ULONG_PTR   OutputBufferLength,
	PULONG_PTR  BytesReturned /* = NULL */
)
{
	NTSTATUS        Status;
	IO_STATUS_BLOCK IoStatus;
	TYPE_OF(NtDeviceIoControlFile)* IoControlRoutine;

	IoControlRoutine = DEVICE_TYPE_FROM_CTL_CODE(IoControlCode) == FILE_DEVICE_FILE_SYSTEM ? NtFsControlFile : NtDeviceIoControlFile;

	Status = IoControlRoutine(
		m_FileHandle,
		nullptr,
		nullptr,
		nullptr,
		&IoStatus,
		IoControlCode,
		InputBuffer,
		InputBufferLength,
		OutputBuffer,
		OutputBufferLength
	);
	if (!NT_SUCCESS(Status))
		return Status;

	if (BytesReturned != nullptr)
		*BytesReturned = IoStatus.Information;

	return Status;
}

NTSTATUS NtFileDisk::GetSize(PLARGE_INTEGER FileSize)
{
	if (FileSize == nullptr)
		return STATUS_INVALID_PARAMETER;

	FileSize->QuadPart = m_FileSize.QuadPart;

	return STATUS_SUCCESS;
}

NTSTATUS NtFileDisk::GetPosition(PLARGE_INTEGER Position)
{
	if (Position == nullptr)
		return STATUS_INVALID_PARAMETER;

	Position->QuadPart = m_Position.QuadPart;

	return STATUS_SUCCESS;
}

NTSTATUS
NtFileDisk::
Seek(
	LARGE_INTEGER   Offset,
	ULONG_PTR       MoveMethod,
	PLARGE_INTEGER  NewPosition /* = NULL */
)
{
	LARGE_INTEGER NewOffset = m_Position;

	switch (MoveMethod)
	{
	case FILE_CURRENT:
		NewOffset.QuadPart += Offset.QuadPart;
		break;

	case FILE_END:
		NewOffset.QuadPart = m_FileSize.QuadPart + Offset.QuadPart;
		break;

	case FILE_BEGIN:
		NewOffset.QuadPart = Offset.QuadPart;
		break;

	default:
		return STATUS_INVALID_PARAMETER_2;
	}

	if (NewOffset.QuadPart < 0)
	{
		return STATUS_INVALID_PARAMETER_1;
	}

	m_Position = NewOffset;
	if (m_Position.QuadPart > m_FileSize.QuadPart)
		m_FileSize.QuadPart = m_Position.QuadPart;

	if (NewPosition != nullptr)
		NewPosition->QuadPart = m_Position.QuadPart;

	return STATUS_SUCCESS;
}

NTSTATUS
NtFileDisk::
Read(
	PVOID           Buffer,
	ULONG_PTR       Size        /* = 0 */,
	PLARGE_INTEGER  BytesRead   /* = NULL */
)
{
	NTSTATUS        Status;
	LARGE_INTEGER   LocalBytesRead;

	if (Size == 0)
		Size = m_FileSize.LowPart;

	Status = ReadInternal(m_FileHandle, Buffer, Size, &LocalBytesRead, &m_Position);
	if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
		return Status;

	m_Position.QuadPart += LocalBytesRead.QuadPart;

	if (BytesRead != nullptr)
		BytesRead->QuadPart = LocalBytesRead.QuadPart;

	return Status;
}

NTSTATUS
NtFileDisk::
Write(
	PVOID           Buffer,
	ULONG_PTR           Size,
	PLARGE_INTEGER  pBytesWritten /* = NULL */
)
{
	NTSTATUS        Status;
	LARGE_INTEGER   BytesWritten;

	Status = WriteInternal(m_FileHandle, Buffer, Size, &BytesWritten, &m_Position);
	if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
		return Status;

	if (pBytesWritten != nullptr)
		pBytesWritten->QuadPart = BytesWritten.QuadPart;

	if (BytesWritten.QuadPart + m_Position.QuadPart > m_FileSize.QuadPart)
	{
		m_FileSize.QuadPart = BytesWritten.QuadPart + m_Position.QuadPart;
	}

	m_Position.QuadPart += BytesWritten.QuadPart;

	return Status;
}

NTSTATUS
NtFileDisk::
Print(
	PLARGE_INTEGER  BytesWritten,
	PWSTR           Format,
	...
)
{
	ULONG_PTR   Length;
	WCHAR   Buffer[0xFF0 / 2];
	va_list Arg;

	va_start(Arg, Format);
	Length = _vsnwprintf(Buffer, countof(Buffer) - 1, Format, Arg);
	if (Length == -1)
		return STATUS_BUFFER_TOO_SMALL;

	return Write(Buffer, Length * sizeof(WCHAR), BytesWritten);
}

NTSTATUS
NtFileDisk::
Print(
	PLARGE_INTEGER  BytesWritten,
	PSTR            Format,
	...
)
{
	ULONG_PTR   Length;
	CHAR        Buffer[0xFF0];
	va_list     Arg;

	va_start(Arg, Format);
	Length = _vsnprintf(Buffer, countof(Buffer) - 1, Format, Arg);
	if (Length == -1)
		return STATUS_BUFFER_TOO_SMALL;

	return Write(Buffer, Length, BytesWritten);
}

NTSTATUS NtFileDisk::MapView(PVOID *BaseAddress, HANDLE ProcessHandle)
{
	NTSTATUS    Status;
	HANDLE      SectionHandle;
	ULONG_PTR   ViewSize;

	Status = NtCreateSection(
		&SectionHandle,
		GENERIC_READ,
		NULL,
		NULL,
		PAGE_READONLY,
		SEC_IMAGE,
		*this
	);
	if (NT_FAILED(Status))
		return Status;

	ViewSize = 0;
	Status = NtMapViewOfSection(
		SectionHandle,
		ProcessHandle,
		BaseAddress,
		0,
		0,
		NULL,
		&ViewSize,
		ViewShare,
		0,
		PAGE_READONLY
	);

	NtClose(SectionHandle);

	return Status;
}

NTSTATUS NtFileDisk::UnMapView(PVOID BaseAddress, HANDLE ProcessHandle)
{
	if (BaseAddress == nullptr)
		return STATUS_INVALID_PARAMETER;

	return NtUnmapViewOfSection(ProcessHandle, BaseAddress);
}


NTSTATUS NtFileDisk::SetEndOfFile()
{
	NTSTATUS Status;

	Status = SetEndOfFileInternal(m_FileHandle, m_Position);
	if (NT_SUCCESS(Status))
	{
		m_FileSize.QuadPart = m_Position.QuadPart;
	}

	return Status;
}

NTSTATUS NtFileDisk::SetEndOfFile(LARGE_INTEGER EndPosition)
{
	NTSTATUS Status;

	Status = SetEndOfFileInternal(m_FileHandle, EndPosition);
	if (NT_SUCCESS(Status))
	{
		m_FileSize.QuadPart = EndPosition.QuadPart;
		m_Position.QuadPart = MIN(m_Position.QuadPart, EndPosition.QuadPart);
	}

	return Status;
}


NTSTATUS NtFileDisk::SetEndOfFile(HANDLE FileHandle)
{
	NTSTATUS                    Status;
	FILE_POSITION_INFORMATION   FilePosition;
	IO_STATUS_BLOCK             IoStatus;

	Status = NtQueryInformationFile(
		FileHandle,
		&IoStatus,
		&FilePosition,
		sizeof(FilePosition),
		FilePositionInformation
	);
	if (!NT_SUCCESS(Status))
		return Status;

	return SetEndOfFileInternal(FileHandle, FilePosition.CurrentByteOffset);
}



ULONG_PTR PrintConsoleA(PCSTR Format, ...)
{
	BOOL        Result;
	ULONG       Length;
	CHAR        Buffer[0xF00 / 2];
	va_list     Args;
	HANDLE      StdHandle;

	va_start(Args, Format);
	Length = _vsnprintf(Buffer, countof(Buffer) - 1, Format, Args);
	if (Length == -1)
		return Length;

	StdHandle = Nt_CurrentPeb()->ProcessParameters->StandardOutput;

	if (((ULONG)StdHandle & 0x10000003) == (ULONG)3)
	{
		Result = WriteConsoleA(
			StdHandle,
			Buffer,
			Length,
			&Length,
			NULL
		);
	}
	else
	{
		LARGE_INTEGER BytesWritten;

		Result = NT_SUCCESS(NtFileDisk::Write(StdHandle, Buffer, Length, &BytesWritten));
		Length = Result ? (ULONG_PTR)BytesWritten.QuadPart : 0;
	}

	return Result ? Length : 0;
}


ULONG_PTR PrintConsoleW(PCWSTR Format, ...)
{
	BOOL        Success, IsConsole;
	ULONG       Length;
	WCHAR       Buffer[0xF00 / 2];
	va_list     Args;
	HANDLE      StdOutput;

	va_start(Args, Format);
	Length = _vsnwprintf(Buffer, countof(Buffer) - 1, Format, Args);
	if (Length == -1)
		return Length;

	StdOutput = Nt_CurrentPeb()->ProcessParameters->StandardOutput;

	IsConsole = TRUE;

	LOOP_ONCE
	{
		IO_STATUS_BLOCK             IoStatus;
		FILE_FS_DEVICE_INFORMATION  VolumeInfo;

		if (((ULONG)StdOutput & 0x10000003) == (ULONG)3)
			break;

		IoStatus.Status = NtQueryVolumeInformationFile(
			StdOutput,
			&IoStatus,
			&VolumeInfo,
			sizeof(VolumeInfo),
			FileFsDeviceInformation
			);
		if (NT_FAILED(IoStatus.Status))
			break;

		IsConsole = VolumeInfo.DeviceType == FILE_DEVICE_CONSOLE;
	}

		if (IsConsole)
		{
			Success = WriteConsoleW(
				StdOutput,
				Buffer,
				Length,
				&Length,
				NULL
			);
		}
		else
		{
			LARGE_INTEGER BytesWritten;

			Success = NT_SUCCESS(NtFileDisk::Write(StdOutput, Buffer, Length * sizeof(WCHAR), &BytesWritten));
			Length = Success ? ((ULONG_PTR)BytesWritten.QuadPart / sizeof(WCHAR)) : 0;
		}

	return Success ? Length : 0;
}




NTSTATUS
ExceptionBox(
	PCWSTR      Text,
	PCWSTR      Title,
	PULONG_PTR  Response,
	ULONG_PTR   Type
)
{
	UNICODE_STRING  HardErrorTitle, HardErrorText;
	ULONG_PTR       HardErrorParameters[3];
	ULONG           LocalResponse;
	NTSTATUS        Status;

	RtlInitUnicodeString(&HardErrorTitle, (PWSTR)Title);
	RtlInitUnicodeString(&HardErrorText,  (PWSTR)Text);

	HardErrorParameters[0] = (ULONG_PTR)&HardErrorText;
	HardErrorParameters[1] = (ULONG_PTR)&HardErrorTitle;
	HardErrorParameters[2] = Type;

	Status = NtRaiseHardError(STATUS_SERVICE_NOTIFICATION, countof(HardErrorParameters), 1 | 2, HardErrorParameters, 0, &LocalResponse);

	if (NT_SUCCESS(Status) && Response != NULL)
		*Response = LocalResponse;

	return Status;
}


#define DECL_NUMBER_TABLE32(name) \
        UINT32 name[] = \
        { \
            0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, \
            0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, \
            0x03020100, 0x07060504, 0xFFFF0908, 0xFFFFFFFF, 0x0C0B0AFF, 0xFF0F0E0D, \
        }

INT32 StringToInt32A(PCCHAR pszString)
{
	/*
	Char NumberTable[] =
	{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,  0,  1,
	2,  3,  4,  5,  6,  7,  8,  9, -1, -1,
	-1, -1, -1, -1, -1, 10, 11, 12, 13, 14,
	15,
	};
	*/
	if (pszString == NULL)
		return 0;

	DECL_NUMBER_TABLE32(NumberTable32);
	PCHAR NumberTable = (PCHAR)NumberTable32;

	INT32 Result, Temp, Sign;
	CHAR ch;

	if (*pszString == '-')
	{
		Sign = 1;
		++pszString;
	}
	else
	{
		Sign = 0;
	}

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('a', ch, 'z'))
			ch &= 0xDF;
		if ((UINT32)ch > sizeof(NumberTable32))
			return 0;

		Temp = NumberTable[ch];
		if ((CHAR)Temp == (CHAR)-1)
			break;

		Result = Result * 10 + Temp;
	}

		if (Sign)
			Result = -Result;

	return Result;
}

INT32 StringToInt32W(PCWCHAR pszString)
{
	/*
	Char NumberTable[] =
	{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,  0,  1,
	2,  3,  4,  5,  6,  7,  8,  9, -1, -1,
	-1, -1, -1, -1, -1, 10, 11, 12, 13, 14,
	15,
	};
	*/
	if (pszString == NULL)
		return 0;

	DECL_NUMBER_TABLE32(NumberTable32);
	PCHAR NumberTable = (PCHAR)NumberTable32;

	INT32 Result, Temp, Sign;
	WCHAR ch;

	if (*pszString == '-')
	{
		Sign = 1;
		++pszString;
	}
	else
	{
		Sign = 0;
	}

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('a', ch, 'z'))
			ch &= 0xFFDF;
		if ((UINT32)ch > sizeof(NumberTable32))
			break;

		Temp = NumberTable[ch];
		if ((CHAR)Temp == (CHAR)-1)
			break;

		Result = Result * 10 + Temp;
	}

		if (Sign)
			Result = -Result;

	return Result;
}

INT32 StringToInt32HexW(PCWSTR pszString)
{
	INT32 Result;
	WCHAR ch;

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('0', ch, '9'))
		{
			ch -= '0';
		}
		else
		{
			ch &= 0xFFDF;
			if (IN_RANGE('A', ch, 'F'))
				ch = ch - 'A' + 10;
			else
				break;
		}

		Result = (Result << 4) | ch;
	}

	return Result;
}

INT64 StringToInt64HexW(PCWSTR pszString)
{
	INT64 Result;
	WCHAR ch;

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('0', ch, '9'))
		{
			ch -= '0';
		}
		else
		{
			ch &= 0xFFDF;
			if (IN_RANGE('A', ch, 'F'))
				ch = ch - 'A' + 10;
			else
				break;
		}

		Result = (Result << 4) | ch;
	}

	return Result;
}

INT64 StringToInt64A(PCHAR pszString)
{
	/*
	Char NumberTable[] =
	{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,  0,  1,
	2,  3,  4,  5,  6,  7,  8,  9, -1, -1,
	-1, -1, -1, -1, -1, 10, 11, 12, 13, 14,
	15,
	};
	*/
	if (pszString == NULL)
		return 0;

	DECL_NUMBER_TABLE32(NumberTable32);
	PCHAR NumberTable = (PCHAR)NumberTable32;

	INT64 Result, Temp, Sign;
	CHAR ch;

	if (*pszString == '-')
	{
		Sign = 1;
		++pszString;
	}
	else
	{
		Sign = 0;
	}

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('a', ch, 'z'))
			ch &= 0xDF;
		if ((UINT32)ch > sizeof(NumberTable32))
			return 0;

		Temp = NumberTable[ch];
		if ((CHAR)Temp == (CHAR)-1)
			break;

		Result = Result * 10 + Temp;
	}

		if (Sign)
			Result = -Result;

	return Result;
}

INT64 StringToInt64W(PCWCHAR pszString)
{
	/*
	Char NumberTable[] =
	{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,  0,  1,
	2,  3,  4,  5,  6,  7,  8,  9, -1, -1,
	-1, -1, -1, -1, -1, 10, 11, 12, 13, 14,
	15,
	};
	*/
	if (pszString == NULL)
		return 0;

	DECL_NUMBER_TABLE32(NumberTable32);
	PCHAR NumberTable = (PCHAR)NumberTable32;

	INT64 Result, Temp, Sign;
	WCHAR ch;

	if (*pszString == '-')
	{
		Sign = 1;
		++pszString;
	}
	else
	{
		Sign = 0;
	}

	Result = 0;
	LOOP_ALWAYS
	{
		ch = *pszString++;
		if (ch == 0)
			break;
		if (IN_RANGE('a', ch, 'z'))
			ch &= 0xFFDF;
		if ((UINT32)ch > sizeof(NumberTable32))
			break;

		Temp = NumberTable[ch];
		if ((CHAR)Temp == (CHAR)-1)
			break;

		Result = Result * 10 + Temp;
	}

	if (Sign)
		Result = -Result;

	return Result;
}

LONG_PTR FASTCALL CmdLineToArgvWorkerA(LPSTR pszCmdLine, LPSTR *pArgv, PLONG_PTR pCmdLineLength)
{
	LONG_PTR  argc;
	CHAR separator, ch;
	LPSTR pCmdLine;

	argc = 0;
	pCmdLine = pszCmdLine;

	LOOP_ALWAYS
	{
		separator = *pszCmdLine++;
		if (separator == 0)
			break;
		if (separator != '"')
			separator = ' ';

		++argc;
		if (pArgv)
			*pArgv++ = (LPSTR)pszCmdLine - 1 + (separator == '"');

		LOOP_ALWAYS
		{
			ch = *pszCmdLine;
			if (ch == separator)
			{
				if (pArgv)
					*pszCmdLine = 0;
				++pszCmdLine;
				break;
			}
			else if (ch == 0)
				break;

			++pszCmdLine;
		}
			/*
			if (pArgv)
			*(pszCmdLine - 1)= 0;
			*/
			while (*pszCmdLine == '\t' || *pszCmdLine == ' ')
				++pszCmdLine;
	}

	if (pCmdLineLength)
		*pCmdLineLength = pszCmdLine - pCmdLine + 0;

	return argc;
}

LPSTR* FASTCALL CmdLineToArgvA(LPSTR pszCmdLine, PLONG_PTR pArgc)
{
	LONG_PTR argc, CmdLineLength;
	LPSTR *argv;

	argc = CmdLineToArgvWorkerA(pszCmdLine, NULL, &CmdLineLength);
	argv = (LPSTR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CmdLineLength * sizeof(*pszCmdLine) + (argc + 1) * 4);
	if (argv == NULL)
		return NULL;

	CopyMemory(&argv[argc + 1], pszCmdLine, CmdLineLength * sizeof(*pszCmdLine));
	argv[argc] = 0;
	pszCmdLine = (LPSTR)&argv[argc + 1];

	argc = CmdLineToArgvWorkerA(pszCmdLine, argv, NULL);
	if (pArgc)
		*pArgc = argc;

	return (LPSTR *)argv;
}

LONG_PTR FASTCALL CmdLineToArgvWorkerW(LPWSTR pszCmdLine, LPWSTR *pArgv, PLONG_PTR pCmdLineLength)
{
	LONG_PTR  argc;
	WCHAR separator, ch;
	LPWSTR pCmdLine;

	argc = 0;
	pCmdLine = pszCmdLine;

	LOOP_ALWAYS
	{
		separator = *pszCmdLine++;
		if (separator == 0)
			break;
		if (separator != '"')
			separator = ' ';

		++argc;
		if (pArgv)
			*pArgv++ = (LPWSTR)pszCmdLine - 1 + (separator == '"');

		LOOP_ALWAYS
		{
			ch = *pszCmdLine;
			if (ch == separator)
			{
				if (pArgv)
					*pszCmdLine = 0;
				++pszCmdLine;
				break;
			}
			else if (ch == 0)
				break;

			++pszCmdLine;
		}
		while (*pszCmdLine == '\t' || *pszCmdLine == ' ')
			++pszCmdLine;
	}

		if (pCmdLineLength)
			*pCmdLineLength = pszCmdLine - pCmdLine + 0;

	return argc;
}

LPWSTR* FASTCALL CmdLineToArgvW(LPWSTR pszCmdLine, PLONG_PTR pArgc)
{
	LONG_PTR argc, CmdLineLength;
	LPWSTR *argv;

	argc = CmdLineToArgvWorkerW(pszCmdLine, NULL, &CmdLineLength);
	argv = (LPWSTR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, CmdLineLength * sizeof(*pszCmdLine) + (argc + 1) * 4);
	if (argv == NULL)
		return NULL;

	CopyMemory(&argv[argc + 1], pszCmdLine, CmdLineLength * sizeof(*pszCmdLine));
	argv[argc] = 0;
	pszCmdLine = (LPWSTR)&argv[argc + 1];

	argc = CmdLineToArgvWorkerW(pszCmdLine, argv, NULL);
	if (pArgc)
		*pArgc = argc;

	return (LPWSTR *)argv;
}





NTSTATUS
ProtectVirtualMemory(
	PVOID       BaseAddress,
	ULONG_PTR   Size,
	ULONG       NewProtect,
	PULONG      OldProtect,
	HANDLE      ProcessHandle   /* = Ps::CurrentProcess */
)
{
	ULONG _OldProtect;
	NTSTATUS Status;

	Status = NtProtectVirtualMemory(ProcessHandle, &BaseAddress, &Size, NewProtect, &_OldProtect);
	if (NT_SUCCESS(Status) && OldProtect != NULL)
		*OldProtect = _OldProtect;

	return Status;
}

NTSTATUS
ProtectMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	ULONG_PTR   Size,
	ULONG       NewProtect,
	PULONG      OldProtect
)
{
	return ProtectVirtualMemory(BaseAddress, Size, NewProtect, OldProtect, ProcessHandle);
}



#undef POINTER_SIZE
#define POINTER_SIZE sizeof(PVOID)

NTSTATUS
CopyOneOpCode(
	PVOID       Target,
	PVOID       Source,
	PULONG_PTR  TargetOpLength,
	PULONG_PTR  SourceOpLength,
	ULONG_PTR   ForwardSize,
	ULONG_PTR   BackwardSize,
	PVOID       TargetIp
)
{
	ULONG_PTR   Length, OpCode, OpCodeLength, OpOffsetLength;
	LONG_PTR    Selector;
	LONG_PTR    OpOffset, *pOpOffset;
	PBYTE       Func, p;
	ULONG_PTR   NextOpAddress;

	enum { OpCodeOffsetShort = 1, OpCodeOffsetLong = POINTER_SIZE };

	TargetIp = TargetIp == IMAGE_INVALID_VA ? Target : TargetIp;

	Func = (PBYTE)Source;
	p = (PBYTE)Target;

	Length = GetOpCodeSize(Func);

	OpCode = *Func;
	pOpOffset = (PLONG_PTR)(Func + 1);
	OpOffsetLength = OpCodeOffsetShort;
	OpCodeLength = 2;
	Selector = -1;

	if (((OpCode & 0xF0) ^ 0x70) == 0)
		//    if (OpCode >= 0x70 && OpCode <= 0x7F)
	{
		OpCode = 0x800F | ((OpCode - 0x70) << 8);
	}
	else if (OpCode == 0xEB)
	{
		OpCode = 0xE9;
		OpCodeLength = 1;
	}
	else if (OpCode == 0xE8 || OpCode == 0xE9)
	{
		OpOffsetLength = OpCodeOffsetLong;
		OpCodeLength = 1;
	}
	else
	{
		ULONG _Op;

		OpCode = *(PUSHORT)Func;
		_Op = SWAP2(OpCode);

		if ((_Op & ~0xF) == 0x0F80)
			// if (_Op >= 0x0F80 && _Op <= 0x0F8F)
		{
			OpOffsetLength = OpCodeOffsetLong;
			pOpOffset = (PLONG_PTR)(Func + 2);
		}
		else
		{
		DEFAULT_OP_CODE:
			CopyMemory(p, Func, Length);
			p += Length;

			if (SourceOpLength != NULL)
				*SourceOpLength = Length;

			if (TargetOpLength != NULL)
				*TargetOpLength = PtrOffset(p, Target);

			return STATUS_SUCCESS;
		}
	}

	OpOffset = 0;
	switch (OpOffsetLength)
	{
	case OpCodeOffsetShort:
		OpOffset = *(PCHAR)pOpOffset;
		break;

	case OpCodeOffsetLong:
	default:
		OpOffset = *(PLONG_PTR)pOpOffset;
		break;
	}

	//    ULONG_PTR NewOffset = (ULONG_PTR)Func + Length + OpOffset;
	//    if (NewOffset >= (ULONG_PTR)Destination && NewOffset < (ULONG_PTR)Destination + BytesToNop + HookOpSize)
	//        goto DEFAULT_OP_CODE;

	LONG_PTR NewOffset = Length + OpOffset;

	if (NewOffset < 0)
	{
		if ((ULONG_PTR)-NewOffset < BackwardSize)
			goto DEFAULT_OP_CODE;
	}
	else if ((ULONG_PTR)NewOffset < ForwardSize)
	{
		goto DEFAULT_OP_CODE;
	}

	NextOpAddress = (ULONG_PTR)Func + Length;
	OpOffset = (NextOpAddress + OpOffset) - ((ULONG_PTR)TargetIp + OpCodeLength + OpCodeOffsetLong + (Selector == -1 ? 0 : 2));
	switch (OpCodeLength)
	{
	case 1:
		*p++ = (BYTE)OpCode;
		break;

	case 2:
	default:
		*(PUSHORT)p = (USHORT)OpCode;
		p += 2;
		break;
	}
	/*
	if (Selector != -1)
	{
	*(PUSHORT)p = Selector;
	p += 2;
	}
	*/
	*(PLONG_PTR)p = OpOffset;
	p += POINTER_SIZE;

	//    Length = PtrOffset(p, Destination);

	if (SourceOpLength != NULL)
		*SourceOpLength = Length;

	if (TargetOpLength != NULL)
		*TargetOpLength = PtrOffset(p, Target);

	return STATUS_SUCCESS;
}



namespace Mp
{

#if !defined(_AMD64_)
#define SIZE_OF_JUMP_CODE   5
#else
#endif // arch

VOID PatchNop(PVOID Address, ULONG_PTR BytesToPatch)
{
	PBYTE Buffer = (PBYTE)Address;

	switch (BytesToPatch)
	{
	case 0:
		return;

	case 1:
		//
		// nop
		//
		Buffer[0] = 0x90;
		break;

	case 2:
		//
		// mov eax, eax
		//
		*(PUSHORT)Buffer = 0xC08B;
		break;

	case 3:
		//
		// lea eax, [eax+0]
		//
		*(PUSHORT)Buffer = 0x408D;
		Buffer[2] = 0x00;
		break;

	case 4:
		//
		// lea esi, [esi]
		//
		*(PULONG)Buffer = 0x0026748D;
		break;

	case 5:
		// 2 + 3
		*Buffer = 0x8B;
		*(PULONG)(Buffer + 1) = 0x408DC0;
		break;

	case 6:
		// lea eax, [eax+0]
		*(PULONG)Buffer = 0x808D;
		*(PUSHORT)(Buffer + 4) = 0;
		break;

	case 7:
		// lea esi, [esi]
		*(PULONG)Buffer = 0x0026B48D;
		*(PULONG)&Buffer[3] = 0;
		break;
	}
}

class MemoryPatchManager
{
protected:
	GrowableArray<PTRAMPOLINE_DATA> TrampolineList;
	HANDLE ExecutableHeap;

public:
	MemoryPatchManager()
	{
		this->ExecutableHeap = RtlCreateHeap(HEAP_CREATE_ENABLE_EXECUTE | HEAP_GROWABLE | HEAP_CREATE_ALIGN_16, nullptr, 0, 0, nullptr, nullptr);
	}

protected:
	NTSTATUS HandleMemoryPatch(PPATCH_MEMORY_DATA PatchData, PVOID BaseAddress)
	{
		auto&       Patch = PatchData->Memory;
		PVOID       Address;
		ULONG       Protect;
		NTSTATUS    Status;

		if (Patch.Size == 0)
			return STATUS_SUCCESS;

		if (Patch.Address == IMAGE_INVALID_RVA)
			return STATUS_SUCCESS;

		Address = PtrAdd(Patch.Options.VirtualAddress ? nullptr : BaseAddress, Patch.Address);
		Status = ProtectMemory(Address, Patch.Size, PAGE_EXECUTE_READWRITE, &Protect);
		FAIL_RETURN(Status);

		if (Patch.Options.DataIsBuffer)
		{
			CopyMemory(Address, (PVOID)Patch.Data, Patch.Size);
		}
		else
		{
			if (Patch.Options.BackupData)
			{
				PTRAMPOLINE_DATA TrampolineData;

				TrampolineData = AllocateTrampolineData();
				if (TrampolineData == nullptr)
					return STATUS_NO_MEMORY;

				TrampolineData->PatchData = *PatchData;
				TrampolineData->PatchData.Memory.Backup = 0;
				TrampolineData->PatchData.Memory.Options.VirtualAddress = TRUE;
				TrampolineData->PatchData.Memory.Address = (ULONG_PTR)Address;
				CopyMemory(&TrampolineData->PatchData.Memory.Backup, Address, Patch.Size);

				*PatchData = TrampolineData->PatchData;

				this->TrampolineList.Add(TrampolineData);
			}

			CopyMemory(&Patch.Backup, Address, Patch.Size);
			CopyMemory(Address, &Patch.Data, Patch.Size);
		}

		Status = ProtectMemory(Address, Patch.Size, Protect, nullptr);

		return STATUS_SUCCESS;
	}

	NTSTATUS HandleFunctionPatch(PPATCH_MEMORY_DATA PatchData, PVOID BaseAddress)
	{
		auto&               Function = PatchData->Function;
		BYTE                LocalHookBuffer[TRAMPOLINE_SIZE];
		PVOID               Address;
		PBYTE               Trampoline;
		ULONG               Protect;
		ULONG_PTR           HookOpSize, CopyOpSize;
		NTSTATUS            Status;
		PTRAMPOLINE_DATA    TrampolineData;

		if (Function.Target == nullptr)
			return STATUS_SUCCESS;

		if (Function.Source == IMAGE_INVALID_RVA)
			return STATUS_SUCCESS;

		Address = PtrAdd(Function.Options.VirtualAddress ? nullptr : BaseAddress, Function.Source);

		HookOpSize = GetSizeOfHookOpSize(Function.HookOp);
		if (HookOpSize == ULONG_PTR_MAX)
			return STATUS_BUFFER_TOO_SMALL;

		Status = GetHookAddressAndSize(Address, HookOpSize, &Address, &CopyOpSize);
		FAIL_RETURN(Status);

		if (Function.Options.NakedTrampoline == FALSE)
		{
			Status = GenerateHookCode(LocalHookBuffer, Address, Function.Target, Function.HookOp, HookOpSize);
			FAIL_RETURN(Status);

			PatchNop(&LocalHookBuffer[HookOpSize], CopyOpSize - HookOpSize);
		}

		TrampolineData = nullptr;
		if (Function.Options.NakedTrampoline != FALSE || (Function.Trampoline != nullptr && Function.Options.DoNotDisassemble == FALSE))
		{
			TrampolineData = AllocateTrampolineData();
			if (TrampolineData == nullptr)
				return STATUS_NO_MEMORY;

			TrampolineData->PatchData = *PatchData;
			TrampolineData->PatchData.Function.NopBytes = CopyOpSize - HookOpSize;
			TrampolineData->PatchData.Function.Source = (ULONG_PTR)Address;
			TrampolineData->PatchData.Function.Options.VirtualAddress = TRUE;

			TrampolineData->OriginSize = CopyOpSize;
			CopyMemory(TrampolineData->OriginalCode, Address, CopyOpSize);

			TrampolineData->JumpBackAddress = PtrAdd(Address, CopyOpSize);
			Trampoline = TrampolineData->Trampoline;

			if (Function.Options.NakedTrampoline == FALSE)
			{
				CopyTrampolineStub(Trampoline, Address, CopyOpSize);
				if (CopyOpSize == HookOpSize && Function.HookOp == OpCall && Function.Options.KeepRawTrampoline == FALSE)
				{
					TrampolineData->Trampoline[0] = 0xE9;
				}
				else
				{
					GenerateJumpBack(Trampoline, &TrampolineData->JumpBackAddress);
				}
			}
			else
			{
				GenerateNakedTrampoline(Trampoline, Address, CopyOpSize, TrampolineData);

				Status = GenerateHookCode(LocalHookBuffer, Address, TrampolineData->Trampoline, Function.HookOp, HookOpSize);
				if (NT_FAILED(Status))
				{
					FreeTrampolineData(TrampolineData);
					return Status;
				}

				PatchNop(&LocalHookBuffer[HookOpSize], CopyOpSize - HookOpSize);
			}

			if (Trampoline - TrampolineData->Trampoline > TRAMPOLINE_SIZE)
			{
				//RaiseDebugBreak();
			}

			FlushInstructionCache(TrampolineData->Trampoline, TRAMPOLINE_SIZE);

			if (Function.Trampoline != nullptr)
				*Function.Trampoline = TrampolineData->Trampoline;

			this->TrampolineList.Add(TrampolineData);
		}

		Status = ProtectMemory(Address, CopyOpSize, PAGE_EXECUTE_READWRITE, &Protect);
		if (NT_FAILED(Status))
		{
			FreeTrampolineData(TrampolineData);
			return Status;
		}

		CopyMemory(Address, LocalHookBuffer, CopyOpSize);
		FlushInstructionCache(Address, CopyOpSize);

		Status = ProtectMemory(Address, CopyOpSize, Protect, &Protect);

		return STATUS_SUCCESS;
	}

	NTSTATUS GenerateHookCode(PBYTE Buffer, PVOID SourceIp, PVOID Target, ULONG_PTR HookOp, ULONG_PTR HookOpSize)
	{
		ULONG_PTR RegisterIndex;

#if !defined(_AMD64_)

		switch (HookOp)
		{
		case OpCall:
		case OpJump:
			//
			// jmp imm
			//
			*Buffer++ = HookOp == OpCall ? 0xE8 : 0xE9;
			*(PULONG)Buffer = PtrOffset(Target, PtrAdd(SourceIp, HookOpSize));
			break;

		case OpPush:
			//
			// push imm
			// ret
			//
			*Buffer++ = 0x68;
			*(PULONG)Buffer = (ULONG)Target;
			Buffer += POINTER_SIZE;
			*Buffer = 0xC3;
			break;

		default:
			if (HookOp < OpJRax || HookOp > OpJRdi)
				return STATUS_INVALID_PARAMETER;

			//
			// mov r32, imm
			// jmp r32
			//
			RegisterIndex = HookOp - OpJRax;
			*Buffer++ = 0xB8 + (BYTE)RegisterIndex;
			*(PVOID *)Buffer = Target;
			Buffer += POINTER_SIZE;
			*(PUSHORT)Buffer = (USHORT)(0xE0FF + RegisterIndex);
			break;
		}

#else

		switch (HookOp)
		{
		case OpPush:
			//
			// push imm.low
			// mov dword ptr [rsp + 4], imm.high
			// ret
			//
			*Buffer++ = 0x68;
			*(PULONG)Buffer = (ULONG)((ULONG_PTR)Target >> 32);
			Buffer += POINTER_SIZE;

			*Buffer++ = 0xC7;
			*Buffer++ = 0x04;
			*Buffer++ = 0x24;
			*(PULONG)Buffer = (ULONG)(ULONG_PTR)Target;

			*Buffer = 0xC3;
			break;

		default:
			if (HookOp < OpJRax || HookOp > OpJRdi)
				return STATUS_INVALID_PARAMETER;

			//
			// mov r32, imm
			// jmp r32
			//
			RegisterIndex = HookOp - OpJRax;
			*Buffer++ = 0x48;
			*Buffer++ = 0xB8 + (BYTE)RegisterIndex;
			*(PVOID *)Buffer = Target;
			Buffer += POINTER_SIZE;
			*(PUSHORT)Buffer = (USHORT)(0xE0FF + RegisterIndex);
			break;
		}

#endif

		return STATUS_SUCCESS;
	}

	NTSTATUS GenerateNakedTrampoline(PBYTE& Trampoline, PVOID Address, ULONG_PTR CopyOpSize, PTRAMPOLINE_DATA TrampolineData)
	{
		NTSTATUS    Status;
		PVOID*      AddressOfReturnAddress;
		auto&       Function = TrampolineData->PatchData.Function;

		if (Function.Options.ExecuteTrampoline != FALSE)
		{
			Status = CopyTrampolineStub(Trampoline, Address, CopyOpSize);
			FAIL_RETURN(Status);
		}

#if !defined(_AMD64_)

		//
		// push     eax
		// push     esp
		// push     eax
		// push     ecx
		// push     edx
		// push     ebx
		// push     ebp
		// push     esi
		// push     edi
		// pushfd
		//
		// mov      eax, ReturnAddress
		// mov      [esp + 24h], eax
		// mov      eax, CallBack
		// lea      ecx, [esp];      ecx = PTRAMPOLINE_NAKED_CONTEXT
		// call     eax
		//
		// popfd
		// pop      edi
		// pop      esi
		// pop      ebp
		// pop      ebx
		// pop      edx
		// pop      ecx
		// pop      eax
		// pop      esp
		//
		// ret
		//

		if (Function.HookOp != OpCall)
		{
			// push eax
			*Trampoline++ = 0x50;
		}

		*Trampoline++ = 0x54;       // push    esp
		*Trampoline++ = 0x50;       // push    eax
		*Trampoline++ = 0x51;       // push    ecx
		*Trampoline++ = 0x52;       // push    edx
		*Trampoline++ = 0x53;       // push    ebx
		*Trampoline++ = 0x55;       // push    ebp
		*Trampoline++ = 0x56;       // push    esi
		*Trampoline++ = 0x57;       // push    edi
		*Trampoline++ = 0x9C;       // pushfd

		// mov eax, imm
		*Trampoline++ = 0xB8;
		*(PVOID *)Trampoline = PtrAdd(Address, CopyOpSize);
		AddressOfReturnAddress = (PVOID *)Trampoline;
		Trampoline += POINTER_SIZE;

		// mov [esp + 24h], eax
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x44;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x24;

		// mov eax, CallBack
		*Trampoline++ = 0xB8;
		*(PVOID *)Trampoline = Function.Target;
		Trampoline += POINTER_SIZE;

		// lea ecx, [esp]
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x0C;
		*Trampoline++ = 0x24;

		// call eax
		*Trampoline++ = 0xFF;
		*Trampoline++ = 0xD0;

		*Trampoline++ = 0x9D;       // popfd
		*Trampoline++ = 0x5F;       // pop     edi
		*Trampoline++ = 0x5E;       // pop     esi
		*Trampoline++ = 0x5D;       // pop     ebp
		*Trampoline++ = 0x5B;       // pop     ebx
		*Trampoline++ = 0x5A;       // pop     edx
		*Trampoline++ = 0x59;       // pop     ecx
		*Trampoline++ = 0x58;       // pop     eax
		*Trampoline++ = 0x5C;       // pop     esp

		// ret
		*Trampoline++ = 0xC3;

#else

		//
		// SIZE_OF_CONTEXT equ 78h
		// push    rax
		// pushfq
		// lea     rsp, [rsp - SIZE_OF_CONTEXT]
		// mov     [rsp + 00h], rax
		// mov     [rsp + 08h], rcx
		// mov     [rsp + 10h], rdx
		// mov     [rsp + 18h], rbx
		// mov     [rsp + 20h], rbp
		// mov     [rsp + 28h], rsi
		// mov     [rsp + 30h], rdi
		// mov     [rsp + 38h], r8
		// mov     [rsp + 40h], r9
		// mov     [rsp + 48h], r10
		// mov     [rsp + 50h], r11
		// mov     [rsp + 58h], r12
		// mov     [rsp + 60h], r13
		// mov     [rsp + 68h], r14
		// mov     [rsp + 70h], r15
		// mov     rax, ReturnAddress
		// mov     [rsp + SIZE_OF_CONTEXT + 10h], rax
		// mov     rax, CallBack
		// lea     rcx, [rsp];      rcx = PTRAMPOLINE_NAKED_CONTEXT
		// lea     rsp, [rsp - 20h]
		// call    rax
		// lea     rsp, [rsp + 20h]
		// mov     rax, [rsp + 00h]
		// mov     rcx, [rsp + 08h]
		// mov     rdx, [rsp + 10h]
		// mov     rbx, [rsp + 18h]
		// mov     rbp, [rsp + 20h]
		// mov     rsi, [rsp + 28h]
		// mov     rdi, [rsp + 30h]
		// mov     r8 , [rsp + 38h]
		// mov     r9 , [rsp + 40h]
		// mov     r10, [rsp + 48h]
		// mov     r11, [rsp + 50h]
		// mov     r12, [rsp + 58h]
		// mov     r13, [rsp + 60h]
		// mov     r14, [rsp + 68h]
		// mov     r15, [rsp + 70h]
		// lea     rsp, [rsp + SIZE_OF_CONTEXT]
		// popfq
		// ret
		//

		if (Function.HookOp != OpCall)
		{
			// push rax
			*Trampoline++ = 0x50;
		}

		// pushfq
		*Trampoline++ = 0x9C;

		// lea     rsp, [rsp-78h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x88;

		// mov     [rsp + 00h], rax
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x04;
		*Trampoline++ = 0x24;

		// mov     [rsp + 08h], rcx
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x08;

		// mov     [rsp + 10h], rdx
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x54;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x10;

		// mov     [rsp + 18h], rbx
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x5C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x18;

		// mov     [rsp + 20h], rbp
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x6C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x20;

		// mov     [rsp + 28h], rsi
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x74;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x28;

		// mov     [rsp + 30h], rdi
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x7C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x30;

		// mov     [rsp + 38h], r8
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x44;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x38;

		// mov     [rsp + 40h], r9
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x40;

		// mov     [rsp + 48h], r10
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x54;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x48;

		// mov     [rsp + 50h], r11
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x5C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x50;

		// mov     [rsp + 58h], r12
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x58;

		// mov     [rsp + 60h], r13
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x6C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x60;

		// mov     [rsp + 68h], r14
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x74;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x68;

		// mov     [rsp + 70h], r15
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x7C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x70;

		// mov     rax, ReturnAddress
		*Trampoline++ = 0x48;
		*Trampoline++ = 0xB8;
		*(PVOID *)Trampoline = PtrAdd(Address, CopyOpSize);
		AddressOfReturnAddress = (PVOID *)Trampoline;
		Trampoline += POINTER_SIZE;

		// mov     [rsp + SIZE_OF_CONTEXT + 10], rax
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x89;
		*Trampoline++ = 0x84;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x80;
		*Trampoline++ = 0x00;
		*Trampoline++ = 0x00;
		*Trampoline++ = 0x00;

		// mov     rax, CallBack
		*Trampoline++ = 0x48;
		*Trampoline++ = 0xB8;
		*(PVOID *)Trampoline = Function.Target;
		Trampoline += POINTER_SIZE;

		// lea     rcx, [rsp];      rcx = PTRAMPOLINE_NAKED_CONTEXT
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x0C;
		*Trampoline++ = 0x24;

		// lea     rsp, [rsp - 20h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0xE0;

		// call    rax
		*Trampoline++ = 0xFF;
		*Trampoline++ = 0xD0;

		// lea     rsp, [rsp + 20h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x20;

		// mov     rax, [rsp + 00h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x04;
		*Trampoline++ = 0x24;

		// mov     rcx, [rsp + 08h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x08;

		// mov     rdx, [rsp + 10h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x54;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x10;

		// mov     rbx, [rsp + 18h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x5C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x18;

		// mov     rbp, [rsp + 20h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x6C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x20;

		// mov     rsi, [rsp + 28h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x74;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x28;

		// mov     rdi, [rsp + 30h]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x7C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x30;

		// mov     r8 , [rsp + 38h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x44;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x38;

		// mov     r9 , [rsp + 40h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x40;

		// mov     r10, [rsp + 48h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x54;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x48;

		// mov     r11, [rsp + 50h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x5C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x50;

		// mov     r12, [rsp + 58h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x58;

		// mov     r13, [rsp + 60h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x6C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x60;

		// mov     r14, [rsp + 68h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x74;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x68;

		// mov     r15, [rsp + 70h]
		*Trampoline++ = 0x4C;
		*Trampoline++ = 0x8B;
		*Trampoline++ = 0x7C;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x70;

		// lea     rsp, [rsp + SIZE_OF_CONTEXT]
		*Trampoline++ = 0x48;
		*Trampoline++ = 0x8D;
		*Trampoline++ = 0x64;
		*Trampoline++ = 0x24;
		*Trampoline++ = 0x78;

		// popfq
		*Trampoline++ = 0x9D;

		// ret
		*Trampoline++ = 0xC3;

		//#error NOT COMPLETE

#endif // arch

		if (Function.Options.ExecuteTrampoline == FALSE)
		{
			*AddressOfReturnAddress = Trampoline;

			Status = CopyTrampolineStub(Trampoline, Address, CopyOpSize);
			FAIL_RETURN(Status);

			Status = GenerateJumpBack(Trampoline, &TrampolineData->JumpBackAddress);
			FAIL_RETURN(Status);
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS GenerateJumpBack(PBYTE& Trampoline, PVOID AddressOfJumpAddress)
	{
		// jmp [imm32]
		*Trampoline++ = 0xFF;
		*Trampoline++ = 0x25;

		*(PVOID *)Trampoline = AddressOfJumpAddress;
		Trampoline += 2 + 4;

		return STATUS_SUCCESS;
	}

	NTSTATUS CopyTrampolineStub(PBYTE& Trampoline, PVOID Address, ULONG_PTR CopyOpSize)
	{
		ULONG_PTR SourceOpLength, ForwardSize, BackwardSize;
		PBYTE Source = (PBYTE)Address;

		ForwardSize = CopyOpSize;
		BackwardSize = 0;

		for (LONG_PTR Bytes = CopyOpSize; Bytes > 0; Bytes -= SourceOpLength)
		{
			ULONG_PTR TargetOpLength;

			if (Source[0] == 0xC2 || Source[0] == 0xC3)
				return STATUS_BUFFER_TOO_SMALL;

			CopyOneOpCode(Trampoline, Source, &TargetOpLength, &SourceOpLength, ForwardSize, BackwardSize, IMAGE_INVALID_VA);

			BackwardSize += SourceOpLength;
			ForwardSize -= SourceOpLength;
			Trampoline += TargetOpLength;
			Source += SourceOpLength;
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS GetHookAddressAndSize(PVOID Address, ULONG_PTR HookOpSize, PVOID *FinalAddress, PULONG_PTR Size)
	{
		ULONG_PTR   Length, OpSize;
		PBYTE       Buffer;

		OpSize = 0;
		Buffer = (PBYTE)Address;
		*FinalAddress = Address;

#if ML_X86 || ML_AMD64

		while (OpSize < HookOpSize)
		{
			if (Buffer[0] == 0xC2 || Buffer[0] == 0xC3)
				return STATUS_BUFFER_TOO_SMALL;

			Length = GetOpCodeSize(Buffer);

			// jmp short const
			if (Buffer[0] == 0xEB)
			{
				if (OpSize == 0)
				{
					if (Buffer[1] != 0)
					{
						Buffer += *(PCHAR)&Buffer[1] + Length;
						*FinalAddress = Buffer;
						continue;
					}
				}
				else if (OpSize < HookOpSize - Length)
				{
					return STATUS_BUFFER_TOO_SMALL;
				}
			}
			else if (Buffer[0] == 0xFF && Buffer[1] == 0x25)
			{
				// jmp [rimm]

#if ML_X86
				if (OpSize != 0 && OpSize + Length < HookOpSize)
					return STATUS_BUFFER_TOO_SMALL;

				if (OpSize == 0 && HookOpSize > Length)
				{
					Buffer = **(PBYTE **)&Buffer[2];
					*FinalAddress = Buffer;
					continue;
				}
#elif ML_AMD64
				if (OpSize != 0)
					return STATUS_BUFFER_TOO_SMALL;

				Buffer = *(PBYTE *)(Buffer + Length + *(PLONG)&Buffer[2]);
				*FinalAddress = Buffer;
				continue;
#endif // arch

			}

			Buffer += Length;
			OpSize += Length;
		}
#endif // arch

		*Size = OpSize;

		return STATUS_SUCCESS;
	}

	ULONG_PTR GetSizeOfHookOpSize(ULONG_PTR HookOp)
	{

#if ML_AMD64

		switch (HookOp)
		{
		case OpPush:
			// 68 00 00 00 80       push    80000000h
			// C7 04 24 00 00 00 80 mov     dword ptr [rsp], 80000000h
			// C3                   ret
			return 0xD;

			//case OpJumpIndirect:
			//    //
			//    // FF 25 00 00 00 00 jmp [rimm]
			//    // 00 00 00 00 00 00 00 00
			//    //
			//    break;
			//    //return 0xE;

		default:
			if (HookOp >= OpJRax && HookOp <= OpJRdi)
			{
				//
				// 48 B8 00 00 00 80 00 00 00 80    mov     rax, 8000000080000000h
				// FF E0                            jmp     rax
				//
				return 0xC;
			}
		}

#elif ML_X86

		switch (HookOp)
		{
		case OpPush:
			//
			// 68 00000000  push const
			// C3           ret
			//
			return 1 + POINTER_SIZE + 1;

		case OpCall:
		case OpJump:
			//
			// E8 00000000  call    const
			// E9 00000000  jmp     const
			//
			return 1 + POINTER_SIZE;

		default:
			if (HookOp >= OpJRax && HookOp <= OpJRdi)
			{
				//
				// b8 88888888      mov     r32, const
				// ffe0             jmp     r32
				//
				return 1 + POINTER_SIZE + 2;
			}
		}

#endif

		return ULONG_PTR_MAX;
	}

	ForceInline
		NTSTATUS
		ProtectMemory(
			PVOID       BaseAddress,
			ULONG_PTR   Size,
			ULONG       NewProtect,
			PULONG      OldProtect
		)
	{
		return ::ProtectMemory(NtCurrentProcess(), BaseAddress, Size, NewProtect, OldProtect);
	}

	ForceInline
		NTSTATUS
		FlushInstructionCache(
			PVOID       BaseAddress,
			ULONG_PTR   NumberOfBytesToFlush
		)
	{
		return NtFlushInstructionCache(NtCurrentProcess(), BaseAddress, NumberOfBytesToFlush);
	}

	PTRAMPOLINE_DATA AllocateTrampolineData()
	{
		return (PTRAMPOLINE_DATA)RtlAllocateHeap(this->ExecutableHeap, HEAP_ZERO_MEMORY, sizeof(TRAMPOLINE_DATA));
	}

	VOID FreeTrampolineData(PTRAMPOLINE_DATA TrampolineData)
	{
		if (TrampolineData != nullptr)
			RtlFreeHeap(this->ExecutableHeap, 0, TrampolineData);
	}

	NTSTATUS RemoveTrampolineData(PTRAMPOLINE_DATA TrampolineData)
	{
		NTSTATUS Status;
		ULONG_PTR Index;

		Index = this->TrampolineList.IndexOf(TrampolineData);
		if (Index == this->TrampolineList.kInvalidIndex)
			return STATUS_NOT_FOUND;

		this->TrampolineList.Remove(Index);
		FreeTrampolineData(TrampolineData);

		return STATUS_SUCCESS;
	}

public:
	NTSTATUS PatchMemory(PPATCH_MEMORY_DATA PatchData, ULONG_PTR PatchCount, PVOID BaseAddress)
	{
		NTSTATUS Status;

		FOR_EACH(PatchData, PatchData, PatchCount)
		{
			switch (PatchData->PatchType)
			{
			case PatchMemoryTypes::MemoryPatch:
				Status = HandleMemoryPatch(PatchData, BaseAddress);
				break;

			case PatchMemoryTypes::FunctionPatch:
				Status = HandleFunctionPatch(PatchData, BaseAddress);
				break;

			default:
				Status = STATUS_NOT_IMPLEMENTED;
				break;
			}

			FAIL_RETURN(Status);
		}

		return STATUS_SUCCESS;
	}

	NTSTATUS RestoreMemory(PTRAMPOLINE_DATA TrampolineData)
	{
		NTSTATUS Status;
		PATCH_MEMORY_DATA PatchData = MemoryPatchVa(&TrampolineData->OriginalCode, TrampolineData->OriginSize, TrampolineData->PatchData.Function.Source);
		Status = HandleMemoryPatch(&PatchData, nullptr);
		NT_SUCCESS(Status) && RemoveTrampolineData(TrampolineData);

		return Status;
	}

public:
	static NTSTATUS CreateInstance(MemoryPatchManager **Manager)
	{
		*Manager = (MemoryPatchManager *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(**Manager));
		if (*Manager == nullptr)
			return STATUS_NO_MEMORY;

		ZeroMemory(*Manager, sizeof(**Manager));
		new (*Manager) MemoryPatchManager;

		return STATUS_SUCCESS;
	}

	static NTSTATUS DestroyInstance(MemoryPatchManager *Manager)
	{
		if (Manager != nullptr)
		{
			Manager->~MemoryPatchManager();
			HeapFree(GetProcessHeap(), 0, Manager);
			Manager = NULL;
		}

		return STATUS_SUCCESS;
	}
};

ForceInline MemoryPatchManager*& MemoryPatchManagerInstance()
{
	static MemoryPatchManager *Manager;
	return Manager;
}

NTSTATUS MP_CALL PatchMemory(PPATCH_MEMORY_DATA PatchData, ULONG_PTR PatchCount, PVOID BaseAddress)
{
	NTSTATUS Status;

	if (MemoryPatchManagerInstance() == nullptr)
	{
		Status = MemoryPatchManager::CreateInstance(&MemoryPatchManagerInstance());
		FAIL_RETURN(Status);
	}

	return MemoryPatchManagerInstance()->PatchMemory(PatchData, PatchCount, BaseAddress);
}

NTSTATUS MP_CALL RestoreMemory(PTRAMPOLINE_DATA TrampolineData)
{
	if (MemoryPatchManagerInstance() == nullptr)
		return STATUS_FLT_NOT_INITIALIZED;

	return MemoryPatchManagerInstance()->RestoreMemory(TrampolineData);
}

};



NTSTATUS
AllocVirtualMemory(
	PVOID*      BaseAddress,
	ULONG_PTR   Size,
	ULONG       Protect,        /* = PAGE_EXECUTE_READWRITE */
	ULONG       AllocationType, /* = MEM_RESERVE | MEM_COMMIT */
	HANDLE      ProcessHandle   /* = Ps::CurrentProcess */
)
{
	return NtAllocateVirtualMemory(ProcessHandle, BaseAddress, 0, &Size, AllocationType, Protect);
}

NTSTATUS
AllocVirtualMemoryEx(
	HANDLE      ProcessHandle,
	PVOID*      BaseAddress,
	ULONG_PTR   Size,
	ULONG       Protect,
	ULONG       AllocationType
)
{
	return AllocVirtualMemory(BaseAddress, Size, Protect, AllocationType, ProcessHandle);
}


NTSTATUS
Nt_FreeMemory(
	HANDLE  ProcessHandle,
	PVOID   BaseAddress
)
{
	SIZE_T Size = 0;

	return NtFreeVirtualMemory(ProcessHandle, &BaseAddress, &Size, MEM_RELEASE);;
}

BOOL FreeVirtualMemory(PVOID Memory, HANDLE ProcessHandle)
{
	return NT_SUCCESS(Nt_FreeMemory(ProcessHandle, Memory));
}


NTSTATUS
ReadMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  BytesRead /* = NULL */
)
{
	return NtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, Size, BytesRead);
}


NTSTATUS
WriteMemory(
	HANDLE      ProcessHandle,
	PVOID       BaseAddress,
	PVOID       Buffer,
	ULONG_PTR   Size,
	PULONG_PTR  BytesWritten /* = NULL */
)
{
	return NtWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, Size, BytesWritten);
}

#if ML_AMD64

NTSTATUS
InjectDllToRemoteProcess(
	HANDLE          ProcessHandle,
	HANDLE          ThreadHandle,
	PUNICODE_STRING DllFullPath,
	ULONG_PTR       Flags,
	PVOID*          InjectBuffer,
	ULONG_PTR       Timeout
)
{
	return STATUS_NOT_IMPLEMENTED;
}

#else // x86

ASM PVOID InjectRemote_LoadDll()
{
	INLINE_ASM
	{
		__emit  0xCC;

		push eax;           // ret addr
		pushad;
		pushfd;
		call SELF_LOCALIZATION;
	SELF_LOCALIZATION:
		pop  esi;
		and  esi, 0FFFFF000h;
		lodsd;
		mov[esp + 0x24], eax;   // ret addr
		lodsd;
		xor  ecx, ecx;
		push ecx;
		push esp;               // pModuleHandle
		push esi;               // ModuleFileName
		push ecx;               // Flags
		push ecx;               // PathToFile
		call eax;               // LdrLoadDll
		pop  eax;               // pop ModuleHandle
		xchg eax, esi;
		and  eax, 0FFFFF000h;
		and  dword ptr[eax], 0;
		popfd;
		popad;
		ret;
	}
}

static ASM VOID InjectRemote_LoadDllEnd() { ASM_UNIQUE() }

ASM VOID InjectRemote_IssueRemoteBreakin()
{
	INLINE_ASM
	{
		call    SELF_LOCATE;
	SELF_LOCATE:
		pop     ecx;
		lea     ecx,[ecx + 14];
		push    ecx;
		xor     eax, eax;
		push    dword ptr fs : [eax];
		mov     dword ptr fs : [eax], esp;
		__emit  0xCC;

		call    SELF_LOCATE2;
	SELF_LOCATE2:
		pop     esi;
		and     esi, not(MEMORY_PAGE_SIZE - 1);
		lodsd;
		mov     ebx, eax;
		lodsd;
		mov     ecx,[esi];
		jecxz   ISSUE_BREAK_IN_ONLY;

		xor     ecx, ecx;
		push    eax;    // sub esp, 4
		push    esp;    // pModuleHandle
		push    esi;    // ModuleFileName
		push    ecx;    // Flags
		push    ecx;    // PathToFile
		call    eax;    // LdrLoadDll
		//        pop     eax;    // add esp, 4;

	ISSUE_BREAK_IN_ONLY:

		xchg eax, esi;
		and  eax, 0FFFFF000h;
		and  dword ptr[eax], 0;

	EXIT_THREAD:
		//        push    0;
		push - 2;
		call    ebx;
		jmp     EXIT_THREAD;
	}
}

static ASM VOID InjectRemote_IssueRemoteBreakinEnd() { ASM_UNIQUE() }

NTSTATUS
InjectDllToRemoteProcess(
	HANDLE          ProcessHandle,
	HANDLE          ThreadHandle,
	PUNICODE_STRING DllFullPath,
	ULONG_PTR       Flags,
	PVOID*          InjectBuffer,
	ULONG_PTR       Timeout
)
{
	NTSTATUS        Status;
	PVOID           Buffer, InjectContext, ShellCode;
	ULONG_PTR       DllPathLength, InjectContextSize, ShellCodeSize;
	CONTEXT         ThreadContext;
	LARGE_INTEGER   TimeOut;

	typedef struct
	{
		ULONG           ReturnAddr;
		PVOID           xLdrLoadDll;
		UNICODE_STRING  DllPath;
	} INJECT_REMOTE_LOAD_DLL;

	typedef struct
	{
		PVOID           xNtTerminateThread;
		PVOID           xLdrLoadDll;
		UNICODE_STRING  DllPath;
		//    WCHAR           Buffer[1];
	} INJECT_REMOTE_ISSUE_BREAK_IN;

	union
	{
		INJECT_REMOTE_LOAD_DLL          LoadDll;
		INJECT_REMOTE_ISSUE_BREAK_IN    Breakin;
	};

	Buffer = NULL;
	if (FLAG_ON(Flags, INJECT_PREALLOC_BUFFER))
	{
		if (InjectBuffer == NULL)
			return STATUS_INVALID_PARAMETER;

		Buffer = *InjectBuffer;
		if (Buffer == NULL)
			return STATUS_NO_MEMORY;
	}
	else if (InjectBuffer != NULL)
	{
		*InjectBuffer = NULL;
	}

	ThreadContext.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
	Status = NtGetContextThread(ThreadHandle, &ThreadContext);
	if (!NT_SUCCESS(Status))
		return Status;

	if (!FLAG_ON(Flags, INJECT_PREALLOC_BUFFER))
	{
		Status = AllocVirtualMemoryEx(ProcessHandle, &Buffer, MEMORY_PAGE_SIZE);
		if (!NT_SUCCESS(Status))
			return Status;
	}

	if (FLAG_OFF(Flags, INJECT_ISSUE_BREAKIN_ONLY) && DllFullPath != nullptr)
	{
		DllPathLength = DllFullPath->Length;
	}
	else
	{
		DllPathLength = 0;
	}

	if (FLAG_ON(Flags, INJECT_ISSUE_REMOTE_BREAKIN | INJECT_ISSUE_BREAKIN_ONLY))
	{
		PVOID ntdll;

		InjectContext = &Breakin;
		InjectContextSize = sizeof(Breakin);
		ShellCode = InjectRemote_IssueRemoteBreakin;
		ShellCodeSize = PtrOffset(InjectRemote_IssueRemoteBreakinEnd, InjectRemote_IssueRemoteBreakin);

		ntdll = GetNtdllHandle();

		Breakin.xLdrLoadDll = EATLookupRoutineByHashPNoFix(ntdll, NTDLL_LdrLoadDll);
		Breakin.xNtTerminateThread = EATLookupRoutineByHashPNoFix(ntdll, NTDLL_NtTerminateThread);

		if (FLAG_ON(Flags, INJECT_ISSUE_BREAKIN_ONLY))
		{
			RtlInitEmptyUnicodeString(&Breakin.DllPath, NULL, 0);
		}
		else
		{
			Breakin.DllPath.Length = (USHORT)DllPathLength;
			Breakin.DllPath.MaximumLength = (USHORT)DllPathLength;
			Breakin.DllPath.Buffer = (PWSTR)((ULONG_PTR)Buffer + sizeof(Breakin));
		}
	}
	else
	{
		PVOID ntdll;

		InjectContext = &LoadDll;
		InjectContextSize = sizeof(LoadDll);
		ShellCode = PtrAdd(InjectRemote_LoadDll, FLAG_OFF(Flags, INJECT_ISSUE_BREAKIN));
		ShellCodeSize = PtrOffset(InjectRemote_LoadDllEnd, InjectRemote_LoadDll);

		ntdll = GetNtdllHandle();

		LoadDll.xLdrLoadDll = EATLookupRoutineByHashPNoFix(ntdll, NTDLL_LdrLoadDll);
		LoadDll.ReturnAddr = ThreadContext.Eip;
		LoadDll.DllPath.Length = (USHORT)DllPathLength;
		LoadDll.DllPath.MaximumLength = (USHORT)DllPathLength;
		LoadDll.DllPath.Buffer = (PWSTR)((ULONG_PTR)Buffer + sizeof(LoadDll));
	}

	Status = STATUS_UNSUCCESSFUL;
	LOOP_ONCE
	{
		Status = WriteMemory(ProcessHandle, Buffer, InjectContext, InjectContextSize);
		if (!NT_SUCCESS(Status))
			break;

		if (DllFullPath != nullptr && FLAG_OFF(Flags, INJECT_ISSUE_BREAKIN_ONLY))
		{
			Status = WriteMemory(
				ProcessHandle,
				(PVOID)((ULONG_PTR)Buffer + InjectContextSize),
				DllFullPath->Buffer,
				DllPathLength
				);
			if (!NT_SUCCESS(Status))
				break;
		}

		ThreadContext.Eip = ROUND_UP((ULONG_PTR)Buffer + InjectContextSize + DllPathLength, 16);
		Status = WriteMemory(
			ProcessHandle,
			(PVOID)ThreadContext.Eip,
			ShellCode,
			ShellCodeSize
			);
		if (!NT_SUCCESS(Status))
			break;

		NtFlushInstructionCache(ProcessHandle, (PVOID)ThreadContext.Eip, ShellCodeSize);

		if (FLAG_OFF(Flags, INJECT_DONT_CHANGE_IP))
		{
			Status = NtSetContextThread(ThreadHandle, &ThreadContext);
			if (!NT_SUCCESS(Status))
				break;

			Status = NtGetContextThread(ThreadHandle, &ThreadContext);
		}

		if (FLAG_ON(Flags, INJECT_THREAD_SUSPENDED))
		{
			if (InjectBuffer != NULL)
			{
				*InjectBuffer = FLAG_ON(Flags, INJECT_POINT_TO_SHELL_CODE) ? (PVOID)ThreadContext.Eip : Buffer;
			}

			return Status;
		}

		Status = NtResumeThread(ThreadHandle, NULL);
		if (!NT_SUCCESS(Status))
			break;

		FormatTimeOut(&TimeOut, Timeout);
		for (ULONG TryTimes = 30; TryTimes; --TryTimes)
		{
			ULONG Val;
			Status = ReadMemory(ProcessHandle, Buffer, &Val, sizeof(Val));
			if (!NT_SUCCESS(Status))
				break;

			if (Val != 0)
			{
				NtDelayExecution(FALSE, &TimeOut);
				continue;
			}

			break;
		}

		if (!NT_SUCCESS(Status))
			break;

		NtDelayExecution(FALSE, &TimeOut);
		Status = NtGetContextThread(ThreadHandle, &ThreadContext);
		if (!NT_SUCCESS(Status))
			break;

		if ((ULONG_PTR)ThreadContext.Eip < (ULONG_PTR)Buffer ||
			(ULONG_PTR)ThreadContext.Eip >(ULONG_PTR)Buffer + MEMORY_PAGE_SIZE)
		{
			Status = STATUS_SUCCESS;
		}
		else
		{
			Status = STATUS_UNSUCCESSFUL;
		}
	}

		if (FLAG_OFF(Flags, INJECT_PREALLOC_BUFFER))
			FreeVirtualMemory(Buffer, ProcessHandle);

	return Status;
}

#endif


PSTR EATLookupNameByHashNoFix(PVOID ImageBase, ULONG_PTR Hash)
{
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS           NtHeaders;
	PIMAGE_EXPORT_DIRECTORY     ExportDirectory;
	ULONG_PTR                   NumberOfNames;
	PULONG_PTR                  AddressOfFuntions;
	PCSTR                      *AddressOfNames;

	if (!IsValidImage(ImageBase, IMAGE_VALID_EXPORT_ADDRESS_TABLE))
		return nullptr;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	NtHeaders = (PIMAGE_NT_HEADERS)((ULONG_PTR)ImageBase + DosHeader->e_lfanew);

	ExportDirectory = nullptr;
	ExportDirectory = PtrAdd(ExportDirectory, ImageBase);
	ExportDirectory = PtrAdd(ExportDirectory, NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	NumberOfNames = ExportDirectory->NumberOfNames;
	AddressOfFuntions = PtrAdd((PULONG_PTR)ImageBase, ExportDirectory->AddressOfFunctions);
	AddressOfNames = PtrAdd((PCSTR *)ImageBase, ExportDirectory->AddressOfNames);

	do
	{
		if (!(HashAPI(PtrAdd((PSTR)(ImageBase), *AddressOfNames)) ^ Hash))
		{
			return PtrAdd((PSTR)(ImageBase), *AddressOfNames);
		}

		++AddressOfNames;

	} while (--NumberOfNames);

	return nullptr;
}

PSTR EATLookupNameByNameNoFix(PVOID ImageBase, PSTR Name)
{
	return EATLookupNameByHashNoFix(ImageBase, HashAPI(Name));
}

PVOID EATLookupRoutineByHashNoFix(PVOID ImageBase, ULONG_PTR Hash)
{
	PIMAGE_DOS_HEADER           DosHeader;
	PIMAGE_NT_HEADERS           NtHeaders;
	PIMAGE_EXPORT_DIRECTORY     ExportDirectory;
	ULONG_PTR                   NumberOfNames;
	PULONG_PTR                  AddressOfFuntions;
	PCSTR                      *AddressOfNames;
	PUSHORT                     AddressOfNameOrdinals;

	if (Hash != 0 && !IsValidImage(ImageBase, IMAGE_VALID_EXPORT_ADDRESS_TABLE))
		return nullptr;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	NtHeaders = (PIMAGE_NT_HEADERS)((ULONG_PTR)ImageBase + DosHeader->e_lfanew);

	switch (NtHeaders->OptionalHeader.Magic)
	{
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		break;

	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		return EATLookupRoutineByHashNoFix64(ImageBase, Hash);
	}

	if (Hash == 0)
	{
		return &NtHeaders->OptionalHeader.AddressOfEntryPoint;
	}

	ExportDirectory = nullptr;
	ExportDirectory = PtrAdd(ExportDirectory, ImageBase);
	ExportDirectory = PtrAdd(ExportDirectory, NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	NumberOfNames = ExportDirectory->NumberOfNames;
	AddressOfFuntions = PtrAdd((PULONG_PTR)ImageBase, ExportDirectory->AddressOfFunctions);
	AddressOfNames = PtrAdd((PCSTR *)ImageBase, ExportDirectory->AddressOfNames);
	AddressOfNameOrdinals = PtrAdd((PUSHORT)ImageBase, ExportDirectory->AddressOfNameOrdinals);

	do
	{
		if (!(HashAPI(PtrAdd((PSTR)(ImageBase), *AddressOfNames)) ^ Hash))
		{
			return AddressOfFuntions + *AddressOfNameOrdinals;
		}

		++AddressOfNameOrdinals;
		++AddressOfNames;
	} while (--NumberOfNames);

	return nullptr;
}

PVOID FASTCALL EATLookupRoutineByHashPNoFix(PVOID ImageBase, ULONG_PTR Hash)
{
	PVOID Pointer;

	Pointer = EATLookupRoutineByHashNoFix(ImageBase, Hash);
	if (Pointer != nullptr)
		Pointer = PtrAdd(ImageBase, *(PULONG)Pointer);

	return Pointer;
}

