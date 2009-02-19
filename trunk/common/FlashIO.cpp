//-------------------------------------------------------------------------------------------------
// flashio: support for different nand flash chips used by s1mp3 players
//          to access the chips, giveio is needed to run on the target player
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//-------------------------------------------------------------------------------------------------
#include "FlashIO.h"

#define FLASHIO_USE_FAST_DMA

//
// TODO
//
// dma-transfer is ultra-slow when v9 device is in recovery mode (not initialized properly)
//

#define SUB_ADDR 0x3C80

#define EM_PAGE_LO_REG        0x01
#define EM_PAGE_HI_REG        0x02

#define NAND_ENABLE_REG       0x28
#define SNAN_CEMODE_REG       0x29
#define SNAN_CMD_REG          0x2a
#define SNAN_CA_REG           0x2b    //write only
#define SNAN_RA_REG           0x2c    //write only
#define SNAN_BA_REG1          0x2d
#define SNAN_BA_REG2          0xec
#define NAND_ECCCTRL_REG      0xcc
#define NAND_ECC_REG0         0xcd
#define NAND_ECC_REG1         0xce    //def=10
#define NAND_ECC_REG2         0xcf

#define SNAN_CMD_STOP         0x37
#define SNAN_CMD_RESET        0xff
#define SNAN_CMD_READ_STATUS  0x70
#define SNAN_CMD_READ_ID      0x90
#define SNAN_CMD_READ         0x00
#define SNAN_CMD_READ_2ND     0x30
#define SNAN_CMD_PROG         0x80
#define SNAN_CMD_PROG_2ND     0x10
#define SNAN_CMD_ERASE        0x60
#define SNAN_CMD_ERASE_2ND    0xD0

#define SNAN_STATE_FAILED     (1<<0)
#define SNAN_STATE_READY      (1<<6)
#define SNAN_STATE_UNPROT     (1<<7)

#define SMALL_BLOCKS          16  //pages
#define MEDIUM_BLOCKS         32  //pages
#define LARGE_BLOCKS          64  //pages

#define BYTE0(_x_) ((uint8)((_x_)&255))
#define BYTE1(_x_) ((uint8)(((_x_)>>8)&255))
#define BYTE2(_x_) ((uint8)(((_x_)>>16)&255))
#define BYTE3(_x_) ((uint8)(((_x_)>>24)&255))
#define BYTE4(_x_) 0 //((uint8)(((_x_)>>32)&255))
#define BYTE5(_x_) 0 //((uint8)(((_x_)>>40)&255))
#define BYTE6(_x_) 0 //((uint8)(((_x_)>>48)&255))
#define BYTE7(_x_) 0 //((uint8)(((_x_)>>56)&255))

// some macros to generate z80 code
#define Z80_NOP 0x00
#define Z80_IN(_port_) 0xDB, _port_
#define Z80_OUT(_port_, _value_) 0x3E, _value_, 0xD3, _port_
#define Z80_LDA(_addr_, _value_) 0x3E, _value_, 0x32, BYTE0(_addr_), BYTE1(_addr_)
#define Z80_LDR_A(_value_) 0x3E, _value_
#define Z80_LDR_B(_value_) 0x06, _value_
#define Z80_LDR_A_ADDR(_addr_) 0x3A, BYTE0(_addr_), BYTE1(_addr_)
#define Z80_BIT_A(_bit_offs_) 0xCB, 0x47 | (((_bit_offs_)&7)<<3)
#define Z80_DJNZ(_rel_addr_) 0x10, _rel_addr_
#define Z80_JRNZ(_rel_addr_) 0x20, _rel_addr_
#define Z80_JRZ(_rel_addr_) 0x28, _rel_addr_
#define Z80_RET 0xC9

#define TIMEOUT_FOR_ERASE 1000
#define TIMEOUT_FOR_WRITE 1000


