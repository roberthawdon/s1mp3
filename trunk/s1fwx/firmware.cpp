#include "include.h"
#include "firmware.h"
#include "device.h"
#include "sum16.h"
#include "sum32.h"
#include "heads.h"


#define DEFAULT_FIRMWARE_SIZE 0x0600000   // 6MB
#define MAX_FIRMWARE_SIZE     0x4000000   //16MB
#define BREC_SIZE                0x4000   //16KB


HANDLE hDevice = INVALID_HANDLE_VALUE;


// -----------------------------------------------------------------------------------------------
char getkey()
{
  int key = _getch();
  if(key <= 0 || key >= 128) {key = 0; _getch();}
  return (char)key;
}


// -----------------------------------------------------------------------------------------------
bool disconnect()
{
  if(hDevice == INVALID_HANDLE_VALUE) return true;
  printf("disconnect from device...\n");

  Sleep(500);

  BYTE buf[CMD_MAX_TX];
  long ret = device_send_cmd(hDevice, CMD_SHUTDOWN, &buf, sizeof(buf));
  if(ret < 0) display_error(GetLastError());

  device_close(&hDevice);
  return true;
}


// -----------------------------------------------------------------------------------------------
bool connect(char drive, bool fmode=true)
{
  disconnect();
  printf("connect to device '%c:\\'...\n", drive);

  if(!device_detect(drive)) {
    printf("error: unknown device\n");
    return false;
  }

  hDevice = device_open(drive);
  if(hDevice == INVALID_HANDLE_VALUE) {display_error(GetLastError()); return false;}

  long ret;
  BYTE buf[16];

  printf("init device...\n");
  ret = device_send_cmd(hDevice, CMD_INIT, &buf, sizeof(buf));
  if(ret < 0) {display_error(GetLastError()); disconnect(); return false;}
  if(ret != 11 || strncmp((char*)buf, "ACTIONSUSBD", 11) != 0) {
    printf("error: unknown device\n");
    return false;
  }

  if(!fmode) return true;
  printf("set device to f-mode...");
  Sleep(3000);
  for(int max_delay=30; max_delay; max_delay--)
  {
    ret = device_send_cmd(hDevice, CMD_ENTER_FMODE, &buf, sizeof(buf));
    if(ret < 0) {printf("\n"); display_error(GetLastError()); disconnect();return false;}
    if(ret > 0 && buf[0] == 0xFF) {  //mode set?
      printf("\n");
      return true;
    }
    Sleep(1000);
    printf(".");
  }
  printf("\n");
  printf("error: failed to set f-mode, maybe write-lock is activated?\n");

  disconnect();
  return false;
}


// -----------------------------------------------------------------------------------------------
bool read_info(LP_ADFU_SYS_INFO info)
{
  if(!info) return false;
  printf("get info from device...\n");

  memset(info, 0, sizeof(ADFU_SYS_INFO));

  Sleep(500);

  long ret = device_send_cmd(hDevice, CMD_READ_SYSINFO, info, sizeof(ADFU_SYS_INFO));
  if(ret < 0) {delete info; display_error(GetLastError()); return false;}
  if(ret < sizeof(ADFU_SYS_INFO) || strncmp(info->id, ADFU_SYS_INFO_ID, sizeof(info->id)) != 0)
    {delete info; display_error(ERROR_READ_FAULT); return false;}

  char type[5]; memcpy(type, info->hw_scan.flash_type, 4); type[4] = 0;
  printf("  ic version = 0x%04X\n", info->hw_scan.ic_version);
  printf("  storage info = 0x%04X, 0x%04X, 0x%04X, 0x%04X\n", info->hw_scan.stginfo.storage_info[0], info->hw_scan.stginfo.storage_info[1], info->hw_scan.stginfo.storage_info[2], info->hw_scan.stginfo.storage_info[3]);
  printf("  device = '%s'\n", info->fw_scan.device);
  printf("  manufacturer = '%s'\n", info->fw_scan.manufacturer);
  printf("  bootflash type = '%s'\n", type);
  printf("  brom version = %d.%d.%d%d\n", (info->hw_scan.brom_version[0]>>4)&0xF, (info->hw_scan.brom_version[0])&0xF, (info->hw_scan.brom_version[1]>>4)&0xF, (info->hw_scan.brom_version[1])&0xF);
  printf("  firmware version = %d.%d.%d%d\n", (info->fw_scan.version>>4)&0xF, (info->fw_scan.version)&0xF, (info->fw_scan.version>>12)&0xF, (info->fw_scan.version>>8)&0xF);
  return true;
}


