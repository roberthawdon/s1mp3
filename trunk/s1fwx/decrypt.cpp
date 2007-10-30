#include "include.h"
#include "decrypt.h"


typedef struct {
  uint8 x[3*16];
} HASH_CHUNK;

HASH_CHUNK hash_chunk_43c0b4[3*16];

bool is_hash24;
bool is_hash16;




// 0x004080E0 - 0x0040812F
void shuffle(uint8 *dst, uint8 *src, uint8 *lookup, uint32 len)
{
  uint8 buf_43bfb0[64];
  for(uint32 n=0; n<len; n++) buf_43bfb0[n] = src[lookup[n]-1];
  memcpy(dst, buf_43bfb0, len);
}


// 0x00408130 - 0x00408153
void xor(uint8 *dst, uint8 *src, uint32 n)
{
  for(; n; n--) *(dst++) ^= *(src++);
}


// 0x004081C0 - 0x004081F2
void expand(uint8 *dst, uint8 *src, uint32 len)
{
  if(!len) return;
  for(uint32 n=0; n<len; n++) dst[n] = (src[n>>3] >> (n&7))&1;
}


// 0x00408200 - 0x0040828A
void shrink(uint8 *dst, uint8 *src, uint32 len)
{
  if(!len) return;
  memset(dst, 0, (len+7)>>3);
  for(uint32 n=0; n<len; n++) dst[n>>3] |= (src[n]<<(n&7));
}


// 0x00408160 - 0x004081BF
void swap(uint8 *ptr, uint32 n, uint32 m)
{
  uint8 buf_43bfb0[64];
  memcpy(buf_43bfb0, ptr, m);
  memcpy(ptr, &ptr[m], n-m);
  memcpy(&ptr[n-m], buf_43bfb0, m);
}


