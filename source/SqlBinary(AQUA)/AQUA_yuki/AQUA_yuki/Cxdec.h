#pragma once

#include <Windows.h>

struct CxdecXcodeStatus
{
	PBYTE Start;
	PBYTE Cur;
	DWORD SpaceSize;
	DWORD Seed;
	int(*XcodeBuilding)(struct CxdecXcodeStatus*, int);
};

struct cxdec_callback 
{
	const char *name;
	DWORD key[2];
	int(*XcodeBuilding)(struct CxdecXcodeStatus*, int);
};

DWORD xcode_rand(struct CxdecXcodeStatus *xcode);
int push_bytexcode(struct CxdecXcodeStatus *xcode, BYTE code);
int push_2bytesxcode(struct CxdecXcodeStatus *xcode,
	BYTE code0, BYTE code1);

int push_3bytesxcode(struct CxdecXcodeStatus *xcode,
	BYTE code0, BYTE code1, BYTE code2);

int push_4bytesxcode(struct CxdecXcodeStatus *xcode,
	BYTE code0, BYTE code1, BYTE code2, BYTE code3);
int push_5bytesxcode(struct CxdecXcodeStatus *xcode,
	BYTE code0, BYTE code1, BYTE code2, BYTE code3, BYTE code4);
int push_6bytesxcode(struct CxdecXcodeStatus *xcode,
	BYTE code0, BYTE code1, BYTE code2, BYTE code3, BYTE code4, BYTE code5);
int push_dwordxcode(struct CxdecXcodeStatus *xcode, DWORD code);


//////////////////////////////////////////////
//Global
void cxdec_decode(struct cxdec_callback *callback, DWORD hash, DWORD offset, BYTE *buf, DWORD len);

/////////////////////////////////////////////////////
//Global

int xcode_building_stage0(struct CxdecXcodeStatus *xcode, int stage);
int xcode_building_stage1(struct CxdecXcodeStatus *xcode, int stage);

/////////////////////////////////////////////////////
