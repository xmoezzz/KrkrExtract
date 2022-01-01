#include "SectionProtector.h"
#include <my.h>
#include <atomic>
#include "KrkrExtract.h"



//
// private version
// for builtin plugins and internal uses
//

using HostTVPCreateIStreamInternal = IStream * (FASTCALL*)(const ttstr &, tjs_uint32);

//
// private version
// no stub, only designed for internal uses
//

using HostTVPCreateBStream = tTJSBinaryStream * (FASTCALL*)(const ttstr &, tjs_uint32);

//
// exported by hash
// for external uses
//

using HostTVPCreateIStream = IStream * (NTAPI*)(const ttstr &, tjs_uint32);

//
// stand c++ rt function
// only designed for internal uses
//

using HostAlloc = PVOID(CDECL*)(ULONG);


/*
exe::TVPCreateIStream.stub
CPU Disasm
Address                 Hex dump                       Command                                                    Comments
0134D830                /. /55                         push ebp
0134D831                |. |8BEC                       mov ebp,esp
0134D833                |. |8B55 0C                    mov edx,dword ptr [ebp+0C]
0134D836                |. |8B4D 08                    mov ecx,dword ptr [ebp+8]
0134D839                |. |E8 E2800000                call 01355920   exe::TVPCreateIStream
0134D83E                |. |5D                         pop ebp
0134D83F                \. |C2 0800                    retn 8

exe::TVPCreateIStream fastcall
01355920                /$  55                         push ebp
01355921                |.  8BEC                       mov ebp,esp
01355923                |.  6A FF                      push -1
01355925                |.  68 BB405C01                push 015C40BB                                              ; Entry point
0135592A                |.  64:A1 00000000             mov eax,dword ptr fs:[0]
01355930                |.  50                         push eax
01355931                |.  83EC 0C                    sub esp,0C
01355934                |.  53                         push ebx
01355935                |.  56                         push esi
01355936                |.  57                         push edi
01355937                |.  A1 B0486901                mov eax,dword ptr [16948B0]
0135593C                |.  33C5                       xor eax,ebp
0135593E                |.  50                         push eax
0135593F                |.  8D45 F4                    lea eax,[ebp-0C]
01355942                |.  64:A3 00000000             mov dword ptr fs:[0],eax
01355948                |.  8965 F0                    mov dword ptr [ebp-10],esp
0135594B                |.  C745 EC 00000000           mov dword ptr [ebp-14],0
01355952                |.  C745 FC 00000000           mov dword ptr [ebp-4],0
01355959                |.  E8 8296FEFF                call 0133EFE0   exe::TVPCreateStream(fastcall)


0135595E                |.  8BF0                       mov esi,eax
01355960                |.  8975 EC                    mov dword ptr [ebp-14],esi
01355963                |.  C745 FC FFFFFFFF           mov dword ptr [ebp-4],-1
0135596A                |.  6A 0C                      push 0C
0135596C                |.  E8 77882000                call 0155E1E8   call operator.new
01355971                |.  83C4 04                    add esp,4
*/

static std::atomic<BOOL>    g_krkrzIsInitialized = FALSE;
static HostTVPCreateIStream g_krkrzTVPCreateIStream = NULL;
static HostTVPCreateIStreamInternal g_krkrzTVPCreateIStreamP = NULL;
static HostTVPCreateBStream g_krkrzTVPCreateBStream = NULL;
static HostAlloc            g_krkrzHostAlloc = NULL;
static PVOID                g_krkrzIStreamAdapterVtable = NULL;

ForceInline
BOOL
CheckKrkrzPtrStatus()
{
	if (!g_krkrzTVPCreateBStream)
		return FALSE;

	if (!g_krkrzHostAlloc)
		return FALSE;

	if (!g_krkrzIStreamAdapterVtable)
		return FALSE;

	return TRUE;
}


//
// call this routine to initiaize pointers
// caller must check engine type
//