// 0x00408070 - 0x004080D2
void lookup(uint8 *dst, uint8 *src)
{
  uint8 lookup_42eb58[512] = {
    0x0E, 0x04, 0x0D, 0x01, 0x02, 0x0F, 0x0B, 0x08, 0x03, 0x0A, 0x06, 0x0C, 0x05, 0x09, 0x00, 0x07,
    0x00, 0x0F, 0x07, 0x04, 0x0E, 0x02, 0x0D, 0x01, 0x0A, 0x06, 0x0C, 0x0B, 0x09, 0x05, 0x03, 0x08,
    0x04, 0x01, 0x0E, 0x08, 0x0D, 0x06, 0x02, 0x0B, 0x0F, 0x0C, 0x09, 0x07, 0x03, 0x0A, 0x05, 0x00,
    0x0F, 0x0C, 0x08, 0x02, 0x04, 0x09, 0x01, 0x07, 0x05, 0x0B, 0x03, 0x0E, 0x0A, 0x00, 0x06, 0x0D,
    0x0F, 0x01, 0x08, 0x0E, 0x06, 0x0B, 0x03, 0x04, 0x09, 0x07, 0x02, 0x0D, 0x0C, 0x00, 0x05, 0x0A,
    0x03, 0x0D, 0x04, 0x07, 0x0F, 0x02, 0x08, 0x0E, 0x0C, 0x00, 0x01, 0x0A, 0x06, 0x09, 0x0B, 0x05,
    0x00, 0x0E, 0x07, 0x0B, 0x0A, 0x04, 0x0D, 0x01, 0x05, 0x08, 0x0C, 0x06, 0x09, 0x03, 0x02, 0x0F,
    0x0D, 0x08, 0x0A, 0x01, 0x03, 0x0F, 0x04, 0x02, 0x0B, 0x06, 0x07, 0x0C, 0x00, 0x05, 0x0E, 0x09,
    0x0A, 0x00, 0x09, 0x0E, 0x06, 0x03, 0x0F, 0x05, 0x01, 0x0D, 0x0C, 0x07, 0x0B, 0x04, 0x02, 0x08,
    0x0D, 0x07, 0x00, 0x09, 0x03, 0x04, 0x06, 0x0A, 0x02, 0x08, 0x05, 0x0E, 0x0C, 0x0B, 0x0F, 0x01,
    0x0D, 0x06, 0x04, 0x09, 0x08, 0x0F, 0x03, 0x00, 0x0B, 0x01, 0x02, 0x0C, 0x05, 0x0A, 0x0E, 0x07,
    0x01, 0x0A, 0x0D, 0x00, 0x06, 0x09, 0x08, 0x07, 0x04, 0x0F, 0x0E, 0x03, 0x0B, 0x05, 0x02, 0x0C,
    0x07, 0x0D, 0x0E, 0x03, 0x00, 0x06, 0x09, 0x0A, 0x01, 0x02, 0x08, 0x05, 0x0B, 0x0C, 0x04, 0x0F,
    0x0D, 0x08, 0x0B, 0x05, 0x06, 0x0F, 0x00, 0x03, 0x04, 0x07, 0x02, 0x0C, 0x01, 0x0A, 0x0E, 0x09,
    0x0A, 0x06, 0x09, 0x00, 0x0C, 0x0B, 0x07, 0x0D, 0x0F, 0x01, 0x03, 0x0E, 0x05, 0x02, 0x08, 0x04,
    0x03, 0x0F, 0x00, 0x06, 0x0A, 0x01, 0x0D, 0x08, 0x09, 0x04, 0x05, 0x0B, 0x0C, 0x07, 0x02, 0x0E,
    0x02, 0x0C, 0x04, 0x01, 0x07, 0x0A, 0x0B, 0x06, 0x08, 0x05, 0x03, 0x0F, 0x0D, 0x00, 0x0E, 0x09,
    0x0E, 0x0B, 0x02, 0x0C, 0x04, 0x07, 0x0D, 0x01, 0x05, 0x00, 0x0F, 0x0A, 0x03, 0x09, 0x08, 0x06,
    0x04, 0x02, 0x01, 0x0B, 0x0A, 0x0D, 0x07, 0x08, 0x0F, 0x09, 0x0C, 0x05, 0x06, 0x03, 0x00, 0x0E,
    0x0B, 0x08, 0x0C, 0x07, 0x01, 0x0E, 0x02, 0x0D, 0x06, 0x0F, 0x00, 0x09, 0x0A, 0x04, 0x05, 0x03,
    0x0C, 0x01, 0x0A, 0x0F, 0x09, 0x02, 0x06, 0x08, 0x00, 0x0D, 0x03, 0x04, 0x0E, 0x07, 0x05, 0x0B,
    0x0A, 0x0F, 0x04, 0x02, 0x07, 0x0C, 0x09, 0x05, 0x06, 0x01, 0x0D, 0x0E, 0x00, 0x0B, 0x03, 0x08,
    0x09, 0x0E, 0x0F, 0x05, 0x02, 0x08, 0x0C, 0x03, 0x07, 0x00, 0x04, 0x0A, 0x01, 0x0D, 0x0B, 0x06,
    0x04, 0x03, 0x02, 0x0C, 0x09, 0x05, 0x0F, 0x0A, 0x0B, 0x0E, 0x01, 0x07, 0x06, 0x00, 0x08, 0x0D,
    0x04, 0x0B, 0x02, 0x0E, 0x0F, 0x00, 0x08, 0x0D, 0x03, 0x0C, 0x09, 0x07, 0x05, 0x0A, 0x06, 0x01,
    0x0D, 0x00, 0x0B, 0x07, 0x04, 0x09, 0x01, 0x0A, 0x0E, 0x03, 0x05, 0x0C, 0x02, 0x0F, 0x08, 0x06,
    0x01, 0x04, 0x0B, 0x0D, 0x0C, 0x03, 0x07, 0x0E, 0x0A, 0x0F, 0x06, 0x08, 0x00, 0x05, 0x09, 0x02,
    0x06, 0x0B, 0x0D, 0x08, 0x01, 0x04, 0x0A, 0x07, 0x09, 0x05, 0x00, 0x0F, 0x0E, 0x02, 0x03, 0x0C,
    0x0D, 0x02, 0x08, 0x04, 0x06, 0x0F, 0x0B, 0x01, 0x0A, 0x09, 0x03, 0x0E, 0x05, 0x00, 0x0C, 0x07,
    0x01, 0x0F, 0x0D, 0x08, 0x0A, 0x03, 0x07, 0x04, 0x0C, 0x05, 0x06, 0x0B, 0x00, 0x0E, 0x09, 0x02,
    0x07, 0x0B, 0x04, 0x01, 0x09, 0x0C, 0x0E, 0x02, 0x00, 0x06, 0x0A, 0x0D, 0x0F, 0x03, 0x05, 0x08,
    0x02, 0x01, 0x0E, 0x07, 0x04, 0x0A, 0x08, 0x0D, 0x0F, 0x0C, 0x09, 0x00, 0x03, 0x05, 0x06, 0x0B
  };

  for(uint32 ofs=0,n=8; n; n--) {
    expand(dst, lookup_42eb58 + ((((((src[1] << 1) + src[2]) << 1) + src[3]) << 1) + src[4]) + (((src[0] << 1) + src[5] + ofs) << 4), 4);
    ofs += 4;
    src += 6;
    dst += 4;
  }
}


