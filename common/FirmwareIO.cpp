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
#include "FirmwareIO.h"
#include "s1def.h"
#include "Sum16.h"
#include "Sum32.h"


//-------------------------------------------------------------------------------------------------
#ifdef _DEBUG_FIRMWARE_IO_
  #define _DEBUG_PRINTF_ printf
#else
  #define _DEBUG_PRINTF_ //
#endif


//-------------------------------------------------------------------------------------------------
#define FLASH_LUT_MASK 0x7FFF




//-------------------------------------------------------------------------------------------------
// read
//-------------------------------------------------------------------------------------------------
void FirmwareIO::read(uint8* &brec, uint32 &brec_size, uint8* &fw, uint32 &fw_size, 
                      FirmwareIO_CALLBACK *cb)
{
  brec = NULL;
  fw = NULL;

  if(lpfio != NULL) read_flash(brec, brec_size, fw, fw_size, cb);
  else if(lpadfu != NULL) read_adfu(brec, brec_size, fw, fw_size, cb);
  else throw AdfuException(ERROR_FUNCTION_FAILED);
}


//-------------------------------------------------------------------------------------------------
// write
//-------------------------------------------------------------------------------------------------
void FirmwareIO::write(uint8* brec, uint32 brec_size, uint8* fw, uint32 fw_size, 
                       FirmwareIO_CALLBACK *cb)
{
  // verify firmware image and header checksums
  //
  // NOTE: all new v9 devices need correct firmware header checksum's
  //       or they will stop booting instead!!
  //
  if(!fw || fw_size < sizeof(FW_HEAD) || !is_fw_valid(fw))
    throw AdfuException(ERROR_INVALID_DATATYPE);
  if(((LP_FW_HEAD)fw)->sum16 != Sum16().generate(fw, 0x1FE))
    throw AdfuException(ERROR_INVALID_DATA);
  if(((LP_FW_HEAD)fw)->sum32 != Sum32().generate(&fw[0x200], sizeof(FW_HEAD)-0x200))
    throw AdfuException(ERROR_INVALID_DATA);
  //for(int n = 0; n < FW_HEAD_ENTRIES; n++)
  //  if(((LP_FW_HEAD)fw)->entry[n].filename[0])


  // redirect firmware writing...
  if(lpfio != NULL) write_flash(brec, brec_size, fw, fw_size, cb);
  else if(lpadfu != NULL) write_adfu(brec, brec_size, fw, fw_size, cb);
  else throw AdfuException(ERROR_FUNCTION_FAILED);
}




//-------------------------------------------------------------------------------------------------
// read from adfu
//-------------------------------------------------------------------------------------------------
void FirmwareIO::read_adfu(uint8* &brec, uint32 &brec_size, uint8* &fw, uint32 &fw_size,
                           FirmwareIO_CALLBACK *cb)
{
  // init
  if(cb) cb->progress(0);
  //Sleep(500);
  //if(cb) cb->progress(5);

  //detect brec size
  brec_size = 0x4000;

  //read brec
  brec = new uint8[brec_size];
  memset(brec, 0, brec_size);
  //
  // unknown operation
  //uint32 uCmdUnkwn95[3] = {0x00950020, 0, 0};
  //lpadfu->write(uCmdUnkwn95, sizeof(uCmdUnkwn95));
  //
  // command: read/write brec
  // cdb[0]   command id (0x09)
  // cdb[1]   command flags (0x80 = read, 0x00 = write)
  // cdb[3:2] source/destination address
  // cdb[8:7] data length >> 9
  //
  uint32 uCmdRdBREC[3] = {0x8009, (brec_size>>9)<<24, 0};
  lpadfu->read(uCmdRdBREC, sizeof(uCmdRdBREC), brec, brec_size);
  //
  if(!is_brec_valid(brec)) throw AdfuException(ERROR_INVALID_DATA);   //brec valid?

  //Sleep(500);
  if(cb) cb->progress(10);

  // unknown operation
  //uint32 uCmdUnkwn90[3] = {0x00900020, 0, 0};
  //lpadfu->write(uCmdUnkwn90, sizeof(uCmdUnkwn90));

  //read the firmware header and detect the firmware size
  uint8 blk[ADFU_MAX_DATA_LENGTH];
  uint32 ofs = 0;
  //
  // command: read/write firmware
  // cdb[0]   command id (0x08)
  // cdb[1]   command flags (0x80 = read, 0x00 = write)
  // cdb[3:2] source/destination address >> 9
  // cdb[6]   ? (=2)
  // cdb[8:7] data length >> 9
  // cbd[9]   ? (=1)
  //
  uint32 uCmdRdFW[3] = {0x8008 + ((ofs >> 9)<<16), (sizeof(blk)>>9)<<24, 0};
  lpadfu->read(uCmdRdFW, sizeof(uCmdRdFW), blk, sizeof(blk));
  //
  if(!is_fw_valid(blk)) throw AdfuException(ERROR_INVALID_DATA);
  fw_size = detect_fw_size((LP_FW_HEAD)blk);

  //read the firmware
  fw = new uint8[fw_size];
  memset(fw, 0, fw_size);
  for(ofs = 0; ofs < fw_size; ofs += sizeof(blk))
  {
    if(cb) cb->progress(15 + (((ofs+1)*85)/(fw_size)));
    uCmdRdFW[0] = 0x8008 + ((ofs >> 9)<<16);
    lpadfu->read(uCmdRdFW, sizeof(uCmdRdFW), blk, sizeof(blk));
    ::CopyMemory(&fw[ofs], blk, ((fw_size - ofs) >= sizeof(blk))? sizeof(blk) : (fw_size - ofs));
  }

  // finished
  if(cb) cb->progress(100);
}


