#include <stdio.h>
#include <stdlib.h>

#ifdef LINUX
  #include <sys/types.h>
  #include <unistd.h>

  #include <string.h>
  #include <stdint.h>
  #include <errno.h>

  
  typedef uint64_t uint64;
  typedef uint32_t uint32;
  typedef uint16_t uint16;
  typedef uint8_t uint8;
  typedef int64_t int64;
  typedef int32_t int32;
  typedef int16_t int16;
  typedef int8_t int8;

  #define DWORD uint32
  #define BYTE uint8
  #define ERROR_NOT_ENOUGH_MEMORY ENOMEM
  #define ERROR_READ_FAULT EIO
  #define ERROR_SUCCESS 0
  #define ERROR_INVALID_DATA EIO
  #define ERROR_WRITE_FAULT EIO
  #define ERROR_BAD_FORMAT EINVAL
  #define ERROR_BAD_COMMAND EBADMSG
  #define LPSTR char *
  #define LPCSTR const char *
  #define LPTSTR char *
  #define LPVOID void *



#else

  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

#include "LoadHex.h"


int main(int argc, char *argv[])
{
  if(argc != 3)
  {
    printf("convert intel hex file into binary file - copyright (c)2006 wiRe@gmx.net\n");
    printf("usage: ihx2bin output.bin input.ihx\n");
    return -1;
  }

  LoadHex hex;
  switch(hex.load(argv[2]))
  {
  case LoadHex::SUCCESS: break;
  case LoadHex::ERROR_ALLOC: printf("error: failed to alloc memory\n"); return -1;
  case LoadHex::ERROR_FOPEN: printf("error: failed to open file \"%s\"\n", argv[2]); return -1;
  case LoadHex::ERROR_UEOF: printf("error: %s(%u): unexpected end of file\n", argv[2], hex.g_uLine); return -1;
  case LoadHex::ERROR_SYNTAX: printf("error: %s(%u): illegal syntax\n", argv[2], hex.g_uLine); return -1;
  case LoadHex::ERROR_CHKSUM: printf("error: %s(%u): illegal checksum\n", argv[2], hex.g_uLine); return -1;
  default: printf("error: %s(%u): unknown error\n", argv[2], hex.g_uLine); return -1;
  }
  FILE *fo = fopen(argv[1], "wb");
  if(!fo)
  {
    printf("error: failed to create file \"%s\"\n", argv[1]);
    return -1;
  }
 //write code
  if(!hex.g_lstData.empty())
  {
    uint32 addr = hex.g_lstData.front().addr;
    for_each(hex.g_lstData, LoadHexEntry, i)
    {
      for(; addr < i->addr; addr++) fputc(0, fo);
      fputc(i->db, fo);
      addr++;
    }
  }

  if(ferror(fo))
  {
    fclose(fo);
    printf("error: failed to write to file \"%s\"\n", argv[1]);
    return -1;
  }
  fclose(fo);
  return 0;
}
