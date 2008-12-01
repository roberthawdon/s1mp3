//=================================================================================================
// GiveIO
//
// descr.    gives low level access to s1mp3 players
// author    wiRe _at_ gmx _dot_ net
// source    http://www.s1mp3.de/
//
// copyright (c)2005-2007 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#include "giveio.h"


//-------------------------------------------------------------------------------------------------
#define GIVEIO_ADDR 0x3400
#define GIVEIO_MAX_DATA_LENGTH 0xA00  //0xFE0 on old devices, 0xC00 on v9 devices, but actually only 0xA00 works

#include "./giveio.bin/giveio.h"


//-------------------------------------------------------------------------------------------------
void GiveIO::init()
{
  lpSession->upload(GIVEIO_ADDR, giveio, sizeof(giveio));
  lpSession->exec(GIVEIO_ADDR);

  //verify giveio
  #pragma pack(1)
  struct {
    uint8 version;
    uint8 v9flag;
    char  id[15];
  } info;
  #pragma pack()

  read("i", 1, &info, sizeof(info));
  if(::strcmp(info.id, "s1giveio") != 0) throw AdfuException(ERROR_FUNCTION_FAILED);
  uVersion = info.version;
  fV9Device = (info.v9flag != 0);
}

//-------------------------------------------------------------------------------------------------
uint8 GiveIO::in(uint8 uPort)
{
  uint8 uResult = 0;
  uint8 uCmd[] = {'p', uPort};
  read(uCmd, sizeof(uCmd), &uResult, 1);
  return uResult;
}

//-------------------------------------------------------------------------------------------------
void GiveIO::out(uint8 uPort, uint8 uData)
{
  uint8 uCmd[] = {'p', uPort, uData};
  write(uCmd, sizeof(uCmd));
}

//-------------------------------------------------------------------------------------------------
void GiveIO::ld(uint16 uAddr, void *lpData, uint32 uLength)
{
  uint8 uCmd[] = {'m', LOBYTE(uAddr), HIBYTE(uAddr)};

  while(uLength > GIVEIO_MAX_DATA_LENGTH)
  {
    read(uCmd, sizeof(uCmd), lpData, GIVEIO_MAX_DATA_LENGTH);

    lpData = &((char*)lpData)[GIVEIO_MAX_DATA_LENGTH];
    ((uint16 *)(&uCmd[1]))[0] += GIVEIO_MAX_DATA_LENGTH;
    uLength -= GIVEIO_MAX_DATA_LENGTH;
  }

  if(uLength > 0) read(uCmd, sizeof(uCmd), lpData, uLength);
}

//-------------------------------------------------------------------------------------------------
void GiveIO::st(uint16 uAddr, const void *lpData, uint32 uLength)
{
  uint8 uCmd[] = {'m', LOBYTE(uAddr), HIBYTE(uAddr)};

  while(uLength > GIVEIO_MAX_DATA_LENGTH)
  {
    write(uCmd, sizeof(uCmd), lpData, GIVEIO_MAX_DATA_LENGTH);

    lpData = &((char*)lpData)[GIVEIO_MAX_DATA_LENGTH];
    ((uint16 *)(&uCmd[1]))[0] += GIVEIO_MAX_DATA_LENGTH;
    uLength -= GIVEIO_MAX_DATA_LENGTH;
  }

  if(uLength > 0) write(uCmd, sizeof(uCmd), lpData, uLength);
}

//-------------------------------------------------------------------------------------------------
void GiveIO::ld_dma(uint32 uAddr, uint8 uIntMemSel, uint8 uDmaMode,
					void *lpData, uint32 uLength)
{
  uint8 uCmd[] = {'d', 
    LOBYTE(LOWORD(uAddr)), HIBYTE(LOWORD(uAddr)), 
    LOBYTE(HIWORD(uAddr)), HIBYTE(HIWORD(uAddr)),
    uIntMemSel, uDmaMode
  };

  while(uLength > GIVEIO_MAX_DATA_LENGTH)
  {
    read(uCmd, sizeof(uCmd), lpData, GIVEIO_MAX_DATA_LENGTH);

    lpData = &((char*)lpData)[GIVEIO_MAX_DATA_LENGTH];
    ((uint16 *)(&uCmd[1]))[0] += GIVEIO_MAX_DATA_LENGTH;
    uLength -= GIVEIO_MAX_DATA_LENGTH;
  }

  if(uLength > 0) read(uCmd, sizeof(uCmd), lpData, uLength);
}

//-------------------------------------------------------------------------------------------------
void GiveIO::st_dma(uint32 uAddr, uint8 uIntMemSel, uint8 uDmaMode,
					const void *lpData, uint32 uLength)
{
  uint8 uCmd[] = {'d', 
    LOBYTE(LOWORD(uAddr)), HIBYTE(LOWORD(uAddr)), 
    LOBYTE(HIWORD(uAddr)), HIBYTE(HIWORD(uAddr)),
    uIntMemSel, uDmaMode
  };

  while(uLength > GIVEIO_MAX_DATA_LENGTH)
  {
    write(uCmd, sizeof(uCmd), lpData, GIVEIO_MAX_DATA_LENGTH);

    lpData = &((char*)lpData)[GIVEIO_MAX_DATA_LENGTH];
    ((uint16 *)(&uCmd[1]))[0] += GIVEIO_MAX_DATA_LENGTH;
    uLength -= GIVEIO_MAX_DATA_LENGTH;
  }

  if(uLength > 0) write(uCmd, sizeof(uCmd), lpData, uLength);
}

//-------------------------------------------------------------------------------------------------
void GiveIO::exec(uint16 uAddr, const void *lpData, uint32 uLength, uint16 uTimeout)
{
  uint8 uCmd[] = {'X', LOBYTE(uAddr), HIBYTE(uAddr)};

  if(lpData == NULL) uLength = 0;
  else if(uLength == 0) lpData = NULL;

  if(uLength > GIVEIO_MAX_DATA_LENGTH)  //first upload remaining tail
  {
    for(uint32 uOfs = GIVEIO_MAX_DATA_LENGTH; uOfs < uLength; uOfs += GIVEIO_MAX_DATA_LENGTH)
    {
      uint8 uCmdUp[] = {'m', LOBYTE(uAddr + uOfs), HIBYTE(uAddr + uOfs)};
      write(uCmdUp, sizeof(uCmdUp), &((char*)lpData)[uOfs], 
        ((uLength - uOfs) >= GIVEIO_MAX_DATA_LENGTH)? GIVEIO_MAX_DATA_LENGTH : (uLength - uOfs));
    }
  }

  write(uCmd, sizeof(uCmd), lpData, uLength, uTimeout);
}

//-------------------------------------------------------------------------------------------------
void GiveIO::reset()
{
  try {write("R", 1);}
  catch(...) {}
}
