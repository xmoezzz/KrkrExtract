#include <Windows.h>
#include <stdio.h>
#include "sqlite3.h"
#include <string>
#include <vector>
#include <map>

#define InfoData  L"scene.sdb"
#define SceneData L"scenedata.sdb"

#define NamePrefix       "lpNameTalk"
#define NameTextPrefix   "lpNameText"
#define SimpleTextPrefix "SimpleText"


/*
SceneInfo: 
1.Name Table
id(pk) int, name varchar
2.Storage Table
id(pk) int, name varchar
3.Scene Table
id(pk) int, storage_id int, title varchar

SceneData: scene(pk) int, name int, text varchar
*/


#define SqlGetSceneTitle "select title from scene;"
#define SqlGetSceneIdStoragePair "select id, storage from scene;"
#define SqlGetSceneName "select id from name;"

using std::vector;
using std::string;
using std::map;

vector<string> SceneName; //单独提取
map<int, string> StorageInfo;
map<int, string> NameInfo;
map<int, int>SceneToStorage;//Storage可能有重复的

typedef struct TextInfo
{
	TextInfo() : TalkName(-1){}

	TextInfo& operator = (const TextInfo& rhs)
	{
		TalkName = rhs.TalkName;
		TextName = rhs.TextName;

		return *this;
	}
	int TalkName;
	string TextName;
}TextInfo;

//一个章节
typedef struct SceneText
{
	SceneText() : SceneName(-1){}

	SceneText& operator = (const SceneText& rhs)
	{
		Res = rhs.Res;
		SceneName = rhs.SceneName;
		return *this;
	}
	vector<TextInfo> Res;
	int SceneName; //对应的scene
}SceneText;

vector<SceneText> AllScenes;

void StartDump()
{
	FILE* fout = _wfopen(L"title.txt", L"wb");
	for (ULONG i = 0; i < SceneName.size(); i++)
	{
		fprintf(fout, "[0x%08x]%s\r\n", i, SceneName[i].c_str());
		fprintf(fout, ";[0x%08x]\r\n\r\n", i);
	}
	fclose(fout);

	for (ULONG i = 0; i < AllScenes.size(); i++)
	{
		if (AllScenes[i].SceneName == -1)
			continue;

		auto it = SceneToStorage.find(AllScenes[i].SceneName);
		if (it == SceneToStorage.end())
		{
			printf("Scene id : %d\n", AllScenes[i].SceneName);
			MessageBoxW(NULL, L"找不到Scene id对应的Storage Id", NULL, MB_OK);
			continue;
		}

		auto it2 = StorageInfo.find(it->second);
		if (it2 == StorageInfo.end())
		{
			MessageBoxW(NULL, L"找不到文件名", NULL, MB_OK);
			continue;
		}

		WCHAR Info[400] = { 0 };
		MultiByteToWideChar(CP_UTF8, 0, it2->second.c_str(),
			it2->second.length(), Info, 400);
		wprintf(L"Writing: %s\n", Info);

		lstrcatW(Info, L".txt");
		FILE* f = _wfopen(Info, L"wb");
		if (f == nullptr)
		{
			MessageBoxW(nullptr, Info, L"Couldn't write", MB_OK);
			continue;
		}
		for (ULONG j = 0; j < AllScenes[i].Res.size(); j++)
		{
			if (AllScenes[i].Res[j].TalkName != -1 && AllScenes[i].Res[j].TalkName != 0)
			{
				auto itr = NameInfo.find(AllScenes[i].Res[j].TalkName);
				fprintf(f, "[%s]%s\r\n", NamePrefix, itr->second.c_str());
				fprintf(f, ";[%s]%s\r\n\r\n", NamePrefix, itr->second.c_str());

				fprintf(f, "[%s]%s\r\n", NameTextPrefix, AllScenes[i].Res[j].TextName.c_str());
				fprintf(f, ";[%s]\r\n\r\n", NameTextPrefix);
			}
			else
			{
				fprintf(f, "[%s]<Xmoe::value::null>\r\n", NamePrefix);
				fprintf(f, ";[%s]<Xmoe::value::null>\r\n\r\n", NamePrefix);

				fprintf(f, "[%s]%s\r\n", SimpleTextPrefix, AllScenes[i].Res[j].TextName.c_str());
				fprintf(f, ";[%s]\r\n\r\n", SimpleTextPrefix);
			}
		}
		fclose(f);
	}
}


int TitleCallBack(void *data, int param, char **value, char **name)
{
	char* Str = new char[lstrlenA(value[0]) + 1];
	lstrcpynA(Str, value[0], lstrlenA(value[0]) + 1);
	string str(Str);
	delete[] Str;
	SceneName.push_back(str);

#if 0
	WCHAR Info[400] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, value[0], lstrlenA(value[0]), Info, 400);
	wprintf(L"%s\n", Info);
#endif
	return 0;
}

