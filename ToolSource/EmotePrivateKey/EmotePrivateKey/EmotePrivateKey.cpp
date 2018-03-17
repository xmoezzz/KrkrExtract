#include <Windows.h>
#include <ntstatus.h>
#include <Psapi.h>
#include <stdio.h>

#define LOOP_ONCE for(ULONG_PTR LoopVar = 0; LoopVar < 1; LoopVar++)
#define LOOP_FOREVER while(1)

/*
CPU Disasm
Address                       Hex dump                      Command                                           Comments
0FEF2E9A                        64:A3 00000000              mov dword ptr fs:[0],eax
0FEF2EA0                        8B45 0C                     mov eax,dword ptr [ebp+0C]
0FEF2EA3                        8B4D 10                     mov ecx,dword ptr [ebp+10]
0FEF2EA6                        8B5D 08                     mov ebx,dword ptr [ebp+8]
0FEF2EA9                        68 5023F60F                 push offset 0FF62350                              ; ASCII "192918854"

64 A3 00 00
00 00 8B 45
0C 8B 4D 10
8B 5D 08 68
*/

/*rdata*/

/*
Dump - emotedriver:.rdata
Address   Hex dump                                         ASCII
55742330  31 23 51 4E|41 4E 00 00|31 23 49 4E|46 00 00 00| 1#QNAN  1#INF
55742340  31 23 49 4E|44 00 00 00|31 23 53 4E|41 4E 00 00| 1#IND   1#SNAN
*/


NTSTATUS NTAPI FindEmoteKeyByMark(PULONG PrivateKey)
{
	HMODULE      hModule;
	NTSTATUS     Status;
	ULONG64      Magic[4];
	MODULEINFO   ModudleInfo;
	ULONG_PTR    iPos;
	ULONG        AddressOfPrivate;

	LOOP_ONCE
	{
		Status = STATUS_UNSUCCESSFUL;

		hModule = GetModuleHandleW(L"emotedriver.dll");

		if (!hModule)
			hModule = LoadLibraryW(L"emotedriver.dll");

		if (!hModule)
			break;

		GetModuleInformation(GetCurrentProcess(), hModule, &ModudleInfo, NULL);

		iPos = 0;
		LOOP_FOREVER
		{
			if (iPos >= ModudleInfo.SizeOfImage)
				break;

			Magic[0] = *(PULONG64)((ULONG_PTR)hModule + iPos + 0);
			Magic[1] = *(PULONG64)((ULONG_PTR)hModule + iPos + 8);
			Magic[2] = *(PULONG64)((ULONG_PTR)hModule + iPos + 16);
			Magic[3] = *(PULONG64)((ULONG_PTR)hModule + iPos + 24);


			if (Magic[0] == 0x00004E414E512331 && 
				Magic[1] == 0x000000464E492331 &&
				Magic[2] == 0x000000444E492331 &&
				Magic[3] == 0x00004E414E532331
				)
			{
				iPos += 0x20;
				AddressOfPrivate = *(PULONG)((ULONG_PTR)hModule + iPos);

				*PrivateKey = atoi((LPCSTR)AddressOfPrivate);
				Status = STATUS_SUCCESS;

				break;
			}
			else
			{
				iPos++;
			}
		}
	}

	return Status;
}

NTSTATUS NTAPI FindEmoteKeyByParse(PULONG PrivateKey)
{
	HMODULE    hModule;
	PVOID      FilterTextureAddress;
	ULONG64    Magic[2];
	ULONG_PTR  iPos;
	ULONG      AddressOfPrivate;
	NTSTATUS   Status;

	if (PrivateKey)
		*PrivateKey = 0;

	Status = STATUS_UNSUCCESSFUL;

	LOOP_ONCE
	{
		hModule = LoadLibraryW(L"emotedriver.dll");

		if (!hModule)
			hModule = LoadLibraryW(L"plugins\\emotedriver.dll");
		
		if (!hModule)
			return STATUS_NO_SUCH_FILE;
		
		FilterTextureAddress = GetProcAddress(hModule, "?EmoteFilterTexture@@YAXPAEKP6AX0K@Z@Z");

		if (!FilterTextureAddress)
			return STATUS_NOT_SUPPORTED;


		iPos = 0;
		
		LOOP_FOREVER
		{
			if (iPos >= 500)
				break;

			Magic[0] = *(PULONG64)((ULONG_PTR)FilterTextureAddress + iPos + 0);
			Magic[1] = *(PULONG64)((ULONG_PTR)FilterTextureAddress + iPos + 8);

			if (Magic[0] == 0x458B00000000A364 && Magic[1] == 0x68085D8B104D8B0C)
			{
				//push Offset
				iPos += 0x10;
				AddressOfPrivate = *(PULONG)((ULONG_PTR)FilterTextureAddress + iPos);
				
				*PrivateKey = atoi((LPCSTR)AddressOfPrivate);
				Status = STATUS_SUCCESS;

				break;
			}
			else
			{
				iPos++;
			}
		}
		
	}

	return Status;
}