/*
  ------------------------------
  access flash on non-v9 devices
  ------------------------------

    init:
      EM_PAGE_LO_REG = 0
      EM_PAGE_HI_REG = cs_line << 3
      SNAN_CEMODE_REG = 1
      NAND_ENABLE_REG = (cs_line==0)? 0 : (1<<cs_line)

    command:
      SNAN_CMD_REG = SNAN_CMD_READ_STATUS
      EM_PAGE_LO_REG = 0
      EM_PAGE_LO_REG = 1
      [0x8888] = SNAN_CMD_1ST
      EM_PAGE_LO_REG = 2
      [0x8888] = address (upto 5 cycles)
      EM_PAGE_LO_REG = 0
      [0x8888] <- input
      SNAN_CMD_REG = SNAN_CMD_2ND
      [0x8888] -> output


  --------------------------
  access flash on v9 devices
  --------------------------

    init:
      EM_PAGE_LO_REG = 0
      EM_PAGE_HI_REG = cs_line << 3
      PORT29 = (PORT29 & 0xF7) | 0x14
      NAND_ENABLE_REG = ((cs_line==0)? 0 : (2<<cs_line)) | 2
      PORT2B = 0

    command:
      cmd_arg_address = 0x3FC0, 0x3FD0, 0x3FE0, 0x3FF0
      [cmd_arg_address] -> 8 bytes command arguments (flash address)
      PORT2B = (PORT2B & 0b11000000) | (((cmd_arg_address - 0x3FC0) << 2) & 0b11000000)
      SNAN_CMD_REG = SNAN_CMD_1ST
      wait while ((NAND_ENABLE_REG&1) == 1)
      [0x8888] <- input
      SNAN_CMD_REG = SNAN_CMD_2ND
      [0x8888] -> output

*/




//-------------------------------------------------------------------------------------------------
// open selects cs-line and detects flash chip
//-------------------------------------------------------------------------------------------------
bool FlashIO::open(uint8 cs_line)
{
  this->cs_line = cs_line;

  // clear interrupt table memory
  uint8 mem[256];
  for(int i=0; i < sizeof(mem); i++) mem[i] = (i&1)? Z80_NOP : Z80_RET;
  lpgio->st(0, mem, sizeof(mem));

  // detect the nand-statemachine we have
  lpgio->out(SNAN_CEMODE_REG, 1);
  v9sm = lpgio->in(SNAN_CEMODE_REG) != 1;   //TODO: may result to true for some v9 devices which don't have a v9sm

  // initialize state machine
  if(v9sm)
  {
    lpgio->out(EM_PAGE_LO_REG, 0);
    lpgio->out(EM_PAGE_HI_REG, cs_line << 3);
    lpgio->out(0x29, 0xF4);
    lpgio->out(NAND_ENABLE_REG, ((cs_line==0)? 0 : (2<<cs_line)) | 2);
    lpgio->out(0x2B, 0xC0); //set cmd_arg_address to 0x3FF0
  }
  else
  {
    lpgio->out(EM_PAGE_LO_REG, 0);
    lpgio->out(EM_PAGE_HI_REG, cs_line << 3);
    lpgio->out(SNAN_CEMODE_REG, 1);
    lpgio->out(NAND_ENABLE_REG, ((cs_line==0)? 0 : (1<<cs_line)));
  }

  // scan for flash chip
  bool result = false;
  for(short t=64; 1; t--)
  {
    lpgio->out(SNAN_CMD_REG, SNAN_CMD_RESET);
    Sleep(1);
    lpgio->out(SNAN_CMD_REG, SNAN_CMD_READ_STATUS);

    uint8 state = lpgio->ld(0x8000);
    if(state & SNAN_STATE_READY)  //ready-status-bit set?
    {
      if(!(state & SNAN_STATE_FAILED))  //error-status-bit cleared?
      {
        prot = !(state & SNAN_STATE_UNPROT);  //get write protection flag

        lpgio->out(SNAN_CMD_REG, SNAN_CMD_READ_ID);   //read id
        Sleep(1);
        lpgio->ld(0x8000, &this->id, sizeof(this->id));
        if( verify_device_id() ) result = true;
        break;
      }
    }

    if(t <= 0)  //tried long enough?
    {
      lpgio->out(SNAN_CMD_REG, SNAN_CMD_RESET);
      break;
    }

    Sleep(1);
  }

  //lpgio->out(SNAN_CMD_REG, SNAN_CMD_STOP);
  return result;
}


//-------------------------------------------------------------------------------------------------
// deinit flash
//-------------------------------------------------------------------------------------------------
void FlashIO::close()
{
  cs_line = 0;
  id = 0;
  blk_size = 0;
  size = 0;
  bool prot = false;
  device_descr_str[0] = 0;
}