//-------------------------------------------------------------------------------------------------
// read from flash
//-------------------------------------------------------------------------------------------------
void FirmwareIO::read_flash(uint8* &brec, uint32 &brec_size, uint8* &fw, uint32 &fw_size,
                            FirmwareIO_CALLBACK *cb)
{
  //search brec  
  uint8 brec_buf[16];
  uint32 brec_blk = 0;
  while(true)
  {
    lpfio->read_page(brec_blk*lpfio->get_block_size(), 0, &brec_buf, sizeof(brec_buf));
    if(is_brec_valid(brec_buf)) break;
    if(++brec_blk >= 256)
    {
      _DEBUG_PRINTF_ ("brec not found\n");
      throw AdfuException(ERROR_UNKNOWN_REVISION);
    }
  }

  _DEBUG_PRINTF_ ("brec@%08X: %c%c%c%c\n", brec_blk*lpfio->get_block_size(), brec_buf[8], brec_buf[9], brec_buf[10], brec_buf[11]);


  //detect brec size (always constant 'till now)
  brec_size = 0x4000;

  //read brec
  brec = new uint8[brec_size];
  memset(brec, 0, brec_size);
  lpfio->read(brec_blk*lpfio->get_block_size(), 0, brec, brec_size);
  if(!is_brec_valid(brec)) throw AdfuException(ERROR_INVALID_DATA);   //brec still valid?


  //search lba-lut
  uint16 lut[0x840/2];
  for(uint32 lut_blk = 0; lut_blk < lpfio->get_blocks()*4; lut_blk++)
  {
    memset(lut, 0, sizeof(lut));
    uint32 lut_pg = lut_blk*lpfio->get_block_size()/4;
    if(lut_pg == 0x60) continue;
    lpfio->read_page((lut_blk <= 0)? 0x60 : lut_pg, 0, lut, sizeof(lut));
    if(!is_lut_pg(lut)) continue;   //no valid lut? -> try with next possible location

    _DEBUG_PRINTF_ ("lut@%08X\n", lut_pg);
    _DEBUG_PRINTF_ ("  fw@blk%04X\n", lut[0]);

    //detect lba block size (could be different from the flash block size)
    for(uint32 lba_size = 0x10/*lpfio->get_block_size()*/; lba_size <= 0x800; lba_size*=2)
    {
      uint32 fw_pg_ofs = (lut[0]&FLASH_LUT_MASK)*lba_size;
      if(fw_pg_ofs >= lpfio->get_pages()) break;    //exceeded flash?

      _DEBUG_PRINTF_ ("    lba_sz=%04X\n", lba_size);

      uint8 fw_pg[0x840];
      memset(fw_pg, 0, sizeof(fw_pg));
      lpfio->read_page(fw_pg_ofs, 0, fw_pg, sizeof(fw_pg));
      if(is_fw_pg(fw_pg)) //found the first firmware page?
      {
        _DEBUG_PRINTF_ ("  fw@%08X\n", fw_pg_ofs);

        //detect if we have an interleaved firmware or not
        FW_HEAD fwh;
        bool interleaved = false;
        //first try to read the firmware header linear
        read_flash(0, &fwh, sizeof(fwh), lut, lba_size, false);
        if(!is_fw_valid(&fwh))
        {
          //try to read an interleaved firmware in a second approach
          read_flash(0, &fwh, sizeof(fwh), lut, lba_size, interleaved = true);
          if(!is_fw_valid(&fwh)) continue;  //try another lba block size
        }

        //detect firmware size
        fw_size = detect_fw_size(&fwh);

        _DEBUG_PRINTF_ ("  fw@%08X: v%u.%u.%02X, fw_sz=%08X, interleaved=%s\n", fw_pg_ofs, fwh.version_id[0]>>4, fwh.version_id[0]&15, fwh.version_id[1], fw_size, interleaved? "yes":"no");

        //read the firmware
        fw = new uint8[fw_size];
        memset(fw, 0, fw_size);
        read_flash(0, fw, fw_size, lut, lba_size, interleaved, cb);

        return; //success!
      }
    }
    _DEBUG_PRINTF_ ("  fw not found\n");
  }

  throw AdfuException(ERROR_NOT_FOUND);
}




