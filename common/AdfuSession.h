//=================================================================================================
// AdfuSession
//
// descr.   wrapper class to manage any adfu session
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#ifdef LINUX
#else
#include <windows.h>
#include <string>
#endif
#include <list>

#include "common.h"
#include "AdfuDevice.h"
#include "AdfuException.h"


//-------------------------------------------------------------------------------------------------
#define ADFU_MAX_DATA_LENGTH 0x4000
#define ADFU_TIMEOUT 5


//-------------------------------------------------------------------------------------------------
class AdfuSession {
public:
  class DeviceDescriptor {  // device descriptor for any GiveIODeviceXxx
    public:enum TYPE {ADFU, IOCTL, LIBUSB};
    public:TYPE g_nType;
    public:std::string g_strName;
    public:DeviceDescriptor(TYPE nType, std::string &strName) {g_nType = nType; g_strName = strName;}
  };

  AdfuSession();
  ~AdfuSession();

  void enumerate(std::list<DeviceDescriptor> &);
  void open(DeviceDescriptor &);
  void close();

  unsigned int write(const void *lpCmd, unsigned char uCdbLength,
    const void *lpData =NULL, unsigned int uDataLength =0, 
    unsigned int uTimeout =ADFU_TIMEOUT);
  unsigned int read(const void *lpCmd, unsigned char uCdbLength, 
          void *lpData =NULL, unsigned int uDataLength =0, 
    unsigned int uTimeout =ADFU_TIMEOUT);

  enum MEMSEL {IPM_L=0, IPM_M=1, IPM_H=2, IDM_L=4, IDM_M=5, IDM_H=6, ZRAM2=7, ZRAM1=8, ZRAM=8};
  unsigned int upload(unsigned int uAddress, 
    const void *lpData, unsigned int uDataLength, MEMSEL nMemSel =ZRAM);
  unsigned int download(unsigned int uAddress,
    const void *lpData, unsigned int uDataLength, MEMSEL nMemSel =ZRAM);

  void exec(unsigned int uAddress, unsigned int uTimeout =ADFU_TIMEOUT);
  void exec(unsigned int uAddress, const void *lpData, unsigned int uDataLength, unsigned int uTimeout =ADFU_TIMEOUT);

  void reset();

  bool isRecoveryMode() {return fRecoveryMode;}

private:
  unsigned int uploadBlock(unsigned int uAddress,
    const void *lpData, unsigned int uDataLength, MEMSEL nMemSel);
  unsigned int downloadBlock(unsigned int uAddress,
    const void *lpData, unsigned int uDataLength, MEMSEL nMemSel);

  void enum_add(std::list<DeviceDescriptor> &, DeviceDescriptor::TYPE, AdfuDevice &);
  bool detectRecoveryMode();

  AdfuDevice *g_lpDevice;
  bool fRecoveryMode;
};