//-------------------------------------------------------------------------------------------------
// detects the kind of flash chip we have found or returns false if not supported
//-------------------------------------------------------------------------------------------------
bool FlashIO::verify_device_id()
{
  /*
  block size:
    S: small
    M: medium (32k)
    L: large

  toshiba chips (BYTE0(id) == 98):
    BYTE1(id):  75      76      79      79      F1      DA
    BYTE0(id2):                 !24     24
    size/block: 32/S    64/S    128/S   128/M   128/L   256/L

  hynix/samsung chips (BYTE0(id) == EC/AD/20):
    BYTE1(id):  75      76      79      F1      71      DA      DC      DC      D3      D5
    BYTE3(id):                                                  !15     15  
    size/block: 32/S    64/S    128/S   128/L   256/S   256/L   512/S   512/L   1G/L    2G/L
  */

  size = 0;
  blk_size = 0;

  switch(BYTE0(this->id))
  {
  case 0x98: //toshiba
    switch(BYTE1(this->id))
    {
    case 0x75: size = 32;  blk_size = SMALL_BLOCKS; return true;
    case 0x76: size = 64;  blk_size = SMALL_BLOCKS; return true;
    case 0x79: size = 128; blk_size = 0; return true;
    case 0xF1: size = 128; blk_size = LARGE_BLOCKS; return true;
    case 0xDA: size = 256; blk_size = LARGE_BLOCKS; return true;
    }
    break;

  case 0xEC: //hynix/samsung
  case 0xAD:
  case 0x20:
    switch(BYTE1(this->id))
    {
    case 0x75: size = 32;   blk_size = SMALL_BLOCKS; return true;
    case 0x76: size = 64;   blk_size = SMALL_BLOCKS; return true;
    case 0x79: size = 128;  blk_size = SMALL_BLOCKS; return true;
    case 0xF1: size = 128;  blk_size = LARGE_BLOCKS; return true;
    case 0x71: size = 256;  blk_size = SMALL_BLOCKS; return true;
    case 0xDA: size = 256;  blk_size = LARGE_BLOCKS; return true;
    case 0xDC: size = 512;  blk_size = (BYTE3(this->id) != 0x15)? SMALL_BLOCKS : LARGE_BLOCKS; return true;
    case 0xD3: size = 1024; blk_size = LARGE_BLOCKS; return true;
    case 0xD5: size = 2048; blk_size = LARGE_BLOCKS; return true;
    }
    break;
  }

  return false;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const char *FlashIO::get_device_descr()
{
  sprintf(device_descr_str, "unknown");

  char bf = '?';
  switch(blk_size)
  {
  case SMALL_BLOCKS:  bf = 'S'; break;
  case MEDIUM_BLOCKS: bf = 'M'; break;
  case LARGE_BLOCKS:  bf = 'L'; break;
  }

  if(size) switch(BYTE0(this->id))
  {
  case 0x98: sprintf(device_descr_str, "TOSHIBA %uMB (%cBF)", size, bf); break;
  case 0xEC: 
  case 0xAD: 
  case 0x20: sprintf(device_descr_str, "HYNIX/SAMSUNG %uMB (%cBF)", size, bf); break;
  }

  return device_descr_str;
}


//-------------------------------------------------------------------------------------------------
// erase one block of the flash
//-------------------------------------------------------------------------------------------------
bool FlashIO::erase_block(uint32 page)
{
  const unsigned __int8 sub[] =                 //erase flash block
  {
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_READ_STATUS),
    Z80_OUT(EM_PAGE_LO_REG, 0),
    Z80_OUT(EM_PAGE_LO_REG, 1),
    Z80_LDA(0x8888, SNAN_CMD_ERASE),            //1st command cycle
    Z80_OUT(EM_PAGE_LO_REG, 2),
    Z80_LDA(0x8888, BYTE0(page)),               //3 address cycles
    Z80_LDA(0x8888, BYTE1(page)),
    Z80_LDA(0x8888, BYTE2(page)),
    Z80_OUT(EM_PAGE_LO_REG, 0),
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_ERASE_2ND),  //2nd command cycle

    Z80_LDR_B(0),                               //wait until operation has finished
    Z80_DJNZ(-2),
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_READ_STATUS),
    Z80_LDR_A_ADDR(0x8888),
    Z80_BIT_A(6),
    Z80_JRZ(-7),

    Z80_RET                                     //return
  };

  const unsigned __int8 sub_v9sm[] =            //same function for the extended statemachine
  {
    Z80_LDA(0x3FF0, 0),                         //place address
    Z80_LDA(0x3FF1, 0),
    Z80_LDA(0x3FF2, BYTE0(page)),
    Z80_LDA(0x3FF3, BYTE1(page)),
    Z80_LDA(0x3FF4, BYTE2(page)),
    Z80_LDA(0x3FF5, BYTE3(page)),
    Z80_LDA(0x3FF6, BYTE4(page)),
    Z80_LDA(0x3FF7, BYTE5(page)),
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_ERASE),      //1st command cycle
    Z80_LDR_B(0),                               //wait until state machine signals ready
    Z80_DJNZ(-2),
    Z80_IN(0x28),
    Z80_BIT_A(0),
    Z80_JRNZ(-6),
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_ERASE_2ND),  //2nd command cycle

    Z80_LDR_B(0),                               //wait until operation has finished
    Z80_DJNZ(-2),
    Z80_IN(0x29),
    Z80_BIT_A(6),
    Z80_JRZ(-6),

    Z80_RET                                     //return
  };

  lpgio->exec(SUB_ADDR, v9sm? sub_v9sm : sub, v9sm? sizeof(sub_v9sm) : sizeof(sub), 10);  //exec sub
  return((lpgio->ld(0x8888) & 1) == 0);   //return false if erase failed
}