PVOID GetTVPCreateStreamCall()
{
	PVOID CallIStreamStub, CallIStream, CallTVPCreateStreamCall;
	ULONG OpSize, OpOffset;
	WORD  WordOpcode;

	static char funcname[] = "IStream * ::TVPCreateIStream(const ttstr &,tjs_uint32)";

	LOOP_ONCE
	{
		CallTVPCreateStreamCall = NULL;

		CallIStreamStub = TVPGetImportFuncPtr(funcname);
		if (!CallIStreamStub)
			break;

		CallIStream = NULL;
		OpOffset = 0;

		LOOP_FOREVER
		{
			if (((PBYTE)CallIStreamStub + OpOffset)[0] == 0xCC)
				break;

			WordOpcode = *(PWORD)((ULONG_PTR)CallIStreamStub + OpOffset);
			//mov edx,dword ptr [ebp+0xC]
			if (WordOpcode == 0x558B)
			{
				OpOffset += 2;
				if (((PBYTE)CallIStreamStub + OpOffset)[0] == 0xC)
				{
					OpOffset++;
					WordOpcode = *(PWORD)((ULONG_PTR)CallIStreamStub + OpOffset);
					//mov edx,dword ptr [ebp+0x8]
					if (WordOpcode == 0x4D8B)
					{
						OpOffset += 2;
						if (((PBYTE)CallIStreamStub + OpOffset)[0] == 0x8)
						{
							OpOffset++;
							if (((PBYTE)CallIStreamStub + OpOffset)[0] == CALL)
							{
								CallIStream = (PVOID)GetCallDestination(((ULONG_PTR)CallIStreamStub + OpOffset));
								OpOffset += 5;
								break;
							}
						}
					}
				}
			}
			//the next opcode
			OpSize = GetOpCodeSize32(((PBYTE)CallIStreamStub + OpOffset));
			OpOffset += OpSize;
		}

		if (!CallIStream)
			break;

		OpOffset = 0;
		LOOP_FOREVER
		{
			if (((PBYTE)CallIStream + OpOffset)[0] == 0xC3)
				break;

		//find the first call
		if (((PBYTE)CallIStream + OpOffset)[0] == CALL)
		{
			CallTVPCreateStreamCall = (PVOID)GetCallDestination(((ULONG_PTR)CallIStream + OpOffset));
			OpOffset += 5;
			break;
		}

		//the next opcode
		OpSize = GetOpCodeSize32(((PBYTE)CallIStream + OpOffset));
		OpOffset += OpSize;
	}

	LOOP_FOREVER
	{
		if (((PBYTE)CallIStream + OpOffset)[0] == 0xC3)
			break;

		if (((PBYTE)CallIStream + OpOffset)[0] == CALL)
		{
			//push 0xC
			//call HostAlloc
			//add esp, 0x4
			if (((PBYTE)CallIStream + OpOffset - 2)[0] == 0x6A &&
				((PBYTE)CallIStream + OpOffset - 2)[1] == 0x0C)
			{
				g_krkrzHostAlloc = (HostAlloc)GetCallDestination(((ULONG_PTR)CallIStream + OpOffset));
				OpOffset += 5;
			}
			break;
		}

		//the next opcode
		OpSize = GetOpCodeSize32(((PBYTE)CallIStream + OpOffset));
		OpOffset += OpSize;
	}

	LOOP_FOREVER
	{
		if (((PBYTE)CallIStream + OpOffset)[0] == 0xC3)
			break;

	//mov eax, mem.offset
	if (((PBYTE)CallIStream + OpOffset)[0] == 0xC7 &&
		((PBYTE)CallIStream + OpOffset)[1] == 0x00)
	{
		OpOffset += 2;
		g_krkrzIStreamAdapterVtable = (PVOID) *(PULONG_PTR)((PBYTE)CallIStream + OpOffset);
		OpOffset += 4;
		break;
	}

	//the next opcode
	OpSize = GetOpCodeSize32(((PBYTE)CallIStream + OpOffset));
	OpOffset += OpSize;
}
	}

		if (g_krkrzHostAlloc && g_krkrzIStreamAdapterVtable)
		{
			return CallTVPCreateStreamCall;
		}
		return NULL;
}

