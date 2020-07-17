#include "ExtraDecoder.h"


__declspec(naked) void create_decrypt_table(/* PVOID t1, PVOID t2, size_t size */)
{
	__asm
	{
                mov     ecx, [esp+ 8]
                sub     esp, 40h
                push    ebx
                push    ebp
                push    esi
                mov     edx, ecx
                lea     esi, [esp+0Ch]
                xor     eax, eax
                push    edi
                sub     edx, esi

loc_E4ECE5:                             ; CODE XREF: create_decrypt_table+26j
                lea     esi, [edx+eax*4]
                mov     esi, [esp+esi+10h]
                not     esi
                mov     [esp+eax*4+10h], esi
                inc     eax
                cmp     eax, 10h
                jl      loc_E4ECE5
                mov     eax, [esp+5ch]
                test    eax, eax
                jle     loc_E4EF1D
                mov     esi, [esp+40h]
                mov     ebp, [esp+30h]
                mov     ecx, [esp+20h]
                mov     ebx, [esp+10h]
                dec     eax
                shr     eax, 1
                inc     eax
                mov     [esp+5ch], eax
                lea     esp, [esp+0]

loc_E4ED20:                             ; CODE XREF: create_decrypt_table+23Bj
                add     ebx, ecx
                mov     eax, esi
                xor     eax, ebx
                rol     eax, 10h
                add     ebp, eax
                mov     edx, [esp+34h]
                mov     edi, ebp
                xor     edi, ecx
                rol     edi, 0Ch
                add     ebx, edi
                xor     eax, ebx
                rol     eax, 8
                mov     ecx, [esp+44h]
                add     ebp, eax
                mov     [esp+40h], eax
                mov     eax, ebp
                xor     eax, edi
                rol     eax, 7
                mov     [esp+20h], eax
                mov     eax, [esp+14h]
                add     eax, [esp+24h]
                mov     esi, [esp+28h]
                xor     ecx, eax
                rol     ecx, 10h
                add     edx, ecx
                mov     edi, edx
                xor     edi, [esp+24h]
                rol     edi, 0Ch
                add     eax, edi
                xor     ecx, eax
                rol     ecx, 8
                add     edx, ecx
                mov     [esp+44h], ecx
                mov     ecx, edx
                xor     ecx, edi
                mov     edi, [esp+38h]
                rol     ecx, 7
                mov     [esp+34h], edx
                mov     edx, [esp+48h]
                mov     [esp+24h], ecx
                mov     ecx, [esp+18h]
                add     ecx, esi
                xor     edx, ecx
                rol     edx, 10h
                add     edi, edx
                mov     [esp+38h], edi
                xor     edi, esi
                mov     esi, [esp+38h]
                rol     edi, 0Ch
                add     ecx, edi
                xor     edx, ecx
                rol     edx, 8
                add     esi, edx
                mov     [esp+48h], edx
                mov     edx, esi
                xor     edx, edi
                mov     edi, [esp+2Ch]
                rol     edx, 7
                mov     [esp+28h], edx
                mov     edx, [esp+1Ch]
                add     edx, edi
                mov     [esp+38h], esi
                mov     esi, [esp+4Ch]
                xor     esi, edx
                mov     [esp+1Ch], edx
                mov     edx, [esp+3Ch]
                rol     esi, 10h
                add     edx, esi
                mov     [esp+3Ch], edx
                xor     edx, edi
                mov     edi, [esp+1Ch]
                rol     edx, 0Ch
                add     edi, edx
                xor     esi, edi
                mov     [esp+1Ch], edi
                mov     edi, [esp+3Ch]
                rol     esi, 8
                add     edi, esi
                mov     [esp+3Ch], edi
                xor     edi, edx
                rol     edi, 7
                add     ebx, [esp+24h]
                add     eax, [esp+28h]
                mov     edx, [esp+38h]
                xor     esi, ebx
                rol     esi, 10h
                add     edx, esi
                mov     [esp+38h], edx
                xor     edx, [esp+24h]
                mov     [esp+4Ch], esi
                rol     edx, 0Ch
                add     ebx, edx
                mov     [esp+24h], edx
                mov     edx, esi
                mov     esi, [esp+38h]
                xor     edx, ebx
                rol     edx, 8
                add     esi, edx
                mov     [esp+4Ch], edx
                mov     edx, eax
                xor     edx, [esp+40h]
                mov     [esp+38h], esi
                xor     esi, [esp+24h]
                add     ecx, edi
                rol     esi, 7
                rol     edx, 10h
                mov     [esp+40h], edx
                mov     [esp+24h], esi
                mov     esi, [esp+3Ch]
                add     esi, edx
                mov     edx, esi
                xor     edx, [esp+28h]
                mov     [esp+3Ch], esi
                rol     edx, 0Ch
                add     eax, edx
                mov     [esp+14h], eax
                xor     eax, [esp+40h]
                rol     eax, 8
                mov     esi, eax
                mov     eax, [esp+3Ch]
                add     eax, esi
                mov     [esp+3Ch], eax
                xor     eax, edx
                rol     eax, 7
                mov     edx, ecx
                xor     edx, [esp+44h]
                mov     [esp+28h], eax
                rol     edx, 10h
                add     ebp, edx
                xor     edi, ebp
                rol     edi, 0Ch
                add     ecx, edi
                mov     [esp+18h], ecx
                xor     ecx, edx
                rol     ecx, 8
                mov     eax, ecx
                mov     ecx, [esp+34h]
                add     ebp, eax
                mov     [esp+44h], eax
                mov     eax, [esp+1Ch]
                add     eax, [esp+20h]
                xor     edi, ebp
                mov     edx, eax
                xor     edx, [esp+48h]
                rol     edi, 7
                rol     edx, 10h
                add     ecx, edx
                mov     [esp+2Ch], edi
                mov     edi, ecx
                xor     edi, [esp+20h]
                mov     [esp+40h], esi
                rol     edi, 0Ch
                add     eax, edi
                mov     [esp+1Ch], eax
                xor     eax, edx
                rol     eax, 8
                add     ecx, eax
                mov     [esp+34h], ecx
                xor     ecx, edi
                rol     ecx, 7
                mov     [esp+48h], eax
                mov     [esp+20h], ecx
                sub     dword ptr [esp+5Ch], 1
                jnz     loc_E4ED20
                mov     ecx, [esp+58h]
                mov     [esp+30h], ebp
                mov     [esp+10h], ebx

loc_E4EF1D:                             ; CODE XREF: create_decrypt_table+2Ej
                mov     eax, [esp+54h]
                lea     ebp, [esp+10h]
                lea     edx, [esp+14h]
                xor     edi, edi
                sub     ebp, ecx
                lea     esi, [ecx+8]
                sub     edx, ecx

loc_E4EF32:                             ; CODE XREF: create_decrypt_table+2F5j
                mov     ecx, [esi-8]
                not     ecx
                add     ecx, [esp+edi*4+10h]
                add     edi, 4
                mov     ebx, ecx
                shr     ebx, 8
                mov     [eax+1], bl
                mov     ebx, ecx
                mov     [eax], cl
                shr     ebx, 10h
                mov     [eax+2], bl
                shr     ecx, 18h
                mov     [eax+3], cl
                mov     ecx, [esi-4]
                not     ecx
                add     ecx, [esp+edi*4+4]
                add     esi, 10h
                mov     ebx, ecx
                shr     ebx, 8
                mov     [eax+5], bl
                mov     ebx, ecx
                mov     [eax+4], cl
                shr     ebx, 10h
                shr     ecx, 18h
                mov     [eax+6], bl
                mov     [eax+7], cl
                mov     ecx, [esi-10h]
                not     ecx
                add     ecx, [esi+ebp-10h]
                add     eax, 10h
                mov     ebx, ecx
                shr     ebx, 8
                mov     [eax-7], bl
                mov     ebx, ecx
                mov     [eax-8], cl
                shr     ebx, 10h
                shr     ecx, 18h
                mov     [eax-6], bl
                mov     [eax-5], cl
                mov     ecx, [esi-0Ch]
                not     ecx
                add     ecx, [edx+esi-10h]
                mov     ebx, ecx
                shr     ebx, 8
                mov     [eax-3], bl
                mov     ebx, ecx
                mov     [eax-4], cl
                shr     ebx, 10h
                shr     ecx, 18h
                cmp     edi, 10h
                mov     [eax-2], bl
                mov     [eax-1], cl
                jl      loc_E4EF32
                pop     edi
                pop     esi
                pop     ebp
                pop     ebx
                add     esp, 40h
                retn
	};
}

