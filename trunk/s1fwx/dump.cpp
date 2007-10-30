#include "include.h"
#include "dump.h"


// -----------------------------------------------------------------------------------------------
void dump(const void *ptr, long n, bool pgbrk)
{
  int col,ccnt=0;
  BYTE *pos = (BYTE*)ptr;
  DWORD ofs = 0;

  while(n >= 16) {
    printf("%04X    ", ofs);
    for(col=0; col<8; col++) printf("%02X ", pos[col]);
    printf("- ");
    for(col=8; col<16; col++) printf("%02X ", pos[col]);
    printf("    ");
    for(col=0; col<16; col++) if(pos[col]>=32) putchar(pos[col]); else putchar('.');
    printf("\n");

    if(pgbrk) if(++ccnt >= 24) {
      ccnt = 0;
      printf("press key to continue, ESC to cancel...");
      col = _getch();
      printf("\n");
      if(col == 27) return;
    }

    pos+=16;
    ofs+=16;
    n-=16;
  }

  if(n>0) {
    printf("%04X    ", ofs);
    for(col=0; col<8; col++) if(col < n) printf("%02X ", pos[col]); else printf("   ");
    printf("- ");
    for(col=8; col<16; col++) if(col < n) printf("%02X ", pos[col]); else printf("   ");
    printf("    ");
    for(col=0; col<n; col++) if(pos[col]>=32) putchar(pos[col]); else putchar('.');
    printf("\n");
  }
}