NTSTATUS KrkrExtractCore::InitializeTVPCreateStreamCallKrkrZ()
{
	ULONG OpSize, OpOffset;
	WORD  WordOpcode;

	static char funcname[] = "IStream * ::TVPCreateIStream(const ttstr &,tjs_uint32)";

	SectionProtector Protector(m_CodeAnaLock.Get());

	auto AttachWithPtrValues = [&]()->void
	{
		m_TVPCreateIStreamStub = g_krkrzTVPCreateIStream;
		m_TVPCreateIStreamP    = g_krkrzTVPCreateIStreamP;
		m_TVPCreateBStream     = g_krkrzTVPCreateBStream;
		m_Allocator            = g_krkrzHostAlloc;
		m_IStreamAdapterVtable = g_krkrzIStreamAdapterVtable;
	};

	if (g_krkrzIsInitialized) 
	{
		AttachWithPtrValues();
		return TRUE;
	}

	g_krkrzTVPCreateBStream = (HostTVPCreateBStream)GetTVPCreateStreamCall();

	if (CheckKrkrzPtrStatus())
	{
		AttachWithPtrValues();
		g_krkrzIsInitialized  = TRUE;
		m_PointersInitialized = TRUE;
		return STATUS_SUCCESS;
	}

	return STATUS_UNSUCCESSFUL;
}




/*
.text:006832D8                         sub_6832D8      proc near               ; DATA XREF: .data:0073930C¡ýo
.text:006832D8
.text:006832D8                         arg_0           = dword ptr  8
.text:006832D8                         arg_4           = dword ptr  0Ch
.text:006832D8
.text:006832D8 55                                      push    ebp
.text:006832D9 8B EC                                   mov     ebp, esp
.text:006832DB 8B 55 0C                                mov     edx, [ebp+arg_4]
.text:006832DE 8B 45 08                                mov     eax, [ebp+arg_0]
.text:006832E1 E8 CA 29 F9 FF                          call    sub_615CB0   ;;BCB modified fastcall
.text:006832E6 5D                                      pop     ebp
.text:006832E7 C2 08 00                                retn    8
.text:006832E7                         sub_6832D8      endp
*/

static std::atomic<BOOL>    g_krkr2IsInitialized = FALSE;
static HostTVPCreateIStream g_krkr2TVPCreateIStream = NULL; 

//
// BCB fastcall call
//
static PVOID                g_krkr2TVPCreateIStreamP = NULL; 

//
// BCB fastcall call
//
static PVOID                g_krkr2TVPCreateBStream = NULL;
static HostAlloc            g_krkr2HostAlloc  = NULL;
static PVOID                g_krkr2IStreamAdapterVtable = NULL;


IStream* FASTCALL krkr2TVPCreateIStreamPWrapper(const ttstr & name, tjs_uint32 mode)
{
	IStream* Stream = NULL;

	if (!g_krkr2TVPCreateIStreamP || !g_krkr2IsInitialized)
		return NULL;

	INLINE_ASM
	{
		mov eax, name
		mov edx, mode
		call g_krkr2TVPCreateIStreamP
		mov Stream, eax
	};

	return Stream;
}


tTJSBinaryStream* FASTCALL krkr2TVPCreateBStreamWrapper(const ttstr &name, tjs_uint32 mode)
{
	tTJSBinaryStream* Stream = NULL;
	
	if (!g_krkr2TVPCreateBStream || !g_krkr2IsInitialized)
		return NULL;
	
	INLINE_ASM
	{
		mov eax, name
		mov edx, mode
		call g_krkr2TVPCreateBStream
		mov Stream, eax
	};

	return Stream;
}



ForceInline
BOOL
CheckKrkr2PtrStatus()
{
	if (!g_krkr2TVPCreateIStream)
		return FALSE;

	if (!g_krkr2TVPCreateIStreamP)
		return FALSE;

	if (!g_krkr2TVPCreateBStream)
		return FALSE;

	if (!g_krkr2HostAlloc)
		return FALSE;

	if (!g_krkr2IStreamAdapterVtable)
		return FALSE;

	return TRUE;
}