// 0x00408020 - 0x00408065
void transform(uint8 *dst, uint8 *src)
{
  uint8 shuffle_42ea90[48] = {
    0x20, 0x01, 0x02, 0x03, 0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x14, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x01
  };

  uint8 shuffle_42eac0[32] = {
    0x10, 0x07, 0x14, 0x15, 0x1D, 0x0C, 0x1C, 0x11, 0x01, 0x0F, 0x17, 0x1A, 0x05, 0x12, 0x1F, 0x0A,
    0x02, 0x08, 0x18, 0x0E, 0x20, 0x1B, 0x03, 0x09, 0x13, 0x0D, 0x1E, 0x06, 0x16, 0x0B, 0x04, 0x19
  };

  uint8 buf_43bec8[48];

  shuffle(buf_43bec8, dst, shuffle_42ea90, 48);
  xor(buf_43bec8, src, 48);
  lookup(dst, buf_43bec8);
  shuffle(dst, dst, shuffle_42eac0, 32);
}


// 0x00407E80 - 0x00407F92
void _decrypt128(uint8 *dst_ptr, uint8 *src_ptr, HASH_CHUNK *hc, bool reverse)
{
  uint8 buf_43bf38[32];
  uint8 buf_43bf58[64];

  uint8 shuffle_42ea10[64] = {
    0x3A, 0x32, 0x2A, 0x22, 0x1A, 0x12, 0x0A, 0x02, 0x3C, 0x34, 0x2C, 0x24, 0x1C, 0x14, 0x0C, 0x04,
    0x3E, 0x36, 0x2E, 0x26, 0x1E, 0x16, 0x0E, 0x06, 0x40, 0x38, 0x30, 0x28, 0x20, 0x18, 0x10, 0x08,
    0x39, 0x31, 0x29, 0x21, 0x19, 0x11, 0x09, 0x01, 0x3B, 0x33, 0x2B, 0x23, 0x1B, 0x13, 0x0B, 0x03,
    0x3D, 0x35, 0x2D, 0x25, 0x1D, 0x15, 0x0D, 0x05, 0x3F, 0x37, 0x2F, 0x27, 0x1F, 0x17, 0x0F, 0x07
  };

  uint8 shuffle_42ea50[64] = {
    0x28, 0x08, 0x30, 0x10, 0x38, 0x18, 0x40, 0x20, 0x27, 0x07, 0x2F, 0x0F, 0x37, 0x17, 0x3F, 0x1F,
    0x26, 0x06, 0x2E, 0x0E, 0x36, 0x16, 0x3E, 0x1E, 0x25, 0x05, 0x2D, 0x0D, 0x35, 0x15, 0x3D, 0x1D,
    0x24, 0x04, 0x2C, 0x0C, 0x34, 0x14, 0x3C, 0x1C, 0x23, 0x03, 0x2B, 0x0B, 0x33, 0x13, 0x3B, 0x1B,
    0x22, 0x02, 0x2A, 0x0A, 0x32, 0x12, 0x3A, 0x1A, 0x21, 0x01, 0x29, 0x09, 0x31, 0x11, 0x39, 0x19
  };

  uint8 n;

  expand(buf_43bf58, src_ptr, 64);
  shuffle(buf_43bf58, buf_43bf58, shuffle_42ea10, 64);

  if(!reverse) {

    for(n = 16; n; n--,hc++) {
      memcpy(buf_43bf38, &buf_43bf58[32], 32);      //ok
      transform(&buf_43bf58[32], (uint8*)&hc[0]);   //ok
      xor(&buf_43bf58[32], &buf_43bf58[0], 32);     //ok
      memcpy(&buf_43bf58[0], buf_43bf38, 32);
    }

  } else {

    for(n = 16; n; n--,hc--) {
      memcpy(buf_43bf38, &buf_43bf58[0], 32);
      transform(&buf_43bf58[0], (uint8*)&hc[15]);
      xor(buf_43bf58, &buf_43bf58[32], 32);
      memcpy(&buf_43bf58[32], buf_43bf38, 32);
    }

  }

  shuffle(buf_43bf58, buf_43bf58, shuffle_42ea50, 64);
  shrink(dst_ptr, buf_43bf58, 64);
}


