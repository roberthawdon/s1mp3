#pragma once
#include "../common/common.h"
#include "../common/s1def.h"
#include <list>

#define CLONEIMAGE_FILE_HEADER_SIGNATURE 0x00656e6f6c633173

#define CLONEIMAGE_ERROR_FILE_ACCESS              "file access error"
#define CLONEIMAGE_ERROR_INVALID_FILE_SIGNATURE   "invalid file signature"
#define CLONEIMAGE_ERROR_INVALID_PAGE_SIZE        "invalid page size"
#define CLONEIMAGE_ERROR_SECTION_BUFFER_OVERFLOW  "section buffer overflow"
#define CLONEIMAGE_ERROR_INVALID_FILE_SECTION     "invalid file section"

class CloneImage
{
  private:

  enum
  {
    PAGE_SIZE = 0x800,
    RAW_PAGE_SIZE = 0x840,
    SIZE_OF_BROM = 16*1024,
  };

  typedef struct {
    uint64 signature;         //"s1clone",0
    uint32 flash_id;          //flash chip device id
    uint32 flash_size;        //flash size in pages
    uint32 block_size;        //block size in pages
    uint32 page_size;         //size of one page (should be FlashIO::REAL_PAGE_SIZE)
    uint8 brom[SIZE_OF_BROM]; //the brom image
  } FILE_HEADER;

  typedef struct {
    uint16 chksum;        //checksum of the decoded section
    uint16 size;          //size of encoded section
  } FILE_SECTION;

  HANDLE hFile;
  FILE_HEADER FileHdr;
  std::list<DWORD> lstSectionOfs;


  //-------------------------------------------------------------------------------------------------
  VOID seekToSection(DWORD dwSection)
  {
    if(lstSectionOfs.size() <= 0)
      lstSectionOfs.push_back( sizeof(FileHdr) ); //auto-add offset of first section

    if(lstSectionOfs.size() > dwSection)  //was section already accessed?
    {
      std::list<DWORD>::iterator i = lstSectionOfs.begin();
      while(dwSection > 0) {dwSection--; i++;}
      ::SetFilePointer(hFile, *i, NULL, FILE_BEGIN);
    }
    else  //otherwise seek through file until we have found the new section
    {
      ::SetFilePointer(hFile, lstSectionOfs.back(), NULL, FILE_BEGIN);
      for(dwSection -= (DWORD)lstSectionOfs.size()-1; dwSection > 0; dwSection--)
      {
        FILE_SECTION FileSec;
        fread(hFile, &FileSec, sizeof(FileSec));
        lstSectionOfs.push_back( ::SetFilePointer(hFile, FileSec.size, NULL, FILE_CURRENT) );
      }
    }
  }


  //-------------------------------------------------------------------------------------------------
  VOID fread(HANDLE hFile, LPVOID lpBuf, DWORD dwSize)
  {
    DWORD dwRead = 0;
    if(!::ReadFile(hFile, lpBuf, dwSize, &dwRead, NULL) || dwRead != dwSize)
      throw "failed to read from file";
  }


  //-------------------------------------------------------------------------------------------------
  uint16 calcChksum(LPBYTE lpBuf, DWORD dwSize)
  {
    uint16 cs = 0;
    while(dwSize--) cs = ((cs&1)?0x8000:0) + (cs>>1) + *lpBuf++;
    return cs;
  }


  //-------------------------------------------------------------------------------------------------
  uint32 decodePage(LPBYTE lpDest, DWORD dwDestSize, LPBYTE lpSrc, DWORD dwSrcSize)
  {
    DWORD dwSize = 0;
    WORD nLastByte = -1;
    while(dwSrcSize--)
    {
      WORD nByte = *lpSrc++;
      if(dwDestSize) {*lpDest++ = (BYTE)nByte; dwDestSize--;}
      dwSize++;

      if(nLastByte == nByte)
      {
        DWORD dwRep = 0;
        for(DWORD dwShift = 0; dwSrcSize; dwShift+=7)
        {
          dwRep |= ((DWORD)*lpSrc & 0x7F) << dwShift;
          dwSrcSize--;
          if((*lpSrc++ & 0x80) == 0) break;
        }
        while(dwRep--)
        {
          if(dwDestSize) {*lpDest++ = (BYTE)nByte; dwDestSize--;}
          dwSize++;
        }
        nByte = -1;
      }

      nLastByte = nByte;
    }
    return dwSize;
  }


  public:

  //-------------------------------------------------------------------------------------------------
  CloneImage()
  {
    hFile = INVALID_HANDLE_VALUE;
  }

  ~CloneImage()
  {
    close();
  }
  
  void open(LPCSTR lpcFile)
  {
    close();

    hFile = ::CreateFileA(lpcFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE) throw CLONEIMAGE_ERROR_FILE_ACCESS;

    fread(hFile, &FileHdr, sizeof(FileHdr));
    if(FileHdr.signature != CLONEIMAGE_FILE_HEADER_SIGNATURE) throw CLONEIMAGE_ERROR_INVALID_FILE_SIGNATURE;
    if(FileHdr.page_size != RAW_PAGE_SIZE) throw CLONEIMAGE_ERROR_INVALID_PAGE_SIZE;
  }

  void close()
  {
    if(hFile != INVALID_HANDLE_VALUE)
    {
      ::CloseHandle(hFile);
      hFile = INVALID_HANDLE_VALUE;
    }
    lstSectionOfs.clear();
  }

  void read(DWORD dwPage, LPBYTE lpBuf, DWORD dwSize = RAW_PAGE_SIZE)
  {
    seekToSection(dwPage);

    BYTE bBuffer[4224];
    FILE_SECTION FileSec;
    fread(hFile, &FileSec, sizeof(FileSec));
    if(FileSec.size > sizeof(bBuffer)) throw CLONEIMAGE_ERROR_SECTION_BUFFER_OVERFLOW;
    fread(hFile, bBuffer, FileSec.size);

    BYTE bPage[RAW_PAGE_SIZE];
    if(decodePage(bPage, sizeof(bPage), bBuffer, FileSec.size) != sizeof(bPage)) throw CLONEIMAGE_ERROR_INVALID_FILE_SECTION;
    if(calcChksum(bPage, sizeof(bPage)) != FileSec.chksum) throw CLONEIMAGE_ERROR_INVALID_FILE_SECTION;

    if(dwSize > sizeof(bPage)) dwSize = sizeof(bPage);
    if(lpBuf && dwSize) memcpy(lpBuf, bPage, dwSize);
  }

  DWORD getBlockSize()
  {
    //return FileHdr.block_size;
    return (FileHdr.block_size == 0)? 32 : FileHdr.block_size;
  }

  DWORD getBlocks()
  {
    return (FileHdr.flash_size == 0)? -1 : (FileHdr.flash_size+getBlockSize()-1) / getBlockSize();
  }

  DWORD getFlashSize()
  {
    return (FileHdr.flash_size == 0)? -1 : FileHdr.flash_size;
  }

};