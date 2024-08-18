#ifndef RV32MASM_H_
#define RV32MASM_H_

#include<stdio.h>

#pragma once

#ifdef _DLL
#define MY_API extern "C" __declspec(dllexport) 
#else
#define MY_API extern "C"
#endif // _DLL

MY_API unsigned int make_1(const char* comm, int id_table_iptr);

MY_API int make_file(const char* src, unsigned cf, const char* dst_bin, const char* dst_read,
	FILE* out_log = stdout , const int id_table = -1);

//never call it!
void _init();

//使用以下标志组合产生编译标志位

#define OUT_BIN_EXEC 1
#define OUT_LITTLE_ENDIAN 2
#define OUT_READBALE_FILE 4
#define USE_ID_TABLE_DEFAULT -1

MY_API int fakemain(int argc, char** argv);

#endif // !RV32MASM_H_