// 0x00407FA0 - 0x0040801F
void generate_hash_chunk(HASH_CHUNK *hc, uint8 *k)
{
  uint8 key64_43bef8[64];
  uint8 swap_lookup_42eb48[] = {1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1};

  uint8 shuffle_a_42eae0[56] = {
    0x39, 0x31, 0x29, 0x21, 0x19, 0x11, 0x09, 0x01, 0x3A, 0x32, 0x2A, 0x22, 0x1A, 0x12, 0x0A, 0x02,
    0x3B, 0x33, 0x2B, 0x23, 0x1B, 0x13, 0x0B, 0x03, 0x3C, 0x34, 0x2C, 0x24, 0x3F, 0x37, 0x2F, 0x27,
    0x1F, 0x17, 0x0F, 0x07, 0x3E, 0x36, 0x2E, 0x26, 0x1E, 0x16, 0x0E, 0x06, 0x3D, 0x35, 0x2D, 0x25,
    0x1D, 0x15, 0x0D, 0x05, 0x1C, 0x14, 0x0C, 0x04
  };

  uint8 shuffle_b_42eb18[48] = {
    0x0E, 0x11, 0x0B, 0x18, 0x01, 0x05, 0x03, 0x1C, 0x0F, 0x06, 0x15, 0x0A, 0x17, 0x13, 0x0C, 0x04,
    0x1A, 0x08, 0x10, 0x07, 0x1B, 0x14, 0x0D, 0x02, 0x29, 0x34, 0x1F, 0x25, 0x2F, 0x37, 0x1E, 0x28,
    0x33, 0x2D, 0x21, 0x30, 0x2C, 0x31, 0x27, 0x38, 0x22, 0x35, 0x2E, 0x2A, 0x32, 0x24, 0x1D, 0x20
  };

  expand(key64_43bef8, k, 64);
  shuffle(key64_43bef8, key64_43bef8, shuffle_a_42eae0, 56);

  for(uint8 n=0; n < 16; n++,hc++) {
    swap(key64_43bef8, 28, swap_lookup_42eb48[n]);
    swap(key64_43bef8 + 28, 28, swap_lookup_42eb48[n]);
    shuffle((uint8*)hc, key64_43bef8, shuffle_b_42eb18, 48);
  }
}


// 0x00407DD0 - 0x00407E7F
void generate_hash(uint8 *key, uint32 len)
{
  uint8 key24_43bf98[24];

  memset(key24_43bf98, 0, 24);
  memcpy(key24_43bf98, key, (len <= 24)?len:24);

  if(len <= 16) is_hash24 = false;
  else {
    is_hash24 = true;
    generate_hash_chunk(&hash_chunk_43c0b4[32], &key24_43bf98[16]);
  }

  if(len <= 8) is_hash16 = false;
  else {
    is_hash16 = true;
    generate_hash_chunk(&hash_chunk_43c0b4[16], &key24_43bf98[8]);
  }

  generate_hash_chunk(&hash_chunk_43c0b4[0], &key24_43bf98[0]);
}