//-------------------------------------------------------------------------------------------------
// read one page from flash
//-------------------------------------------------------------------------------------------------
bool FlashIO::read_page(uint32 page, uint16 offs, void *ptr, uint32 size)
{
  if(!ptr) return false;
  if(size <= 0) return true;
  if(size > REAL_PAGE_SIZE) size = REAL_PAGE_SIZE;

  const unsigned __int8 sub[] =                 //read flash page
  {
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_READ_STATUS),
    Z80_OUT(EM_PAGE_LO_REG, 0),
    Z80_OUT(EM_PAGE_LO_REG, 1),
    Z80_LDA(0x8888, SNAN_CMD_READ),             //1st command cycle
    Z80_OUT(EM_PAGE_LO_REG, 2),
    Z80_LDA(0x8888, BYTE0(offs)),               //4 address cycles
    Z80_LDA(0x8888, BYTE1(offs)),
    Z80_LDA(0x8888, BYTE0(page)),
    Z80_LDA(0x8888, BYTE1(page)),
    Z80_LDA(0x8888, BYTE2(page)),               //samsung chips need a 5th adress cycle
    Z80_OUT(EM_PAGE_LO_REG, 0),
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_READ_2ND),   //2nd command cycle

    Z80_LDR_B(0),                               //wait at least 25us before reading data
    Z80_DJNZ(-2),
    
    Z80_RET                                     //return
  };

  const unsigned __int8 sub_v9sm[] =            //same function for the extended statemachine
  {
    Z80_LDA(0x3FF0, BYTE0(offs)),               //place address
    Z80_LDA(0x3FF1, BYTE1(offs)),
    Z80_LDA(0x3FF2, BYTE0(page)),
    Z80_LDA(0x3FF3, BYTE1(page)),
    Z80_LDA(0x3FF4, BYTE2(page)),
    Z80_LDA(0x3FF5, BYTE3(page)),
    Z80_LDA(0x3FF6, BYTE4(page)),
    Z80_LDA(0x3FF7, BYTE5(page)),
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_READ),       //1st command cycle
    Z80_LDR_B(0),                               //wait until state machine signals ready
    Z80_DJNZ(-2),
    Z80_IN(0x28),
    Z80_BIT_A(0),
    Z80_JRNZ(-6),
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_READ_2ND),   //2nd command cycle

    Z80_LDR_B(0),                               //wait until operation has finished
    Z80_DJNZ(-2),
    Z80_IN(0x29),
    Z80_BIT_A(6),
    Z80_JRZ(-6),
    
    Z80_RET                                     //return
  };

  lpgio->exec(SUB_ADDR, v9sm? sub_v9sm : sub, v9sm? sizeof(sub_v9sm) : sizeof(sub), 10);  //exec sub

  if(ptr && size)
  {
    #ifdef FLASHIO_USE_FAST_DMA
      lpgio->ld_dma((0x80 + (cs_line << 3)) << 24, 0, 0xC0, ptr, size);	//rx data via fast dma transfer (3-wait-cycles)
    #else
      lpgio->ld(0x8000, ptr, size);
    #endif
  }
  return true;
}


