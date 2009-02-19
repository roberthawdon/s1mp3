//-------------------------------------------------------------------------------------------------
// fwio: access s1mp3 players firmware
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//-------------------------------------------------------------------------------------------------
#pragma once
#include "s1def.h"
#include "FlashIO.h"


class FirmwareIO_CALLBACK
{
public:
  virtual void progress(unsigned char uPercent) = 0;
};

class FirmwareIO
{
public:
  FirmwareIO(AdfuSession &adfu) {lpadfu = &adfu; lpfio = NULL;}
  FirmwareIO(IFlashIO &fio) {lpadfu = NULL; lpfio = &fio;}
  //~FirmwareIO() {}

  void read(uint8* &brec, uint32 &brec_size, uint8* &fw, uint32 &fw_size,
    FirmwareIO_CALLBACK *cb = NULL);
  void write(uint8* brec, uint32 brec_size, uint8* fw, uint32 fw_size,
    FirmwareIO_CALLBACK *cb = NULL);

private:
  void read_adfu(uint8* &brec, uint32 &brec_size, uint8* &fw, uint32 &fw_size,
    FirmwareIO_CALLBACK *cb = NULL);
  void read_flash(uint8* &brec, uint32 &brec_size, uint8* &fw, uint32 &fw_size,
    FirmwareIO_CALLBACK *cb = NULL);

  void read_flash(uint16 blk, void *ptr, uint32 size, uint16 *lba_lut, uint32 lba_size, 
    bool interleaved, FirmwareIO_CALLBACK *cb = NULL);

  void write_adfu(uint8* brec, uint32 brec_size, uint8* fw, uint32 fw_size,
    FirmwareIO_CALLBACK *cb = NULL);
  void write_flash(uint8* brec, uint32 brec_size, uint8* fw, uint32 fw_size,
    FirmwareIO_CALLBACK *cb = NULL);

  bool is_brec_valid(const void *brec);
  uint32 detect_fw_size(const LP_FW_HEAD lpfwh);
  bool is_fw_valid(const void *fw);
  bool is_fw_pg(const void *pg);
  bool is_lut_pg(const void *pg);

  AdfuSession *lpadfu;
  IFlashIO *lpfio;
};