NTSTATUS KrkrExtractCore::InitializeTVPCreateStreamCallKrkr2()
{
	PBYTE IStreamAdapterConstructor, LastVtable = NULL;
	ULONG OpSize, OpOffset;
	WORD  WordOpcode;

	static char funcname[] = "IStream * ::TVPCreateIStream(const ttstr &,tjs_uint32)";

	SectionProtector Protector(m_CodeAnaLock.Get());

	auto AttachWithPtrValues = [&]()->void
	{
		m_TVPCreateIStreamStub = g_krkr2TVPCreateIStream;
		m_TVPCreateIStreamP    = krkr2TVPCreateIStreamPWrapper;
		m_TVPCreateBStream     = krkr2TVPCreateBStreamWrapper;
		m_Allocator            = g_krkr2HostAlloc;
		m_IStreamAdapterVtable = g_krkr2IStreamAdapterVtable;
	};

	if (g_krkrzIsInitialized)
	{
		AttachWithPtrValues();
		return TRUE;
	}

	LOOP_ONCE
	{
		g_krkr2TVPCreateIStream = (HostTVPCreateIStream)TVPGetImportFuncPtr(funcname);
		if (!g_krkr2TVPCreateIStream)
			break;

		OpOffset = 0;

		LOOP_FOREVER
		{
			if (((PBYTE)g_krkr2TVPCreateIStream + OpOffset)[0] == 0xCC)
				break;

			if (OpOffset >= 0x50)
				break;

			WordOpcode = *(PWORD)((ULONG_PTR)g_krkr2TVPCreateIStream + OpOffset);

			//
			// stdcall to BCB fastcall
			// 8B 55 0C                                mov     edx, [ebp+arg_4]
			//

			if (WordOpcode == 0x558B)
			{
				OpOffset += 2;
				if (((PBYTE)g_krkr2TVPCreateIStream + OpOffset)[0] == 0xC)
				{
					OpOffset++;
					WordOpcode = *(PWORD)((ULONG_PTR)g_krkr2TVPCreateIStream + OpOffset);

					//
					// stdcall to BCB fastcall
					// 8B 45 08                                mov     eax, [ebp+arg_0]
					//

					if (WordOpcode == 0x458B)
					{
						OpOffset += 2;
						if (((PBYTE)g_krkr2TVPCreateIStream + OpOffset)[0] == 0x8)
						{
							OpOffset++;
							if (((PBYTE)g_krkr2TVPCreateIStream + OpOffset)[0] == CALL)
							{
								//
								// also BCB style fastcall
								// E8 CA 29 F9 FF                          call    sub_615CB0
								//

								g_krkr2TVPCreateIStreamP = (PVOID)GetCallDestination(((ULONG_PTR)g_krkr2TVPCreateIStream + OpOffset));
								OpOffset += 5;
								break;
							}
						}
					}
				}
			}
			//the next opcode
			OpSize = GetOpCodeSize32(((PBYTE)g_krkr2TVPCreateIStream + OpOffset));
			OpOffset += OpSize;
		}

		if (!g_krkr2TVPCreateIStreamP)
			break;


		OpOffset = 0;
		LOOP_FOREVER
		{
			if (((PBYTE)g_krkr2TVPCreateIStreamP + OpOffset)[0] == 0xC3)
				break;

			if (OpOffset >= 0x100)
				break;

			//
			// find the first call
			// TODO : VSA
			//

			if (((PBYTE)g_krkr2TVPCreateIStreamP + OpOffset)[0] == CALL)
			{
				g_krkr2TVPCreateBStream = (PVOID)GetCallDestination(((ULONG_PTR)g_krkr2TVPCreateIStreamP + OpOffset));
				OpOffset += 5;
				break;
			}

			//
			// the next opcode
			//

			OpSize = GetOpCodeSize32(((PBYTE)g_krkr2TVPCreateIStreamP + OpOffset));
			OpOffset += OpSize;
		}

		LOOP_FOREVER
		{
			if (((PBYTE)g_krkr2TVPCreateIStreamP + OpOffset)[0] == 0xC3)
				break;

			if (OpOffset >= 0x100)
				break;

			//
			// the next call
			//

			if (((PBYTE)g_krkr2TVPCreateIStreamP + OpOffset)[0] == CALL)
			{
				//
				// push 0xC
				// call HostAlloc
				// .text:00615D5A 6A 0C                                   push    0Ch             ; Size
				// .text:00615D5C E8 67 AD F5 FF                          call    @$bnew$qui; operator new(uint)
				//

				if (((PBYTE)g_krkr2TVPCreateIStreamP + OpOffset - 2)[0] == 0x6A &&
					((PBYTE)g_krkr2TVPCreateIStreamP + OpOffset - 2)[1] == 0x0C)
				{
					g_krkr2HostAlloc = (HostAlloc)GetCallDestination(((ULONG_PTR)g_krkr2TVPCreateIStreamP + OpOffset));
					OpOffset += 5;
				}
				break;
			}

			//
			// the next opcode
			//

			OpSize = GetOpCodeSize32(((PBYTE)g_krkr2TVPCreateIStreamP + OpOffset));
			OpOffset += OpSize;
		}

		//
		// find IStreamAdapterConstructor
		//
		
		IStreamAdapterConstructor = NULL;
		LOOP_FOREVER
		{
			if (((PBYTE)g_krkr2TVPCreateIStream + OpOffset)[0] == 0xC3)
				break;

			if (OpOffset >= 0x100)
				break;

			//
			// the next call
			//

			if (((PBYTE)g_krkr2TVPCreateIStreamP + OpOffset)[0] == CALL)
			{
				IStreamAdapterConstructor = (PBYTE)GetCallDestination(((ULONG_PTR)g_krkr2TVPCreateIStreamP + OpOffset));
				OpOffset += 5;
				break;
			}

			//
			// the next opcode
			//

			OpSize = GetOpCodeSize32(((PBYTE)g_krkr2TVPCreateIStreamP + OpOffset));
			OpOffset += OpSize;
		}

		if (!IStreamAdapterConstructor)
			break;

		if (IsBadCodePtr((FARPROC)IStreamAdapterConstructor))
			break;


		//
		// find the last one `mov eax, mem.offset`
		//

		OpOffset = 0;
		LOOP_FOREVER
		{
			//
			// until reach ret
			//
			if (((PBYTE)IStreamAdapterConstructor + OpOffset)[0] == 0xC3)
				break;

			if (OpOffset >= 0x50)
				break;

			//
			// the last one (BCB stupid code optimization)
			// mov eax, mem.offset
			//

			if (((PBYTE)IStreamAdapterConstructor + OpOffset)[0] == 0xB8)
			{
				//
				//.text:006157BD B8 4C 6D 71 00                          mov     eax, offset off_716D4C
				//

				OpOffset += 1;
				LastVtable = (PBYTE)*(PULONG_PTR)((PBYTE)IStreamAdapterConstructor + OpOffset);
				OpOffset += 4;
				break;
			}

			//
			// the next opcode
			//

			OpSize = GetOpCodeSize32(((PBYTE)IStreamAdapterConstructor + OpOffset));
			OpOffset += OpSize;
		}

		if (LastVtable && !IsBadReadPtr(LastVtable, sizeof(PVOID)))
		{
			g_krkr2IStreamAdapterVtable = LastVtable;
			break;
		}
	}

	if (CheckKrkr2PtrStatus())
	{
		AttachWithPtrValues();
		g_krkr2IsInitialized  = TRUE;
		m_PointersInitialized = TRUE;
		return TRUE;
	}

	return FALSE;
}