//-------------------------------------------------------------------------------------------------
// read firmware from flash
//-------------------------------------------------------------------------------------------------
void FirmwareIO::read_flash(uint16 blk, void *ptr, uint32 size, 
                                 uint16 *lut, uint32 lba_size, bool interleaved,
                                 FirmwareIO_CALLBACK *cb)
{
  uint32 pg = 0;
  uint8 *lpd = (uint8*)ptr;
  unsigned long percent = 100*size;
  uint32 total = size;
  while(size > 0)
  {
    if(cb) cb->progress(((total-size)*100)/total);
    _DEBUG_PRINTF_ ("\r  progress: %i%%", ((total-size)*100)/total);

    uint32 unit = lpfio->PAGE_SIZE;
    if(unit > size) unit = size;
    uint32 pos = interleaved?
        ((lut[blk]&FLASH_LUT_MASK)*lba_size + (pg>>1) + (pg&1)*(lba_size>>1))
      : ((lut[blk]&FLASH_LUT_MASK)*lba_size + pg);

    lpfio->read_page(pos, 0, lpd, unit);
    lpd += unit;
    size -= unit;

    pg++;
    if(pg >= lba_size)
    {
      pg = 0;
      blk++;
    }
  }

  if(cb) cb->progress(100);
  _DEBUG_PRINTF_ ("\r  progress: 100%%\n");
}




//-------------------------------------------------------------------------------------------------
// write to adfu
//-------------------------------------------------------------------------------------------------
void FirmwareIO::write_adfu(uint8* brec, uint32 brec_size, uint8* fw, uint32 fw_size,
                            FirmwareIO_CALLBACK *cb)
{
  // init
  if(cb) cb->progress(0);
  //Sleep(500);
  //if(cb) cb->progress(5);


  // write brec
  if(brec)
  {
    if(brec_size < 16 || !is_brec_valid(brec)) throw AdfuException(ERROR_INVALID_DATATYPE);

    // unknown operation
    uint32 uCmdUnkwn95[3] = {0x00950020, 0, 0x100};
    lpadfu->write(uCmdUnkwn95, sizeof(uCmdUnkwn95));

    //
    // command: read/write brec
    // cdb[0]   command id (0x09)
    // cdb[1]   command flags (0x80 = read, 0x00 = write)
    // cdb[3:2] source/destination address
    // cdb[8:7] data length >> 9
    //
    uint32 uCmdWrBREC[3] = {0x09, (brec_size>>9)<<24, 0};
    lpadfu->write(uCmdWrBREC, sizeof(uCmdWrBREC), brec, brec_size);

    //Sleep(500);
    if(cb) cb->progress(10);
  }


  // write firmware

  // unknown operation
  uint32 uCmdUnkwn96[3] = {0x00960020, 0, 0};
  lpadfu->write(uCmdUnkwn96, sizeof(uCmdUnkwn96));

  //
  // command: read/write 2-byte firmware version (eg. "0x90 0x50" for fw ver 9.0.5.0)
  // cdb[0]   command id (0x05)
  // cdb[1]   command flags (0x80 = read, 0x00 = write)
  // cdb[3:2] 0x084E
  // cdb[6]   ? (=8)
  // cdb[8:7] data length in bytes (=2)
  //
  uint32 uCmdWrFwVer[3] = {0x05 + (0x084E<<16), 0x02080000, 0};
  lpadfu->write(uCmdWrFwVer, sizeof(uCmdWrFwVer), &((LP_FW_HEAD)fw)->version_id, 2);

  // unknown operation
  uint32 uCmdUnkwn90[3] = {0xF3900020, 0x00000202, 0x00E58000};
  lpadfu->write(uCmdUnkwn90, sizeof(uCmdUnkwn90));

  // write firmware blocks
  uint8 blk[ADFU_MAX_DATA_LENGTH];
  for(uint32 ofs = 0; ofs < fw_size; ofs += sizeof(blk))
  {
    uint32 blk_size = fw_size - ofs;
    if(blk_size >= sizeof(blk)) blk_size = sizeof(blk);

    if(cb) cb->progress(15 + (85*ofs)/fw_size);

    ::ZeroMemory(blk, sizeof(blk));
    ::CopyMemory(blk, &fw[ofs], blk_size);

    //
    // command: read/write firmware
    // cdb[0]   command id (0x08)
    // cdb[1]   command flags (0x80 = read, 0x00 = write)
    // cdb[3:2] source/destination address >> 9
    // cdb[6]   ? (=2)
    // cdb[8:7] data length >> 9
    // cbd[9]   ? (=1)
    //
    uint32 uCmdWrFW[3] = {0x08 + ((ofs >> 9)<<16), ((blk_size+511)>>9)<<24, 0};
    lpadfu->write(uCmdWrFW, sizeof(uCmdWrFW), blk, ((blk_size+511)>>9)<<9);
  }

  // unknown operation
  uint32 uCmdUnkwn91[3] = {0x00910020, 0, 0};
  lpadfu->write(uCmdUnkwn91, sizeof(uCmdUnkwn91));


  // finished
  if(cb) cb->progress(100);
}


