//=================================================================================================
// AdfuDeviceLibusb
//
// descr.   adfu device implementation: access s1mp3 players via LibUsb
// author   wiRe _at_ gmx _dot_ net
// source   http://www.s1mp3.de/
// ref.     http://libusb.sourceforge.net/
//
// copyright (c)2005-2007 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#ifndef _REMOVE_ADFU_DEVICE_LIBUSB_


#pragma once
#include <windows.h>
#include <string>
#include <list>

#include "AdfuDevice.h"
#include "AdfuException.h"

#include "libusb/libusb.h"  //also needs "libusb/libusb.lib"
#pragma comment(lib, "libusb.lib")


//-------------------------------------------------------------------------------------------------
#define	LIBUSB_VENDOR_ID   0x10D6
#define	LIBUSB_PRODUCT_ID  0xFF51 //NOTE: not verified anymore since there are multiple values


//-------------------------------------------------------------------------------------------------
// implement interface
//-------------------------------------------------------------------------------------------------
class AdfuDeviceLibusb : AdfuDevice {

///////////////////////////////////////////////////////////////////////////////////////////////////
public:AdfuDeviceLibusb() : hDevice(NULL) {/*::usb_set_debug(0);*/ ::usb_init();}
public:~AdfuDeviceLibusb() {close();}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void close()
{
  if(hDevice != NULL)
  {
    ::usb_close(hDevice);
    hDevice = NULL;
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void open(std::string &strName)
{
  close();

  ::usb_find_busses();
  ::usb_find_devices();

  for(usb_bus *lpBus = ::usb_get_busses(); lpBus; lpBus = lpBus->next)
  {
    for(struct usb_device *lpDev = lpBus->devices; lpDev; lpDev = lpDev->next)
    {
      if(strName.compare(lpDev->filename) == 0)
      {
        hDevice = ::usb_open(lpDev);
        if(hDevice == NULL) throw AdfuException(ERROR_OPEN_FAILED);
        if(::usb_set_configuration(hDevice, 1) != 0) throw AdfuException(ERROR_OPEN_FAILED);
        if(::usb_claim_interface(hDevice, 0) != 0) throw AdfuException(ERROR_OPEN_FAILED);
        if(::usb_set_altinterface(hDevice, 0) != 0) throw AdfuException(ERROR_OPEN_FAILED);
        return; //success
      }
    }
  }

  throw AdfuException(ERROR_OPEN_FAILED);
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
    else if(nDir == IO_READ) uResult = read(lpData, uDataLength, uTimeout);
  }

  AdfuCsw csw;
  read(csw.get(), csw.size(), uTimeout);
  if(!csw.verify()) throw AdfuException(ERROR_FUNCTION_FAILED);

  return uResult;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
public:void enumerate(std::list<std::string> &lstDevice)
{
  lstDevice.clear();
  
  ::usb_find_busses();
  ::usb_find_devices();

  for(usb_bus *lpBus = ::usb_get_busses(); lpBus; lpBus = lpBus->next)
  {
    for(struct usb_device *lpDev = lpBus->devices; lpDev; lpDev = lpDev->next)
    {
      if(lpDev->descriptor.idVendor == LIBUSB_VENDOR_ID /*&& lpDev->descriptor.idProduct == LIBUSB_PRODUCT_ID*/)
      {
        std::string str = lpDev->filename;
        lstDevice.push_back(str);
      }
    }
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:unsigned int write(const void *lp, unsigned int u, unsigned int uTimeout =3)
{
  if(hDevice == NULL) throw AdfuException(ERROR_INVALID_HANDLE);
  int nResult = 
    ::usb_bulk_write(hDevice, 0x01, (char *)lp, u, ((uTimeout > 0)? uTimeout : 0xFFFF)*1000);
  if(nResult < 0) throw AdfuException(ERROR_WRITE_FAULT);
  return (unsigned int)nResult;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:unsigned int read(const void *lp, unsigned int u, unsigned int uTimeout =3)
{
  if(hDevice == NULL) throw AdfuException(ERROR_INVALID_HANDLE);
  int nResult = 
    ::usb_bulk_read(hDevice, 0x82, (char *)lp, u, ((uTimeout > 0)? uTimeout : 0xFFFF)*1000);
  if(nResult < 0) throw AdfuException(ERROR_READ_FAULT);
  return (unsigned int)nResult;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
private:usb_dev_handle *hDevice;
};


#endif