#ifndef _SUM32_H_
#define _SUM32_H_

#ifndef uint32
  typedef unsigned __int32 uint32;
#endif

uint32 calc_sum32(FILE *fh);
uint32 calc_sum32(FILE *fh, size_t ofs, size_t size);
uint32 calc_sum32(const void *ptr, size_t size);

#endif