#ifndef _DEVICE_H_
#define _DEVICE_H_


typedef __int8  int8;
typedef __int16 int16;
typedef __int32 int32;
typedef unsigned __int8  uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;


enum CMD_ID {
  CMD_INIT,
  CMD_ENTER_FMODE,
  CMD_SHUTDOWN,
  CMD_READ_SYSINFO,
  CMD_READ_BREC,
  CMD_WRITE_BREC,
  CMD_READ_FLASH,
  CMD_WRITE_FLASH
};

#define CMD_HEADER_ID "USBC"
#define CMD_MAX_TX 0x4000
#define CMD_FLAG_READ 0x80

#define _CMD_INIT 0xCC
#define _CMD_ENTER_FMODE 0xCB
#define _CMD_SHUTDOWN 0x16  //0x20
#define _CMD_READ_MEM(_offset)      0x8005 | ((_offset) << 16)
#define _CMD_READ_BREC(_offset)     0x8009 | ((_offset) << 16)
#define _CMD_WRITE_BREC(_offset)    0x2009 | ((_offset) << 16)
#define _CMD_READ_FLASH(_offset)    0x8008 | ((_offset) << 16)
#define _CMD_WRITE_FLASH(_offset)   0x0C08 | ((_offset) << 16)


#pragma pack(1)
typedef struct _CMD {
  uint8   id[4];      //[00] = cbw signature ("USBC" = 0x55,0x53,0x42,0x43)
  uint32  tag;        //[04] = cbw tag
  uint32  length;     //[08] = data transfer length
  uint8   flags;      //[0C] = transfer flags (0x80 on read, 0 otherwise)
  uint8   lun;        //[0D] = lun
  uint8   cdb_length; //[0E] = length of cmd data (commonly = 12)
  uint32  cdb[4];     //[0F] = 0x05=>info, 0x09=>boot flash, 0x08=>flash
} CMD, *LP_CMD;       //[1F]
#pragma pack()


void    device_set_cmd(LP_CMD cmd, CMD_ID cmd_id, uint32 length =0, uint16 offset =0);

HANDLE  device_open(char drive);
bool    device_close(HANDLE *hd);
long    device_send(HANDLE hd, void *data, uint32 size);
long    device_send_cmd(HANDLE hd, LP_CMD cmd, void *data =NULL, uint32 size =0);
long    device_send_cmd(HANDLE hd, CMD_ID cmd_id, void *data =NULL, uint32 size =0, uint16 offset =0);

bool    device_detect(char drive);
DWORD   device_scan();


#endif