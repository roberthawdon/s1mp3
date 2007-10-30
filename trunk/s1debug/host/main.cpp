//=================================================================================================
// s1debug v1.0
//
// file license: LGPL
// copyright (c)2006 wiRe@gmx.net
//
// debug console controlled through an i2c bus
// needs the "s1mp3 i²c debug interface" to connect the device to the pc's parallel port (SPP)
//
// see http://www.s1mp3.de/ and http://wiki.s1mp3.org/index.php/Debug_interface
//=================================================================================================
#include "common.h"
#include "io.h"
#include "debugger.h"




//-------------------------------------------------------------------------------------------------
short parse_arg(const char *arg)
{
  if(!arg) return(-1);
  while(*arg == ' ' || *arg == '\t') arg++;
  char *end = NULL;
  unsigned long ul = strtoul(arg, &end, 0);
  if(ul > 0xFFF) return(-1);
  if(!end) return(-1);
  while(*end == ' ' || *end == '\t') end++;
  if(*end) return(-1);
  return((short)ul);
}




//-------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  puts("s1debug v1.0 - copyright (c)2006 wiRe@gmx.net");

  if(argc > 1) nPort = parse_arg(argv[1]);
  if(argc > 2) nAddr = parse_arg(argv[2]) & 0x07;
  if(argc > 3 || nPort < 0 || nAddr < 0)
  {
    puts("error: wrong command line parameters");
    puts("usage: s1debug [PORT [ADDR]]");
    return(-1);
  }

  if(!io_open())
  {
    puts("error: no debug interface found");
    return(-1);
  }

  printf("debug interface base address: 0x%03X\n", nPort);
  printf("debug interface i2c address: 0x%02X\n", (nAddr<<1) | 0x70);
  puts("press ctrl+c to leave debug console\n");

  if(!debugger_init())
  {
    puts("error: failed to init the debugger");
    return(-1);
  }

  while(1)
  {
    short nSync = io_sync();
    switch(nSync)
    {
    case 1: //puts
      while(1)
      {
        short n = io_rx();
        if(n < 0) break;
        putch(n);
      }
      break;

    case 2: //putch
      while(1)
      {
        short l = io_rx();
        if(l < 0) break;
        short h = io_rx();
        if(h < 0) break;
        putch(l);
      }
      break;

    case 3: //puti
      {
        short l = io_rx();
        if(l < 0) break;
        short h = io_rx();
        if(h < 0) break;
        printf("%i", (int16)((h<<8)|l));
      }
      break;

    case 4: //putui
      {
        short l = io_rx();
        if(l < 0) break;
        short h = io_rx();
        if(h < 0) break;
        printf("0x%04X", (uint16)((h<<8)|l));
      }
      break;

    case 5: //getch
      while(1)
      {
        if(_kbhit())
        {
          io_tx( _getch() );
          break;
        }
        else
        {
          short l =  io_tx(0);
          if(l < 0) break;
        }
      }
      break;

    case 7: //breakpoint
      if(!debugger())
      {
        puts("error: failed to run the debugger");
      }
      break;

    default:
      printf("\nerror: unknown sync code (%u)\n", nSync);
    }
  }

  return(0);
}
