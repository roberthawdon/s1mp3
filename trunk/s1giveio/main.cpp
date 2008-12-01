//=================================================================================================
// s1giveio v1.4
//
// descr.    debug console, supports analyzation of s1mp3 players hardware in real time
// author    wiRe _at_ gmx _dot_ net
// source    http://www.s1mp3.de/
//
// copyright (c)2005-2007 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#include "../common/common.h"
#include "../common/AdfuSession.h"
#include "../common/giveio.h"
#include "dasm.h"
#include <exception>


// -----------------------------------------------------------------------------------------------
AdfuSession session;
GiveIO gio(session);
// -----------------------------------------------------------------------------------------------




// -----------------------------------------------------------------------------------------------
void dump(const void *ptr, uint32 ofs, uint32 n, uint8 ofs_bits = 16, FILE *fh = stdout)
{
  uint32 col, ccnt=0;
  uint8 *pos = (uint8*)ptr;

  while(n >= 16)
  {
    if(ofs_bits == 8) fprintf(fh, "%02X      ", ofs&0xFF);
    else if(ofs_bits == 16) fprintf(fh, "%04X    ", ofs&0xFFFF);
    else fprintf(fh, "        ");
    for(col=0; col<8; col++) fprintf(fh, "%02X ", pos[col]);
    fprintf(fh, "- ");
    for(col=8; col<16; col++) fprintf(fh, "%02X ", pos[col]);
    fprintf(fh, "    ");
    for(col=0; col<16; col++) if(pos[col]>=32) fprintf(fh, "%c", pos[col]); else fprintf(fh, ".");
    fprintf(fh, "\n");

    /*if(pgbrk) if(++ccnt >= 24) {
      ccnt = 0;
      fprintf(fh, "press key to continue, ESC to cancel...");
      col = getch();
      fprintf(fh, "\n");
      if(col == 27) return;
    }*/

    pos+=16;
    ofs+=16;
    n-=16;
  }

  if(n>0)
  {
    if(ofs_bits == 8) fprintf(fh, "%02X      ", ofs&0xFF);
    else if(ofs_bits == 16) fprintf(fh, "%04X    ", ofs&0xFFFF);
    else fprintf(fh, "        ");
    for(col=0; col<8; col++) if(col < n) fprintf(fh, "%02X ", pos[col]); else fprintf(fh, "   ");
    fprintf(fh, "%c ", (n>7)?'-':' ');
    for(col=8; col<16; col++) if(col < n) fprintf(fh, "%02X ", pos[col]); else fprintf(fh, "   ");
    fprintf(fh, "    ");
    for(col=0; col<n; col++) if(pos[col]>=32) fprintf(fh, "%c", pos[col]); else fprintf(fh, ".");
    fprintf(fh, "\n");
  }
}


// -----------------------------------------------------------------------------------------------
uint32 value[256];
short parse_values(char *sz)
{
  for(short pos=0; 1; pos++)
  {
    while(*sz==' ' || *sz=='\t') sz++;
    if(*sz == 0 || pos >= 256) return pos;

    value[pos] = strtoul(sz, &sz, 16);

    if(*sz!=' ' && *sz!='\t' && *sz!=0) return -1;  //parse error
    while(*sz==' ' || *sz=='\t') sz++;
  }
}


// -----------------------------------------------------------------------------------------------
uint8 *read_mem(uint16 addr, uint32 size, bool addr_inc = true)
{
  if(!size) return NULL;

  uint8 *buf = new uint8[size];
  if(!buf) return NULL;
  memset(buf, 0, size);

  if(addr_inc)
  {
    gio.ld(addr, buf, size);
    addr+=(uint16)size;
  }
  else for(uint32 ofs = 0; ofs < size; ofs++)
  {
    gio.ld(addr, &buf[ofs], 1);
  }

  return buf;
}


// -----------------------------------------------------------------------------------------------
uint8 *read_port(uint8 port, uint32 size)
{
  uint8 *buf = new uint8[size];
  if(!buf) return NULL;
  memset(buf, 0, size);

  uint8 *bptr = buf;
  for(; size > 0; size--, port++, bptr++)
  {
    bptr[0] = gio.in(port);
  }

  return buf;
}


// -----------------------------------------------------------------------------------------------
bool upload(uint16 addr)
{
  char fname[MAX_PATH];
  printf("filename: ");
  fgets(fname, 1000, stdin);
  fname[strlen(fname)-1] = 0;
  if(!fname[0]) return false;

  FILE *fh = fopen(fname, "rb");
  if(!fh)
  {
    printf("error: failed to open file\n");
    return false;
  }

  uint8 buf[0x10000];
  memset(buf, 0, sizeof(buf));
  uint32 rx = (uint32)fread(buf, 1, sizeof(buf), fh);
  if(ferror(fh))
  {
    fclose(fh);
    printf("error: failed to read from file");
    return false;
  }
  fclose(fh);

  gio.st(addr, buf, rx);

  printf("transfered %u bytes\n", rx);
  return true;
}




