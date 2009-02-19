//=================================================================================================
// AdfuCsw
//
// descr.   class to describe a command status wrapper (CSW)
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#include "common.h"


//-------------------------------------------------------------------------------------------------
class AdfuCsw {

///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack(1)
public:typedef struct {
  uint32  id;         //[00] csw signature ("USBS" = 0x55,0x53,0x42,0x53)
  uint32  tag;        //[04] tag
  uint32  residue;    //[08] data transfer length
  uint8   status;     //[0C] status
} CSW;
#pragma pack()

///////////////////////////////////////////////////////////////////////////////////////////////////
public:AdfuCsw() {::memset(&g_csw, 0, sizeof(g_csw));}

///////////////////////////////////////////////////////////////////////////////////////////////////
public:bool verify() {return(g_csw.id == 0x53425355 && g_csw.status == 0);}

///////////////////////////////////////////////////////////////////////////////////////////////////
public:CSW *get() {return &g_csw;}
public:unsigned int size() {return sizeof(g_csw);}

///////////////////////////////////////////////////////////////////////////////////////////////////
private:CSW g_csw;
};
