//=================================================================================================
// AdfuSession
//
// descr.   wrapper class to manage any adfu session
// author   wiRe _at_ gmx _dot_ net
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2007 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#ifdef LINUX
#else
#include <windows.h>
#endif
#include <string>
#include <list>

#include "AdfuSession.h"
#include "AdfuDevice.h"
#ifndef LINUX
#include "AdfuDeviceUsb.h"
#include "AdfuDeviceIoctl.h"
#endif
#include "AdfuDeviceLibusb.h"
#include "AdfuException.h"


//-------------------------------------------------------------------------------------------------
// CONSTRUCTOR
//-------------------------------------------------------------------------------------------------
AdfuSession::AdfuSession() : g_lpDevice(NULL)
{
}


//-------------------------------------------------------------------------------------------------
// DESTRUCTOR
//-------------------------------------------------------------------------------------------------
AdfuSession::~AdfuSession()
{
  close();
}


//-------------------------------------------------------------------------------------------------
// close device
//-------------------------------------------------------------------------------------------------
void AdfuSession::close()
{
  if(g_lpDevice != NULL)
  {
    delete g_lpDevice;
    g_lpDevice = NULL;
  }
}


//-------------------------------------------------------------------------------------------------
// open device
//-------------------------------------------------------------------------------------------------
void AdfuSession::open(DeviceDescriptor &DevDescr)
{
  close();

  switch(DevDescr.g_nType)
  {
 #ifndef LINUX
  #ifndef _REMOVE_ADFU_DEVICE_USB_
    case DeviceDescriptor::ADFU:    g_lpDevice = (AdfuDevice*)new AdfuDeviceUsb(); break;
  #endif


  #ifndef _REMOVE_ADFU_DEVICE_IOCTL_
    case DeviceDescriptor::IOCTL:   g_lpDevice = (AdfuDevice*)new AdfuDeviceIoctl(); break;
  #endif
 #endif
  
  #ifndef _REMOVE_ADFU_DEVICE_LIBUSB_
    case DeviceDescriptor::LIBUSB:  g_lpDevice = (AdfuDevice*)new AdfuDeviceLibusb(); break;
  #endif

    default:                        throw AdfuException(ERROR_INVALID_PARAMETER);
  }

  g_lpDevice->open(DevDescr.g_strName);  
  fRecoveryMode = (DevDescr.g_nType != DeviceDescriptor::IOCTL) && detectRecoveryMode();
}


//-------------------------------------------------------------------------------------------------
// forward write
//-------------------------------------------------------------------------------------------------
unsigned int AdfuSession::write(const void *lpCdb, unsigned char uCdbLength, 
                                const void *lpData, unsigned int uDataLength,
                                unsigned int uTimeout)
{
  if(g_lpDevice == NULL) throw AdfuException(ERROR_INVALID_HANDLE);
  return g_lpDevice->write(lpCdb, uCdbLength, lpData, uDataLength, uTimeout);
}


//-------------------------------------------------------------------------------------------------
// forward read
//-------------------------------------------------------------------------------------------------
unsigned int AdfuSession::read(const void *lpCdb, unsigned char uCdbLength, 
                               void *lpData, unsigned int uDataLength, 
                               unsigned int uTimeout)
{
  if(g_lpDevice == NULL) throw AdfuException(ERROR_INVALID_HANDLE);
  return g_lpDevice->read(lpCdb, uCdbLength, lpData, uDataLength, uTimeout);
}


//-------------------------------------------------------------------------------------------------
// upload memory
//-------------------------------------------------------------------------------------------------
unsigned int AdfuSession::upload(unsigned int uAddress,
                                 const void *lpData, unsigned int uDataLength, MEMSEL nMemSel)
{
  unsigned int uResult = 0;

  for(; uDataLength > ADFU_MAX_DATA_LENGTH; uDataLength -= ADFU_MAX_DATA_LENGTH)
  {
    uResult += uploadBlock(uAddress, lpData, ADFU_MAX_DATA_LENGTH, nMemSel);
    uAddress += ADFU_MAX_DATA_LENGTH;
    lpData = &((uint8*)lpData)[ADFU_MAX_DATA_LENGTH];    
  }

  if(uDataLength > 0) uResult += uploadBlock(uAddress, lpData, uDataLength, nMemSel);

  return uResult;
}


//-------------------------------------------------------------------------------------------------
// download memory
//-------------------------------------------------------------------------------------------------
unsigned int AdfuSession::download(unsigned int uAddress,
                                   const void *lpData, unsigned int uDataLength, MEMSEL nMemSel)
{
  unsigned int uResult = 0;

  for(; uDataLength > ADFU_MAX_DATA_LENGTH; uDataLength -= ADFU_MAX_DATA_LENGTH)
  {
    uResult += downloadBlock(uAddress, lpData, ADFU_MAX_DATA_LENGTH, nMemSel);
    uAddress += ADFU_MAX_DATA_LENGTH;
    lpData = &((uint8*)lpData)[ADFU_MAX_DATA_LENGTH];    
  }

  if(uDataLength > 0) uResult += downloadBlock(uAddress, lpData, uDataLength, nMemSel);

  return uResult;
}


//-------------------------------------------------------------------------------------------------
// call address
//-------------------------------------------------------------------------------------------------
void AdfuSession::exec(unsigned int uAddress, unsigned int uTimeout)
{
  //
  // command: call address
  // cdb[0]   command id (0x10/0x20)
  // cdb[2:1] destination address
  //
  uint32 uCmd[3] = {(fRecoveryMode? 0x10 : 0x20) | (((uint16)uAddress) << 8), 0, 0};
  write(uCmd, sizeof(uCmd), NULL, 0, uTimeout);
  //
}


