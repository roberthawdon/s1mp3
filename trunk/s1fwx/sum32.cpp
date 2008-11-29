#include <stdio.h>
#include "sum32.h"


// -----------------------------------------------------------------------------------------------
uint32 calc_sum32(FILE *fh)
{
  register uint32 crc = 0;
  if(!fh) return(0);
  for(register int ch; !feof(fh);) {
    if((ch = getc(fh)) != EOF) crc += (((uint32)ch) << 0);
    if((ch = getc(fh)) != EOF) crc += (((uint32)ch) << 8);
    if((ch = getc(fh)) != EOF) crc += (((uint32)ch) << 16);
    if((ch = getc(fh)) != EOF) crc += (((uint32)ch) << 24);
  }
  return crc;
}


// -----------------------------------------------------------------------------------------------
uint32 calc_sum32(FILE *fh, size_t ofs, size_t size)
{
  uint32 cp;
  register uint32 crc = 0;  
  if(!fh) return(0);
  if(fseek(fh, (long)ofs, SEEK_SET) != 0) return(0);
  for(; size >= 4; size-=4) {
    if(fread(&cp, 4, 1, fh) != 1) return(0);
    crc += cp;
  }
  if(size > 0) {
    cp = 0;
    if(fread(&cp, size, 1, fh) != 1) return(0);
    crc += cp;
  }
  return crc;
}


// -----------------------------------------------------------------------------------------------
uint32 calc_sum32(const void *ptr, size_t size)
{
  register uint32 crc = 0;
  register uint32 *cp = (uint32 *)ptr;
  for(; size >= 4; size-=4) crc += *cp++;
  if(size == 1) crc += *((unsigned __int8*)cp);
  else if(size == 2) crc += *((unsigned __int16*)cp);
  else if(size == 3) crc += *((unsigned __int16*)cp) + (((unsigned __int8*)cp)[2] << 16);
  return crc;
}
