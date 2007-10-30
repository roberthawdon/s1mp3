#ifndef _HEADS_H_
#define _HEADS_H_


// -----------------------------------------------------------------------------------------------
#define AFI_ENTRIES 126

#define AFI_ID "AFI"

#define AFI_ENTRY_DLADR_BREC  0x00000006    // 'B'
#define AFI_ENTRY_DLADR_FWSC  0x00020008    // 'F'
#define AFI_ENTRY_DLADR_ADFUS 0x000C0008    // 'A'
#define AFI_ENTRY_DLADR_ADFU  0x00000000    // 'U'
#define AFI_ENTRY_DLADR_FW    0x00000011    // 'I'


typedef struct _AFI_HEAD_ENTRY {
  char    filename[8];
  char    extension[3];
  char    type;           //= H,F,B,I,A,S
  uint32  download_addr;  //download address
  uint32  fofs;
  uint32  fsize;
  char    desc[4];        //subtype
  uint32  sum32;
} AFI_HEAD_ENTRY;

typedef struct _AFI_HEAD_EOHDR {
  uint8   reserved[28];
  uint32  sum32;
} AFI_HEAD_EOHDR;

typedef struct {
  char    id[4];
  uint16  vendor_id;
  uint16  product_id;
  uint8   version_id[2];
  uint8   ext_version_id[2];
  struct {uint8 year[2], month, day;} date;
  uint8   reserved0x20[16];
  struct _AFI_HEAD_ENTRY entry[AFI_ENTRIES];
  struct _AFI_HEAD_EOHDR eoh;
} AFI_HEAD, *LP_AFI_HEAD;


// -----------------------------------------------------------------------------------------------
#define MAGIC_COMVAL            0xdead
#define MAGIC_MUSIC             0xbeef
#define MAGIC_VOICE             0xfee0
#define MAGIC_RECORD            0x3d3d
#define MAGIC_FMRADIO           0xad01
#define MAGIC_SETTING           0xbaba
#define MAGIC_UDISK             0xee77
#define MAGIC_TESTER            0x9801
#define MAGIC_STANDBY           0x3935
#define MAGIC_UPGRADE           0x3951

//language id
#define LAN_ID_SCHINESE		0
#define LAN_ID_ENGLISH		1
#define LAN_ID_TCHINESE		2

//batt type
#define BATT_TYPE_ALKALINE	0
#define BATT_TYPE_NIH		    1
#define BATT_TYPE_LITHIUM	  2

#pragma pack(1)
typedef struct _COMVAL
{
  uint16  magic;
  uint32  system_time;
  uint16  rtc_rate;
  uint8   display_contrast;
  uint8   light_time;
  uint8   standby_time;
  uint8   sleep_time;
  uint8   language_id;
  uint8   replay_mode;
  uint8   online_mode;
  uint8   battery_type;
  uint8   radio_disable;
        
  uint8   reserved[32-17];
  //int16     checksum;
} COMVAL, *LP_COMVAL;
#pragma pack()

#define COMVAL_INIT(comval_p) {\
  memset((LP_COMVAL)(comval_p), 0, sizeof(COMVAL));\
  ((LP_COMVAL)comval_p)->magic = MAGIC_COMVAL; \
  ((LP_COMVAL)comval_p)->system_time = 0L; \
  ((LP_COMVAL)comval_p)->rtc_rate = 0xB6; \
  ((LP_COMVAL)comval_p)->display_contrast = 11; \
  ((LP_COMVAL)comval_p)->light_time = 3; \
  ((LP_COMVAL)comval_p)->sleep_time = 0; \
  ((LP_COMVAL)comval_p)->standby_time = 30; \
  ((LP_COMVAL)comval_p)->language_id = LAN_ID_ENGLISH; \
  ((LP_COMVAL)comval_p)->replay_mode = 0; \
  ((LP_COMVAL)comval_p)->online_mode = 0; \
  ((LP_COMVAL)comval_p)->battery_type = BATT_TYPE_ALKALINE; \
  ((LP_COMVAL)comval_p)->radio_disable = 1; \
}


// -----------------------------------------------------------------------------------------------
#define FIRMWARE_ENTRIES 240

#define FIRMWARE_ID 0x0FF0AA55

typedef struct _FW_HEAD_ENTRY {
  char    filename[8];
  char    extension[3];
  uint8   attr;             //=0
  uint8   reserved0x0C[2];  //=0
  uint16  version;          //=0
  uint32  fofs_s9;
  uint32  fsize;
  uint32  reserved;
  uint32  sum32;
} FW_HEAD_ENTRY;


typedef struct {
  //0x00..0x0F
  uint32  id; //= FIRMWARE_ID
  uint8   version_id[2];
  uint8   ext_version_id[2];  //=0
  struct {uint8 year[2], month, day;} date;
  uint16  vendor_id;
  uint16  product_id;

  //0x10..0x1F
  uint32  sum32;            //32bit sum-checksum across header entrys (0x0200..0x1FFF)
  uint8   zero0x14[12];

  //0x20..0xFF
  struct {
    char    global[32];     //info[32];
    char    manufacturer[32];
    char    device[32];
    uint8   zero0x80[128];
  } info;
  
  //0x100..0x14F
  struct {
    char    attr[8];        //vendor_info[8];
    char    ident[16];      //product_ident[16];
    char    rev[4];         //product_rev[8];
    uint8   zero0x11C[4];
    uint16  desc[24];       // wide character string of attr + ident + rev
  } usb;

  //0x150..0x16F
  struct _COMVAL comval;

  //0x170..0x1FF
  uint8   zero0x170;
  uint8   zero0x171[15];
  uint8   zero0x180[126];
  uint16  sum16;            //16bit sum-checksum across firmware header (0x0000..0x01FD)

  struct _FW_HEAD_ENTRY entry[FIRMWARE_ENTRIES];
} FW_HEAD, *LP_FW_HEAD;


// -----------------------------------------------------------------------------------------------
#define ADFU_FWSCAN_INFO_ID "FW"
typedef struct {
  char    id[2];               //=ADFU_FWSCAN_INFO_ID
  uint16  vendor_id;
  uint16  product_id;
  uint16  version;
  uint8   reserved1[2];
  char    manufacturer[32];
  char    device[32];
  uint8   reserved2[14];
} ADFU_FWSCAN_INFO, *LP_ADFU_FWSCAN_INFO;

typedef struct {
  uint16  storage_info[4];
  uint16  storage_caps[8];
} ADFU_STORAGE_INFO, *LP_ADFU_STORAGE_INFO;

#define ADFU_HWSCAN_INFO_ID "HW"
typedef struct {
  char    id[2];            //=ADFU_HWSCAN_INFO_ID
  uint16  ic_version;       //3935,3951
  uint8   sub_version[2];   //A,B,C,D
  uint8   brom_version[4];  //e.g. "3.0.00.8888"
  struct {uint8 year[2], month, day;} brom_date;
  char    flash_type[4];    //e.g. "F644"/"F641"/"F321"/"N---" of flash containing firmware (bootflash)
  ADFU_STORAGE_INFO stginfo;
  uint8   reserved[22];
} ADFU_HWSCAN_INFO, *LP_ADFU_HWSCAN_INFO;

#define ADFU_SYS_INFO_ID "SYS INFO"
typedef struct {
  char              id[8];    //=ADFU_SYS_INFO_ID
  ADFU_HWSCAN_INFO  hw_scan;
  ADFU_FWSCAN_INFO  fw_scan;
  uint8             reserved[32];
} ADFU_SYS_INFO, *LP_ADFU_SYS_INFO;

#endif