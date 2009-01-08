#include "common.h"
#include "dump.h"


// -----------------------------------------------------------------------------------------------
#define COLOR_HEX       10
#define COLOR_ASCII     11
#define COLOR_UNDEFINED 3
#define COLOR_ADDRESS   11
#define COLOR_SEPERATOR 3
#define COLOR_INPUT     15
#define COLOR_DEFAULT   7


// -----------------------------------------------------------------------------------------------
void dump(const void *ptr, long n, bool pgbrk)
{
  int col,ccnt=0;
  unsigned char *pos = (unsigned char*)ptr;
  unsigned int ofs = 0;

  while(n >= 16) {
    SET_COLOR(COLOR_ADDRESS);
    printf("%04X    ", ofs);
    SET_COLOR(COLOR_HEX);
    for(col=0; col<8; col++) printf("%02X ", pos[col]);
    SET_COLOR(COLOR_SEPERATOR);
    printf("- ");
    SET_COLOR(COLOR_HEX);
    for(col=8; col<16; col++) printf("%02X ", pos[col]);
    SET_COLOR(COLOR_ASCII);
    printf("    ");
    for(col=0; col<16; col++) if(pos[col]>=32 && pos[col] < 0xFF) putchar(pos[col]);
      else {SET_COLOR(COLOR_UNDEFINED); putchar('.'); SET_COLOR(COLOR_ASCII);}
    printf("\n");

    if(pgbrk) if(++ccnt >= 24) {
      ccnt = 0;
      SET_COLOR(COLOR_INPUT);
      printf("press key to continue, ESC to cancel...");
      col = _getch();
      SET_COLOR(COLOR_DEFAULT);
      printf("\n");
      if(col == 27) return;
    }

    pos+=16;
    ofs+=16;
    n-=16;
  }

  if(n>0) {
    SET_COLOR(COLOR_ADDRESS);
    printf("%04X    ", ofs);
    SET_COLOR(COLOR_HEX);
    for(col=0; col<8; col++) if(col < n) printf("%02X ", pos[col]); else printf("   ");
    SET_COLOR(COLOR_SEPERATOR);
    printf("- ");
    SET_COLOR(COLOR_HEX);
    for(col=8; col<16; col++) if(col < n) printf("%02X ", pos[col]); else printf("   ");
    SET_COLOR(COLOR_ASCII);
    printf("    ");
    for(col=0; col<n; col++) if(pos[col]>=32 && pos[col] < 0xFF) putchar(pos[col]);
      else {SET_COLOR(COLOR_UNDEFINED); putchar('.'); SET_COLOR(COLOR_ASCII);}
    printf("\n");
  }

  SET_COLOR(COLOR_DEFAULT);
}