__declspec(naked) void init_table()
{
	__asm
	{
		mov     eax, [esp + 8]
		movzx   edx, byte ptr[eax + 3]
		shl     edx, 8
		movzx   ecx, byte ptr[eax + 2]
		or      edx, ecx
		shl     edx, 8
		movzx   ecx, byte ptr[eax + 1]
		or      edx, ecx
		shl     edx, 8
		movzx   ecx, byte ptr[eax]
		or      edx, ecx
		not     edx
		mov     ecx, [esp + 4]
		mov[ecx + 10h], edx
		movzx   edx, byte ptr[eax + 7]
		shl     edx, 8
		push    esi
		movzx   esi, byte ptr[eax + 6]
		or      edx, esi
		movzx   esi, byte ptr[eax + 5]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 4]
		shl     edx, 8
		or      edx, esi
		not     edx
		mov[ecx + 14h], edx
		movzx   edx, byte ptr[eax + 0Bh]
		movzx   esi, byte ptr[eax + 0Ah]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 9]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 8]
		shl     edx, 8
		or      edx, esi
		not     edx
		mov[ecx + 18h], edx
		movzx   edx, byte ptr[eax + 0Fh]
		movzx   esi, byte ptr[eax + 0Eh]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 0Dh]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 0Ch]
		shl     edx, 8
		or      edx, esi
		not     edx
		mov[ecx + 1Ch], edx
		movzx   edx, byte ptr[eax + 13h]
		movzx   esi, byte ptr[eax + 12h]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 11h]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 10h]
		shl     edx, 8
		or      edx, esi
		not     edx
		mov[ecx + 20h], edx
		movzx   edx, byte ptr[eax + 17h]
		movzx   esi, byte ptr[eax + 16h]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 15h]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 14h]
		shl     edx, 8
		or      edx, esi
		not     edx
		mov[ecx + 24h], edx
		movzx   edx, byte ptr[eax + 1Bh]
		movzx   esi, byte ptr[eax + 1Ah]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 19h]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 18h]
		shl     edx, 8
		or      edx, esi
		not     edx
		mov[ecx + 28h], edx
		movzx   edx, byte ptr[eax + 1Fh]
		movzx   esi, byte ptr[eax + 1Eh]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 1Dh]
		movzx   eax, byte ptr[eax + 1Ch]
		shl     edx, 8
		or      edx, esi
		shl     edx, 8
		or      edx, eax
		mov     eax, [esp + 10h]
		not     edx
		mov[ecx + 2Ch], edx
		movzx   edx, byte ptr[eax + 3]
		movzx   esi, byte ptr[eax + 2]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 1]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax]
		shl     edx, 8
		or      edx, esi
		mov[ecx], edx
		movzx   edx, byte ptr[eax + 7]
		movzx   esi, byte ptr[eax + 6]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 5]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 4]
		shl     edx, 8
		or      edx, esi
		mov[ecx + 4], edx
		movzx   edx, byte ptr[eax + 0Bh]
		movzx   esi, byte ptr[eax + 0Ah]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 9]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 8]
		shl     edx, 8
		or      edx, esi
		mov[ecx + 8], edx
		movzx   edx, byte ptr[eax + 0Fh]
		movzx   esi, byte ptr[eax + 0Eh]
		shl     edx, 8
		or      edx, esi
		movzx   esi, byte ptr[eax + 0Dh]
		movzx   eax, byte ptr[eax + 0Ch]
		shl     edx, 8
		or      edx, esi
		shl     edx, 8
		or      edx, eax
		mov     eax, [esp + 18h]
		mov[ecx + 0Ch], edx
		mov     edx, [esp + 14h]
		not     edx
		not     eax
		mov[ecx + 38h], edx
		mov     edx, [esp + 1Ch]
		mov[ecx + 3Ch], eax
		mov     eax, [esp + 20h]
		not     edx
		not     eax
		mov[ecx + 30h], edx
		mov[ecx + 34h], eax
		pop     esi
		retn
	};
}


