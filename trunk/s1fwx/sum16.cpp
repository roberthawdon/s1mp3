#include <stdio.h>
#include "sum16.h"


// -----------------------------------------------------------------------------------------------
uint16 calc_sum16(FILE *fh)
{
  register uint16 crc = 0;
  if(!fh) return(0);
  for(register int ch; !feof(fh);) {
    if((ch = getc(fh)) != EOF) crc += (((uint16)ch) << 0);
    if((ch = getc(fh)) != EOF) crc += (((uint16)ch) << 8);
  }
  return crc;
}


// -----------------------------------------------------------------------------------------------
uint16 calc_sum16(FILE *fh, size_t ofs, size_t size)
{
  uint16 cp;
  register uint16 crc = 0;  
  if(!fh) return(0);
  if(fseek(fh, (long)ofs, SEEK_SET) != 0) return(0);
  for(; size >= 2; size-=2) {
    if(fread(&cp, 2, 1, fh) != 1) return(0);
    crc += cp;
  }
  if(size > 0) {
    cp = 0;
    if(fread(&cp, 1, 1, fh) != 1) return(0);
    crc += cp;
  }
  return crc;
}


// -----------------------------------------------------------------------------------------------
uint16 calc_sum16(const void *ptr, size_t size)
{
  register uint16 crc = 0;
  register uint16 *cp = (uint16 *)ptr;
  for(; size >= 2; size-=2) crc += *cp++;
  if(size > 0) crc += *((unsigned __int8*)cp);
  return crc;
}
