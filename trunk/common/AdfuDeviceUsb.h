//=================================================================================================
// AdfuDeviceUsb
//
// descr.   adfu device implementation: access for the original usb driver from action
// author   wiRe _at_ gmx _dot_ net
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2007 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#ifndef _REMOVE_ADFU_DEVICE_USB_

#ifdef LINUX
#else
#pragma once
#include <windows.h>
#include <setupapi.h>   //also needs "setupapi.lib"
#pragma comment(lib, "setupapi.lib")
#endif

#include <string>
#include <list>

#include "AdfuCbw.h"
#include "AdfuCsw.h"
#include "AdfuDevice.h"
#include "AdfuException.h"


//-------------------------------------------------------------------------------------------------
#define	ADFU_DEVICE_USB_MAX_DATA_LENGTH 0x200


//-------------------------------------------------------------------------------------------------
// implement interface
//-------------------------------------------------------------------------------------------------
class AdfuDeviceUsb : AdfuDevice {

///////////////////////////////////////////////////////////////////////////////////////////////////
public:AdfuDeviceUsb() : hDevice(INVALID_HANDLE_VALUE) {}
public:~AdfuDeviceUsb() {close();}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void close()
{
  if(hDevice != INVALID_HANDLE_VALUE) ::CloseHandle(hDevice);
  hDevice = INVALID_HANDLE_VALUE;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void open(std::string &strName)
{
  close();

  hDevice = ::CreateFileA(strName.c_str(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 
    NULL, OPEN_EXISTING, 0, NULL);
  if(hDevice == INVALID_HANDLE_VALUE) throw AdfuException();
}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:unsigned int io(DIR nDir, const void *lpCdb, unsigned char uCdbLength, 
                       void *lpData, unsigned int uDataLength, unsigned int uTimeout)
{
  unsigned int uResult = 0;

  AdfuCbw cbw(lpCdb, uCdbLength, 
    (nDir == IO_WRITE)? AdfuCbw::HOST_TO_DEVICE : AdfuCbw::DEVICE_TO_HOST, uDataLength);

  write(cbw.get(), cbw.size());
  if(lpData != NULL && uDataLength > 0) 
  {
    if(nDir == IO_WRITE) uResult = write(lpData, uDataLength);
    else if(nDir == IO_READ) uResult = read(lpData, uDataLength);
  }

  AdfuCsw csw;
  read(csw.get(), csw.size());
  if(!csw.verify()) throw AdfuException(ERROR_FUNCTION_FAILED);

  return uResult;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void enumerate(std::list<std::string> &lstDevice)
{
  lstDevice.clear();
  
  const unsigned char ucADFU[16] =
  {
    0x40, 0x81, 0xB1, 0xBD, 0x71, 0x75, 0xD7, 0x11,
    0x96, 0xC6, 0x52, 0x54, 0xAB, 0x1A, 0xFF, 0x33
  };

  HDEVINFO hDevInf = ::SetupDiGetClassDevsA((CONST GUID*)ucADFU, 0, NULL, 0x12);
  if(hDevInf == INVALID_HANDLE_VALUE) throw AdfuException();

  for(DWORD dwIndex = 0; 1; dwIndex++)
  {
    SP_DEVICE_INTERFACE_DATA data;	
    data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if(!::SetupDiEnumDeviceInterfaces(hDevInf, NULL, (CONST GUID*)ucADFU, dwIndex, &data)) 
    {
      if(::GetLastError() == ERROR_NO_MORE_ITEMS) break;
      ::SetupDiDestroyDeviceInfoList(hDevInf);
      throw AdfuException();
    }

    DWORD dwSize = 0;
    if(!::SetupDiGetDeviceInterfaceDetailA(hDevInf, &data, NULL, 0, &dwSize, NULL))
    {
      if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
      {
        SetupDiDestroyDeviceInfoList(hDevInf);
        throw AdfuException();
      }
    }
    dwSize += sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

    char *lpBuf = new char[dwSize];
    PSP_DEVICE_INTERFACE_DETAIL_DATA_A lpDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)lpBuf;
    lpDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
    if(!::SetupDiGetDeviceInterfaceDetailA(hDevInf, &data, lpDetail, dwSize, NULL, NULL))
    {
      DWORD dwError = ::GetLastError();
      ::SetupDiDestroyDeviceInfoList(hDevInf);
      delete lpBuf;
      throw dwError;
    }
    std::string str = lpDetail->DevicePath;
    lstDevice.push_back(str);
    delete lpBuf;
  }

  ::SetupDiDestroyDeviceInfoList(hDevInf);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:unsigned int write(const void *lp, unsigned int u, unsigned int uTimeout =3)
{
  DWORD dwRet = 0;
  if(hDevice == INVALID_HANDLE_VALUE) throw AdfuException(ERROR_INVALID_HANDLE);
  while(u > 0)
  {
    DWORD dwIoSize = (u < ADFU_DEVICE_USB_MAX_DATA_LENGTH)? 0 : ADFU_DEVICE_USB_MAX_DATA_LENGTH;
    u -= dwIoSize;
    if(!::DeviceIoControl(hDevice, 0x22200C, (LPVOID)lp, dwIoSize, NULL, 0, &dwIoSize, NULL)) throw AdfuException();
    lp = &((LPBYTE)lp)[ADFU_DEVICE_USB_MAX_DATA_LENGTH];
    dwRet += dwIoSize;
  }
  return dwRet;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:unsigned int read(void *lp, unsigned int u, unsigned int uTimeout =3)
{
  DWORD dwRet = 0;
  if(hDevice == INVALID_HANDLE_VALUE) throw AdfuException(ERROR_INVALID_HANDLE);
  while(u > 0)
  {
    DWORD dwIoSize = (u < ADFU_DEVICE_USB_MAX_DATA_LENGTH)? 0 : ADFU_DEVICE_USB_MAX_DATA_LENGTH;
    u -= dwIoSize;
    if(!::DeviceIoControl(hDevice, 0x222010, NULL, 0, (LPVOID)lp, dwIoSize, &dwIoSize, NULL)) throw AdfuException();
    lp = &((LPBYTE)lp)[ADFU_DEVICE_USB_MAX_DATA_LENGTH];
    dwRet += dwIoSize;
  }
  return dwRet;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:HANDLE hDevice;
};


#endif
