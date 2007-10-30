#ifndef _CRC32_H_
#define _CRC32_H_

#ifndef uint32
  typedef unsigned __int32 uint32;
#endif

// generate the crc table. Must be called before calculating the crc value
void init_crc32(uint32 poly = 0xEDB88320L);

uint32 calc_crc32(FILE *fh);
uint32 calc_crc32(FILE *fh, size_t ofs, size_t size);
uint32 calc_crc32(void *ptr, size_t size);

#endif