DWORD const_val_1 = 0x0BDD72518;
DWORD const_val_2 = 0x0D541D24C;


BYTE unk_E58780[0x16] =
{
	0x9A, 0x87, 0x8F, 0x9E, 0x91, 0x9B, 0xDF, 0xCC, 0xCD, 0xD2, 0x9D, 0x86, 0x8B, 0x9A, 0xDF, 0x94
};


void* unk_E58780p = unk_E58780;


BYTE unk_E5F5E8[] = 
{
	0xbe, 0xed, 0x7e, 0x41, 0x44, 0x4c, 0xe2, 0x6c, 0x2e, 0x97, 0x83, 0x0c, 0xcf, 0x01, 0xf4, 0x26,
	0xe9, 0xcf, 0x1d, 0x66, 0x51, 0xe4, 0xe8, 0xda, 0x4e, 0xa1, 0xef, 0x0b, 0xbe, 0x0d, 0x70, 0xdb,
	0xdf, 0xbb, 0xc4, 0x5b, 0x9f, 0xcb, 0xfe, 0xe2, 0xce, 0xd6, 0xcc, 0xc6, 0xc0, 0x00, 0x45, 0x9b,
	0x1a, 0x3e, 0x27, 0xce, 0xc1, 0x94, 0x96, 0x51, 0xcd, 0x55, 0x08, 0x02, 0xe1, 0x01, 0xd5, 0xcc,
	0x0a, 0x5a, 0x14, 0x9a, 0xd1, 0x90, 0x0b, 0x37, 0x6e, 0xd6, 0xed, 0xbc, 0xfb, 0x84, 0x1a, 0x3d,
	0x23, 0x9e, 0x53, 0x25, 0x70, 0xe0, 0xd3, 0x33, 0x1c, 0x30, 0x01, 0x81, 0x08, 0x69, 0xfa, 0x05,
	0x87, 0x0e, 0x19, 0xd4, 0xf1, 0x8d, 0x7b, 0x3d, 0x59, 0x14, 0x5e, 0xe2, 0x55, 0x75, 0xd1, 0x35,
	0xca, 0x1a, 0xb5, 0x11, 0xee, 0x79, 0x01, 0x97, 0x2e, 0xab, 0x77, 0x0d, 0xe9, 0x74, 0x18, 0x9d,
	0x75, 0xe3, 0xab, 0x8b, 0xed, 0x67, 0x18, 0x4f, 0x16, 0xb6, 0x7b, 0x11, 0xb0, 0x36, 0xcf, 0x55,
	0x48, 0xe4, 0x9a, 0xe8, 0x5e, 0x9d, 0xe6, 0xc1, 0xd4, 0x37, 0x14, 0xd4, 0x9d, 0x0f, 0x90, 0x50,
	0x28, 0x88, 0x2c, 0xf3, 0x02, 0xbf, 0x9e, 0x51, 0x2e, 0x2d, 0xc4, 0xab, 0x78, 0xc5, 0xe5, 0xa9,
	0xa0, 0xae, 0x86, 0x43, 0x78, 0x8d, 0xc4, 0xbe, 0x31, 0xd6, 0x41, 0x93, 0x8c, 0x25, 0xe6, 0xe5,
	0x88, 0x95, 0xc3, 0x49, 0x47, 0x0d, 0xc0, 0x85, 0xf4, 0xcf, 0xed, 0x5f, 0x91, 0x1f, 0x5c, 0x8e,
	0xc5, 0xb2, 0x0d, 0x4f, 0xc8, 0x56, 0x65, 0x31, 0xbe, 0xf3, 0x8c, 0x39, 0xc2, 0xec, 0x19, 0xbf,
	0x0b, 0xb7, 0x77, 0x81, 0xf1, 0xda, 0x01, 0xf8, 0x43, 0xaa, 0x9e, 0x0b, 0x45, 0x0f, 0xfc, 0xee,
	0xd7, 0xd5, 0x90, 0x49, 0x6a, 0xa4, 0xa6, 0x59, 0x32, 0x8b, 0x49, 0x1b, 0x10, 0x6c, 0x25, 0x5e
};

