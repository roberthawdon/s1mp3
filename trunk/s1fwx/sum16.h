#ifndef _SUM16_H_
#define _SUM16_H_

#ifndef uint16
  typedef unsigned __int16 uint16;
#endif

uint16 calc_sum16(FILE *fh);
uint16 calc_sum16(FILE *fh, size_t ofs, size_t size);
uint16 calc_sum16(const void *ptr, size_t size);

#endif