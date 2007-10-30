#ifndef _INCLUDE_H_
#define _INCLUDE_H_


#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef __int8  int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;
typedef unsigned __int8  uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;


int display_error(int);
int extract_to_file(char *filename, FILE *fh_src, size_t fofs, uint32 fsize, uint32 *sum32p=NULL);


#endif