void* unk_E5F5E8p = unk_E5F5E8;

__declspec(naked) int rj_decodep(/*BYTE *input, BYTE *output, unsigned int size*/)
{
	__asm
	{
                sub     esp, 0D4h
				xor     eax, eax
                xor     eax, esp
                mov     [esp+0D0h], eax
                mov     eax, const_val_2
                mov     ecx, const_val_1
                push    ebx
                mov     ebx, [esp+0E0h]
                push    ebp
                push    esi
                push    edi
                push    0
                push    0
                push    eax
                push    ecx
				mov     ecx, unk_E58780p
                push    ecx
                lea     edx, [esp+74h]
				mov     ecx, unk_E5F5E8p
                push    ecx
                push    edx
                mov     dword ptr [esp+78h], 8
                call    init_table
                mov     ecx, [esp+10Ch]
                mov     ebp, [esp+104h]
                xor     eax, eax
                xor     edx, edx
                add     esp, 1Ch
                mov     [esp+10h], eax
                mov     [esp+14h], edx
                cmp     ecx, 40h
                jb      loc_E4F226
                shr     ecx, 6
                mov     [esp+18h], ecx
                lea     esp, [esp+0]

loc_E4F170:                             ; CODE XREF: sub_E4F0F0+129j
                not     eax
                mov     ecx, 10h
                lea     esi, [esp+60h]
                lea     edi, [esp+1Ch]
                rep movsd
                mov     [esp+4Ch], eax
                mov     eax, [esp+5Ch]
                not     edx
                push    eax
                lea     ecx, [esp+20h]
                mov     [esp+54h], edx
                push    ecx
                lea     edx, [esp+0A8h]
                push    edx
                call    create_decrypt_table
                add     esp, 0Ch
                mov     esi, 1
                add     [esp+10h], esi
                mov     eax, 0
                adc     [esp+14h], eax
                jmp     loc_E4F1C0
; ---------------------------------------------------------------------------
                align 10h

loc_E4F1C0:                             ; CODE XREF: sub_E4F0F0+C7j
                                        ; sub_E4F0F0+113j
                movzx   ecx, byte ptr [esp+eax+0A0h]
                xor     cl, [ebp+0]
                add     eax, 4
                mov     [ebx], cl
                movzx   edx, byte ptr [ebp+1]
                xor     dl, [esp+eax+9Dh]
                movzx   ecx, byte ptr [esp+eax+9Eh]
                mov     [ebx+1], dl
                xor     cl, [ebp+2]
                movzx   edx, byte ptr [esp+eax+9Fh]
                mov     [ebx+2], cl
                xor     dl, [ebp+3]
                add     ebp, 4
                mov     [ebx+3], dl
                add     ebx, 4
                cmp     eax, 40h
                jb      loc_E4F1C0
                sub     dword ptr [esp+0F0h], 40h
                sub     [esp+18h], esi
                mov     eax, [esp+10h]
                mov     edx, [esp+14h]
                jnz     loc_E4F170
                mov     ecx, [esp+0F0h]

loc_E4F226:                             ; CODE XREF: sub_E4F0F0+6Cj
                test    ecx, ecx
                jbe     loc_E4F283
                not     eax
                mov     ecx, 10h
                lea     esi, [esp+60h]
                lea     edi, [esp+1Ch]
                rep movsd
                mov     [esp+4Ch], eax
                mov     eax, [esp+5Ch]
                not     edx
                push    eax
                lea     ecx, [esp+20h]
                mov     [esp+54h], edx
                push    ecx
                lea     edx, [esp+0A8h]
                push    edx
                call    create_decrypt_table
                mov     esi, [esp+0FCh]
                add     esp, 0Ch
                xor     eax, eax
                test    esi, esi
                jbe     loc_E4F283
                lea     esp, [esp+0]

loc_E4F270:                             ; CODE XREF: sub_E4F0F0+191j
                mov     cl, [esp+eax+0A0h]
                xor     cl, [ebp+0]
                inc     eax
                mov     [ebx], cl
                inc     ebx
                inc     ebp
                cmp     eax, esi
                jb      loc_E4F270

loc_E4F283:                             ; CODE XREF: sub_E4F0F0+138j
                                        ; sub_E4F0F0+17Aj
                mov     ecx, [esp+0E0h]
                pop     edi
                pop     esi
                pop     ebp
                pop     ebx
                xor     ecx, esp
                add     esp, 0D4h
                ret     0xc
	}
}


ULONG RJ_ExtraDecode(BYTE *Input, BYTE *Output, ULONG BufSize)
{
	ULONG Result;

	__asm
	{
		mov  eax, BufSize;
		push  eax;
		mov eax, Output;
		push eax;
		mov eax, Input;
		push eax;
		call rj_decodep;
		mov Result, eax;
	}
	return Result;
}
