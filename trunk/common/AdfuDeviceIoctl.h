//=================================================================================================
// AdfuDeviceIoctl
//
// descr.   adfu device implementation: access s1mp3 players via SCSI I/O control codes
// author   wiRe _at_ gmx _dot_ net
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2007 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#ifndef _REMOVE_ADFU_DEVICE_IOCTL_


#pragma once
#ifdef LINUX
#else
#include <windows.h>
#endif
#include <string>
#include <list>

#include "AdfuDevice.h"
#include "AdfuException.h"


//-------------------------------------------------------------------------------------------------
// implement interface
//-------------------------------------------------------------------------------------------------
class AdfuDeviceIoctl : AdfuDevice {

///////////////////////////////////////////////////////////////////////////////////////////////////
public:AdfuDeviceIoctl() : hDevice(INVALID_HANDLE_VALUE) {}
public:~AdfuDeviceIoctl() { close(); }


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void close()
{
  if(hDevice != INVALID_HANDLE_VALUE) ::CloseHandle(hDevice);
  hDevice = INVALID_HANDLE_VALUE;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
protected:void preopen(std::string &strName)
{
  close();

  #ifdef _DEBUG
    std::string strDebug = "giveioDeviceIoctl::preopen: ";
    strDebug += strName;
    ::OutputDebugStringA(strDebug.c_str());
  #endif

  //if(GetDriveType(strName.c_str()) != 2) throw AdfuException(ERROR_OPEN_FAILED);

  hDevice = ::CreateFileA(strName.c_str(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ,
    NULL, OPEN_EXISTING, 0, NULL);
  if(hDevice == INVALID_HANDLE_VALUE) throw AdfuException();

  DWORD dwRet = 0;
  char cBuf[256];
  if(!::DeviceIoControl(hDevice, /*IOCTL_DISK_GET_DRIVE_GEOMETRY*/ 0x70000, NULL, 0, cBuf, sizeof(cBuf), &dwRet, NULL))
    throw AdfuException();
  if(!dwRet) throw AdfuException(ERROR_OPEN_FAILED);
  if(((DWORD*)cBuf)[2] != 0x0000000B) throw AdfuException(ERROR_OPEN_FAILED);

  const uint32 uCmdInit[3] = {0xCC, 0, 0};
  read(uCmdInit, sizeof(uCmdInit), cBuf, 11);
  if(::strncmp(cBuf, "ACTIONSUSBD", 11) != 0) throw AdfuException(ERROR_OPEN_FAILED);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void open(std::string &strName)
{
  preopen(strName);

  //put device into fw mode now
  uint8 uResult = 0;
  const uint32 uCmdEnterFMode[3] = {0xCB, 0, 0};
  for(int nRepeat = 10; nRepeat > 0; nRepeat--)
  {
    Sleep(1000);
    if((read(uCmdEnterFMode, sizeof(uCmdEnterFMode), &uResult, 1) == 1) && (uResult == 0xFF))
    {
      Sleep(500);
      return;
    }
  }

  throw AdfuException(ERROR_TIMEOUT);  
}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:unsigned int io(DIR nDir, const void *lpCdb, unsigned char uCdbLength, 
                       void *lpData, unsigned int uDataLength, unsigned int uTimeout)
{
  struct {
    SCSI_PASS_THROUGH header;
    char sense[4];
    char data[ADFU_MAX_DATA_LENGTH];
  } iob;

  if(uCdbLength > sizeof(iob.header.Cdb)) throw AdfuException(ERROR_INVALID_PARAMETER); //uCdbLength = sizeof(iob.header.Cdb);
  if(uDataLength > ADFU_MAX_DATA_LENGTH) throw AdfuException(ERROR_INVALID_PARAMETER); //uDataLength = ADFU_MAX_DATA_LENGTH;

  ::memset(&iob, 0, sizeof(iob));
  iob.header.Length = sizeof(iob.header);
  //iob.header.PathId = 0;
  iob.header.TargetId = 1;
  //iob.header.Lun = 0;
  //iob.header.SenseInfoLength = 0;
  iob.header.DataTransferLength = uDataLength;
  iob.header.TimeOutValue = (uTimeout > 0)? uTimeout : 0xFFFF;
  iob.header.DataBufferOffset = (ULONG)((ULONGLONG)&iob.data - (ULONGLONG)&iob);
  iob.header.SenseInfoOffset = (ULONG)((ULONGLONG)&iob.sense - (ULONGLONG)&iob);

  if(nDir == IO_READ) iob.header.DataIn = 1; //= SCSI_IOCTL_DATA_OUT
  else if(lpData != NULL && uDataLength > 0) ::memcpy(&iob.data, lpData, uDataLength); //= SCSI_IOCTL_DATA_IN

  if(lpCdb != NULL && uCdbLength > 0)
  {
    iob.header.CdbLength = uCdbLength;
    ::memcpy(iob.header.Cdb, lpCdb, uCdbLength);
  }

  unsigned int uResult = wrd(&iob, sizeof(iob));
  if(nDir == IO_READ && lpData != NULL && uDataLength > 0) ::memcpy((void*)lpData, iob.data, uDataLength);

  return iob.header.DataTransferLength;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void enumerate(std::list<std::string> &lstDevice)
{
  lstDevice.clear();
  
  DWORD dwDrives = GetLogicalDrives();
  for(char cDrive = 'A'; cDrive <= 'Z'; cDrive++, dwDrives >>= 1) if(dwDrives &1)
  {
    try {        
      std::string strDrive = "\\\\.\\";
      strDrive += cDrive;
      strDrive += ':';

      AdfuDeviceIoctl d;
      d.preopen(strDrive);
      d.close();

      lstDevice.push_back(strDrive);
    }
    catch(AdfuException e) {
      e.what();
    }
    catch(...) {}
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:typedef struct _SCSI_PASS_THROUGH {
  uint16 Length;                //Contains the sum of the length of the SCSI_PASS_THROUGH structure and the lengths of the accompanying data and request sense buffers. 
  uint8  ScsiStatus;            //Reports the SCSI status that was returned by the HBA or the target device. 
  uint8  PathId;                //Indicates the SCSI port or bus for the request. 
  uint8  TargetId;              //Indicates the target controller or device on the bus. 
  uint8  Lun;                   //Indicates the logical unit number of the device. 
  uint8  CdbLength;             //Indicates the size in bytes of the SCSI command descriptor block. 
  uint8  SenseInfoLength;       //Indicates the size in bytes of the request-sense buffer. 
  uint8  DataIn;                //Indicates whether the SCSI command will read or write data. This field must have one of three values: SCSI_IOCTL_DATA_IN (write to device), SCSI_IOCTL_DATA_OUT (read from device), SCSI_IOCTL_DATA_UNSPECIFIED
  uint32 DataTransferLength;    //Indicates the size in bytes of the data buffer. If an underrun occurs, the miniport must update this member to the number of bytes actually transferred.
  uint32 TimeOutValue;          //Indicates the interval in seconds that the request can execute before the OS-specific port driver might consider it timed out.
  uint32 DataBufferOffset;      //Contains an offset from the beginning of this structure to the data buffer.
  uint32 SenseInfoOffset;       //Offset from the beginning of this structure to the request-sense buffer.
  uint8  Cdb[16];               //Specifies the SCSI command descriptor block to be sent to the target device.
} SCSI_PASS_THROUGH, *PSCSI_PASS_THROUGH;


///////////////////////////////////////////////////////////////////////////////////////////////////
private:unsigned int wrd(const void *lp, unsigned int u)
{
  DWORD dwRet = 0;
  if(hDevice == INVALID_HANDLE_VALUE) throw AdfuException(ERROR_INVALID_PARAMETER);
  if(!::DeviceIoControl(hDevice, /*IOCTL_SCSI_PASS_THROUGH*/ 0x4D004, 
    (LPVOID)lp, u, (LPVOID)lp, u, &dwRet, NULL)) throw AdfuException();
  return dwRet;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:HANDLE hDevice;
};


#endif