//-------------------------------------------------------------------------------------------------
// upload code and call address
//-------------------------------------------------------------------------------------------------
void AdfuSession::exec(unsigned int uAddress, const void *lpData, unsigned int uDataLength, 
                       unsigned int uTimeout)
{
  upload(uAddress, lpData, uDataLength);
  exec(uAddress, uTimeout);
}


//-------------------------------------------------------------------------------------------------
// reset device
//-------------------------------------------------------------------------------------------------
void AdfuSession::reset()
{
  //
  // command: reset
  // cdb[0]   command id (0x16)
  //
  uint32 uCmd[3] = {0x16, 0, 0};
  write(uCmd, sizeof(uCmd));
  //
}








//-------------------------------------------------------------------------------------------------
// upload one block of memory
//-------------------------------------------------------------------------------------------------
unsigned int AdfuSession::uploadBlock(unsigned int uAddress,
                                      const void *lpData, unsigned int uDataLength, 
                                      MEMSEL nMemSel)
{
  if(uDataLength > ADFU_MAX_DATA_LENGTH) uDataLength = ADFU_MAX_DATA_LENGTH;
  unsigned int uBlockLength = (uDataLength+3)&0xFFFFFFFC;

  uint8 uBuf[ADFU_MAX_DATA_LENGTH];
  memset(uBuf, 0, sizeof(uBuf));
  memcpy(uBuf, lpData, uDataLength);

  //
  // command: r/w memory
  // cdb[0]   command id (0x05)
  // cdb[1]   command flags (0x80 = read, 0x00 = write)
  // cdb[3:2] source/destination address
  // cdb[6]   memory select (0..7 = IPM/IDM/ZRAM2, 8 = ZRAM)
  // cdb[8:7] data length
  //
  uint32 uCmd[3] = {0x0005 | (((uint16)uAddress) << 16), 
    ((uint8)uBlockLength<<24) + (((uint8)nMemSel)<<16), uBlockLength>>8};
  unsigned int uResult = write(uCmd, sizeof(uCmd), uBuf, uBlockLength);
  //

  return (uResult > uDataLength)? uDataLength : uResult;
}


//-------------------------------------------------------------------------------------------------
// download one block of memory
//-------------------------------------------------------------------------------------------------
unsigned int AdfuSession::downloadBlock(unsigned int uAddress,
                                        const void *lpData, unsigned int uDataLength, 
                                        MEMSEL nMemSel)
{
  if(uDataLength > ADFU_MAX_DATA_LENGTH) uDataLength = ADFU_MAX_DATA_LENGTH;
  unsigned int uBlockLength = (uDataLength+3)&0xFFFC;
  uint8 uBuf[ADFU_MAX_DATA_LENGTH];

  //
  // command: r/w memory
  // cdb[0]   command id (0x05)
  // cdb[1]   command flags (0x80 = read, 0x00 = write)
  // cdb[3:2] source/destination address
  // cdb[6]   memory select (0..7 = IPM/IDM/ZRAM2, 8 = ZRAM)
  // cdb[8:7] data length
  //
  uint32 uCmd[3] = {0x8005 | (((uint16)uAddress) << 16),
    ((uint8)uBlockLength<<24) + (((uint8)nMemSel)<<16), uBlockLength>>8};
  unsigned int uResult = read(uCmd, sizeof(uCmd), uBuf, uBlockLength);
  //

  if(lpData != NULL) memcpy((void*)lpData, uBuf, uDataLength);
  return (uResult > uDataLength)? uDataLength : uResult;
}


//-------------------------------------------------------------------------------------------------
// enumerate devices
//-------------------------------------------------------------------------------------------------
void AdfuSession::enumerate(std::list<DeviceDescriptor> &lstDevDescr)
{
  lstDevDescr.clear();
 #ifndef LINUX
  #ifndef _REMOVE_ADFU_DEVICE_USB_
    enum_add(lstDevDescr, DeviceDescriptor::ADFU, (AdfuDevice &)AdfuDeviceUsb());
  #endif

  #ifndef _REMOVE_ADFU_DEVICE_IOCTL_
    enum_add(lstDevDescr, DeviceDescriptor::IOCTL, (AdfuDevice &)AdfuDeviceIoctl());
  #endif
 #endif
  #ifndef _REMOVE_ADFU_DEVICE_LIBUSB_
    enum_add(lstDevDescr, DeviceDescriptor::LIBUSB, *((AdfuDevice *)new AdfuDeviceLibusb()));
  #endif
}


//-------------------------------------------------------------------------------------------------
// enumerate device helper
//-------------------------------------------------------------------------------------------------
void AdfuSession::enum_add(std::list<DeviceDescriptor> &lstDevDescr, 
                      DeviceDescriptor::TYPE nType, AdfuDevice &Dev)
{
  std::list<std::string> lstDevName;

  Dev.enumerate(lstDevName);
  for(std::list<std::string>::iterator i = lstDevName.begin(); i != lstDevName.end(); i++)
  {
    lstDevDescr.push_back( DeviceDescriptor(nType, *i) );
  }
}


//-------------------------------------------------------------------------------------------------
// detect if we are in recovery mode
//-------------------------------------------------------------------------------------------------
bool AdfuSession::detectRecoveryMode()
{
  uint8 uBuf[16] = "";
  download(0x0800, uBuf, sizeof(uBuf));
  return(memcmp(uBuf, "SYS INFO", 8) != 0);
}

