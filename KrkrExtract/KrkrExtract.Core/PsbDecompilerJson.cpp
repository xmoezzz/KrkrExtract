#include "my.h"
#include <Psapi.h>
#include <map>
#include <string>
#include <Shlobj.h>
#include "json/json.h"
#include "PsbFile.h"
#include "lz4.h"
#include "lz4frame.h"
#include "zlib.h"
#include "SafeMemory.h"
#include "PsbDecompression.h"
#include "PsbDecryption.h"

#pragma comment(lib, "Shell32.lib")

using std::string;
using std::vector;
using std::map;

#ifndef LZ4_MAGIC
#define LZ4_MAGIC 0x184D2204
#endif


void
traversal_object_tree(psb_t& psb,
	const psb_objects_t *objects, string object_name, Json::Value &root);

void
traversal_offsets_tree(psb_t& psb,
	const psb_collection_t *offsets, string entry_name, Json::Value &root) {
	psb_value_t *value = NULL;

	for (uint32_t i = 0; i < offsets->size(); i++) {
		unsigned char* entry_buff = offsets->get(i);
		psb.unpack(value, entry_buff);


		if (value != NULL) {
			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_COLLECTION) {

				Json::Value node(Json::arrayValue);
				traversal_offsets_tree(psb, (const psb_collection_t *)value, entry_name, node);
				root.append(node);

			}
			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_OBJECTS) {

				Json::Value node(Json::objectValue);
				traversal_object_tree(psb, (const psb_objects_t *)value, entry_name, node);
				root.append(node);
			}

			if (value->get_type() == psb_value_t::TYPE_TRUE || value->get_type() == psb_value_t::TYPE_FALSE) {

				Json::Value node(Json::booleanValue);
				psb_boolean_t *psb_boolean = (psb_boolean_t*)value;
				node = psb_boolean->get_boolean();
				root.append(node);
			}

			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_STRING_N1) {

				Json::Value node(Json::stringValue);
				psb_string_t *psb_string = (psb_string_t*)value;
				node = psb_string->get_string();
				root.append(node);
			}
			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_NUMBER_N0 || value->get_type() == psb_value_t::TYPE_NUMBER_N1 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N2 || value->get_type() == psb_value_t::TYPE_NUMBER_N3 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N4) {

				Json::Value node(Json::intValue);
				psb_number_t *number = (psb_number_t*)value;
				node = number->get_integer();
				root.append(node);
			}


			if (value->get_type() == psb_value_t::TYPE_FLOAT0 || value->get_type() == psb_value_t::TYPE_FLOAT || value->get_type() == psb_value_t::TYPE_DOUBLE) {

				Json::Value node(Json::realValue);

				psb_number_t *number = (psb_number_t*)value;
				if (value->get_type() == psb_value_t::TYPE_FLOAT || value->get_type() == psb_value_t::TYPE_FLOAT0) {
					node = number->get_float();
				}
				if (value->get_type() == psb_value_t::TYPE_DOUBLE) {
					node = number->get_double();
				}

				root.append(node);
			}

			if (value->get_type() == psb_value_t::TYPE_NUMBER_N5 || value->get_type() == psb_value_t::TYPE_NUMBER_N6 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N7 || value->get_type() == psb_value_t::TYPE_NUMBER_N8) {

				Json::Value node(Json::intValue);
				psb_number_t *number = (psb_number_t*)value;
				node = number->get_integer();
				root.append(node);
			}

			if (value->get_type() == psb_value_t::TYPE_NULL) {
				Json::Value node(Json::nullValue);
				root.append(node);
			}
			if (value->get_type() == psb_value_t::TYPE_RESOURCE_N1 || value->get_type() == psb_value_t::TYPE_RESOURCE_N2 ||
				value->get_type() == psb_value_t::TYPE_RESOURCE_N3 || value->get_type() == psb_value_t::TYPE_RESOURCE_N4) {
				psb_resource_t *resource = (psb_resource_t *)value;
				Json::Value node(Json::stringValue);
				char temp[32];
				_itoa_s(resource->get_index(), temp, 10);
				node = "#resource#" + (string)temp;
				root.append(node);
			}
		}

		else {
			printf("invalid_type:%s,%02X\n", entry_name.c_str(), entry_buff[0]);
		}
	}
}
void
traversal_object_tree(psb_t& psb,
	const psb_objects_t *objects, string object_name, Json::Value &root) {
	psb_value_t *value = NULL;
	for (uint32_t i = 0; i < objects->size(); i++) {
		string entry_name = objects->get_name(i);
		unsigned char* entry_buff = objects->get_data(i);

		psb.unpack(value, entry_buff);

		if (value != NULL) {
			//=======================================================

			if (value->get_type() == psb_value_t::TYPE_COLLECTION) {

				Json::Value node(Json::arrayValue);
				traversal_offsets_tree(psb, (const psb_collection_t *)value, entry_name, node);
				root[entry_name] = node;
			}
			//=======================================================

			if (value->get_type() == psb_value_t::TYPE_OBJECTS) {

				Json::Value node(Json::objectValue);
				traversal_object_tree(psb, (const psb_objects_t *)value, entry_name, node);
				root[entry_name] = node;
			}

			if (value->get_type() == psb_value_t::TYPE_TRUE || value->get_type() == psb_value_t::TYPE_FALSE) {

				Json::Value node(Json::booleanValue);
				psb_boolean_t *psb_boolean = (psb_boolean_t*)value;
				node = psb_boolean->get_boolean();
				root[entry_name] = node;
			}

			//=======================================================

			if (value->get_type() == psb_value_t::TYPE_STRING_N1) {

				Json::Value node(Json::stringValue);
				psb_string_t *psb_string = (psb_string_t*)value;
				node = psb_string->get_string();
				root[entry_name] = node;
			}
			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_NUMBER_N0 || value->get_type() == psb_value_t::TYPE_NUMBER_N1 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N2 || value->get_type() == psb_value_t::TYPE_NUMBER_N3 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N4) {

				Json::Value node(Json::intValue);
				psb_number_t *number = (psb_number_t*)value;
				node = number->get_integer();
				root[entry_name] = node;
			}

			if (value->get_type() == psb_value_t::TYPE_FLOAT0 || value->get_type() == psb_value_t::TYPE_FLOAT || value->get_type() == psb_value_t::TYPE_DOUBLE) {

				Json::Value node(Json::realValue);

				psb_number_t *number = (psb_number_t*)value;
				if (value->get_type() == psb_value_t::TYPE_FLOAT || value->get_type() == psb_value_t::TYPE_FLOAT0) {
					node = number->get_float();
				}
				if (value->get_type() == psb_value_t::TYPE_DOUBLE) {
					node = number->get_double();
				}
				root[entry_name] = node;
			}

			if (value->get_type() == psb_value_t::TYPE_NUMBER_N5 || value->get_type() == psb_value_t::TYPE_NUMBER_N6 ||
				value->get_type() == psb_value_t::TYPE_NUMBER_N7 || value->get_type() == psb_value_t::TYPE_NUMBER_N8) {

				Json::Value node(Json::intValue);
				psb_number_t *number = (psb_number_t*)value;
				node = number->get_integer();
				root[entry_name] = node;
			}

			//=======================================================
			if (value->get_type() == psb_value_t::TYPE_NULL) {
				Json::Value node(Json::nullValue);
				root[entry_name] = node;
			}

			if (value->get_type() == psb_value_t::TYPE_RESOURCE_N1 || value->get_type() == psb_value_t::TYPE_RESOURCE_N2 ||
				value->get_type() == psb_value_t::TYPE_RESOURCE_N3 || value->get_type() == psb_value_t::TYPE_RESOURCE_N4) {
				psb_resource_t *resource = (psb_resource_t *)value;

				Json::Value node(Json::stringValue);
				char temp[32];
				_itoa_s(resource->get_index(), temp, 10);
				node = "#resource#" + (string)temp;
				root[entry_name] = node;
			}
		}
		else {
			printf("invalid_type:%s:%02X\n", entry_name.c_str(), entry_buff[0]);
		}
	}
}