tTJSBinaryStream* KrkrExtractCore::HostTVPCreateStream(PCWSTR FilePath)
{
	NTSTATUS             Status;
	HostTVPCreateBStream Routine;

	if (!GetTVPCreateBStream()) {
		Status = InitializePrivatePointers();
		if (NT_FAILED(Status)) {
			TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"CallTVPCreateStream : InitializePrivatePointers failed : %08x",
				Status
			);
			return NULL;
		}
	}

	Routine = (HostTVPCreateBStream)GetTVPCreateBStream();
	if (Routine) {
		return Routine(FilePath, TJS_BS_READ);
	}

	return NULL;
}



IStream* KrkrExtractCore::HostConvertBStreamToIStream(tTJSBinaryStream* BStream)
{
	NTSTATUS         Status;
	IStream*         Stream;
	PVOID            CallHostAlloc;
	PVOID            IStreamAdapterVTableOffset;

	if (!GetAllocator()) {
		Status = InitializePrivatePointers();
		if (NT_FAILED(Status)) {
			TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"ConvertBStreamToIStream : InitializePrivatePointers failed : %08x",
				Status
			);
			return NULL;
		}
	}

	CallHostAlloc = GetAllocator();
	IStreamAdapterVTableOffset = GetIStreamAdapterVtable();
	Stream = NULL;

	INLINE_ASM
	{
		push 0xC;
		call CallHostAlloc;
		add  esp, 0x4;
		test eax, eax;
		jz   NO_CREATE_STREAM;
		mov  esi, IStreamAdapterVTableOffset;
		mov  dword ptr[eax], esi; //Vtable 
		mov  esi, BStream;
		mov  dword ptr[eax + 4], esi; //StreamHolder
		mov  dword ptr[eax + 8], 1;   //ReferCount
		mov  Stream, eax;

		NO_CREATE_STREAM:
	}

	return Stream;
}



