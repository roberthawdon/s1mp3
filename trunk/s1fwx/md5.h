#ifndef _MD5_H
#define _MD5_H

#ifndef uint8
  typedef  unsigned __int8 uint8;
#endif

#ifndef uint32
  typedef unsigned __int32 uint32;
#endif

typedef struct
{
    uint32 total[2];
    uint32 state[4];
    uint8 buffer[64];
}
md5_context;

void md5_starts( md5_context *ctx );
void md5_update( md5_context *ctx, uint8 *input, uint32 length );
void md5_finish( md5_context *ctx, uint8 digest[16] );

#endif /* md5.h */