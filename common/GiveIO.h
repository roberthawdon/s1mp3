//=================================================================================================
// GiveIO
//
// descr.   gives low level access to s1mp3 players
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#include "common.h"
#include "AdfuSession.h"


//-------------------------------------------------------------------------------------------------
#define GIVEIO_TIMEOUT 5


//-------------------------------------------------------------------------------------------------
class GiveIO {
public:
  GiveIO(AdfuSession &session) {lpSession = &session;}
  uint8 version() {return uVersion;}
  bool isV9Device() {return fV9Device;}

  uint8 ld(uint16 uAddr) {uint8 uResult = 0; ld(uAddr, &uResult, sizeof(uResult)); return uResult;}
  void st(uint16 uAddr, uint8 uData) {st(uAddr, &uData, sizeof(uData));}
  void exec(uint16 uAddr, uint16 uTimeout =0) {exec(uAddr, NULL, 0, uTimeout);}

  void init();
  uint8 in(uint8 uPort);
  void out(uint8 uPort, uint8 uData);
  void ld(uint16 uAddr,       void *lpData, uint32 uLength);
  void st(uint16 uAddr, const void *lpData, uint32 uLength);
  void ld_dma(uint32 uAddr, uint8 uIntMemSel, uint8 uDmaMode,       void *lpData, uint32 uLength);
  void st_dma(uint32 uAddr, uint8 uIntMemSel, uint8 uDmaMode, const void *lpData, uint32 uLength);
  void exec(uint16 uAddr, const void *lpData, uint32 uLength, uint16 uTimeout =0);
  void reset();

private:
  unsigned int write(const void *lpCmd, unsigned char uCdbLength,
    const void *lpData =NULL, unsigned int uDataLength =0, 
    unsigned int uTimeout =GIVEIO_TIMEOUT) {return lpSession->write(lpCmd, uCdbLength, lpData, uDataLength);}
  unsigned int read(const void *lpCmd, unsigned char uCdbLength, 
          void *lpData =NULL, unsigned int uDataLength =0, 
    unsigned int uTimeout =GIVEIO_TIMEOUT) {return lpSession->read(lpCmd, uCdbLength, lpData, uDataLength);}

  AdfuSession *lpSession;
  uint8 uVersion;
  bool fV9Device;

  #ifdef GIVEIO_V9
    uint8 uImage[0x80];  //an image of the original memory content of the buffer
    void ld64(uint16 uAddr, const void *lpData, uint32 uLength);
  #endif
};