int StorageCallBack(void *data, int param, char **value, char **name)
{
	char* Str = new char[lstrlenA(value[1]) + 1];
	lstrcpynA(Str, value[1], lstrlenA(value[1]) + 1);
	string str(Str);
	delete[] Str;

	int num = atoi(value[0]);

	WCHAR Info[400] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, value[1], lstrlenA(value[1]), Info, 400);
	wprintf(L"Find : %s\n", Info);

	StorageInfo.insert(std::make_pair(num, str));
	return 0;
}


int NameCallBack(void *data, int param, char **value, char **name)
{
	char* Str = new char[lstrlenA(value[1]) + 1];
	lstrcpynA(Str, value[1], lstrlenA(value[1]) + 1);
	string str(Str);
	delete[] Str;

	int num = atoi(value[0]);

	NameInfo.insert(std::make_pair(num, str));
	return 0;
}


int PairCallBack(void *data, int param, char **value, char **name)
{
	int SceneId = atoi(value[0]);
	int StorageId = atoi(value[1]);

	SceneToStorage.insert(std::make_pair(SceneId, StorageId));
	return 0;
}

//sceneid, name, text
int MainCallBack(void *data, int param, char **value, char **name)
{
	char* TextStr = new char[lstrlenA(value[2]) + 1];
	lstrcpynA(TextStr, value[2], lstrlenA(value[2]) + 1);
	string Textstr(TextStr);
	delete[] TextStr;

	int sceneId = atoi(value[0]);
	int nameId = atoi(value[1]);

	while(AllScenes.size() <= sceneId)
	{
		SceneText atom;
		AllScenes.push_back(atom);
	}
	
	AllScenes[sceneId].SceneName = sceneId;
	TextInfo text;
	text.TalkName = nameId;
	text.TextName = Textstr;

	//printf("%s\n", TextStr);
	try
	{
		AllScenes[sceneId].Res.push_back(text);
	}
	catch (std::exception& e)
	{
		printf("%s\n", e.what());
		getchar();
		ExitProcess(0);
	}
	return 0;
}

int wmain(int argc, WCHAR* argv[])
{
	char    *strError = nullptr;
	char    *sqlStr = nullptr;
	int  Result;

	setlocale(LC_ALL, "chs");

	sqlite3* hInfoData = nullptr;
	sqlite3* hSceneData = nullptr;

	sqlite3_open16(InfoData, &hInfoData);
	sqlite3_open16(SceneData, &hSceneData);

	if (!hSceneData || !hInfoData)
	{
		MessageBoxW(NULL, L"Couldn't open database files", NULL, MB_OK);
		return 0;
	}

	sqlStr = "select title from scene;";
	Result = sqlite3_exec(hInfoData, sqlStr, TitleCallBack, &SceneName, &strError);
	if (Result != SQLITE_OK) 
	{
		if (strError != nullptr) 
		{
			printf(" SQL error : %s\n ", strError);
			getchar();
			sqlite3_free(strError);
			sqlite3_close(hSceneData);
			sqlite3_close(hInfoData);
			ExitProcess(0);
		}
	}

	sqlStr = "select id, name from storage;";
	Result = sqlite3_exec(hInfoData, sqlStr, StorageCallBack, nullptr, &strError);
	if (Result != SQLITE_OK)
	{
		if (strError != nullptr)
		{
			printf(" SQL error : %s\n ", strError);
			getchar();
			sqlite3_free(strError);
			sqlite3_close(hSceneData);
			sqlite3_close(hInfoData);
			ExitProcess(0);
		}
	}

	sqlStr = "select id, name from name;";
	Result = sqlite3_exec(hInfoData, sqlStr, NameCallBack, nullptr, &strError);
	if (Result != SQLITE_OK)
	{
		if (strError != nullptr)
		{
			printf(" SQL error : %s\n ", strError);
			getchar();
			sqlite3_free(strError);
			sqlite3_close(hSceneData);
			sqlite3_close(hInfoData);
			ExitProcess(0);
		}
	}



	sqlStr = "select id, storage from scene;";
	Result = sqlite3_exec(hInfoData, sqlStr, PairCallBack, nullptr, &strError);
	if (Result != SQLITE_OK)
	{
		if (strError != nullptr)
		{
			printf(" SQL error : %s\n ", strError);
			getchar();
			sqlite3_free(strError);
			sqlite3_close(hSceneData);
			sqlite3_close(hInfoData);
			ExitProcess(0);
		}
	}

	sqlStr = "select scene, name, text from text;";
	Result = sqlite3_exec(hSceneData, sqlStr, MainCallBack, nullptr, &strError);
	if (Result != SQLITE_OK)
	{
		if (strError != nullptr)
		{
			printf(" SQL error : %s\n ", strError);
			getchar();
			sqlite3_free(strError);
			sqlite3_close(hSceneData);
			sqlite3_close(hInfoData);
			ExitProcess(0);
		}
	}

	StartDump();

	sqlite3_close(hSceneData);
	sqlite3_close(hInfoData);
	return 0;
}

