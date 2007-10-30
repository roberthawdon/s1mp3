#include "include.h"
#include "device.h"
#include "heads.h"


// -----------------------------------------------------------------------------------------------
void device_set_cmd(LP_CMD cmd, CMD_ID cmd_id, uint32 length, uint16 offset)
{
  memset(cmd, 0, sizeof(CMD));
  memcpy(cmd->id, CMD_HEADER_ID, sizeof(cmd->id));

  switch(cmd_id)
  {
  case CMD_INIT:
    cmd->flags  = CMD_FLAG_READ;
    cmd->cdb[0] = _CMD_INIT;
    cmd->length = 0x0B;
    cmd->cdb_length = 12;
    break;

  case CMD_ENTER_FMODE:
    cmd->flags  = CMD_FLAG_READ;
    cmd->cdb[0] = _CMD_ENTER_FMODE;
    cmd->length = 1;
    cmd->cdb_length = 12;
    break;

  case CMD_SHUTDOWN:
    cmd->cdb[0] = _CMD_SHUTDOWN;
    cmd->cdb_length = 12;
    break;

  case CMD_READ_SYSINFO:
    cmd->flags  = CMD_FLAG_READ;
    cmd->cdb[0] = _CMD_READ_MEM(0x0800);
    cmd->cdb[1] = (sizeof(ADFU_SYS_INFO)<<24) + (8<<16);
    cmd->length = sizeof(ADFU_SYS_INFO);
    cmd->cdb_length = 12;
    break;

  case CMD_READ_BREC:
    cmd->flags  = CMD_FLAG_READ;
    cmd->cdb[0] = _CMD_READ_BREC(offset);
    cmd->cdb[1] = ((length>>9)<<24) + (7<<16);
    cmd->length = length;
    cmd->cdb_length = 12;
    break;

  case CMD_WRITE_BREC:
    cmd->cdb[0] = _CMD_WRITE_BREC(offset);
    cmd->cdb[1] = ((length>>9)<<24);
    cmd->length = length;
    cmd->cdb_length = 12;
    break;

  case CMD_READ_FLASH:
    cmd->flags  = CMD_FLAG_READ;
    cmd->cdb[0] = _CMD_READ_FLASH(offset);
    cmd->cdb[1] = ((length>>9)<<24) + (0<<16);
    cmd->length = length;
    cmd->cdb_length = 12;
    break;

  case CMD_WRITE_FLASH:
    cmd->cdb[0] = _CMD_WRITE_FLASH(offset);
    cmd->cdb[1] = ((length>>9)<<24);
    cmd->length = length;
    cmd->cdb_length = 12;
    break;
  }
}




// -----------------------------------------------------------------------------------------------
HANDLE device_open(char drive)
{
  char device[16];
  sprintf(device, "\\\\.\\%c:", drive);
  return CreateFile(device, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
}


// -----------------------------------------------------------------------------------------------
bool device_close(HANDLE *hd)
{
  if(!hd) return false;
  if(*hd == INVALID_HANDLE_VALUE) return true;
  CloseHandle(*hd);
  *hd = INVALID_HANDLE_VALUE;
  return true;
}


// -----------------------------------------------------------------------------------------------
long device_send(HANDLE hd, void *data, uint32 size)
{
  DWORD ret;
  if(!DeviceIoControl(hd, 0x4D004, data, size, data, size, &ret, NULL)) return -1;
  return (long)ret;
}


// -----------------------------------------------------------------------------------------------
long device_send_cmd(HANDLE hd, LP_CMD cmd, void *data, uint32 size)
{
  if(!cmd) return -1;
  if(size > cmd->length) size = cmd->length;
  if(size > CMD_MAX_TX) size = CMD_MAX_TX;

  struct {
    struct {
      uint16 Length;
      uint8  ScsiStatus;
      uint8  PathId;
      uint8  TargetId;
      uint8  Lun;
      uint8  CdbLength;
      uint8  SenseInfoLength;
      uint8  DataIn;
      uint32 DataTransferLength;
      uint32 TimeOutValue;
      uint32 DataBufferOffset;
      uint32 SenseInfoOffset;
      uint8  Cdb[16];
    } header;
    uint8 sense[4];
    uint8 data[CMD_MAX_TX];
  } iob;

  memset(&iob, 0, sizeof(iob));
  iob.header.Length = sizeof(iob.header);
  iob.header.TargetId = 1;
  iob.header.DataTransferLength = size;
  iob.header.TimeOutValue = 3;
  iob.header.DataBufferOffset = (ULONG)((ULONGLONG)&iob.data - (ULONGLONG)&iob);
  iob.header.SenseInfoOffset = (ULONG)((ULONGLONG)&iob.sense - (ULONGLONG)&iob);
  if(cmd->flags & CMD_FLAG_READ) iob.header.DataIn = 1;
  else if(data != NULL && size > 0) memcpy(&iob.data, data, size);

  if(cmd->cdb_length > 16) cmd->cdb_length = 16;
  if(cmd->cdb_length > 0)
  {
    iob.header.CdbLength = cmd->cdb_length;
    memcpy(iob.header.Cdb, &cmd->cdb, cmd->cdb_length);
  }
  if(device_send(hd, &iob, sizeof(iob)) < 0) return -1;
  if(data != NULL && size > 0) memcpy(data, iob.data, size);
  
  return iob.header.DataTransferLength;
}


// -----------------------------------------------------------------------------------------------
long device_send_cmd(HANDLE hd, CMD_ID cmd_id, void *data, uint32 size, uint16 offset)
{
  CMD cmd;
  device_set_cmd(&cmd, cmd_id, size, offset);
  return device_send_cmd(hd, &cmd, data, size);
}




// -----------------------------------------------------------------------------------------------
bool device_detect(char drive)
{
  char device[16];

  sprintf(device, "%c:", drive);  
  if(GetDriveType(device) != 2) return false;

  sprintf(device, "\\\\.\\%c:", drive);  
  HANDLE hd = CreateFile(device, 0, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if(hd == INVALID_HANDLE_VALUE) return false;

  DWORD ret;
  uint8 buf[256];
  if(!DeviceIoControl(hd, 0x70000, NULL, 0, buf, sizeof(buf), &ret, NULL));
  else if(ret) {
    CloseHandle(hd);
    if(((DWORD*)buf)[2] != 0x0000000B) return false;
  } else {
    CloseHandle(hd);
    return false;
  }

  hd = device_open(drive);
  if(hd == INVALID_HANDLE_VALUE) return false;
  if(device_send_cmd(hd, CMD_INIT, &device, sizeof(device)) != 11) {
    device_close(&hd);
    return false;
  }
  device_close(&hd);
  return(strncmp(device, "ACTIONSUSBD", 11) == 0);
}


// -----------------------------------------------------------------------------------------------
// returns bit mask
// bit=0 => not detected, bit=1 => detected
// offset 0 => A:, 1 => B:, 2 => C:, ... 
DWORD device_scan()
{
  DWORD dwDevices = 0;
  DWORD dwDrives = GetLogicalDrives();
  for(char ofs=0, drive='A'; drive <= 'Z'; drive++,ofs++,dwDrives >>= 1)
    if((dwDrives & 0x1) && device_detect(drive)) dwDevices |= (1 << ofs);
  return dwDevices;
}