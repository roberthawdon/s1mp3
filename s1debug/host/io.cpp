//=================================================================================================
// low-level routines to access the "s1mp3 i²c debug interface"
//
// file license: LGPL
// copyright (c)2006 wiRe@gmx.net
//=================================================================================================
#include "common.h"
#include "io.h"


//-------------------------------------------------------------------------------------------------
//prototypes (function typedef) of inpout32 library
#pragma comment(lib, "inpout32.lib")
short _stdcall Inp32(short PortAddress);
void _stdcall Out32(short PortAddress, short data);

//-------------------------------------------------------------------------------------------------
//convenience defines for the parallel port
#define LPT_BASE  (nPort)
#define LPT_DATA  (LPT_BASE+0x000)  //centronics data port
#define LPT_STAT  (LPT_BASE+0x001)  //centronics status port
#define LPT_CTRL  (LPT_BASE+0x002)  //centronics control port

#define INP(a)    Inp32(a)
#define OUTP(a,d) Out32(a,d)

//-------------------------------------------------------------------------------------------------
const short nDefaultPort[] = {0x3BC, 0x378, 0x278, 0};

short nPort = 0;    //the base address of the choosen port
short nAddr = 0;    //the choosen address of the i²c module

short nClkIn  = 0;  //current state of the input clock
short nClkOut = 0;  //current state of the output clock (ready bit)


//-------------------------------------------------------------------------------------------------
bool io_init()
{
  OUTP(LPT_CTRL, 0x04);   //turn all power lines high

  OUTP(LPT_DATA, 0x88);
  Sleep(1);
  if(INP(LPT_DATA) != 0x88) return false;
  if((INP(LPT_STAT) & 8) == 0) return false;

  OUTP(LPT_DATA, 0x0E);
  Sleep(1);
  if(INP(LPT_DATA) != 0x0E) return false;
  if((INP(LPT_STAT) & 8) != 0) return false;

  OUTP(LPT_DATA, (nClkOut << 3) | (nAddr << 4));
  nClkIn = (INP(LPT_STAT) >> 4) &1;
  return true;
}


//-------------------------------------------------------------------------------------------------
bool io_autodetect()
{
  for(int n=0; nDefaultPort[n]; n++)
  {
    nPort = nDefaultPort[n];
    if(io_init()) return true;
  }

  nPort = 0;
  return false;
}

//-------------------------------------------------------------------------------------------------
bool io_open()
{
  return (nPort == 0 && io_autodetect()) || (io_init());
}




//-------------------------------------------------------------------------------------------------
short io_read_nibble()
{
  //nClkIn = (INP(LPT_STAT) >> 4) &1;
  nClkIn ^= 1;

  while(1)
  {
    short n = (INP(LPT_STAT)^0x80) >> 4;
    if(((n^nClkIn)&1) == 0) 
    {
      n = ((n>>3)&1) | (((n>>2)&1) <<1) | (((n>>1)&1) <<2);
      //printf("<%u>", n);
      return n;
    }
  }
}

//-------------------------------------------------------------------------------------------------
void io_write_nibble(short nData)
{
  nData = ((nData>>2)&1) | (((nData>>1)&1) <<1) | (((nData)&1) <<2) | (nClkOut <<3) | (nAddr <<4);
  OUTP(LPT_DATA, nData);
  nClkOut ^= 1;
  nData ^= (1 << 3);
  OUTP(LPT_DATA, nData);
}




//-------------------------------------------------------------------------------------------------
short io_calc_parity(short nByte)
{
  char p = 0;
  for(char b=8; b>0; b--, nByte>>=1) p += (nByte&1);
  return((p&1)^1);  //return odd parity
}

//-------------------------------------------------------------------------------------------------
short io_tx(short nData)
{
  nData = (nData&0xFF) | (io_calc_parity(nData)<<8);

  short nResult = io_read_nibble() << 6;
  io_write_nibble(nData >> 6);

  nResult |= io_read_nibble() << 3;
  io_write_nibble(nData >> 3);

  nResult |= io_read_nibble() << 0;
  io_write_nibble(nData >> 0);

  //printf("<RX%03X|%c>", nResult, nResult&0xFF);

  return(io_calc_parity(nResult) == (nResult >> 8))? (nResult&0xFF) : -1;
}

//-------------------------------------------------------------------------------------------------
short io_rx()
{
  short nResult = io_read_nibble() << 6;
  io_write_nibble(0);

  nResult |= io_read_nibble() << 3;
  io_write_nibble(0);

  nResult |= io_read_nibble() << 0;
  if(io_calc_parity(nResult) == (nResult >> 8))
  {
    io_write_nibble(1);
    //printf("<RC%03X|%c>", nResult, nResult&0xFF);
    return(nResult&0xFF);
  }
  else
  {
    io_write_nibble(5);
    return(-1);
  }
}

//-------------------------------------------------------------------------------------------------
short io_sync(short reply)   //returns sync code, if reply is zero then send an echo instead
{
  for(char z = 0; 1;)  //wait for 3 clocks of zero data
  {
    short n = io_read_nibble();
    if(n == 0) z = (z < 3)? (z+1) : 3;
    else if(z < 3) z = 0;
    else
    {
      io_write_nibble((reply == 0)? n : reply);
      //printf("<S%u>", n);
      return n;
    }
    io_write_nibble(0);
  }
}