// 0x00407C70 - 0x00407DCF
bool _decrypt(uint8 *dst_ptr, uint8 *src_ptr, uint32 size, uint8 *key, uint32 keylen, bool reverse)
{
  if(!dst_ptr) return false;
  if(!src_ptr) return false;
  if(!key) return false;
	size = (size + 7) & 0xFFFFFFF8;
  if(!size) return false;

  generate_hash(key, keylen);
  //dump(hash_chunk_43c0b4, sizeof(hash_chunk_43c0b4));

  if(is_hash24) {

    for(size >>= 3; size; size--) {
      _decrypt128(dst_ptr, src_ptr, &hash_chunk_43c0b4[0],  reverse);
      _decrypt128(dst_ptr, dst_ptr, &hash_chunk_43c0b4[16], !reverse);
      _decrypt128(dst_ptr, src_ptr, &hash_chunk_43c0b4[32], reverse);
      _decrypt128(dst_ptr, dst_ptr, &hash_chunk_43c0b4[16], !reverse);
      _decrypt128(dst_ptr, dst_ptr, &hash_chunk_43c0b4[0],  reverse);
      dst_ptr += 8;
      src_ptr += 8;
    }

  } else if(is_hash16) {

    for(size >>= 3; size; size--) {
      _decrypt128(dst_ptr, src_ptr, &hash_chunk_43c0b4[0],  reverse);
      _decrypt128(dst_ptr, dst_ptr, &hash_chunk_43c0b4[16], !reverse);
      _decrypt128(dst_ptr, dst_ptr, &hash_chunk_43c0b4[0],  reverse);
      dst_ptr += 8;
      src_ptr += 8;
    }

  } else {

    for(size >>= 3; size ; size--) {
      _decrypt128(dst_ptr, src_ptr, &hash_chunk_43c0b4[0], reverse);
      dst_ptr += 8;
      src_ptr += 8;
    }

  }

  return true;
}


// 0x004078C0 - 0x00407950
// example: key="RY2085", keylen=6
bool decrypt(void *dst_ptr, void *src_ptr, uint32 size, void *key, uint32 keylen)
{
  bool ret = _decrypt((uint8 *)dst_ptr, (uint8 *)src_ptr, (size>>3)<<3, (uint8 *)key, keylen, true);
  if(size & 7) memcpy(((uint8 *)dst_ptr)+((size>>3)<<3), ((uint8 *)src_ptr)+((size>>3)<<3), size&7);
  return ret;
}


// -----------------------------------------------------------------------------------------------
// be shure password points at a buffer of at least 20 chararcters
void extract_key(char *key, uint8 *enc_file, size_t enc_file_size, uint8 *file_out)
{
  uint8 enc_key[32];
  memset(enc_key, 0, sizeof(enc_key));  

  enc_file += 10;  //jump over "Enciphered" id
  uint32 step = (uint32)((((__int64)0xCCCCCCCD) * enc_file_size) >> (32+2));

  for(uint8 cntr = 0; cntr<4; cntr++)
  {
    if(file_out) {
      memcpy(file_out, enc_file, step);
      file_out += step;
    }
    enc_file += step;
    memcpy(&enc_key[cntr*5], enc_file, 5);
    enc_file += 5;
  }

  if(file_out) memcpy(file_out, enc_file, enc_file_size - (step << 2));

//dump(enc_key, 20);
  if(key) decrypt(key, enc_key, 20, "ArestekToActions", 16);
}


/* TESTING
#include "dump.h"
void main()
{
  printf("test decrypt...\n");
  uint8 src[] = {
    0xAA, 0xCE, 0x78, 0x49, 0xE7, 0xE8, 0x08, 0x7F, 0x23, 0xD9, 0xA4, 0x4F, 0xA1, 0xFE, 0xA2, 0xDD,
    0xCA, 0x84, 0xA1, 0xA2, 0xBF, 0x85, 0xB5, 0x9A, 0xCA, 0x84, 0xA1, 0xA2, 0xBF, 0x85, 0xB5, 0x9A,
    0x34, 0x5B, 0xB5, 0xD0, 0x7E, 0x58, 0x5E, 0xFD, 0x82, 0xBB, 0x41, 0x11, 0x90, 0x9C, 0xC6, 0x0D,
    0xF6, 0x7F, 0xAD, 0xB4, 0xC1, 0xA6, 0x19, 0x06, 0xCB, 0x2B, 0x56, 0x6A, 0x9C, 0x09, 0x45, 0x77
  };
  char dst[sizeof(src)];
  ZeroMemory(dst, sizeof(dst));
  decrypt(dst, src, sizeof(src), "RY2085", 6);
  printf("result:\n");
  dump(dst, sizeof(dst));
}
//*/