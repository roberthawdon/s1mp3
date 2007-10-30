#include <stdio.h>
#include "crc32.h"


bool crc_table_init = false;
static uint32 crc_table[256];

// generate the crc table. Must be called before calculating the crc value
void init_crc32(uint32 poly)
{
  uint32 crc;
  int i, j;

  for(i = 0; i < 256; i++) {
    crc = i;
    for(j = 8; j > 0; j--) {
      if(crc & 1) crc = (crc >> 1) ^ poly;
      else crc >>= 1;
    }
    crc_table[i] = crc;
  }

  crc_table_init = true;
}


uint32 calc_crc32(FILE *fh)
{
  if(!crc_table_init) init_crc32();
  register uint32 crc = 0xFFFFFFFF;
  for(int ch = getc(fh); ch != EOF; ch = getc(fh))
    crc = (crc>>8) ^ crc_table[ (crc^ch) & 0xFF ];
  return(crc^0xFFFFFFFF);
}


uint32 calc_crc32(FILE *fh, size_t ofs, size_t size)
{
  if(!crc_table_init) init_crc32();
  register uint32 crc = 0xFFFFFFFF;  
  if(!fh) return(0);
  if(fseek(fh, (long)ofs, SEEK_SET) != 0) return(0);
  for(int ch=fgetc(fh); size > 0; size--, ch=fgetc(fh))
    if(ch != EOF) crc = (crc>>8) ^ crc_table[ (crc^ch) & 0xFF ];
  return(crc^0xFFFFFFFF);
}


uint32 calc_crc32(void *ptr, size_t size)
{
  if(!crc_table_init) init_crc32();
  register uint32 crc = 0xFFFFFFFF;
  unsigned char *cp = (unsigned char*)ptr;
  for(; size > 0; size--,cp++)
    crc = (crc>>8) ^ crc_table[ (crc^*cp) & 0xFF ];
  return(crc^0xFFFFFFFF);
}
