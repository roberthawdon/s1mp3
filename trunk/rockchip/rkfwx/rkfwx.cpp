/*
 * Rockchip Firmware Extractor
 *
 * desc: try to extract rockchip firmare from usb device
 * from: http://www.s1mp3.de/
 *
 * (c)2009 wiRe (mailto:mail _at_ s1mp3 _dot_ net)
 *
 * THIS FILE IS LICENSED UNDER THE GNU GPL
 * 
 */
#include "common.h"
#include "dump.h"

#include "libusb/libusb.h"  //also needs "libusb.lib"
#pragma comment(lib, "libusb/libusb.lib")

#define RK_VID 0x071B
#define RK_PID 0x3202


usb_dev_handle *hDevice = NULL;


///////////////////////////////////////////////////////////////////////////////////////////////////
int write(const void *lp, unsigned int u, unsigned int uTimeout = 0)
{
  if(hDevice == NULL) throw "invalid handle";
  int nResult = 
    ::usb_bulk_write(hDevice, 0x01, (char *)lp, u, ((uTimeout > 0)? uTimeout : 0xFFFF)*1000);
  //if(nResult < 0) throw "failed to write to device";
  return nResult;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
int read(const void *lp, unsigned int u, unsigned int uTimeout = 0)
{
  if(hDevice == NULL) throw "invalid handle";
  int nResult = 
    ::usb_bulk_read(hDevice, 0x81, (char *)lp, u, ((uTimeout > 0)? uTimeout : 0xFFFF)*1000);
  //if(nResult < 0) throw "failed to read from device";
  return nResult;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void cmd(const void *c, int read_len =0)
{
  printf("-------------------------------------------------------------------------------\n");
  dump(c, 0x1F);
  printf("\n");
  write(c, 0x1F);
  if(read_len > 0)
  {
    unsigned char buf[0x4000];
    memset(buf, 0xEE, sizeof(buf));
    read(buf, read_len);
    dump(buf, read_len);
    printf("\n");
  }

  unsigned char rpl[0x0D];
  memset(rpl, 0, sizeof(rpl));
  read(rpl, sizeof(rpl));
  dump(rpl, sizeof(rpl));
}


void main()
{
  try
  {
    printf("rkfwx v1.0 - extract rockchip firmware from usb device - (c)2009 wiRe\n");

    ::usb_init();
    ::usb_find_busses();
    ::usb_find_devices();

    for(usb_bus *lpBus = ::usb_get_busses(); lpBus; lpBus = lpBus->next)
    {
      for(struct usb_device *lpDev = lpBus->devices; lpDev; lpDev = lpDev->next)
      {
        if(lpDev->descriptor.idVendor == RK_VID && lpDev->descriptor.idProduct == RK_PID)
        {
          printf("open %s...\n", lpDev->filename);

          hDevice = ::usb_open(lpDev);
          if(hDevice == NULL) throw "open device failed";
          if(::usb_set_configuration(hDevice, 1) != 0) throw "open device failed";
          if(::usb_claim_interface(hDevice, 0) != 0) throw "open device failed";
          //if(::usb_set_altinterface(hDevice, 0) != 0) throw "open device failed";

          unsigned char cmd_nop[31] = {
            0x55, 0x53, 0x42, 0x43, 0x57, 0xf6, 0x4e, 0x75, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x06, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
          cmd(cmd_nop);

          unsigned char cmd_read5[31] = {
            0x55, 0x53, 0x42, 0x43, 0x57, 0xf6, 0x4e, 0x75, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x06, 0x01,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
          cmd(cmd_read5, 5);

          unsigned char cmd_init[31] = {
            0x55, 0x53, 0x42, 0x43, 0x57, 0xf6, 0x4e, 0x75, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x06, 0x02,
            0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
          cmd(cmd_init);

          unsigned char cmd_read16[31] = {
            0x55, 0x53, 0x42, 0x43, 0x57, 0xf6, 0x4e, 0x75, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x08,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00};
          cmd(cmd_read16, 16);

          unsigned char cmd_read[31] = {
            0x55, 0x53, 0x42, 0x43, 0x57, 0xf6, 0x4e, 0x75, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x0a, 0x04,
            0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
          cmd(cmd_read, 0x200);

          ::usb_close(hDevice);
          hDevice = NULL;
        }
      }
    }
  }
  catch(const char *s)
  {
    printf("error: %s\n", s);
  }
  catch(...)
  {
    printf("error: unknown exception\n");
  }
}