// -----------------------------------------------------------------------------------------------
// used by read_fw(...) only
DWORD read_from_flash(void *dest, uint32 size)
{
  uint8 buf[CMD_MAX_TX];

  Sleep(500);

  for(uint32 ofs=0; size > 0; ofs+=CMD_MAX_TX)
  {
    long ret = device_send_cmd(hDevice, CMD_READ_FLASH, &buf, CMD_MAX_TX, (uint16)(ofs >> 9));
    if(ret < 0) return GetLastError();
    if(ret < CMD_MAX_TX || (ofs == 0 && *((uint32*)buf) != FIRMWARE_ID)) return ERROR_READ_FAULT;

    if((ofs % (CMD_MAX_TX*3)) == 0) printf(".");
    if(size < CMD_MAX_TX) {memcpy(dest, buf, size); break;}    
    memcpy(dest, buf, CMD_MAX_TX);
    dest = (((uint8*)dest) + CMD_MAX_TX);
    size -= CMD_MAX_TX;
  }

  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
bool read_fw(uint8 **pbuf, uint32 *psize)
{
  if(pbuf) *pbuf = NULL;
  if(psize) *psize = 0;

  DWORD res;
  uint32 size = 0;

  // read firmware header and detect firmware size
  FW_HEAD fwh;
  printf("detect firmware size...");
  res = read_from_flash(&fwh, sizeof(FW_HEAD));
  printf("\n");
  if(res != ERROR_SUCCESS) {display_error(res); return false;}
  if(fwh.id != FIRMWARE_ID)
  {
    size = DEFAULT_FIRMWARE_SIZE;
    printf("warning: unknown firmware file\n", DEFAULT_FIRMWARE_SIZE);
  }
  else
  {
    for(uint32 i=0; i<FIRMWARE_ENTRIES; i++) if(fwh.entry[i].filename[0])
    {
      size_t _max = (((size_t)fwh.entry[i].fofs_s9)<<9) + (size_t)fwh.entry[i].fsize;
      if(_max > MAX_FIRMWARE_SIZE) {
        printf("warning: size overflow, firmware too big\n");
        size = MAX_FIRMWARE_SIZE;
        break;
      } else if(_max > size) size = (uint32)_max;
    }
    size = ((size+0x1ff) >> 9) << 9;
    if(size != 0) printf("  firmware size = 0x%08X\n", size);
    else {
      size = DEFAULT_FIRMWARE_SIZE;
      printf("warning: unknown firmware size, found no content\n", size);
    }
  }

  // read
  printf("read flash...");
  uint8 *buf = new uint8[size];
  if(!buf) {printf("\n"); display_error(ERROR_NOT_ENOUGH_MEMORY); return false;}
  res = read_from_flash(buf, size);
  printf("\n");
  if(res != ERROR_SUCCESS) {delete buf; display_error(res); return false;}

  // return
  if(pbuf) *pbuf = buf; else delete buf;
  if(psize) *psize = size;
  return true;
}


// -----------------------------------------------------------------------------------------------
bool read_brec(uint8 **pbuf)
{
  if(pbuf) *pbuf = NULL;
  printf("read boot record...\n");

  uint8 *buf = new uint8[BREC_SIZE];
  if(!buf) {display_error(ERROR_NOT_ENOUGH_MEMORY); return false;}
  memset(buf, 0, BREC_SIZE);

  Sleep(500);

  long ret = device_send_cmd(hDevice, CMD_READ_BREC, buf, BREC_SIZE);
  if(ret < 0) {delete buf; display_error(GetLastError()); return false;}
  if(ret < BREC_SIZE) {delete buf; display_error(ERROR_READ_FAULT); return false;}

  if(pbuf) *pbuf = buf; else delete buf;


  /* BETA CODE *//*
  printf("read boot record (ADFU part)...\n");

  #define BOOTREC2_SIZE 0x4000
  uint8 *buf2 = new uint8[BOOTREC2_SIZE];
  if(!buf2) {display_error(ERROR_NOT_ENOUGH_MEMORY); return false;}
  memset(buf2, 0, BOOTREC2_SIZE);

  Sleep(500);

  ret = device_send_cmd(hDevice, CMD_READ_ADFU, buf2, BOOTREC2_SIZE);
  if(ret < 0) {delete buf2; display_error(GetLastError()); return false;}
  if(ret < BOOTREC2_SIZE) {delete buf2; display_error(ERROR_READ_FAULT); return false;}

  dump(buf2, ret, false);

  FILE *fh = fopen("betadump.bin", "wb");
  if(!fh) {delete buf2; display_error(ERROR_WRITE_FAULT); return false;}
  fwrite(buf2, BOOTREC2_SIZE, 1, fh);
  fclose(fh);

  delete buf2;
  /* BETA CODE */


  return true;
}


// -----------------------------------------------------------------------------------------------
bool write_fw(uint8 *buf, uint32 size)
{
  printf("write firmware image...");

  if(!buf || !size)
  {
    printf("\n  no data to write\n");
    return true;
  }

  Sleep(500);

  for(uint32 ofs=0; 1; ofs+=CMD_MAX_TX)
  {
    long ret = device_send_cmd(hDevice, CMD_WRITE_FLASH, &buf[ofs], 
      (size >= CMD_MAX_TX)? CMD_MAX_TX : size, (uint16)(ofs >> 9));
    if(ret < 0) {printf("\n"); display_error(GetLastError()); return false;}
    if((ofs % (CMD_MAX_TX*3)) == 0) printf(".");    

    if(size < CMD_MAX_TX) break;
    size-=CMD_MAX_TX;
  }
  printf("\n");

  return true;
}


// -----------------------------------------------------------------------------------------------
bool write_brec(uint8 *buf)
{
  printf("write boot record...\n");

  if(!buf)
  {
    printf("  no data to write\n");
    return true;
  }

  Sleep(500);

  uint8 org[BREC_SIZE];
  long ret = device_send_cmd(hDevice, CMD_READ_BREC, &org, BREC_SIZE);
  if(ret < 0) {delete buf; display_error(GetLastError()); return false;}
  if(ret < BREC_SIZE) {delete buf; display_error(ERROR_READ_FAULT); return false;}

  if(memcmp(buf, &org, BREC_SIZE) == 0)
  {
    printf("  boot record already uptodate\n");
    return true;
  }

  Sleep(500);

  if(device_send_cmd(hDevice, CMD_WRITE_BREC, buf, BREC_SIZE) < 0)
    {delete buf; display_error(GetLastError()); return false;}

  return true;
}


// -----------------------------------------------------------------------------------------------
extern int list(char *filename);


// -----------------------------------------------------------------------------------------------
bool write_file(char *filename, LP_ADFU_SYS_INFO info, uint8 *brec, uint8 *fw, uint32 fw_size, bool repair)
{
  FW_HEAD_ENTRY repair_file[3];
  LP_FW_HEAD fwh = (LP_FW_HEAD)fw;

  // repair firmware file?
  if(repair)
  {
    memset(&repair_file, 0, sizeof(repair_file));

    if(fwh->id != FIRMWARE_ID) {
      printf("error: unknown firmware file, repairing file failed\n");
      return false;
    }

    // detect ADFUS.BIN
    for(int i=0; i<FIRMWARE_ENTRIES; i++) if(_strnicmp(fwh->entry[i].filename, "ADFUS   BIN", 11) == 0)
    {
      memcpy(&repair_file[0], &fwh->entry[i], sizeof(FW_HEAD_ENTRY));
      break;
    }

    // detect last entry
    int last = FIRMWARE_ENTRIES-1;
    for(; last>=0; last--) if(fwh->entry[last].filename[0]) break;
    if(last >= 0 && (_strnicmp(fwh->entry[last].filename, "ADFUF", 5) == 0 || _strnicmp(fwh->entry[last].filename, "FWSCF", 5) == 0))
    {
      memcpy(&repair_file[1], &fwh->entry[last], sizeof(FW_HEAD_ENTRY));
      memset(&fwh->entry[last], 0, sizeof(FW_HEAD_ENTRY));

      last--; //check another entry from the back
      if(last >= 0 && (_strnicmp(fwh->entry[last].filename, "ADFUF", 5) == 0 || _strnicmp(fwh->entry[last].filename, "FWSCF", 5) == 0))
      {
        memcpy(&repair_file[2], &fwh->entry[last], sizeof(FW_HEAD_ENTRY));
        memset(&fwh->entry[last], 0, sizeof(FW_HEAD_ENTRY));
      }
    }
    else
    {
      printf("error: repairing file failed\n");
      return false;
    }

    // calc new size
    fw_size = 0;
    for(uint32 i=0; i<FIRMWARE_ENTRIES; i++) if(fwh->entry[i].filename[0])
    {
      size_t _max = (((size_t)fwh->entry[i].fofs_s9)<<9) + (size_t)fwh->entry[i].fsize;
      if(_max > MAX_FIRMWARE_SIZE) {
        printf("warning: size overflow, firmware too big\n");
        fw_size = MAX_FIRMWARE_SIZE;
        break;
      } else if(_max > fw_size) fw_size = (uint32)_max;
    }
    fw_size = ((fw_size+0x1ff) >> 9) << 9;

    // repair fw-header checksum
    fwh->sum32 = calc_sum32(&fwh->entry, sizeof(FW_HEAD)-0x200);  // 32bit checksum across entries
    fwh->sum16 = calc_sum16(fwh, 0x1FE);                          // 16bit checksum across header
  }

  // create file
  printf("write to file '%s'...\n", filename);
  FILE *fh = fopen(filename, "wb");
  if(!fh) {display_error(GetLastError()); return false;}

  // fillup afi header
  AFI_HEAD afi;
  memset(&afi, 0, sizeof(afi));
  strcpy(afi.id, "AFI");
  memcpy(afi.version_id, &info->fw_scan.version, sizeof(afi.version_id));
  memcpy(&afi.vendor_id, "wiRe", 4);

  uint32 fofs = sizeof(afi);
  uint16 i = -1;

  if(repair) for(int j=0; j<3; j++) if(repair_file[j].filename[0]) //should we try to repair file?
  {
    i++;
    if(_strnicmp(repair_file[j].filename, "ADFUS", 5) == 0) {
      afi.entry[i].type = 'A';
      afi.entry[i].download_addr = AFI_ENTRY_DLADR_ADFUS;
    } else if(_strnicmp(repair_file[j].filename, "ADFUF", 5) == 0) {
      afi.entry[i].type = 'U';
      afi.entry[i].download_addr = AFI_ENTRY_DLADR_ADFU;
      memcpy(afi.entry[i].desc, info->hw_scan.flash_type, 4);
    } else if(_strnicmp(repair_file[j].filename, "FWSCF", 5) == 0) {
      afi.entry[i].type = 'F';
      afi.entry[i].download_addr = AFI_ENTRY_DLADR_FWSC;
      memcpy(afi.entry[i].desc, info->hw_scan.flash_type, 4);
    } else afi.entry[i].type = '?';

    memcpy(afi.entry[i].filename, repair_file[j].filename, 8);  
    memcpy(afi.entry[i].extension, repair_file[j].extension, 3);
    afi.entry[i].fofs = fofs;
    afi.entry[i].fsize = (uint32)repair_file[j].fsize;
    afi.entry[i].sum32 = repair_file[j].sum32;
    fofs += afi.entry[i].fsize;
  }

  i++;
  afi.entry[i].type = 'B';
  afi.entry[i].download_addr = AFI_ENTRY_DLADR_BREC;
  memcpy(afi.entry[i].desc, info->hw_scan.flash_type, 4);
  memcpy(afi.entry[i].filename, "BREC", 4);
  memcpy(afi.entry[i].filename+4, info->hw_scan.flash_type, 4);
  memcpy(afi.entry[i].extension, "BIN", 3);
  afi.entry[i].fofs = fofs;
  afi.entry[i].fsize = BREC_SIZE;
  afi.entry[i].sum32 = calc_sum32(brec, BREC_SIZE);
  fofs += afi.entry[i].fsize;

  i++;
  afi.entry[i].type = 'I';
  afi.entry[i].download_addr = AFI_ENTRY_DLADR_FW;
  //memcpy(afi.entry[i].desc, info->hw_scan.flash_type, 4);
  memcpy(afi.entry[i].filename, "FWIMAGE ", 8);  
  memcpy(afi.entry[i].extension, "FW ", 3);
  afi.entry[i].fofs = fofs;
  afi.entry[i].fsize = (uint32)fw_size;
  afi.entry[i].sum32 = calc_sum32(fw, fw_size);
  fofs += afi.entry[i].fsize;

  //---------------------------------------------------------------------------------
  //this file entry isnt compatible to the original firmware file format
  //its only used to retrieve the complete informations about the current firmware
  //---------------------------------------------------------------------------------
  i++;
  afi.entry[i].type = ' ';
  afi.entry[i].download_addr = 0;
  //memcpy(afi.entry[i].desc, info->hw_scan.flash_type, 4);
  memcpy(afi.entry[i].filename, "SYSINFO ", 8);  
  memcpy(afi.entry[i].extension, "BIN", 3);
  afi.entry[i].fofs = fofs;
  afi.entry[i].fsize = (uint32)sizeof(ADFU_SYS_INFO);
  afi.entry[i].sum32 = calc_sum32(info, sizeof(ADFU_SYS_INFO));
  fofs += afi.entry[i].fsize;
  //---------------------------------------------------------------------------------

  // calc afi checksum
  afi.eoh.sum32 = calc_sum32(&afi, sizeof(AFI_HEAD)-4);

  // write file
  if(fwrite(&afi, sizeof(afi), 1, fh) != 1) {display_error(GetLastError()); fclose(fh); return false;}
  if(repair) for(int j=0; j<3; j++) if(repair_file[j].filename[0]) //should we try to repair file?
  {
    if(fwrite(&fw[((size_t)repair_file[j].fofs_s9)<<9], repair_file[j].fsize, 1, fh) != 1)
      {display_error(GetLastError()); fclose(fh); return false;}
  }
  if(fwrite(brec, BREC_SIZE, 1, fh) != 1) {display_error(GetLastError()); fclose(fh); return false;}
  if(fwrite(fw, fw_size, 1, fh) != 1) {display_error(GetLastError()); fclose(fh); return false;}

  //---------------------------------------------------------------------------------
  //same for this line of code...
  if(fwrite(info, sizeof(ADFU_SYS_INFO), 1, fh) != 1) {display_error(GetLastError()); fclose(fh); return false;}
  //---------------------------------------------------------------------------------

  fclose(fh);
  list(filename);
  if(!repair) printf("\n");
  return true;
}




// -----------------------------------------------------------------------------------------------
bool legal()
{
  printf(
    "\n"\
    "-------------------------------------------------------------------------------\n"\
    "NO WARRANTY NOTE\n"\
    "\n"\
    "BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY FOR THE\n"\
    "PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE\n"\
    "STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE\n"\
    "PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,\n"\
    "INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND\n"\
    "FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND\n"\
    "PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM CAUSE DAMAGE TO\n"\
    "ANY KIND OF DEVICE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR\n"\
    "OR CORRECTION.\n"\
    "\n"\
    "IF YOU DONT AGREE TO THIS TERMS THEN PRESS ESC NOW!\n"\
    "-------------------------------------------------------------------------------\n"\
    "\n"
  );
  return(getkey() != 27);
}


// -----------------------------------------------------------------------------------------------
int firmware_repair(char *filename)
{
  int res;
  printf("repair dumped firmware file...\n");

  // read inputfile
  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();
  struct stat fs;
  if(stat(filename, &fs) != 0) {res = GetLastError(); fclose(fh); return res;}
  uint8 *file = new uint8[fs.st_size];
  if(!file) {fclose(fh); return ERROR_NOT_ENOUGH_MEMORY;}
  if(fread(file, fs.st_size, 1, fh) != 1)  {fclose(fh); delete file; return ERROR_READ_FAULT;}
  fclose(fh);

  // verify head
  LP_AFI_HEAD afi = (LP_AFI_HEAD)file;
  if(strcmp("AFI", afi->id) != 0) {delete file; return ERROR_INVALID_DATA;}

  // backup file
  char *bak_name = new char[strlen(filename)+1];
  if(!bak_name) {delete file; return ERROR_NOT_ENOUGH_MEMORY;}
  strcpy(bak_name, filename);
  bak_name[strlen(bak_name)-1] = '$';   //generate new filename
  printf("backup file to '%s'...\n", bak_name);
  fh = fopen(bak_name, "wb");
  if(!fh) {res = GetLastError(); delete file; return res;}
  if(fwrite(file, fs.st_size, 1, fh) != 1)  {fclose(fh); delete file; return ERROR_WRITE_FAULT;}
  fclose(fh);

  //load sys info from firmware-file
  ADFU_SYS_INFO info;
  memset(&info, 0, sizeof(info));
  for(int i=0; i<AFI_ENTRIES; i++) if(strncmp(afi->entry[i].filename, "SYSINFO BIN", 11) == 0)
  {
    if(afi->entry[i].fsize >= sizeof(ADFU_SYS_INFO)) 
    {
      memcpy(&info, &file[ afi->entry[i].fofs ], sizeof(info));
      break;
    }
  }

  //fill-in pseudo-default sys-info values if no valid firmware info was loaded
  if(strncmp(info.id, ADFU_SYS_INFO_ID, 8) != 0)
  {
    memcpy(&info.fw_scan.version, afi->version_id, 2);
    memcpy(info.hw_scan.flash_type, "XXXX", 4);
    for(int i=0; i<AFI_ENTRIES; i++) if(afi->entry[i].type == 'B')
    {
      memcpy(info.hw_scan.flash_type, afi->entry[i].filename+4, 4);
      break;
    }    
  }

  // repair file
  printf("try to repair firmware...\n");
  uint8 *brec = NULL;
  for(int i=0; i<AFI_ENTRIES; i++) if(afi->entry[i].type == 'B')
  {
    brec = &file[afi->entry[i].fofs];
    break;
  }
  uint8 *fw = NULL;
  for(int i=0; i<AFI_ENTRIES; i++) if(afi->entry[i].type == 'I')
  {
    fw = &file[afi->entry[i].fofs];
    break;
  }  
  if(!brec || !fw) {delete file; return ERROR_INVALID_DATA;}
  write_file(filename, &info, brec, fw, 0, true);
  delete file;
  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int firmware_dump(char *filename)
{
  printf("dump firmware from device...\n");
  if(!legal()) return ERROR_SUCCESS;

  printf("scan for devices...\n");
  DWORD dwDevices = device_scan();
  printf("%s", (dwDevices)?"detected drives:":"no device found");
  if(dwDevices) for(char d='a'; d<='z'; d++) if((dwDevices>>(d-'a'))&1) printf(" <%c>", d);
  printf("\n\n");

  char k = '?';
  for(;;) {
    if(k == '?') {
      printf("<a>..<z> select device\n<.> scan for devices\n<?> display help\n<~> exit program\n");
    } else if(k == '.') {
      printf("scan for devices...\n");
      dwDevices = device_scan();
      printf("%s", (dwDevices)?"detected drives:":"no device found");
      if(dwDevices) for(char d='a'; d<='z'; d++) if((dwDevices>>(d-'a'))&1) printf(" <%c>", d);
      printf("\n");
    }
    else if(k >= 'a' && k <= 'z' && (dwDevices>>(k-'a'))&1) {k+='A'-'a'; break;}
    else if(k >= 'A' && k <= 'Z' && (dwDevices>>(k-'A'))&1) break;
    else if(k == '~') return ERROR_SUCCESS;
    else printf("bad command\n");

    printf("\n$"); k = getkey(); printf("%c\n", k);
  }

  if(connect(k)) {    
    ADFU_SYS_INFO sys_info;
    uint8 *brec=NULL, *fw=NULL;
    uint32 fw_size;

    if(read_info(&sys_info) && read_brec(&brec) && read_fw(&fw, &fw_size) && write_file(filename, &sys_info, brec, fw, fw_size, false) && disconnect())
      printf("success.\n");

    if(fw) delete fw;
    if(brec) delete brec;
    disconnect();
  }

  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int firmware_upload(char *filename)
{
  int res;
  printf("upload firmware to device...\n");

  // read inputfile
  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();
  struct stat fs;
  if(stat(filename, &fs) != 0) {res = GetLastError(); fclose(fh); return res;}
  uint8 *file = new uint8[fs.st_size];
  if(!file) {fclose(fh); return ERROR_NOT_ENOUGH_MEMORY;}
  if(fread(file, fs.st_size, 1, fh) != 1)  {fclose(fh); delete file; return ERROR_READ_FAULT;}
  fclose(fh);

  // verify head
  LP_AFI_HEAD afi = (LP_AFI_HEAD)file;
  if(strcmp("AFI", afi->id) != 0) {delete file; return ERROR_INVALID_DATA;}
  if(afi->entry[0].type != 'B' || afi->entry[1].type != 'I' || afi->entry[2].type != ' ' || afi->entry[3].filename[0] != 0)
  {
    delete file;
    printf("error: original or repaired firmware image, only dumped and unrepaired firmware images are allowed\n");
    return ERROR_SUCCESS;
  }

  // upload firmware
  if(!legal()) return ERROR_SUCCESS;

  printf("scan for devices...\n");
  DWORD dwDevices = device_scan();
  printf("%s", (dwDevices)?"detected drives:":"no device found");
  if(dwDevices) for(char d='a'; d<='z'; d++) if((dwDevices>>(d-'a'))&1) printf(" <%c>", d);
  printf("\n\n");

  char k = '?';
  for(;;) {
    if(k == '?') {
      printf("<a>..<z> select device\n<.> scan for devices\n<?> display help\n<~> exit program\n");
    } else if(k == '.') {
      printf("scan for devices...\n");
      dwDevices = device_scan();
      printf("%s", (dwDevices)?"detected drives:":"no device found");
      if(dwDevices) for(char d='a'; d<='z'; d++) if((dwDevices>>(d-'a'))&1) printf(" <%c>", d);
      printf("\n");
    }
    else if(k >= 'a' && k <= 'z' && (dwDevices>>(k-'a'))&1) {k+='A'-'a'; break;}
    else if(k >= 'A' && k <= 'Z' && (dwDevices>>(k-'A'))&1) break;
    else if(k == '~') {delete file; return ERROR_SUCCESS;}
    else printf("bad command\n");

    printf("\n$"); k = getkey(); printf("%c\n", k);
  }

  if(connect(k)) {    
    ADFU_SYS_INFO sys_info;
    if(read_info(&sys_info) && memcmp(afi->entry[0].desc, sys_info.hw_scan.flash_type, 4) != 0)
    {
      printf("error: firmware file is not compatible, abort command\n");
    }
    else
    {
      if(write_brec(&file[ afi->entry[0].fofs ]) && write_fw(&file[ afi->entry[1].fofs ], afi->entry[1].fsize) 
        && disconnect()) printf("success.\n");
    }

    disconnect();
  }

  delete file;
  return ERROR_SUCCESS;
}