//-------------------------------------------------------------------------------------------------
// write one page into flash
//-------------------------------------------------------------------------------------------------
bool FlashIO::write_page(uint32 page, uint16 offs, void *ptr, uint32 size)
{
  if(!ptr) return false;
  if(size <= 0) return true;
  if(size > REAL_PAGE_SIZE) size = REAL_PAGE_SIZE;

  const unsigned __int8 sub[] =
  {
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_READ_STATUS),
    Z80_OUT(EM_PAGE_LO_REG, 0),
    Z80_OUT(EM_PAGE_LO_REG, 1),
    Z80_LDA(0x8888, SNAN_CMD_PROG),             //1st command cycle
    Z80_OUT(EM_PAGE_LO_REG, 2),
    Z80_LDA(0x8888, BYTE0(offs)),               //4 address cycles
    Z80_LDA(0x8888, BYTE1(offs)),
    Z80_LDA(0x8888, BYTE0(page)),
    Z80_LDA(0x8888, BYTE1(page)),
    Z80_LDA(0x8888, BYTE2(page)),               //samsung chips need a 5th adress cycle
    Z80_OUT(EM_PAGE_LO_REG, 0),

    //Z80_NOP,                                  //wait at least 100ns before writing data

    Z80_RET                                     //return
  };

  const unsigned __int8 sub_v9sm[] =            //same function for the extended statemachine
  {
    Z80_LDA(0x3FF0, BYTE0(offs)),               //place address
    Z80_LDA(0x3FF1, BYTE1(offs)),
    Z80_LDA(0x3FF2, BYTE0(page)),
    Z80_LDA(0x3FF3, BYTE1(page)),
    Z80_LDA(0x3FF4, BYTE2(page)),
    Z80_LDA(0x3FF5, BYTE3(page)),
    Z80_LDA(0x3FF6, BYTE4(page)),
    Z80_LDA(0x3FF7, BYTE5(page)),
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_PROG),       //1st command cycle
    
    Z80_LDR_B(0),                               //wait until state machine signals ready
    Z80_DJNZ(-2),
    Z80_IN(0x28),
    Z80_BIT_A(0),
    Z80_JRNZ(-6),

    Z80_RET                                     //return
  };

  lpgio->exec(SUB_ADDR, v9sm? sub_v9sm : sub, v9sm? sizeof(sub_v9sm) : sizeof(sub), 10);  //exec sub

  #ifdef FLASHIO_USE_FAST_DMA
    lpgio->st_dma((0x80 + (cs_line << 3)) << 24, 0, 0xC0, ptr, size); //tx data via fast dma transfer (3-wait-cycles)
  #else
    lpgio->st(0x8000, ptr, size);               //tx data
  #endif

  const unsigned __int8 sub_fin[] =             //finalize the command
  {
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_PROG_2ND),   //2nd command cycle

    Z80_LDR_B(0),                               //wait until operation has finished
    Z80_DJNZ(-2),
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_READ_STATUS),
    Z80_LDR_A_ADDR(0x8888),
    Z80_BIT_A(6),
    Z80_JRZ(-7),

    Z80_RET                                     //return
  };

  const unsigned __int8 sub_fin_v9sm[] =        //same function for the extended statemachine
  {
    Z80_OUT(SNAN_CMD_REG, SNAN_CMD_PROG_2ND),   //2nd command cycle

    Z80_LDR_B(0),                               //wait until operation has finished
    Z80_DJNZ(-2),
    Z80_IN(0x29),
    Z80_BIT_A(6),
    Z80_JRZ(-6),

    Z80_RET                                     //return
  };

  lpgio->exec(SUB_ADDR, v9sm? sub_fin_v9sm : sub_fin, v9sm? sizeof(sub_fin_v9sm) : sizeof(sub_fin), 10);
  return((lpgio->ld(0x8888) & 1) == 0);   //return false if write failed
}
