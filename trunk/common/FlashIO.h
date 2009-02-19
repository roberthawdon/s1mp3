//-------------------------------------------------------------------------------------------------
// flashio: support for different nand flash chips used by s1mp3 players
//          to access the chips, giveio is needed to run on the target player
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//-------------------------------------------------------------------------------------------------
#pragma once
#include "GiveIO.h"


class IFlashIO
{
public:
  //-----------------------------------------------------------------------------------------------
  enum {
    PAGE_SIZE       = 0x800, //2k (data)
    SPARE_AREA_SIZE = 0x040, //            64 bytes (spare area)
    REAL_PAGE_SIZE  = 0x840, //2k (data) + 64 bytes (spare area)
  };

  //-----------------------------------------------------------------------------------------------
  virtual bool erase_block(uint32 page) = 0;

  virtual bool read_page(uint32 page, uint16 offs, void *ptr, uint32 size) = 0;
  virtual bool write_page(uint32 page, uint16 offs, void *ptr, uint32 size) = 0;

  virtual bool is_protected() = 0;      //is device protected?

  virtual const char *get_device_descr() = 0;

  virtual uint32 get_device_id() = 0;   //return device id of flash chip
  virtual uint32 get_size() = 0;        //return size in MB
  virtual uint32 get_pages() = 0;       //return number of pages
  virtual uint32 get_blocks() = 0;      //return number of blocks
  virtual uint32 get_block_size() = 0;  //return number of pages per block

  //-----------------------------------------------------------------------------------------------
  // test flash page against ecc value inside the spare area
  // NOTE: ptr must point to a full flash page including the spare area
  //-----------------------------------------------------------------------------------------------
  bool validateECC(const void *ptr) 
  { 
    uint32 *ecc = (uint32*)&((uint8*)ptr)[0x800];
    if(ecc[5] != -1 || ecc[6] != -1 || ecc[7] != -1 || ecc[8] != -1 || ecc[9] != -1 || ecc[10] != -1
      || ecc[11] != -1 || ecc[12] != -1 || ecc[13] != -1 || ecc[14] != -1 || ecc[15] != -1) return true;

    if(ecc[1] == -1 || ecc[2] == -1 || ecc[3] == -1 || ecc[4] == -1) return true;

    return
      (ecc[1] == calcECC(&((uint8*)ptr)[0x000])) &&
      (ecc[2] == calcECC(&((uint8*)ptr)[0x200])) &&
      (ecc[3] == calcECC(&((uint8*)ptr)[0x400])) &&
      (ecc[4] == calcECC(&((uint8*)ptr)[0x600])) ;
  }

  //-----------------------------------------------------------------------------------------------
  // calculate hamming code of a 512 byte data block
  //-----------------------------------------------------------------------------------------------
  uint32 calcECC(const void *ptr)
  {
    LPBYTE pData = (LPBYTE)ptr;
    WORD wDataSize = 512;

    WORD wEvenECC = 0;
    WORD wOddECC = 0;

    WORD wEvenOffs = (wDataSize << 3) - 1;
    WORD wOddOffs = 0;
    while(wDataSize--)
    {
      for(WORD b = *(pData++) | 0x100; b > 1; b>>=1, wEvenOffs--, wOddOffs++)
      {
        if(b&1)
        {
          wEvenECC ^= wEvenOffs;
          wOddECC ^= wOddOffs;
        }
      }
    }

    return 
      (((wOddECC     >>3)                      & 0xFF) << 0 ) |
      ((((wOddECC &7)<<5) | ((wOddECC>>11)&7))         << 8 ) |
      (((wEvenECC    >>3)                      & 0xFF) << 16) |
      ((((wEvenECC&7)<<5) | ((wEvenECC>>11)&7) | 0x10) << 24) ;
  }

  //-----------------------------------------------------------------------------------------------
  // continuously read from flash, skip spare area and access data only
  //-----------------------------------------------------------------------------------------------
  bool read(uint32 page, uint16 offs, void *ptr, uint32 size)
  {
    uint8 *bp = (uint8*)ptr;
    page += offs / PAGE_SIZE;
    offs = offs % PAGE_SIZE;
    while(size > 0)
    {
      uint32 step = PAGE_SIZE - offs;
      if(step > size) step = size;
      if(!read_page(page, offs, bp, step)) return false;
      page++;
      bp += step;
      size -= step;
      offs = 0;
    }

    return true;
  }

  //-----------------------------------------------------------------------------------------------
  // continuously write to flash, skip spare area and access data only
  //-----------------------------------------------------------------------------------------------
  bool write(uint32 page, uint16 offs, void *ptr, uint32 size)
  {
    uint8 *bp = (uint8*)ptr;
    page += offs / PAGE_SIZE;
    offs = offs % PAGE_SIZE;
    while(size > 0)
    {
      uint32 step = PAGE_SIZE - offs;
      if(step > size) step = size;
      if(!write_page(page, offs, bp, step)) return false;
      page++;
      bp += step;
      size -= step;
      offs = 0;
    }

    return true;
  }
};


class FlashIO : public IFlashIO
{
public:
  FlashIO(GiveIO &gio) {lpgio = &gio; cs_line = 0; id = 0; blk_size = 0; size = 0; prot = v9sm = false; device_descr_str[0] = 0;}

  bool open(uint8 cs_line);
  void close();

  bool erase_block(uint32 page);
  bool read_page(uint32 page, uint16 offs, void *ptr, uint32 size);
  bool write_page(uint32 page, uint16 offs, void *ptr, uint32 size);

  const char *get_device_descr();

  uint32 get_device_id() {return id;}
  bool is_protected() {return prot;}
  uint32 get_size() {return(size);}                   //return size in MB
  uint32 get_pages() {return(size*(1024*1024/2048));} //return number of pages
  uint32 get_blocks() {return(get_pages()/blk_size);} //return number of blocks
  uint32 get_block_size() {return(blk_size);}         //return number of pages per block

private:
  bool verify_device_id();


private:
  GiveIO *lpgio;
  uint8 cs_line;
  uint32 id;
  uint32 blk_size;  //in pages
  uint32 size;      //in MB
  bool prot;
  bool v9sm;        //do we have an extended state machine?

  char device_descr_str[64];
};