//-------------------------------------------------------------------------------------------------
// write to flash
//-------------------------------------------------------------------------------------------------
void FirmwareIO::write_flash(uint8* brec, uint32 brec_size, uint8* fw, uint32 fw_size,
                             FirmwareIO_CALLBACK *cb)
{
  throw AdfuException(ERROR_FUNCTION_FAILED);
}




//-------------------------------------------------------------------------------------------------
// detect if the brec header is valid
//-------------------------------------------------------------------------------------------------
bool FirmwareIO::is_brec_valid(const void *brec)
{
  return memcmp(((char*)brec)+4, "BREC", 4) == 0;
}


//-------------------------------------------------------------------------------------------------
// detect firmware size in bytes
//-------------------------------------------------------------------------------------------------
uint32 FirmwareIO::detect_fw_size(const LP_FW_HEAD lpfwh)
{
  uint32 size = sizeof(FW_HEAD);

  if(is_fw_valid(lpfwh)) for(uint32 i=0; i<FW_HEAD_ENTRIES; i++) if(lpfwh->entry[i].filename[0])
  {
    uint32 _max = (((uint32)lpfwh->entry[i].fofs_s9)<<9) + (uint32)lpfwh->entry[i].fsize;
    if(_max < (16*1024*1024) && _max > size) size = _max;
  }

  return ((size+0x1ff) >> 9) << 9;
}


//-------------------------------------------------------------------------------------------------
// detect if the firmware header is valid
//-------------------------------------------------------------------------------------------------
bool FirmwareIO::is_fw_valid(const void *fw)
{
  if(((uint32*)fw)[0] != FW_HEAD_ID) return false;
  if(((LP_FW_HEAD)fw)->sum16 != Sum16().generate(fw, 0x1FE)) return false;
  if(((LP_FW_HEAD)fw)->sum32 != Sum32().generate(&((uint8*)fw)[0x200], sizeof(FW_HEAD)-0x200)) return false;
  return true;
}


//-------------------------------------------------------------------------------------------------
// verify if we have found the first firmware flash page
//-------------------------------------------------------------------------------------------------
bool FirmwareIO::is_fw_pg(const void *pg)
{
  return (((uint32*)pg)[0] == FW_HEAD_ID) && ((((uint32*)pg)[512] & 0xFFFF00FF) == 0x000000FF);
}


//-------------------------------------------------------------------------------------------------
// verify if we have found a valid lba-lut flash page
//-------------------------------------------------------------------------------------------------
bool FirmwareIO::is_lut_pg(const void *pg)
{
  const uint16 *lut = (uint16*)pg;

  if(((uint8*)pg)[0x800] != 0xFF) return false;  //is page valid?
  //if(((uint8*)pg)[0x802] != 0x00) return false;  //first lut?

  //if(check_terminator_id)
  //{
  //  if(lut[0x800/2-4] != 0xFFFF) return false;
  //  if(lut[0x800/2-1] != 0x55AA) return false;
  //}

  if(lut[0] == 0xFFFF) return false; //a valid lut couldn't start with an empty index

  for(int x = 0; x < 1024 - 4; x++) {
    if(lut[x] == 0) return false;   //a valid lut couldn't contain a zero index entry
    if(lut[x] != 0xFFFF) for(int y = 0; y < x; y++) {
      if(lut[x] == lut[y]) return false;  //a valid lut only contains uniqe index entries
    }
  }

  return true;  //we found a valid lut
}
