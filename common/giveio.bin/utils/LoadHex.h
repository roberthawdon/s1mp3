#pragma once
#include <stdio.h>
#include <list>

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

  typedef unsigned __int8  uint8;
  typedef unsigned __int16 uint16;
  typedef unsigned __int32 uint32;

#endif


//-------------------------------------------------------------------------------------------------
#define for_each(_list_, _template_, _iterator_) for(\
  std::list<_template_>::iterator _iterator_ = (_list_).begin();\
  _iterator_ != (_list_).end();\
  _iterator_++\
)


//-------------------------------------------------------------------------------------------------
class LoadHexEntry
{
  public:LoadHexEntry(uint32 addr, uint8 db) {this->addr = addr; this->db = db;}
  public:uint32 addr;
  public:uint8 db;

  public:bool operator< (LoadHexEntry &o) {return(addr < o.addr);}
};


//-------------------------------------------------------------------------------------------------
class LoadHex
{
  public:enum RESULT
  {
    SUCCESS,          //no error
    ERROR_UNKNOWN,    //unknown error
    ERROR_ALLOC,      //failed to allocate memory
    ERROR_FOPEN,      //failed to open file
    ERROR_UEOF,       //unexpected end of file
    ERROR_SYNTAX,     //illegal syntax
    ERROR_CHKSUM      //illegal checksum
  };


  public:LoadHex() {g_uLine = 0; g_result = SUCCESS;}


  public:RESULT load(const char *lpszFilename)
  {
    g_result = ERROR_UNKNOWN;
    g_uLine = 0;
    g_lstData.empty();

    uint8 *lpRecData = NULL;
    FILE *fh = fopen(lpszFilename, "rt");
    if(!fh) return ERROR_FOPEN;
    const uint32 dwAddress = 0;

    g_uLine++;
    try {
      while(1)
      {
        switch(fgetc(fh))
        {
        case '\n': g_uLine++; case '\r': continue;
        case ':': break;
        case EOF: throw ERROR_UEOF;
        default: throw ERROR_SYNTAX;
        }

        uint8 bRecLength = load_hex_byte(fh);
        uint16 wRecAddr = load_hex_word(fh);
        uint8 bRecType = load_hex_byte(fh);

        if(lpRecData) delete lpRecData;
        lpRecData = new(std::nothrow) uint8[bRecLength];
        if(!lpRecData) throw ERROR_ALLOC;

        uint8 bChkSum = bRecLength+(wRecAddr&255)+(wRecAddr>>8)+bRecType;
        for(uint8 b = 0; b < bRecLength; b++)
        {
          lpRecData[b] = load_hex_byte(fh);
          bChkSum += lpRecData[b];
        }
        bChkSum = -bChkSum;
        if(load_hex_byte(fh) != bChkSum) throw ERROR_CHKSUM;
        switch(fgetc(fh))
        {
          case '\n': g_uLine++; case '\r': break;
          default: throw ERROR_SYNTAX;
        }

        switch(bRecType)
        {
          case 0: for(uint8 b = 0; b < bRecLength; b++)  //data
            {
              LoadHexEntry lie( dwAddress + wRecAddr + b, lpRecData[b] );
              g_lstData.push_back( lie );
            }
            break;
          case 1: throw SUCCESS; //end of file
          //case 2: dwAddress = (((uint32)lpRecData[0]) << 24) | (((uint32)lpRecData[1]) << 16); break;  //ext. segment address
          //case 4: dwAddress = (((uint32)lpRecData[0]) << 12) | (((uint32)lpRecData[1]) <<  4); break;  //ext. linear address
          default: throw ERROR_SYNTAX;
        }
      }
    }
    catch(RESULT r) {g_result = r;}
    catch(...) {}

    if(lpRecData) delete lpRecData;
    if(fh) fclose(fh);
    g_lstData.sort();
    return g_result;
  }


  public:RESULT g_result;
  public:unsigned int g_uLine;
  public:std::list<LoadHexEntry> g_lstData;


  private:uint8 load_hex_halfbyte(FILE *fh)
  {
    switch(fgetc(fh))
    {
    case EOF: throw ERROR_UEOF;
    case '0': return 0; case '1': return 1; case '2': return 2; case '3': return 3; 
    case '4': return 4; case '5': return 5; case '6': return 6; case '7': return 7;
    case '8': return 8; case '9': return 9; case 'A': return 10; case 'B': return 11;
    case 'C': return 12; case 'D': return 13; case 'E': return 14; case 'F': return 15;
    default: throw ERROR_SYNTAX;
    }
  }

  private:uint8 load_hex_byte(FILE *fh)
  {
    uint8 h = load_hex_halfbyte(fh);
    uint8 l = load_hex_halfbyte(fh);
    return((h << 4) | l);
  }

  private:uint16 load_hex_word(FILE *fh)
  {
    uint16 h = load_hex_byte(fh);
    uint16 l = load_hex_byte(fh);
    return((h << 8) | l);
  }
};