void export_res(psb_t &psb, LPCWSTR FileName, LPCWSTR DirName)
{
	NTSTATUS Status;

	Json::Value res(Json::arrayValue);

	std::wstring ResDirName = FileName;
	ResDirName += L"\\";
	ResDirName += FileName;
	ResDirName += L".dir";

	if (GetFileAttributesW(ResDirName.c_str()) == 0xFFFFFFFF) {
		SHCreateDirectory(NULL, ResDirName.c_str());
	}

	for (uint32_t i = 0; i < psb.chunk_offsets->size(); i++)
	{
		WCHAR OutputName[MAX_PATH];
		CHAR  Utf8Name[MAX_PATH * 2];

		uint32_t offset = psb.chunk_offsets->get(i);
		uint32_t length = psb.chunk_lengths->get(i);

		wsprintfW(OutputName, L"%s\\%d.bin", ResDirName, i);

		NtFileDisk File;
		Status = File.Create(OutputName);
		if (NT_FAILED(Status))
			continue;

		File.Write((PVOID)(psb.chunk_data + offset), length);
		File.Close();

		WideCharToMultiByte(CP_UTF8, 0, OutputName, lstrlenW(OutputName), Utf8Name, countof(Utf8Name) - 1, NULL, NULL);
		res[i] = Utf8Name;
	}

	NtFileDisk File;
	WCHAR      OutputName[MAX_PATH];
	wsprintfW(OutputName, L"%s\\%s.res.json", DirName, FileName);
	
	if (psb.chunk_offsets->size() != 0)
	{
		Status = File.Create(OutputName);
		if (NT_FAILED(Status))
			return;

		auto&& resJson = res.toStyledString();
		File.Write((PBYTE)resJson.c_str(), resJson.length());
		File.Close();
	}
}