// -----------------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
  printf("s1giveio v1.4 - some small debug console to access the players hardware\n");
  printf("copyright (c)2008 wiRe <wiRe@gmx.net> - http://www.s1mp3.de/ - FREEWARE\n\n");


  try
  {
    //give access to device
    printf("give access to the device...\n");
    std::list<AdfuSession::DeviceDescriptor> lstDevDescr;
    session.enumerate(lstDevDescr);
    if(lstDevDescr.empty()) throw "no device found";
    printf("open device: %s\n", lstDevDescr.front().g_strName.c_str());
    session.open(lstDevDescr.front());
    if(session.isRecoveryMode()) printf("device is in recovery state\n");
    gio.init();    
    printf("giveio version: %u.%u\n", gio.version()>>4, gio.version()&15);
    if(gio.isV9Device()) printf("v9-device detected\n");

    char cl[1024];
    bool leave = false;
    uint16 addr = 0;
    uint8 port = 0;
    uint32 size;

    printf("use command '?' for help!\n");
    do
    {
      printf("\n-");
      fgets(cl, 1000, stdin);
      cl[strlen(cl)-1]=0;
      char *clp = cl;
      while(*clp==' ' || *clp=='\t') clp++;
      

      short args = parse_values(clp+1);
      if(args < 0) printf("error parsing arguments: check syntax\n");
      else if(args >= 256) printf("error parsing arguments: too many arguments\n");
      else switch(tolower(*clp))
      {
// -----------------------------------------------------------------------------------------------
      case 'h': //display help
      case '?': //
        printf(
                  "unassemble memory              u [address] [size]\n"\
                  "dump memory                    d [address] [size]\n"\
                  "enter data into memory         e address d0 [d1] [d2] [d3]...\n"\
                  "fill memory with patterns      f address times d0 [d1] [d2]...\n\n"\
                  "multiply read same address     r address [times]\n"\
                  "multiply write same address    w address d0 [d1] [d2] [d3]...\n\n"\
                  "read from input port           i [port] [size]\n"\
                  "write to output port           o port d0 [d1] [d2] [d3]...\n\n"\
                  "dump memory to txt-file        t [address] [size]\n"\
                  "dump memory to bin-file        b [address] [size]\n"\
                  "load bin-file into memory      l [address]\n\n"\
                  "call address                   c [address] [max_time]\n"\
                  "execute bin-file               x [address] [max_time]\n\n"\
                  "quit this program              q\n"
              );
        break;


// -----------------------------------------------------------------------------------------------
      case 'u': //unassemble memory
        if(args > 1) size = value[1]; else size = 32;
        if(args > 2 || size > 0x400) printf("unknown command syntax\n");
        else if(size > 0)
        {
          if(args > 0) addr = (uint16)value[0];
          uint8 *pbuf = read_mem(addr, size+2);
          if(!pbuf) printf("failed to allocate memory\n");
          else
          {
            char buf[64];
            uint8 *data = pbuf;
            while(1)
            {
              uint16 instr = dasm(buf, data);
              if(instr > size) break;
              printf("%04X    ", addr);
              if(instr > 0) printf("%02X ", data[0]); else printf("   ");
              if(instr > 1) printf("%02X ", data[1]); else printf("   ");
              if(instr > 2) printf("%02X ", data[2]); else printf("   ");
              if(instr > 3) printf("%02X ", data[3]); else printf("   ");
              if(instr > 4) printf("%02X%c", data[4], (instr > 5)?'~':' '); else printf("   ");
              printf(" %s\n", buf);
              data += instr;
              addr += instr;
              size -= instr;
            }
            delete pbuf;
          }
        }
        break;

// -----------------------------------------------------------------------------------------------
      case 'd': //dump memory
        if(args > 1) size = value[1]; else size = 128;
        if(args > 2 || size > 0x400) printf("unknown command syntax\n");
        else if(size > 0)
        {
          if(args > 0) addr = (uint16)value[0];
          uint8 *pbuf = read_mem(addr, size);
          if(!pbuf) printf("failed to allocate memory\n");
          else
          {
            dump(pbuf, addr, size, 16);
            addr += (uint16)size;
            delete pbuf;
          }
        }
        break;

// -----------------------------------------------------------------------------------------------
      case 'b': //dump memory to file
        if(args > 1) size = value[1]; else size = 128;
        if(args > 2 || size > 0x10000) printf("unknown command syntax\n");
        else if(size > 0)
        {
          if(args > 0) addr = (uint16)value[0];
          uint8 *pbuf = read_mem(addr, size);
          if(!pbuf) printf("failed to allocate memory\n");
          else
          {
            char fname[256];
            sprintf(fname, "dump%04X.bin", addr);
            printf("write to file '%s'...\n", fname);
            FILE *fh = fopen(fname, "wb");
            if(!fh) printf("failed to create file\n");
            else
            {
              if(fwrite(pbuf, 1, size, fh) != size) printf("failed to write data to file\n");
              fclose(fh);
            }

            addr += (uint16)size;
            delete pbuf;
          }
        }
        break;

// -----------------------------------------------------------------------------------------------
      case 't': //dump memory to text file
        if(args > 1) size = value[1]; else size = 128;
        if(args > 2 || size > 0x10000) printf("unknown command syntax\n");
        else if(size > 0)
        {
          if(args > 0) addr = (uint16)value[0];
          uint8 *pbuf = read_mem(addr, size);
          if(!pbuf) printf("failed to allocate memory\n");
          else
          {
            char fname[256];
            sprintf(fname, "dump%04X.txt", addr);
            printf("write to file '%s'...\n", fname);
            FILE *fh = fopen(fname, "wt");
            if(!fh) printf("failed to create file\n");
            else
            {
              dump(pbuf, addr, size, 16, fh);
              fclose(fh);
            }

            addr += (uint16)size;
            delete pbuf;
          }
        }
        break;

// -----------------------------------------------------------------------------------------------
      case 'e': //enter data into memory
        if(args < 2) printf("too few arguments\n");
        else
        {
          addr = (uint16)value[0];
          for(short n=1; n < args; n++)
          {
            gio.st(addr, &value[n], 1);
            addr++;
          }
        }
        break;

// -----------------------------------------------------------------------------------------------
      case 'f': //fill memory with patterns
        if(args < 3) printf("too few arguments\n");
        else
        {
          addr = (uint16)value[0];
          if(value[1] > 0x1000) printf("unknown command syntax\n");
          else for(; value[1] > 0; value[1]--) for(short n=2; n < args; n++)
          {
            gio.st((uint16)addr, &value[n], 1);
            addr++;
          }
        }
        break;

// -----------------------------------------------------------------------------------------------
      case 'r': //multiply read same address
        if(args < 1) printf("too few arguments\n");
        else
        {
          if(args > 1) size = value[1]; else size = 1;
          if(args > 2 || size > 0x400)
          {
            printf("unknown command syntax\n");
          }
          else if(size > 0)
          {
            addr = (uint16)value[0];
            uint8 *pbuf = read_mem(addr, size, false);
            if(!pbuf) printf("failed to allocate memory\n");
            else
            {
              dump(pbuf, addr, size, 0);
              delete pbuf;
            }
          }
        }
        break;

// -----------------------------------------------------------------------------------------------
      case 'w': //multiply write same address
        if(args < 2) printf("too few arguments\n");
        else
        {
          addr = (uint16)value[0];
          for(short n=1; n < args; n++)
          {
            gio.st(addr, &value[n], 1);
          }
        }
        break;

// -----------------------------------------------------------------------------------------------
      case 'i': //read input port
        if(args > 1) size = value[1]; else size = 1;
        if(args > 2 || size > 0x100) printf("unknown command syntax\n");
        else
        {
          if(args > 0) port = (uint8)value[0];
          uint8 *pbuf = read_port(port, size);
          if(!pbuf) printf("failed to allocate memory\n");
          else
          {
            dump(pbuf, port, size, 8);
            port += (uint8)size;
            delete pbuf;
          }
        }
        break;

// -----------------------------------------------------------------------------------------------
      case 'o': //write output port
        if(args < 2) printf("too few arguments\n");
        else
        {
          port = (uint8)value[0];
          for(short n=1; n < args; n++)
          {
            gio.out(port, (uint8)value[n]);
            port++;
          }
        }
        break;


// -----------------------------------------------------------------------------------------------
      case 'l': //load binary file into memory
        if(args > 1 || (args > 0 && value[0] > 0xFFFF)) printf("unknown command syntax\n");
        else
        {
          if(args > 0) addr = (uint16)value[0];
          upload(addr);
        }
        break;


// -----------------------------------------------------------------------------------------------
      case 'c': //call address
        if(args > 2 || (args > 0 && value[0] > 0xFFFF)) printf("unknown command syntax\n");
        else
        {
          if(args > 0) addr = (uint16)value[0];
          if(args < 2) gio.exec(addr);
          else gio.exec(addr, (value[1]>0xFFFF)?0xFFFF:(uint16)value[1]);
        }
        break;


// -----------------------------------------------------------------------------------------------
      case 'x': //execute binary file
        if(args > 2 || (args > 0 && value[0] > 0xFFFF)) printf("unknown command syntax\n");
        else
        {
          if(args > 0) addr = (uint16)value[0];
          if(upload(addr))
          {
            if(args < 2) gio.exec(addr);
            else gio.exec(addr, (value[1]>0xFFFF)?0xFFFF:(uint16)value[1]);
          }
        }
        break;


// -----------------------------------------------------------------------------------------------
      case 'q': //quit
        leave = true;
        break;

// -----------------------------------------------------------------------------------------------
      default:
        printf("unknown command\n");
      }

    }
    while(!leave);

    //reset and close device
    gio.reset();
    session.close();
  }
  catch(const char *szError)
  {
    printf("error: %s\n", szError);
  }
  catch(AdfuException &e)
  {
    printf("error(%u): %s\n", e.id(), e.what());
  }
  catch(std::bad_alloc e)
  {
    printf("error: bad allocation exception\n");
  }
  catch(...)
  {
    printf("critical error: unhandled exception\n");
  }

  return(0);
}
