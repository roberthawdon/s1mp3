//=================================================================================================
// debugger console
//
// file license: LGPL
// copyright (c)2006 wiRe@gmx.net
//=================================================================================================
#include "common.h"
#include "debugger.h"
#include "io.h"
#include "dasm.h"




//-------------------------------------------------------------------------------------------------
// DEFS, VARS
//-------------------------------------------------------------------------------------------------
#define DEBUGGER_SYNC_CODE 7
enum {CMD_RET, CMD_SMA, CMD_RDM, CMD_WRM, CMD_RDP, CMD_WRP, CMD_RDS, CMD_WRS} CMD_VALUES;

#define UI16(lo8, hi8) (((uint16)(lo8)) | (((uint16)(hi8))<<8))

enum {                       // Bits in Z80 F register:    
  S_FLAG      = 0x80,        // 1: Result negative         
  Z_FLAG      = 0x40,        // 1: Result is zero          
  H_FLAG      = 0x10,        // 1: Halfcarry/Halfborrow    
  P_FLAG      = 0x04,        // 1: Result is even          
  V_FLAG      = 0x04,        // 1: Overflow occured        
  N_FLAG      = 0x02,        // 1: Subtraction occured     
  C_FLAG      = 0x01         // 1: Carry/Borrow occured    
} Z80_FLAG;

typedef union {
  struct {uint8 l,h;} b;
  uint16 w;
} pair;

struct {
  pair af, bc, de, hl, ix, iy, pc, sp;    // main registers
  pair af1, bc1, de1, hl1;                // shadow registers
  uint8 iff, i;                           // interrupt registers
  uint8 r;                                // refresh register
} regs, regs_org;

struct {
  bool    valid;        //breakpoint set/activated?
  uint16  addr;         //the address of the breakpoint
  uint8   org;          //the original content of the breakpoint address
} brkpnt;

uint8 pc_opcode[16];    //holds the opcodes located at the current PC

bool first_time = true; //output help message one time only




