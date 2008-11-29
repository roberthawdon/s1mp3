const char szAppInfo[] = 

// -----------------------------------------------------------------------------------------------
"s1fwx v3.3 - copyright (c)2005-2008 wiRe (http://www.s1mp3.de/)\n";
// -----------------------------------------------------------------------------------------------
// v3.3
// -wasn't able to extract empty files without errors
// v3.2
// -refactored firmware extraction routine
// -implement firmware upload function
// v3.1
// -improved firmware repair function
// v3.0
// -also extracts SYS-INFO from players
// v2.6
// -new script command "DATE"
// -removed some bugs inside archive function
// -firmware-read autodetects firmware size, no buffer-overrun anymore
// -----------------------------------------------------------------------------------------------

#include "include.h"
#include "decrypt.h"
#include "sum16.h"
#include "sum32.h"
#include "crc32.h"
#include "md5.h"
#include "firmware.h"
#include "parser.h"

#include "heads.h"


// -----------------------------------------------------------------------------------------------
enum FILETYPE {
  FILETYPE_ENC,
  FILETYPE_FW,
  FILETYPE_AFI,
  FILETYPE_UNKNOWN=127,
  FILETYPE_OVERWRITE=128
};

uint8 g_filetype = FILETYPE_UNKNOWN;




// -----------------------------------------------------------------------------------------------
char trunc_buf[256];
char *trunc(char *str, size_t n = 0)
{
  ZeroMemory(trunc_buf, sizeof(trunc_buf));
  if(!n) n = strlen(str);
  if(n >= sizeof(trunc_buf)) n = sizeof(trunc_buf)-1;
  strncpy(trunc_buf, str, n);
  str = trunc_buf;
  char *end = str + n-1;
  while(*str == ' ' || *str == '\t') str++;
  while((end >= str) && (*end == ' ' || *end == '\t')) *end-- = 0;
  return str;
}


// -----------------------------------------------------------------------------------------------
char fsize_buf[256];
char *fsize(uint32 fsize)
{
  if(fsize == 1) sprintf(fsize_buf, "1byte");
  else if(fsize < 1024) sprintf(fsize_buf, "%ubytes", fsize);
  else {
    if(fsize < 1048576) {
      if(fsize % 1024 == 0) sprintf(fsize_buf, "%ukb", fsize/1024);
      else if((fsize*10) % 1024 == 0) sprintf(fsize_buf, "%.01fkb", fsize/1024.0);
      else sprintf(fsize_buf, "%.02fkb", fsize/1024.0);
    } else {
      if(fsize % 1048576 == 0) sprintf(fsize_buf, "%umb", fsize/1048576);
      else if((fsize*10) % 1048576 == 0) sprintf(fsize_buf, "%.01fmb", fsize/1048576.0);
      else sprintf(fsize_buf, "%.02fmb", fsize/1048576.0);
    }
  }
  return fsize_buf;
}


// -----------------------------------------------------------------------------------------------
int display_error(int result)
{
  if(result == ERROR_SUCCESS) {
    fprintf(stderr, "success\n");
    return result;
  }

  LPVOID pTextBuffer = NULL;
  if(FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, 
    NULL, (DWORD)result, 0, (LPTSTR)&pTextBuffer, 0, NULL) != 0)
  {
    fprintf(stderr, "error: %s", (LPCSTR)pTextBuffer);
    LocalFree(pTextBuffer);
  }
  else
  {    
    fprintf(stderr, "error: unknown error (0x%04X)\n", result);
  }
  return result;
}


// -----------------------------------------------------------------------------------------------
int detect_filetype(char *filename, bool fFileExists = true)
{
  if(g_filetype != FILETYPE_UNKNOWN) return(g_filetype);

  char *ext = strrchr(filename, ':');
  if(ext) {
    if(_stricmp(ext+1, "afi") == 0) {*ext = 0; return(g_filetype=FILETYPE_AFI|FILETYPE_OVERWRITE);}
    else if(_stricmp(ext+1, "fw") == 0) {*ext = 0; return(g_filetype=FILETYPE_FW|FILETYPE_OVERWRITE);}
    else if(_stricmp(ext+1, "enc") == 0) {*ext = 0; return(g_filetype=FILETYPE_ENC|FILETYPE_OVERWRITE);}
  }

  if(!fFileExists && g_filetype == FILETYPE_UNKNOWN) return FILETYPE_UNKNOWN;

  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();
  union {
    char    str[16];
    uint8   db[16];
    uint16  dh[8];
    uint32  dw[4];
  } id;
  if(fread(&id, sizeof(id), 1, fh) != 1) {
    fclose(fh);
    return ERROR_READ_FAULT;
  }
  fclose(fh);

  if(strncmp("Enciphered", id.str, 10)==0) return(g_filetype=FILETYPE_ENC);
  else if(strcmp(AFI_ID, id.str) == 0) return(g_filetype=FILETYPE_AFI);
  else if(id.dw[0] == FIRMWARE_ID) return(g_filetype=FILETYPE_FW);
  else return ERROR_INVALID_DATA;
}