/*
CPU Disasm
Address                      Hex dump                       Command                                      Comments
05B37A29                     |.  C785 38FFFFFF 15CD5B07     mov dword ptr [ebp-0C8],75BCD15
05B37A33                     |.  C785 3CFFFFFF E5559A15     mov dword ptr [ebp-0C4],159A55E5
05B37A3D                     |.  C785 40FFFFFF B53B121F     mov dword ptr [ebp-0C0],1F123BB5

*/
NTSTATUS NTAPI FindEmoteKeyByEmotePlayer(PULONG PrivateKey)
{
	ULONG64    Magic[3];
	PVOID      ModuleAddress;
	ULONG_PTR  iPos, SizeOfImage;
	ULONG      AddressOfPrivate;
	MODULEINFO ModuleInfo;
	NTSTATUS   Status;

	if (PrivateKey)
		*PrivateKey = 0;

	Status = STATUS_UNSUCCESSFUL;


	LOOP_ONCE
	{
		ModuleAddress = GetModuleHandleW(L"emoteplayer.dll");

		if (!ModuleAddress)
			ModuleAddress = LoadLibraryW(L"emoteplayer.dll");

		if (!ModuleAddress)
			ModuleAddress = LoadLibraryW(L"plugin\\emoteplayer.dll");

		if (!ModuleAddress)
			return STATUS_NO_SUCH_FILE;

		RtlZeroMemory(&ModuleInfo, sizeof(ModuleInfo));
		GetModuleInformation(GetCurrentProcess(), (HMODULE)ModuleAddress, &ModuleInfo, sizeof(ModuleInfo));
		iPos = 0;
		SizeOfImage = ModuleInfo.SizeOfImage;

		LOOP_FOREVER
		{
			if (iPos >= SizeOfImage)
				break;

			Magic[0] = *(PULONG64)((ULONG_PTR)ModuleAddress + iPos + 0);
			Magic[1] = *(PULONG64)((ULONG_PTR)ModuleAddress + iPos + 8);

			if (Magic[0] == 0xCD15FFFFFF3885C7 && Magic[1] == 0xFFFFFF3C85C7075B)
			{
				//push Offset
				/*
				CPU Disasm
				Address                      Hex dump                       Command                                      Comments
				05B37A0F                     |> \68 5C9AC005                push offset 05C09A5C                         ; ASCII "149203383"
				05B37A14                     |.  89B5 78FFFFFF              mov dword ptr [ebp-88],esi
				05B37A1A                     |.  E8 4D650800                call 05BBDF6C

				*/


				iPos -= (10 + 5 + 6 + 4);
				
				//push
				if (((PBYTE)ModuleAddress)[iPos - 1] == 0x68)
				{
					AddressOfPrivate = *(PULONG)((ULONG_PTR)ModuleAddress + iPos);

					*PrivateKey = atoi((LPCSTR)AddressOfPrivate);
					Status = STATUS_SUCCESS;

					break;
				}
				else
				{
					iPos += (10 + 5 + 6 + 4) + sizeof(Magic[0]) * 2;
				}
			}
			else
			{
				iPos++;
			}
		}

	}

	return Status;
}

int wmain(int argc, WCHAR* argv[])
{
	ULONG     Key;
	if (FindEmoteKeyByParse(&Key) >= 0 || FindEmoteKeyByMark(&Key) >= 0 || FindEmoteKeyByEmotePlayer(&Key) >= 0)
	{
		printf("Private Key : %d\n", Key);
	}
	else
	{
		printf("Cannot Find Key\n");
	}
	getchar();
	return 0;
}

