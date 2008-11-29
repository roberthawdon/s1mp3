//=================================================================================================
// AdfuCbw
//
// descr.   class to describe a command block wrapper (CBW)
// author   wiRe _at_ gmx _dot_ net
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2007 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#include "common.h"


//-------------------------------------------------------------------------------------------------
class AdfuCbw {

///////////////////////////////////////////////////////////////////////////////////////////////////
public:enum FLAGS {DEVICE_TO_HOST = (1<<7), HOST_TO_DEVICE = 0};

///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack(1)
public:typedef struct {
  uint32  id;         //[00] cbw signature ("USBC" = 0x55,0x53,0x42,0x43)
  uint32  tag;        //[04] cbw tag
  uint32  length;     //[08] data transfer length
  uint8   flags;      //[0C] cbw flags (0x80 on read, 0 otherwise)
  uint8   lun;        //[0D] lun
  uint8   cdb_length; //[0E] command descriptor block length (=12)
  uint8   cdb[16];    //[0F] command descriptor block
} CBW;
#pragma pack()

///////////////////////////////////////////////////////////////////////////////////////////////////
public:AdfuCbw(const void *lpCdb, uint8 uCdbLength, 
         FLAGS nFlags =HOST_TO_DEVICE, uint32 uLength =0)
{
  ::memset(&g_cbw, 0, sizeof(g_cbw));
  g_cbw.id = 0x43425355;
  g_cbw.length = uLength;
  g_cbw.flags = nFlags;
  g_cbw.cdb_length = (uCdbLength > 16)? 16 : uCdbLength;
  if(lpCdb != NULL && g_cbw.cdb_length > 0) ::memcpy(g_cbw.cdb, lpCdb, g_cbw.cdb_length);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
public:const CBW *get() {return &g_cbw;}
public:unsigned int size() {return sizeof(g_cbw);}

///////////////////////////////////////////////////////////////////////////////////////////////////
private:CBW g_cbw;
};