#pragma pack(1)
typedef struct PSB_HEADER_V2
{
	DWORD  HeaderMagic;
	USHORT Version;
	USHORT Flag;
	LPBYTE pHeader;
	LPBYTE pStringIndex;
	LPBYTE pStringOffsetArray;
	LPCSTR pStringPool;
	LPBYTE pBinOffsetArray;
	LPBYTE pBinSizeArray;
	LPBYTE pBinaryPool;
	LPBYTE pRootCode;
} PSB_HEADER_V2, *pPSB_HEADER_V2;
#pragma pack()


NTSTATUS DecompilePsbJson(KrkrClientProxyer* Proxyer, IStream* PsbStream, LPCWSTR BasePath, LPCWSTR FileName)
{
	NTSTATUS           Status;
	Json::Value        Root;
	NtFileDisk         File;
	std::wstring       JsonFileName;
	ULONG              Length, PrivateKey;
	LARGE_INTEGER      Offset;
	STATSTG            Stat;

	JsonFileName = BasePath;
	JsonFileName += L"\\";
	JsonFileName += FileName;
	JsonFileName += L".json";

	Offset.QuadPart = 0;
	PsbStream->Seek(Offset, FILE_BEGIN, NULL);
	PsbStream->Stat(&Stat, STATFLAG_DEFAULT);

	Length = Stat.cbSize.LowPart;
	auto Buffer = AllocateMemorySafeP<BYTE>(Length);
	if (!Buffer)
		return STATUS_NO_MEMORY;

	PsbStream->Read(Buffer.get(), Length, NULL);

	if (Length > 8)
	{
		if (RtlCompareMemory(Buffer.get(), "PSB", 4) != 4)
		{
			if (*(PDWORD)(Buffer.get()) == LZ4_MAGIC) {
				Buffer = GetDataFromLZ4(Buffer, Length);
			}
			else {
				Buffer = GetDataFromMDF(Buffer, Length);
			}
		}

		if (!Buffer)
			return STATUS_BAD_DATA;
	}
		
	if (Length < 64 || RtlCompareMemory(Buffer.get(), "PSB", 4) != 4)
		return STATUS_INVALID_BLOCK_LENGTH;

	PSB_HEADER_V2* pInfo = (PSB_HEADER_V2*)Buffer.get();

	if (pInfo->Version != 2 && pInfo->Version != 3)
		return STATUS_NO_MATCH;

	PrivateKey = 0;
	Status = Proxyer->GetEmotePrivateKey(&PrivateKey);
	PsbDecryption Cipher(0x075BCD15, 0x159A55E5, 0x1F123BB5, PrivateKey, 0, 0);

	if (pInfo->Flag & 1 && pInfo->Version == 3) 
	{
		if (NT_FAILED(Status)) {
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"emote : Private key is not available"
			);
		}

		Cipher.Decrypt(Buffer.get() + 0x8, 0, pInfo->Version == 3 ? 0x24 : 0x20);
	}

	//
	// PSB V2 will not mark the field
	// root is a directory?
	//

	if (pInfo->Flag & 2 || ((ULONG_PTR)pInfo->pRootCode >= Length) || Buffer.get()[(ULONG_PTR)pInfo->pRootCode] != 0x21)
	{

		if (NT_FAILED(Status)) {
			Proxyer->TellServerLogOutput(
				LogLevel::LOG_ERROR,
				L"emote : Private key is not available"
			);
		}

		Cipher.Decrypt(Buffer.get() + (ULONG_PTR)pInfo->pStringIndex, 0,
				(ULONG_PTR)pInfo->pBinOffsetArray - (ULONG_PTR)pInfo->pStringIndex);

		if (Buffer.get()[(ULONG_PTR)pInfo->pRootCode] != 0x21)
			return STATUS_UNSUCCESSFUL;
	}

	psb_t Psb(Buffer.get());
	auto Objects = Psb.get_objects();

	try  {
		traversal_object_tree(Psb, Objects, "", Root);
	}
	catch (...) 
	{
		PrintConsoleW(L"failed to decode json : %s\n", JsonFileName.c_str());
		return STATUS_UNSUCCESSFUL;
	}

	Status = File.Create(JsonFileName.c_str());
	if (NT_FAILED(Status))
		return Status;

	string Output = Root.toStyledString();
	File.Write((PVOID)Output.c_str(), Output.length());
	File.Close();

	export_res(Psb, FileName, BasePath);
	return Status;
}


