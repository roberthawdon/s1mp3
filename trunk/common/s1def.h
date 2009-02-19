//=================================================================================================
// s1def.h : define some s1mp3 structures
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#include "common.h"


// -----------------------------------------------------------------------------------------------
#define AFI_HEAD_ENTRIES 126

#define AFI_HEAD_ID 0x00494641  //"AFI",0x00

#define AFI_ENTRY_TYPE_BREC   0x00000006    // 'B'
#define AFI_ENTRY_TYPE_FWSC   0x00020008    // 'F'
#define AFI_ENTRY_TYPE_ADFU   0x00000000    // 'U'
#define AFI_ENTRY_TYPE_FW     0x00000011    // 'I'

#define AFI_ENTRY_TYPE_ADFUS  0x000C0008    // 'A'
#define AFI_ENTRY_TYPE_HWSCAN 0x00020008    // 'H'


typedef struct _AFI_HEAD_ENTRY {
  char    filename[8];
  char    extension[3];
  char    content;
  uint32  type;
  uint32  fofs;
  uint32  fsize;
  char    desc[4];
  uint32  sum32;
} AFI_HEAD_ENTRY, *LP_AFI_HEAD_ENTRY;

typedef struct _AFI_HEAD_EOHDR {
  uint8   dmy[28];
  uint32  sum32;
} AFI_HEAD_EOHDR;

typedef struct {
  uint32  id;
  uint16  vendor_id;
  uint16  product_id;
  uint8   version_id[2];
  uint8   dmy[6+16];
  struct _AFI_HEAD_ENTRY entry[AFI_HEAD_ENTRIES];
  struct _AFI_HEAD_EOHDR eoh;
} AFI_HEAD, *LP_AFI_HEAD;


// -----------------------------------------------------------------------------------------------
#define MAGIC_COMVAL    0xdead
#define MAGIC_MUSIC     0xbeef
#define MAGIC_VOICE     0xfee0
#define MAGIC_RECORD    0x3d3d
#define MAGIC_FMRADIO   0xad01
#define MAGIC_SETTING   0xbaba
#define MAGIC_UDISK     0xee77
#define MAGIC_TESTER    0x9801
#define MAGIC_STANDBY   0x3935
#define MAGIC_UPGRADE   0x3951

//language id
#define LANG_ID_SCHINESE		0
#define LANG_ID_ENGLISH		  1
#define LANG_ID_TCHINESE		2

//batt type
#define BATT_TYPE_ALKALINE	0
#define BATT_TYPE_NIMH	    1
#define BATT_TYPE_LITHIUM	  2

#pragma pack(1)
typedef struct _COMVAL
{
  uint16  magic;
  uint32  system_time;
  uint16  rtc_rate;
  uint8   display_contrast;   //0..15
  uint8   light_time;         //
  uint8   standby_time;       //0..60 (10min/step)
  uint8   sleep_time;         //0..120 (10min/step)
  uint8   language_id;        //see LANG_ID_xxx
  uint8   replay_mode;        //manual/auto
  uint8   online_mode;        //multi/normal/encrypted
  uint8   battery_type;       //see BATT_TYPE_xxx
  uint8   radio_disable;      //enable/disable radio
        
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
#pragma pack(1)
typedef struct _MTP
{
  uint8   manufacturer_sz;    //if this value is zero (or >32), then the mtp field isn't valid
  char    manufacturer[32];

  uint8   info_sz;
  char    info[32];

  uint8   version_sz;
  char    version[16];

  uint8   serial_sz;          //no string, the hexadecimal representation forms the serial
  char    serial[16];

  uint16  vendor_id;
  uint16  product_id;
} MTP, *LP_MTP;
#pragma pack()


// -----------------------------------------------------------------------------------------------
#define FW_HEAD_ENTRIES 240

#define FW_HEAD_ID 0x0FF0AA55

typedef struct _FW_HEAD_ENTRY {
  char    filename[8];
  char    extension[3];
  uint8   zero[5];
  uint32  fofs_s9;
  uint32  fsize;
  uint32  unkwn;
  uint32  sum32;
} FW_HEAD_ENTRY, *LP_FW_HEAD_ENTRY;


typedef struct {
  //0x00..0x0F
  uint32  id; //= FW_HEAD_ID
  uint8   version_id[2];
  uint16  zero0x06;
  struct {uint8 year[2], month, day;} date;
  uint16  vendor_id;
  uint16  product_id;

  //0x10..0x1F
  uint32  sum32;            //32bit sum-checksum across header entrys (0x0200..0x1FFF)
  uint8   zero0x14[12];

  //0x20..0x14F
  struct {
    char    general[32];
    char    manufacturer[32];
    char    device[32];

    uint8   zero0x80[128];

    char    vendor[8];
    char    product[16];
    char    revision[4];

    uint8   zero0x11C[6];

    uint16  usb_desc[23];   //usb device name (wide character string)
  } info;

  //0x150..0x16F
  struct _COMVAL comval;

  //0x170..0x1D7
  struct _MTP mtp;

  //0x1D8..0x1FF
  uint8   zero0x1D8[38];
  uint16  sum16;            //16bit sum-checksum across firmware header (0x000..0x1FD)

  struct _FW_HEAD_ENTRY entry[FW_HEAD_ENTRIES];
} FW_HEAD, *LP_FW_HEAD;


// -----------------------------------------------------------------------------------------------
#define RES_ENTRY_TYPE_LOGO     0x04  //one entire screen-filling monochrome image
#define RES_ENTRY_TYPE_MSTRING  0x03  //contains text of every language
#define RES_ENTRY_TYPE_STRING   0x02  //data?
#define RES_ENTRY_TYPE_ICON     0x01  //one icon of variable size
#define RES_ENTRY_TYPE_EMPTY    0x00

enum TEXT_LANGUAGE {
  TEXT_LANGUAGE_SIMPLE_CHINESE=0,
  TEXT_LANGUAGE_TRAD_CHINESE,
  TEXT_LANGUAGE_ENGLISH,
  TEXT_LANGUAGE_JAPANESE,
  TEXT_LANGUAGE_KOREAN,
  TEXT_LANGUAGE_GERMAN,
  TEXT_LANGUAGE_FRENCH,
  TEXT_LANGUAGE_ITALIAN,
  TEXT_LANGUAGE_SPANISH,
  TEXT_LANGUAGE_SWEDISH,
  TEXT_LANGUAGE_CZECH,
  TEXT_LANGUAGE_DUTCH,
  TEXT_LANGUAGE_PORTUGUESE,
  TEXT_LANGUAGE_DANMARK,
  TEXT_LANGUAGE_POLISH,
  TEXT_LANGUAGE_RUSSIAN,
  _MAX_TEXT_LANGUAGE_
};


typedef struct _RES_ICON_HEAD {
  uint16 width;
  uint16 height;
} RES_ICON_HEAD, *LP_RES_ICON_HEAD;

typedef struct _RES_HEAD_ENTRY {
  uint32  fofs;
  uint16  size;
  uint8   type;
  char    name[9];
} RES_HEAD_ENTRY, *LP_RES_HEAD_ENTRY;


#define RES_HEAD_ID 0x19534552  //= "RES",0x19

typedef struct _RES_HEAD {
  uint32  id;                     // = RES_HEAD_ID
  uint16  num_entries;
  uint16  unkwn0x06;
  uint16  unkwn0x08;
  uint16  unkwn0x0A;
  uint16  unkwn0x0C;
  uint16  unkwn0x0E;

  //struct _RES_HEAD_ENTRY entry[1];
} RES_HEAD, *LP_RES_HEAD;


/*
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
*/