// -----------------------------------------------------------------------------------------------
int display_fileinfo(char *filename)
{
  md5_context ctx;
  uint8 md5[16];
  uint8 buf[512];
  uint32 len;

  printf("\n---[ info ]--------------------------------------------------------------------\n");
  int ret = detect_filetype(filename);  //to correct filename, removes :afi or :fw
  printf("filename = '%s'\nfiletype = ", filename);
  if(ret<0 && ret != ERROR_INVALID_DATA) return ret;
  switch(ret&FILETYPE_UNKNOWN) {
    case FILETYPE_ENC: printf("encoded"); break;
    case FILETYPE_AFI: printf("afi"); break;
    case FILETYPE_FW: printf("fw"); break;
    default: printf("unknown");
  }
  if(ret&FILETYPE_OVERWRITE) printf(" (overwritten)");
  printf("\n");

  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();
  md5_starts(&ctx);
  for(;!feof(fh);) {
    len = (uint32)fread(buf, 1, sizeof(buf), fh);
    if(len > 0) md5_update(&ctx, buf, len);
  }
  md5_finish(&ctx, md5);
  printf("md5 = ");
  for(len=0; len<16; len++) printf("%02X", md5[len]);
  printf("\n");
  /*fseek(fh, 32, SEEK_SET);
  uint32 sum32 = calc_sum32(fh);
  if(sum32 != 0) printf("checksum = 0x%08X\n", sum32);*/
  fclose(fh);

  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int decrypt(char *filename)
{
  display_fileinfo(filename);
  printf("decrypting file...\n");

  int ret = ERROR_SUCCESS;

  struct stat fs;
  if(stat(filename, &fs) != 0) return GetLastError();
  if(fs.st_size <= 10) return ERROR_BAD_FORMAT;
  FILE *fh_in = fopen(filename, "rb");
  if(!fh_in) return GetLastError();  

  uint8 *buf_in = new uint8[fs.st_size];
  if(!buf_in) ret = ERROR_NOT_ENOUGH_MEMORY;
  else {
    uint8 *buf_out = new uint8[fs.st_size];
    if(!buf_out) ret = ERROR_NOT_ENOUGH_MEMORY;
    else {
      if(fread(buf_in, fs.st_size, 1, fh_in) != 1) ret = ERROR_READ_FAULT;
      else if(strncmp((char*)buf_in, "Enciphered", 10) != 0) ret = ERROR_BAD_FORMAT;
      else {
        fs.st_size -= 10;
        fs.st_size = (fs.st_size >> 9) << 9;

        char key[32];
        extract_key(key, buf_in, fs.st_size, buf_out);
        key[20] = 0;
        printf("key = '%s'\n", key);
        memcpy(buf_in, buf_out, fs.st_size);

        if(filename[0]) filename[strlen(filename)-1] = '~';   //generate new filename
        FILE *fh_out = fopen(filename, "wb");
        if(!fh_out) ret = GetLastError();
        else {

          memset(buf_out, 0, fs.st_size);
          decrypt(buf_out, buf_in, fs.st_size, key, (uint32)strlen(key));
          if(fwrite(buf_out, fs.st_size, 1, fh_out) != 1) ret = ERROR_WRITE_FAULT;

        }
        fclose(fh_out);
        if(ret != ERROR_SUCCESS) DeleteFile(filename);
      }
      delete buf_out;
    }
    delete buf_in;
  }
  fclose(fh_in);
  if(ret != ERROR_SUCCESS) return ret;

  g_filetype = FILETYPE_UNKNOWN;
  ret = detect_filetype(filename);
  return (ret<0)?ret:ERROR_SUCCESS;
}




// -----------------------------------------------------------------------------------------------
int display_afi(char *filename)
{
  display_fileinfo(filename);

  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();
  AFI_HEAD afi;
  if(fread(&afi, sizeof(afi), 1, fh) != 1) {
    fclose(fh);
    return ERROR_READ_FAULT;
  }

  printf("\n---[ info:afi ]----------------------------------------------------------------\n");
  if(afi.eoh.sum32 != calc_sum32(&afi, sizeof(AFI_HEAD)-4)) 
    printf("invalid checksum, file header maybe damaged\n");
  printf("file id = '%c%c%c%c'%s\n",
    afi.id[0], afi.id[1], afi.id[2], afi.id[3], (strcmp(AFI_ID, afi.id)==0)?"":" (unknown)");
  printf("vendor id = 0x%04X\n", afi.vendor_id);
  printf("product id = 0x%04X\n", afi.product_id);
  printf("version = %u.%u.%02X\n", afi.version_id[0]>>4, afi.version_id[0]&15, afi.version_id[1]);
  printf("date = %02X/%02X/%02X%02X\n", afi.date.month, afi.date.day, afi.date.year[0], afi.date.year[1]);

  printf("\n+---filename---+ +---size---+ +-type-+ +---+ +--chksum--+\n");
  for(int n=0; n<AFI_ENTRIES; n++) if(afi.entry[n].filename[0]) {
    int x=0;
    x += printf("| %s", trunc(afi.entry[n].filename, 8));
    x += printf(".%s", trunc(afi.entry[n].extension, 3));
    for(; x<15; x++) printf(" ");
    x += printf("| | %s", fsize(afi.entry[n].fsize));
    for(; x<28; x++) printf(" ");
    printf("| | %c%c%c%c ", afi.entry[n].desc[0], afi.entry[n].desc[1], afi.entry[n].desc[2], afi.entry[n].desc[3]);    
    printf("| | %c ", afi.entry[n].type);
    uint32 crc32 = calc_crc32(fh, afi.entry[n].fofs, afi.entry[n].fsize);
    uint32 sum32 = calc_sum32(fh, afi.entry[n].fofs, afi.entry[n].fsize);
    printf("| | %08X |", crc32);
    /*if(afi.entry[n].sum32 != 0)*/ if(sum32 != afi.entry[n].sum32) printf("  <- invalid checksum");
    printf("\n");
  }
  printf("+--------------+ +----------+ +------+ +---+ +----------+\n");

  fclose(fh);
  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int display_fw(char *filename)
{
  display_fileinfo(filename);

  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();
  FW_HEAD fw;
  if(fread(&fw, sizeof(fw), 1, fh) != 1) {
    fclose(fh);
    return ERROR_READ_FAULT;
  }

  printf("\n---[ info:fw ]-----------------------------------------------------------------\n");
  if(fw.sum16 != calc_sum16(&fw, 0x1FE) || fw.sum32 != calc_sum32(fh, 0x200, sizeof(FW_HEAD)-0x200))
    printf("invalid checksum, file header maybe damaged\n");
  printf("file id = 0x%08X%s\n", fw.id, (fw.id==FIRMWARE_ID)?"":" (unknown)");
  printf("vendor id = 0x%04X\n", fw.vendor_id);
  printf("product id = 0x%04X\n", fw.product_id);
  printf("version = %u.%u.%02X\n", fw.version_id[0]>>4, fw.version_id[0]&15, fw.version_id[1]);
  printf("date = %02X/%02X/%02X%02X\n", fw.date.month, fw.date.day, fw.date.year[0], fw.date.year[1]);
  printf("info = '%s'\n", trunc(fw.info.global, 32));
  printf("manufacturer = '%s'\n", trunc(fw.info.manufacturer, 32));
  printf("device name = '%s'\n", trunc(fw.info.device, 32));
  //printf("usb desc = '%s'\n", trunc(fw.usb.desc, 23));
  printf("usb attr = '%s'\n", trunc(fw.usb.attr, 8));
  printf("usb ident = '%s'\n", trunc(fw.usb.ident, 16));
  printf("usb rev = '%s'\n", trunc(fw.usb.rev, 4));
  //print comval entries
  printf("rtc rate = 0x%02X\n", fw.comval.rtc_rate);
  printf("display contrast = %u\n", fw.comval.display_contrast);
  printf("light time = %u\n", fw.comval.light_time);
  printf("standby time = %u\n", fw.comval.standby_time);
  printf("sleep time = %u\n", fw.comval.sleep_time);
  printf("language = ");
  const char *language[] = {"simple chinese", "english", "traditional chinese"};
  if(fw.comval.language_id>2) printf("unknown (0x%02X)\n", fw.comval.language_id);
    else printf("%s\n", language[fw.comval.language_id]);
  printf("replay mode = 0x%02X\n", fw.comval.replay_mode);
  printf("online mode = 0x%02X\n", fw.comval.online_mode);
  printf("battery type = ");
  const char *battery[] = {"1.5V alkaline", "1.2V Ni-H", "3.6V li-polymer"};
  if(fw.comval.battery_type>2) printf("unknown (0x%02X)\n", fw.comval.battery_type);
    else printf("%s\n", battery[fw.comval.battery_type]);
  printf("radio/fm support = %s\n", (fw.comval.radio_disable == 0)?"yes":"no");

  printf("\n+---filename---+ +---size---+ +--chksum--+\n");
  for(int n=0; n<AFI_ENTRIES; n++) if(fw.entry[n].filename[0]) {
    int x=0;
    x += printf("| %s", trunc(fw.entry[n].filename, 8));
    x += printf(".%s", trunc(fw.entry[n].extension, 3));
    for(; x<15; x++) printf(" ");
    x += printf("| | %s", fsize(fw.entry[n].fsize));
    for(; x<28; x++) printf(" ");
    uint32 crc32 = calc_crc32(fh, ((size_t)fw.entry[n].fofs_s9)<<9, fw.entry[n].fsize);
    uint32 sum32 = calc_sum32(fh, ((size_t)fw.entry[n].fofs_s9)<<9, fw.entry[n].fsize);
    printf("| | %08X |", crc32);
    /*if(afi.entry[n].sum32 != 0)*/ if(sum32 != fw.entry[n].sum32) printf("  <- invalid checksum");
    printf("\n");
  }
  printf("+--------------+ +----------+ +----------+\n");

  fclose(fh);
  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int display(char *filename)
{
  int ret = detect_filetype(filename);
  switch(ret&FILETYPE_UNKNOWN) {
    case FILETYPE_FW:  return display_fw(filename);
    case FILETYPE_AFI: return display_afi(filename);
    case FILETYPE_ENC: ret = decrypt(filename); if(ret == ERROR_SUCCESS) return display(filename);
    case ERROR_INVALID_DATA: return display_fileinfo(filename);
    default: return ret;
  }
}




// -----------------------------------------------------------------------------------------------
int extract_to_file(char *filename, FILE *fh_src, size_t fofs, size_t fsize, uint32 *sum32p/*=NULL*/)
{
  // by converting fofs to long we support only files up to 2GB!
  if(fseek(fh_src, (long)fofs, SEEK_SET) != 0) return GetLastError();
  if(sum32p) *sum32p = 0;

  FILE *fh = fopen(filename, "wb");
  if(!fh) return GetLastError();   

  int ret = ERROR_SUCCESS;
  if(fsize > 0)
  {
    uint8 *buf = new uint8[fsize];
    if(!buf) ret = ERROR_NOT_ENOUGH_MEMORY;
    else {
    
      if(fread(buf, fsize, 1, fh_src) != 1) ret = ERROR_READ_FAULT;
      if(fwrite(buf, fsize, 1, fh) != 1) ret = ERROR_WRITE_FAULT;
      if(sum32p != NULL) *sum32p = calc_sum32(buf, fsize);

      delete buf;
    }
  }

  fclose(fh);
  return ret;
}


// -----------------------------------------------------------------------------------------------
int extract_from_file(char *filename, FILE *fh_dest, uint32 *fsizep, uint32 *sum32p)
{
  struct stat fs;
  if(stat(filename, &fs) != 0) return GetLastError();
  if(fsizep) *fsizep = (uint32)fs.st_size;
  if(sum32p) *sum32p = 0;

  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();

  int ret = ERROR_SUCCESS;
  if(fs.st_size > 0)
  {
    uint8 *buf = new uint8[fs.st_size];
    if(!buf) ret = ERROR_NOT_ENOUGH_MEMORY;
    else {
    
      if(fread(buf, fs.st_size, 1, fh) != 1) ret = ERROR_READ_FAULT;
      if(fwrite(buf, fs.st_size, 1, fh_dest) != 1) ret = ERROR_WRITE_FAULT;
      if(sum32p) *sum32p = calc_sum32(buf, fs.st_size);

      delete buf;
    }
  }

  fclose(fh);
  return ret;
}




// -----------------------------------------------------------------------------------------------
int extract_afi(char *filename)
{
  display_fileinfo(filename);

  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();

  AFI_HEAD afi;
  if(fread(&afi, sizeof(afi), 1, fh) != 1) {
    fclose(fh);
    return ERROR_READ_FAULT;
  }

  printf("\n---[ extract:afi ]-------------------------------------------------------------\n");
  int cnt=0, err=0, wrn=0;
  if(afi.eoh.sum32 != calc_sum32(&afi, sizeof(AFI_HEAD)-4)) {
    wrn++;
    printf("warning: invalid checksum, file header maybe damaged\n");
  }
  for(int n=0; n<AFI_ENTRIES; n++) if(afi.entry[n].filename[0]) {
    cnt++;
    char xfile[16];
    strcpy(xfile, trunc(afi.entry[n].filename, 8));
    strcat(xfile, ".");
    strcat(xfile, trunc(afi.entry[n].extension, 3));
    printf("extracting '%s'\n", xfile);
    uint32 sum32;
    int ret = extract_to_file(xfile, fh, afi.entry[n].fofs, afi.entry[n].fsize, &sum32);
    if(ret != ERROR_SUCCESS) {
      err++;
      printf("  -> ");
      display_error(ret);
    } else if(sum32 != afi.entry[n].sum32) {
      wrn++;
      printf("  -> warning: invalid checksum\n");
    }
  }
  printf("extracted %u file%s: %u error%s, %u warning%s\n",
    cnt, (cnt==1)?"":"s", err, (err==1)?"":"s", wrn, (wrn==1)?"":"s");

  fclose(fh);
  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int extract_fw(char *filename)
{
  display_fileinfo(filename);

  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();

  FW_HEAD fw;
  if(fread(&fw, sizeof(fw), 1, fh) != 1) {
    fclose(fh);
    return ERROR_READ_FAULT;
  }

  printf("\n---[ extract:fw ]--------------------------------------------------------------\n");
  int cnt=0, err=0, wrn=0;
  if(fw.sum16 != calc_sum16(&fw, 0x1FE) || fw.sum32 != calc_sum32(fh, 0x200, sizeof(FW_HEAD)-0x200)) {
    wrn++;
    printf("warning: invalid checksum, file header maybe damaged\n");
  }
  for(int n=0; n<FIRMWARE_ENTRIES; n++) if(fw.entry[n].filename[0]) {
    cnt++;
    char xfile[16];
    strcpy(xfile, trunc(fw.entry[n].filename, 8));
    strcat(xfile, ".");
    strcat(xfile, trunc(fw.entry[n].extension, 3));
    printf("extracting '%s'\n", xfile);
    uint32 sum32;
    int ret = extract_to_file(xfile, fh, ((size_t)fw.entry[n].fofs_s9)<<9, fw.entry[n].fsize, &sum32);
    if(ret != ERROR_SUCCESS) {
      err++;
      printf("  -> ");
      display_error(ret);
    } else if(sum32 != fw.entry[n].sum32) {
      wrn++;
      printf("  -> warning: invalid checksum\n");
    }
  }
  printf("extracted %u file%s: %u error%s, %u warning%s\n",
    cnt, (cnt==1)?"":"s", err, (err==1)?"":"s", wrn, (wrn==1)?"":"s");

  fclose(fh);
  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int extract(char *filename)
{
  int ret = detect_filetype(filename);
  switch(ret&FILETYPE_UNKNOWN) {
    case FILETYPE_FW:  return extract_fw(filename);
    case FILETYPE_AFI: return extract_afi(filename);
    case FILETYPE_ENC: ret = decrypt(filename); if(ret == ERROR_SUCCESS) return extract(filename);
    default: return ret;
  }
}




// -----------------------------------------------------------------------------------------------
int extract_script_afi(char *filename)
{
  fprintf(stderr, "\n---[ script:afi ]--------------------------------------------------------------\n");
  printf("//%s", szAppInfo);
  printf("//auto-generated script from \"%s:afi\"\n", filename);

  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();

  AFI_HEAD afi;
  if(fread(&afi, sizeof(afi), 1, fh) != 1) {
    fclose(fh);
    return ERROR_READ_FAULT;
  }

  printf("\n[AFI]\nVID = 0x%04X\nPID = 0x%04X\nVER = %u.%u.%02X\nDATE = %02X/%02X/%02X%02X\n\nPATH = \".\\\"\n",
    afi.vendor_id, afi.product_id, afi.version_id[0]>>4, afi.version_id[0]&15, afi.version_id[1],
    afi.date.month, afi.date.day, afi.date.year[0], afi.date.year[1]);

  for(int n=0; n<AFI_ENTRIES; n++) if(afi.entry[n].filename[0]) {
    char xfile[16];
    strcpy(xfile, trunc(afi.entry[n].filename, 8));
    strcat(xfile, ".");
    strcat(xfile, trunc(afi.entry[n].extension, 3));
    switch(afi.entry[n].type) {
      case 'B': printf("BREC = \"%s\", \"%s\"\n", xfile, trunc(afi.entry[n].desc, 4)); break;
      case 'F': printf("FWSC = \"%s\", \"%s\"\n", xfile, trunc(afi.entry[n].desc, 4)); break;
      case 'U': printf("ADFU = \"%s\", \"%s\"\n", xfile, trunc(afi.entry[n].desc, 4)); break;
      case 'I': printf("FWIMAGE = \"%s\"\n", xfile); break;
      default:
      printf("FILE = \"%s\", \"%s\", \"%c\", 0x%04X\n", xfile, trunc(afi.entry[n].desc, 4),
        afi.entry[n].type, afi.entry[n].download_addr);
    }
  }

  printf("\n[EOF]\n");
  fprintf(stderr, "done\n");

  fclose(fh);
  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int extract_script_fw(char *filename)
{
  fprintf(stderr, "\n---[ script:fw ]---------------------------------------------------------------\n");
  printf("//%s", szAppInfo);
  printf("//auto-generated script from \"%s:fw\"\n", filename);

  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();

  FW_HEAD fw;
  if(fread(&fw, sizeof(fw), 1, fh) != 1) {
    fclose(fh);
    return ERROR_READ_FAULT;
  }

  printf("\n[FW]\nVID = 0x%04X\nPID = 0x%04X\nVER = %u.%u.%02X\nDATE = %02X/%02X/%02X%02X\n\n",
    fw.vendor_id, fw.product_id, fw.version_id[0]>>4, fw.version_id[0]&15, fw.version_id[1],
    fw.date.month, fw.date.day, fw.date.year[0], fw.date.year[1]);
  printf("INF = \"%s\"\n", trunc(fw.info.global, 32));
  printf("INF_DEVICE = \"%s\"\n", trunc(fw.info.device, 32));
  printf("INF_MANUFACTURER = \"%s\"\n\n", trunc(fw.info.manufacturer, 32));
  //printf("USB_DESC = \"%s\"\n", trunc(fw.usb.desc, 23));
  printf("USB_ATTR = \"%s\"\n", trunc(fw.usb.attr, 8));
  printf("USB_IDENT = \"%s\"\n", trunc(fw.usb.ident, 16));
  printf("USB_REV = \"%s\"\n\n", trunc(fw.usb.rev, 4));
  printf("SET_RTCRATE = 0x%04X\nSET_DISPLAYCONTRAST = %u\nSET_LIGHTTIME = %u\nSET_STANDBYTIME = %u\nSET_SLEEPTIME = %u\n" \
    "SET_LANGUAGEID = %u\nSET_REPLAYMODE = %u\nSET_ONLINEMODE = %u\nSET_BATTERYTYPE = %u\nSET_RADIODISABLE = %u\n\n", 
    fw.comval.rtc_rate, fw.comval.display_contrast, fw.comval.light_time, fw.comval.standby_time,
    fw.comval.sleep_time, fw.comval.language_id, fw.comval.replay_mode, fw.comval.online_mode, 
    fw.comval.battery_type, fw.comval.radio_disable);
  printf("PATH = \".\\\"\n");

  for(int n=0; n<FIRMWARE_ENTRIES; n++) if(fw.entry[n].filename[0]) {
    char xfile[16];
    strcpy(xfile, trunc(fw.entry[n].filename, 8));
    strcat(xfile, ".");
    strcat(xfile, trunc(fw.entry[n].extension, 3));
    printf("FILE = \"%s\"\n", xfile);
  }

  printf("\n[EOF]\n");
  fprintf(stderr, "done\n");

  fclose(fh);
  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int extract_script(char *filename)
{
  int ret = detect_filetype(filename);
  switch(ret&FILETYPE_UNKNOWN) {
    case FILETYPE_FW:  return extract_script_fw(filename);
    case FILETYPE_AFI: return extract_script_afi(filename);
    case FILETYPE_ENC: ret = decrypt(filename); if(ret == ERROR_SUCCESS) return extract_script(filename);
    default: return ret;
  }
}




// -----------------------------------------------------------------------------------------------
int archive_afi(char *filename)
{
  printf("\n---[ archive:afi ]-------------------------------------------------------------\n");

  int ret = ERROR_SUCCESS;
  FILE *fh = NULL;

  LPSTR lppFileList[AFI_ENTRIES];
  ZeroMemory(lppFileList, sizeof(lppFileList));

  do {
    AFI_HEAD ah;
    ZeroMemory(&ah, sizeof(ah));

    // run parser
    PARSER Parser(stdin);
    ret = Parser.parse_script(&ah, lppFileList, NULL, NULL);
    if(ret != ERROR_SUCCESS) break;

    //create archive and write beta-header
    fh = fopen(filename, "wb");
    if(!fh) {ret = GetLastError(); break;}
    if(fwrite(&ah, sizeof(ah), 1, fh) != 1) {ret = ERROR_WRITE_FAULT; break;}

    // write files
    size_t fofs = sizeof(ah);
    for(int file_cnt=0; file_cnt<AFI_ENTRIES; file_cnt++) if(lppFileList[file_cnt])
    {
      printf("add file \"%s\"\n", lppFileList[file_cnt]);

      //align file to 16
      while(fofs % 16) {
        char zero = 0;
        if(fwrite(&zero, 1, 1, fh) != 1) {ret = ERROR_WRITE_FAULT; break;}
        fofs++;
      }
      if(ret != ERROR_SUCCESS) break;

      //add new file
      ah.entry[file_cnt].fofs = (uint32)(fofs);
      ret = extract_from_file(lppFileList[file_cnt], fh, &ah.entry[file_cnt].fsize, &ah.entry[file_cnt].sum32);
      if(ret != ERROR_SUCCESS) break;
      fofs += ah.entry[file_cnt].fsize;
    }
    if(ret != ERROR_SUCCESS) break;

    // calc checksum
    ah.eoh.sum32 = calc_sum32(&ah, sizeof(AFI_HEAD)-4);

    //write new header
    if(fseek(fh, 0, SEEK_SET) != 0) {ret = GetLastError(); break;}
    else if(fwrite(&ah, sizeof(ah), 1, fh) != 1) {ret = ERROR_WRITE_FAULT; break;}
  } while(0);

  for(int n=0; n<AFI_ENTRIES; n++) if(lppFileList[n]) {delete lppFileList[n]; break;}
  if(fh) fclose(fh);
  if(ret == ERROR_SUCCESS) display_afi(filename);
  return ret;
}


// -----------------------------------------------------------------------------------------------
int archive_fw(char *filename)
{
  printf("\n---[ archive:fw ]--------------------------------------------------------------\n");

  int ret = ERROR_SUCCESS;
  FILE *fh = NULL;

  LPSTR lppFileList[FIRMWARE_ENTRIES];
  ZeroMemory(lppFileList, sizeof(lppFileList));

  do {
    FW_HEAD fwh;
    ZeroMemory(&fwh, sizeof(fwh));

    // run parser
    PARSER Parser(stdin);
    ret = Parser.parse_script(NULL, NULL, &fwh, lppFileList);
    if(ret != ERROR_SUCCESS) break;

    // create wide character string fwh.desc
    fwh.usb.desc[0] = 0x0330;
    for(int c=0; c<23; c++) fwh.usb.desc[1+c] = (uint16)fwh.usb.attr[c];

    //create archive and write beta-header
    fh = fopen(filename, "wb");
    if(!fh) {ret = GetLastError(); break;}
    if(fwrite(&fwh, sizeof(fwh), 1, fh) != 1) {ret = ERROR_WRITE_FAULT; break;}

    // write files
    size_t fofs = sizeof(fwh);
    for(int file_cnt=0; file_cnt<FIRMWARE_ENTRIES; file_cnt++) if(lppFileList[file_cnt])
    {
      printf("add file \"%s\"\n", lppFileList[file_cnt]);

      //align file to 512
      while(fofs % 512) {
        char zero = 0;
        if(fwrite(&zero, 1, 1, fh) != 1) {ret = ERROR_WRITE_FAULT; break;}
        fofs++;
      }
      if(ret != ERROR_SUCCESS) break;

      //add new file
      fwh.entry[file_cnt].fofs_s9 = (uint32)(fofs >> 9);
      ret = extract_from_file(lppFileList[file_cnt], fh, &fwh.entry[file_cnt].fsize, &fwh.entry[file_cnt].sum32);
      if(ret != ERROR_SUCCESS) break;
      fofs += fwh.entry[file_cnt].fsize;
    }
    if(ret != ERROR_SUCCESS) break;

    // calc checksum
    fwh.sum32 = calc_sum32(&fwh.entry, sizeof(FW_HEAD)-0x200);  // 32bit checksum across entries
    fwh.sum16 = calc_sum16(&fwh, 0x1FE);                        // 16bit checksum across header

    //write new header
    if(fseek(fh, 0, SEEK_SET) != 0) {ret = GetLastError(); break;}
    else if(fwrite(&fwh, sizeof(fwh), 1, fh) != 1) {ret = ERROR_WRITE_FAULT; break;}
  } while(0);

  for(int n=0; n<FIRMWARE_ENTRIES; n++) if(lppFileList[n]) delete lppFileList[n];
  if(fh) fclose(fh);
  if(ret == ERROR_SUCCESS) display_fw(filename);
  return ret;
}


// -----------------------------------------------------------------------------------------------
int archive(char *filename)
{
  int ret = detect_filetype(filename, false);
  switch(ret&FILETYPE_UNKNOWN) {
    case FILETYPE_UNKNOWN: return ERROR_BAD_COMMAND;
    case FILETYPE_FW:  return archive_fw(filename);
    case FILETYPE_AFI: return archive_afi(filename);
    case FILETYPE_ENC: printf("error: writing encrypted files is not supported\n"); return ERROR_SUCCESS;
    default: return ret;
  }  
}




// -----------------------------------------------------------------------------------------------
int list_fw(char *filename)
{
  FILE *fh = fopen(filename, "rb");
  if(!fh) return GetLastError();

  AFI_HEAD afi;
  if(fread(&afi, sizeof(afi), 1, fh) != 1) {
    fclose(fh);
    return ERROR_READ_FAULT;
  }

  for(int n=0; n<AFI_ENTRIES; n++) if(afi.entry[n].filename[0] && afi.entry[n].type == 'I') {
    char xfile[1024];
    strcpy(xfile, filename);
    strcat(xfile, ".");
    strcat(xfile, trunc(afi.entry[n].filename, 8));
    strcat(xfile, ".");
    strcat(xfile, trunc(afi.entry[n].extension, 3));
    int ret = extract_to_file(xfile, fh, afi.entry[n].fofs, afi.entry[n].fsize);
    g_filetype = FILETYPE_UNKNOWN;
    if(ret == ERROR_SUCCESS) display(xfile);
    else display_error(ret);
    DeleteFile(xfile);
  }

  fclose(fh);
  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int list(char *filename)
{
  int ret = display(filename);
  if(ret != ERROR_SUCCESS) return ret;
  if((g_filetype&FILETYPE_UNKNOWN) == FILETYPE_AFI) return list_fw(filename);
  return ERROR_SUCCESS;
}




// -----------------------------------------------------------------------------------------------
int info(char *filename)
{
  int ret;
  BOOL fNext;
  WIN32_FIND_DATA fd;
  if(!filename || !*filename) filename = "*.bin";
  printf("show file info (%s)...\n", filename);
  HANDLE hFF = FindFirstFile(filename, &fd);
  if(hFF == INVALID_HANDLE_VALUE) return GetLastError();
  do {
    g_filetype = FILETYPE_UNKNOWN;
    ret = display(fd.cFileName);
    if(ret != ERROR_SUCCESS) display_error(ret);

    if((g_filetype&FILETYPE_UNKNOWN) == FILETYPE_AFI) {
      ret = list_fw(fd.cFileName);
      if(ret != ERROR_SUCCESS) display_error(ret);
    }
    fNext = FindNextFile(hFF, &fd);
    if(fNext) printf("\n===============================================================================\n");
  } while(fNext);
  CloseHandle(hFF);
  return ERROR_SUCCESS;
}




// -----------------------------------------------------------------------------------------------
int _execute(char cmd, char *filename)
{
  switch(tolower(cmd)) {
    case 0: return display(filename);
    case 'i': return display_fileinfo(filename);
    case 'x': return extract(filename);
    case 'l': return list(filename);    
    default: return ERROR_BAD_COMMAND;
  }
}

// -----------------------------------------------------------------------------------------------
int execute_mul(char cmd, char *filename)
{
  bool first=true;
  WIN32_FIND_DATA fd;
  HANDLE hFF = FindFirstFile(filename, &fd);
  if(hFF == INVALID_HANDLE_VALUE) return GetLastError();

  //get rel. path from filename  
  char *path = strrchr(filename, '\\');
  if(!path) path = strrchr(filename, ':');
  if(path) {
    *(++path) = 0;
    path = new char[MAX_PATH + strlen(filename)];
    if(!path) return ERROR_NOT_ENOUGH_MEMORY;
  }

  do {
    if(!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      if(!first) printf("\n===============================================================================\n");
      first = false;
      g_filetype = FILETYPE_UNKNOWN;
      int ret;
      if(!path) ret = _execute(cmd, fd.cFileName);
      else {
        strcpy(path, filename);
        strcat(path, fd.cFileName);
        ret = _execute(cmd, path);
      }
      if(ret == ERROR_BAD_COMMAND) {if(path) delete path; CloseHandle(hFF); return ret;}
      if(ret != ERROR_SUCCESS) display_error(ret);
    }
  } while(FindNextFile(hFF, &fd));

  if(path) delete path;
  CloseHandle(hFF);
  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int execute(char cmd, char *filename)
{
  switch(tolower(cmd)) {
    case 'f': return firmware_dump(filename);
    case 'u': return firmware_upload(filename);
    case 'r': return firmware_repair(filename);
    case 'a': return archive(filename);
    case 's': return extract_script(filename);
  }
  detect_filetype(filename);
  if(g_filetype & FILETYPE_OVERWRITE) return _execute(cmd, filename);
  return execute_mul(cmd, filename);
}


// -----------------------------------------------------------------------------------------------
int syntax()
{
  fprintf(stderr,
    "\n"\
    "usage:\n"\
    "  s1fwx {i|x|s|a|l|f|u|r} {filename{:[afi|fw|enc]}}\n"\
    "\n"\
    "examples:\n"\
    "  display file info                s1fwx i fname.ext\n"\
    "  extract firmware file            s1fwx x fname.ext\n"\
    "  generate script from file        s1fwx s fname.ext > new.script\n"\
    "  create firmware file by script   s1fwx a new.fw:fw < def_fw.script\n"\
    "  create afi file by script        s1fwx a new.bin:afi < def_afi.script\n"\
    "  list entire content of any file  s1fwx l *.bin\n"\
    "  extract firmware from player     s1fwx f dump.bin\n"\
    "  upload firmware to player        s1fwx u dump.bin\n"\
    "  repair dumped firmware files     s1fwx r dump.bin\n"
  );
  return ERROR_SUCCESS;
}


// -----------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  fprintf(stderr, szAppInfo);

  int ret = ERROR_SUCCESS;
  if(argc < 2) return syntax();
  else if(argc == 2) ret = execute_mul(0, argv[1]);
  else if(argc > 3) ret = ERROR_BAD_COMMAND;
  else if(argv[1][1] != 0) ret = ERROR_BAD_COMMAND;
  else ret = execute(argv[1][0], argv[2]);
  if(ret != ERROR_SUCCESS) return display_error(ret);
  //printf("-------------------------------------------------------------------------------\n");
  return(0);
}