//-------------------------------------------------------------------------------------------------
// MISC FUNCTIONS
//-------------------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////////////////
void dump(const void *ptr, uint32 ofs, uint32 n, uint8 ofs_bits = 16, FILE *fh = stdout)
{
  uint32 col, ccnt=0;
  uint8 *pos = (uint8*)ptr;

  while(n >= 16)
  {
    if(ofs_bits == 8) fprintf(fh, "[%02X]      ", ofs&0xFF);
    else if(ofs_bits == 16) fprintf(fh, "[%04X]    ", ofs&0xFFFF);
    else fprintf(fh, "          ");
    for(col=0; col<8; col++) fprintf(fh, "%02X ", pos[col]);
    fprintf(fh, "- ");
    for(col=8; col<16; col++) fprintf(fh, "%02X ", pos[col]);
    fprintf(fh, "   ");
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
    if(ofs_bits == 8) fprintf(fh, "[%02X]      ", ofs&0xFF);
    else if(ofs_bits == 16) fprintf(fh, "[%04X]    ", ofs&0xFFFF);
    else fprintf(fh, "          ");
    for(col=0; col<8; col++) if(col < n) fprintf(fh, "%02X ", pos[col]); else fprintf(fh, "   ");
    fprintf(fh, "%c ", (n>7)?'-':' ');
    for(col=8; col<16; col++) if(col < n) fprintf(fh, "%02X ", pos[col]); else fprintf(fh, "   ");
    fprintf(fh, "   ");
    for(col=0; col<n; col++) if(pos[col]>=32) fprintf(fh, "%c", pos[col]); else fprintf(fh, ".");
    fprintf(fh, "\n");
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool condition(uint8 cc)   //test flag register for condition cc
{
  switch(cc&7)
  {
  case 0: return((regs.af.b.l & Z_FLAG) == 0);  //NZ
  case 1: return((regs.af.b.l & Z_FLAG) != 0);  //Z
  case 2: return((regs.af.b.l & C_FLAG) == 0);  //NC
  case 3: return((regs.af.b.l & C_FLAG) != 0);  //C
  case 4: return((regs.af.b.l & P_FLAG) == 0);  //PO
  case 5: return((regs.af.b.l & P_FLAG) != 0);  //PE
  case 6: return((regs.af.b.l & S_FLAG) == 0);  //P
  case 7: return((regs.af.b.l & S_FLAG) != 0);  //M
  }
  return false;
}




//-------------------------------------------------------------------------------------------------
// SEND DEBUGGER-COMMANDS
//-------------------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////////////////
bool dbgr_sync()
{
  return(io_sync(DEBUGGER_SYNC_CODE) == DEBUGGER_SYNC_CODE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool dbgr_leave()
{
  while(dbgr_sync()) if(io_tx(CMD_RET) >= 0) return true;
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool dbgr_mem_rd(uint16 addr, void *ptr, uint32 size)
{
  memset(ptr, 0, size);

  while(dbgr_sync())
  {
    if(io_tx(CMD_SMA) < 0) continue;  //set memory address
    if(io_tx(addr)    < 0) continue;
    if(io_tx(addr>>8) < 0) continue;

    uint32 sz = size;
    for(uint8 *bp = (uint8*)ptr; sz > 0; sz--, bp++)
    {
      if(io_tx(CMD_RDM) < 0) break;
      short d = io_tx(0);
      if(d < 0) break;
      *bp = (uint8)d;
    }
    if(sz <= 0) return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool dbgr_mem_wr(uint16 addr, const void *ptr, uint32 size)
{
  if(!size) return true;

  while(dbgr_sync())
  {
    if(io_tx(CMD_SMA) < 0) continue;  //set memory address
    if(io_tx(addr)    < 0) continue;
    if(io_tx(addr>>8) < 0) continue;

    uint32 sz = size;
    for(uint8 *bp = (uint8*)ptr; sz > 0; sz--, bp++, addr++)
    {
      if(io_tx(CMD_WRM) < 0) break;
      if(io_tx(*bp) < 0) break;

      if(addr >= regs.pc.w && addr < (regs.pc.w+sizeof(pc_opcode))) //update opcode buffer?
      {
        pc_opcode[addr - regs.pc.w] = *bp;
      }
    }
    if(sz <= 0) return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool dbgr_port_rd(short port, uint8 &data)
{
  while(dbgr_sync())
  {
    if(io_tx(CMD_RDP) < 0) continue;
    if(io_tx(port)    < 0) continue;
    short r = io_tx(0);
    if(r < 0) continue;
    data = (uint8)r;
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool dbgr_port_wr(short port, uint8 data)
{
  while(dbgr_sync())
  {
    if(io_tx(CMD_WRP) < 0) continue;
    if(io_tx(port)    < 0) continue;
    if(io_tx(data)    < 0) continue;
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool dbgr_stack_rd(short index, uint8 &data)
{
  while(dbgr_sync())
  {
    if(io_tx(CMD_RDS) < 0) continue;
    if(io_tx(index)   < 0) continue;
    short r = io_tx(0);
    if(r < 0) continue;
    data = (uint8)r;
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool dbgr_stack_wr(short index, uint8 data)
{
  while(dbgr_sync())
  {
    if(io_tx(CMD_WRS) < 0) continue;
    if(io_tx(index)   < 0) continue;
    if(io_tx(data)    < 0) continue;
    return true;
  }
  return false;
}




//-------------------------------------------------------------------------------------------------
// WORK WITH REGISTERS
//-------------------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////////////////
bool regs_read()
{
  memset(&regs, 0, sizeof(regs));
  memset(&regs_org, 0, sizeof(regs_org));

  if(!dbgr_stack_rd(-4, regs.i)) return false;
  if(!dbgr_stack_rd(-3, regs.r)) return false;
  if(!dbgr_stack_rd(-2, regs.sp.b.l)) return false;
  if(!dbgr_stack_rd(-1, regs.sp.b.h)) return false;
  regs.sp.w += 14;  //repair stack pointer
  if(!dbgr_stack_rd(0, regs.af.b.l)) return false;
  if(!dbgr_stack_rd(1, regs.af.b.h)) return false;
  if(!dbgr_stack_rd(2, regs.bc.b.l)) return false;
  if(!dbgr_stack_rd(3, regs.bc.b.h)) return false;
  if(!dbgr_stack_rd(4, regs.de.b.l)) return false;
  if(!dbgr_stack_rd(5, regs.de.b.h)) return false;
  if(!dbgr_stack_rd(6, regs.hl.b.l)) return false;
  if(!dbgr_stack_rd(7, regs.hl.b.h)) return false;
  if(!dbgr_stack_rd(8, regs.ix.b.l)) return false;
  if(!dbgr_stack_rd(9, regs.ix.b.h)) return false;
  if(!dbgr_stack_rd(10, regs.iy.b.l)) return false;
  if(!dbgr_stack_rd(11, regs.iy.b.h)) return false;
  if(!dbgr_stack_rd(12, regs.pc.b.l)) return false;
  if(!dbgr_stack_rd(13, regs.pc.b.h)) return false;

  memcpy(&regs_org, &regs, sizeof(regs_org));
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool regs_write()
{
  if(regs.af.b.l != regs_org.af.b.l)
    if(!dbgr_stack_wr(0, regs.af.b.l)) return false;
  if(regs.af.b.h != regs_org.af.b.h)
    if(!dbgr_stack_wr(1, regs.af.b.h)) return false;

  if(regs.bc.b.l != regs_org.bc.b.l)
    if(!dbgr_stack_wr(2, regs.bc.b.l)) return false;
  if(regs.bc.b.h != regs_org.bc.b.h)
    if(!dbgr_stack_wr(3, regs.bc.b.h)) return false;

  if(regs.de.b.l != regs_org.de.b.l)
    if(!dbgr_stack_wr(4, regs.de.b.l)) return false;
  if(regs.de.b.h != regs_org.de.b.h)
    if(!dbgr_stack_wr(5, regs.de.b.h)) return false;

  if(regs.hl.b.l != regs_org.hl.b.l)
    if(!dbgr_stack_wr(6, regs.hl.b.l)) return false;
  if(regs.hl.b.h != regs_org.hl.b.h)
    if(!dbgr_stack_wr(7, regs.hl.b.h)) return false;

  if(regs.ix.b.l != regs_org.ix.b.l)
    if(!dbgr_stack_wr(8, regs.ix.b.l)) return false;
  if(regs.ix.b.h != regs_org.ix.b.h)
    if(!dbgr_stack_wr(9, regs.ix.b.h)) return false;

  if(regs.iy.b.l != regs_org.iy.b.l)
    if(!dbgr_stack_wr(10, regs.iy.b.l)) return false;
  if(regs.iy.b.h != regs_org.iy.b.h)
    if(!dbgr_stack_wr(11, regs.iy.b.h)) return false;

  if(regs.pc.b.l != regs_org.pc.b.l)
    if(!dbgr_stack_wr(12, regs.pc.b.l)) return false;
  if(regs.pc.b.h != regs_org.pc.b.h)
    if(!dbgr_stack_wr(13, regs.pc.b.h)) return false;

  memcpy(&regs, &regs_org, sizeof(regs));
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool regs_print()
{
  char flags[] = "SZ.H.PNC";
  for(uint8 i=0, r=regs.af.b.l; i<8; i++, r<<=1) if(!(r&0x80)) flags[i] = '.';

  printf(
    "AF=%04X BC=%04X DE=%04X HL=%04X IX=%04X IY=%04X PC=%04X SP=%04X  I=%02X  R=%02X\n",
    regs.af.w, regs.bc.w, regs.de.w, regs.hl.w, 
    regs.ix.w, regs.iy.w, regs.pc.w, regs.sp.w,
    regs.i, regs.r
  );

  char instr[64] = "";
  uint16 sz = dasm(instr, pc_opcode);
  printf("[%04X]    ", regs.pc.w); //print program counter
  if(sz > 0) printf("%02X ", pc_opcode[0]); else printf("   ");
  if(sz > 1) printf("%02X ", pc_opcode[1]); else printf("   ");
  if(sz > 2) printf("%02X ", pc_opcode[2]); else printf("   ");
  if(sz > 3) printf("%02X%c", pc_opcode[3], (sz > 4)?'~':' '); else printf("   ");
  while(strlen(instr) < 32) strcat(instr, " ");
  printf("   %s FLAGS: [%s]\n", instr, flags);

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool pc_opcode_read()
{
  memset(pc_opcode, 0, sizeof(pc_opcode));
  return dbgr_mem_rd(regs.pc.w, &pc_opcode, 4);
}




//-------------------------------------------------------------------------------------------------
// BREAKPOINT SUPPORT
//-------------------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////////////////
bool brkpnt_available()   //return true if another breakpoint could be set
{
  return(!brkpnt.valid);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool brkpnt_remove()
{
  if(brkpnt.valid && brkpnt.addr == regs.pc.w-1)  //got our breakpoint?
  {
    regs.pc.w--;      //fix program counter    
    if(!dbgr_stack_wr(12, regs.pc.b.l)) return false;
    if(!dbgr_stack_wr(13, regs.pc.b.h)) return false;
    regs_org.pc.w--;  //we already have updated this one
  }
  else  //static breakpoint
  {
    printf("<BREAKPOINT@%04X>\n", regs.pc.w-1);
  }

  if(brkpnt.valid)  //remove last breakpoint
  {
    if(!dbgr_mem_wr(brkpnt.addr, &brkpnt.org, sizeof(brkpnt.org))) return false;
    brkpnt.valid = false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool brkpnt_set(uint16 addr)
{
  const uint8 bpi = 0xFF;

  if(!brkpnt_available()) return false;
  if(!dbgr_mem_rd(addr, &brkpnt.org, sizeof(brkpnt.org))) return false;
  if(brkpnt.org == bpi) return true;  //is a static breakpoint
  if(!dbgr_mem_wr(addr, &bpi, sizeof(bpi))) return false;
  brkpnt.addr = addr;
  brkpnt.valid = true;
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool brkpnt_set_at_next_instr()
{
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  //calc address of next instruction
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  char instr[64] = "";
  uint16 addr = regs.pc.w + dasm(instr, pc_opcode);
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  if(
    (pc_opcode[0] == 0xC3) ||                                         //JP nn
    (pc_opcode[0] == 0xCD)                                            //CALL nn
    )
  {
    addr = UI16(pc_opcode[1], pc_opcode[2]);
  }
  else if(
    ((pc_opcode[0] & 0xC7) == 0xC2) ||                                //JP cc, nn
    ((pc_opcode[0] & 0xC7) == 0xC4)                                   //CALL cc, nn
         )
  {
    if(condition( pc_opcode[0]>>3 ))
      addr = UI16(pc_opcode[1], pc_opcode[2]);
  }
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  else if(pc_opcode[0] == 0xE9)                                       //JP (HL)
  {
    if(!dbgr_mem_rd(regs.hl.w, &addr, sizeof(addr))) return false;
  }
  else if(pc_opcode[0] == 0xDD && pc_opcode[1] == 0xE9)               //JP (IX)
  {
    if(!dbgr_mem_rd(regs.ix.w, &addr, sizeof(addr))) return false;
  }
  else if(pc_opcode[0] == 0xFD && pc_opcode[1] == 0xE9)               //JP (IY)
  {
    if(!dbgr_mem_rd(regs.iy.w, &addr, sizeof(addr))) return false;
  }
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  else if(
    (pc_opcode[0] == 0xC9) ||                                         //RET
    (pc_opcode[0] == 0xED && pc_opcode[1] == 0x4D) ||                 //RETI
    (pc_opcode[0] == 0xED && pc_opcode[1] == 0x45)                    //RETN
         )
  {
    if(!dbgr_mem_rd(regs.sp.w, &addr, sizeof(addr))) return false;
  }
  else if((pc_opcode[0] & 0xC7) == 0xC0)                              //RET cc
  {
    if(condition( pc_opcode[0]>>3 ))
      if(!dbgr_mem_rd(regs.sp.w, &addr, sizeof(addr))) return false;
  }
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  else if(
    (pc_opcode[0] == 0x18) ||                                         //JR e
    (pc_opcode[0] == 0x38 && (regs.af.b.l & C_FLAG)) ||               //JR C, e
    (pc_opcode[0] == 0x30 && !(regs.af.b.l & C_FLAG)) ||              //JR NC, e
    (pc_opcode[0] == 0x28 && (regs.af.b.l & Z_FLAG)) ||               //JR Z, e
    (pc_opcode[0] == 0x20 && !(regs.af.b.l & Z_FLAG)) ||              //JR NZ, e
    (pc_opcode[0] == 0x10 && regs.bc.b.h != 0x01)                     //DJNZ e
         )
  {
    addr += (int8)pc_opcode[1];
  }
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  else if(
    (pc_opcode[0] & 0xC7) == 0xC7 &&                                  //RST p
    (pc_opcode[0] != 0xF7) &&                                         //  (without RST 30h)
    (pc_opcode[0] != 0xFF)                                            //  (without RST 38h)
         )
  {
    addr = (pc_opcode[0] & 0x38);
  }
  //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

  //set breakpoint and return
  return brkpnt_set(addr);
}




//-------------------------------------------------------------------------------------------------
// CONSOLE PARSER
//-------------------------------------------------------------------------------------------------
char cl[1024] = "";
char *clp = cl;

///////////////////////////////////////////////////////////////////////////////////////////////////
void parse_error(const char *msg = NULL)
{
  for(int i = (int)(clp-cl); i >= 0; i--) putch(' ');
  putch('^');
  putch(' ');
  if(msg) puts(msg);
  else puts("syntax error");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void parse_skip()
{
  while(*clp==' ' || *clp=='\t') clp++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool parse_str(char* str, uint32 min, uint32 max)
{
  memset(str, 0, max);
  parse_skip();
  for(; *clp!=0; max--, str++, clp++)
  {
    if(*clp==' ' || *clp=='\t') break;
    if(max <= 1) return false;
    *str = *clp;
    if(min > 0) min--;
  }
  return(min == 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool parse_uint32(uint32 &val, uint32 min, uint32 max)
{
  parse_skip();
  if(*clp == 0) return false;
  unsigned long ul = strtoul(clp, &clp, 16);
  if(*clp!=' ' && *clp!='\t' && *clp!=0) return false;
  if(ul < min || ul > max) return false;
  val = (uint32)ul;
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool parse_uint16(uint16 &val, uint16 min, uint16 max)
{
  parse_skip();
  if(*clp == 0) return false;
  unsigned long ul = strtoul(clp, &clp, 16);
  if(*clp!=' ' && *clp!='\t' && *clp!=0) return false;
  if(ul < min || ul > max) return false;
  val = (uint16)ul;
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool parse_uint8(uint8 &val, uint8 min, uint8 max)
{
  parse_skip();
  if(*clp == 0) return false;
  unsigned long ul = strtoul(clp, &clp, 16);
  if(*clp!=' ' && *clp!='\t' && *clp!=0) return false;
  if(ul < min || ul > max) return false;
  val = (uint8)ul;
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool parse_end()
{
  parse_skip();
  return(*clp == 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
char parse_gets() //returns first character (command)
{
  putch('-');

  gets(cl);
  clp = cl;

  parse_skip();
  if(!*clp) return 0;
  return tolower(*(clp++));
}




//-------------------------------------------------------------------------------------------------
// DEBUG CONSOLE
//-------------------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////////////////////
bool debugger()
{
  puts("");

  //load registers
  if(!regs_read()) return false;

  //remove dynamic breakpoints
  if(!brkpnt_remove()) return false;

  //read the current opcode and display the registers/opcode
  if(!pc_opcode_read()) return false;
  if(!regs_print()) return false;

  //print help one time
  if(first_time)
  {
    puts("\nuse '?' command to display help");
    first_time = false;
  }

  //finally, the debug console...
  bool leave = false;
  uint16 addr = regs.pc.w;
  uint16 port = 0;

  do
  {
    puts("");
    switch( parse_gets() )
    {
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 0: //no command specified
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'h': //display help
    case '?': //
      {
        if(!parse_end()) parse_error();
        else puts(
              "display/alter registers        r [register value]\n"\
              "jump to address (set pc)       j [address]\n\n"\

              "unassemble memory              u [address [range]]\n"\
              "dump memory                    d [address [range]]\n"\
              "multiply read same address     m address [times]\n"\
              "enter data into memory         e address d0 [d1 [d2 ... d15]]\n"\
              "fill memory with pattern       f address times d0 [d1 [d2 ... d15]]\n\n"\

              "read from input port           i [port [range]]\n"\
              "write to output port           o port d0 [d1 [d2 ... d15]]\n\n"\

              "load file into memory          l filename [address [range]]\n"\
              "store memory into file         s filename [address [range]]\n\n"\

              "proceed to address             p [address]\n"\
              "trace through code             t\n"\
              "execute (leave debugger)       x"
            );
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'r': //display/alter registers
      {
        if(!parse_end())
        {
          char reg[3];
          uint16 val;
          uint16 *rp = NULL;

          if(!parse_str(reg, 2, sizeof(reg))) {parse_error(); break;}
              if(stricmp(reg, "af") == 0) rp = &regs.af.w;
          else if(stricmp(reg, "bc") == 0) rp = &regs.bc.w;
          else if(stricmp(reg, "de") == 0) rp = &regs.de.w;
          else if(stricmp(reg, "hl") == 0) rp = &regs.hl.w;
          else if(stricmp(reg, "ix") == 0) rp = &regs.ix.w;
          else if(stricmp(reg, "iy") == 0) rp = &regs.iy.w;
          else if(stricmp(reg, "pc") == 0 ||
                  stricmp(reg, "sp") == 0) {parse_error("register not allowed"); break;}
          else {parse_error("register expected"); break;}
          if(!parse_uint16(val, 0, 0xFFFF) || !parse_end()) {parse_error(); break;}
          if(rp) *rp = val;
        }
        if(!regs_print()) return false;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'j': //jump to address (set new program counter)
      {
        if(!parse_end() && (!parse_uint16(addr, 0, 0xFFFF) || !parse_end())) {parse_error(); break;}
        if(regs.pc.w != addr) //update program counter
        {
          regs.pc.w = addr;
          pc_opcode_read();
        }
        if(!regs_print()) return false;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'u': //unassemble memory
      {
        uint16 range = 32;

        if(!parse_end())
        {
          if(!parse_uint16(addr, 0, 0xFFFF)) {parse_error(); break;}
          if(!parse_end() && (!parse_uint16(range, 1, 128) || !parse_end())) {parse_error(); break;}
        }

        uint8 *pbuf = new uint8[range+16];
        if(!pbuf) {puts("error: failed to allocate memory"); break;}
        if(!dbgr_mem_rd(addr, pbuf, range)) {delete pbuf; return false;}
          
        char instr[64];
        uint8 *ip = pbuf;
        while(1)
        {
          uint16 sz = dasm(instr, ip);
          if(sz > range) break;
          printf("[%04X]    ", addr);
          if(sz > 0) printf("%02X ", ip[0]); else printf("   ");
          if(sz > 1) printf("%02X ", ip[1]); else printf("   ");
          if(sz > 2) printf("%02X ", ip[2]); else printf("   ");
          if(sz > 3) printf("%02X%c", ip[3], (sz > 4)?'~':' '); else printf("   ");
          printf("   %s\n", instr);
          ip += sz;
          addr += sz;
          range -= sz;
        }

        delete pbuf;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'd': //dump memory
      {
        uint16 range = 128;

        if(!parse_end())
        {
          if(!parse_uint16(addr, 0, 0xFFFF)) {parse_error(); break;}
          if(!parse_end() && (!parse_uint16(range, 1, 0xFFF) || !parse_end())) {parse_error(); break;}
        }

        uint8 *pbuf = new uint8[range];
        if(!pbuf) {puts("error: failed to allocate memory"); break;}
        if(!dbgr_mem_rd(addr, pbuf, range)) {delete pbuf; return false;}

        dump(pbuf, addr, range, 16);
        addr += range;

        delete pbuf;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'm': //multiply read same address
      {
        uint16 times = 1;

        if(!parse_uint16(addr, 0, 0xFFFF)) {parse_error(); break;}
        if(!parse_end() && (!parse_uint16(times, 1, 0xFFF) || !parse_end())) {parse_error(); break;}

        uint8 *pbuf = new uint8[times];
        if(!pbuf) {puts("error: failed to allocate memory"); break;}
        for(uint16 n=0; n < times; n++)
          if(!dbgr_mem_rd(addr, &pbuf[n], 1)) {delete pbuf; return false;}
        dump(pbuf, addr, times, 0);
        delete pbuf;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'e': //enter data into memory
      {        
        uint16 items = 0;
        uint8 data[16];

        if(!parse_uint16(addr, 0, 0xFFFF)) {parse_error(); break;}
        do {
          if(!parse_uint8(data[items++], 0, 0xFF)) {parse_error(); break;}
          if(items > 15 && !parse_end()) {parse_error(); break;}
        } while(!parse_end());

        if(!dbgr_mem_wr(addr, data, items)) return false;
        addr += items;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'f': //fill memory with patterns
      {        
        uint16 times = 0;
        uint16 items = 0;
        uint8 data[16];

        if(!parse_uint16(addr, 0, 0xFFFF)) {parse_error(); break;}
        if(!parse_uint16(times, 1, 0xFFF)) {parse_error(); break;}
        do {
          if(!parse_uint8(data[items++], 0, 0xFF)) {parse_error(); break;}
          if(items > 15 && !parse_end()) {parse_error(); break;}
        } while(!parse_end());

        for(; times > 0; times--)
        {
          if(!dbgr_mem_wr(addr, data, items)) return false;
          addr += items;
        }
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'i': //read input port
      {
        uint16 range = 1;

        if(!parse_uint16(port, 0, 0xFF)) {parse_error(); break;}
        if(!parse_end() && (!parse_uint16(range, 1, 0xFF) || !parse_end())) {parse_error(); break;}

        uint8 *pbuf = new uint8[range];
        if(!pbuf) {puts("error: failed to allocate memory"); break;}
        for(uint16 n=0; n < range; n++)
          if(!dbgr_port_rd(port+n, pbuf[n])) {delete pbuf; return false;}
        dump(pbuf, port, range, 8);
        port += range;
        delete pbuf;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'o': //write output port
      {        
        uint16 items = 0;
        uint8 data[16];

        if(!parse_uint16(port, 0, 0xFF)) {parse_error(); break;}
        do {
          if(!parse_uint8(data[items++], 0, 0xFF)) {parse_error(); break;}
          if(items > 15 && !parse_end()) {parse_error(); break;}
        } while(!parse_end());

        for(uint16 n=0; n < items; n++, port++)
          if(!dbgr_port_wr(port, data[n])) return false;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'l': //load file into memory
      {
        uint16 range = 0xFFFF;
        char fname[512];

        if(!parse_str(fname, 1, sizeof(fname))) {parse_error(); break;}
        if(!parse_end())
        {
          if(!parse_uint16(addr, 0, 0xFFFF)) {parse_error(); break;}
          if(!parse_end() && (!parse_uint16(range, 1, 0xFFFF) || !parse_end())) {parse_error(); break;}
        }

        uint8 *pbuf = new uint8[range];
        if(!pbuf) {puts("error: failed to allocate memory"); break;}

        FILE *fh = fopen(fname, "rb");
        if(!fh) puts("error: failed to open file");
        else
        {
          range = (uint16)fread(pbuf, 1, range, fh);
          fclose(fh);

          if(!range) puts("error: failed to read data from file");
          else
          {
            if(!dbgr_mem_wr(addr, pbuf, range)) return false;
            printf("%u bytes loaded to address %04X\n", range, addr);
            addr += range;
          }
        }

        delete pbuf;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 's': //save memory into file
      {
        uint16 range = 128;
        char fname[512];

        if(!parse_str(fname, 1, sizeof(fname))) {parse_error(); break;}
        if(!parse_end())
        {
          if(!parse_uint16(addr, 0, 0xFFFF)) {parse_error(); break;}
          if(!parse_end() && (!parse_uint16(range, 1, 0xFFFF) || !parse_end())) {parse_error(); break;}
        }

        uint8 *pbuf = new uint8[range];
        if(!pbuf) {puts("error: failed to allocate memory"); break;}
        if(!dbgr_mem_rd(addr, pbuf, range)) {delete pbuf; return false;}

        FILE *fh = fopen(fname, "wb");
        if(!fh) puts("error: failed to create file");
        else
        {
          range = (uint16)fwrite(pbuf, 1, range, fh);
          fclose(fh);

          printf("%u bytes saved from address %04X\n", range, addr);
          addr += range;
        }

        delete pbuf;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'p': //proceed to address (place dynamic breakpoint at address and leave debugger)
      {        
        if(!parse_end() && (!parse_uint16(addr, 0, 0xFFFF) || !parse_end())) {parse_error(); break;}
        if(!brkpnt_available()) {puts("no more breakpoint slots available"); break;}
        if(!brkpnt_set(addr)) return false;
        leave = true;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 't': //trace (place dynamic breakpoint after current instruction and leave debugger)
      { 
        if(!parse_end()) {parse_error(); break;}
        if(!brkpnt_available()) {puts("no more breakpoint slots available"); break;}
        if(!brkpnt_set_at_next_instr()) return false;
        leave = true;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    case 'x': //execute/exit
    case 'q': //quit
      {
        if(!parse_end()) {parse_error(); break;}
        leave = true;
      }
      break;

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    default:
      parse_error("unknown command");
    }
  }
  while(!leave);

  //write registers back and return
  if(!regs_write()) return false;
  return dbgr_leave();
}


///////////////////////////////////////////////////////////////////////////////////////////////////
bool debugger_init()
{
  first_time = true;
  memset(&regs, 0, sizeof(regs));
  memset(&brkpnt, 0, sizeof(brkpnt));
  return true;
}
