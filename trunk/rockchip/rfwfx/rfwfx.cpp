/*
 * Rockchip Firmware File Extractor
 *
 * desc: extracts rockchip firmare files into their basic blocks
 * from: http://www.s1mp3.de/
 *
 * (c)2008 wiRe (mailto:mail _at_ s1mp3 _dot_ net)
 *
 * THIS FILE IS LICENSED UNDER THE GNU GPL
 * 
 */

#include <stdio.h>
#include <conio.h>

#pragma pack(1)
typedef struct {
  unsigned __int32 flag;
  unsigned __int32 fofs;
  unsigned __int32 size;
  unsigned __int32 unkwn;
} RFW_HEADER_ENTRY;

typedef struct {
  char id[8];   //"ROCK260x"
  char year[4]; //eg. "2005"
  char ver[7];  //eg. "VER5.00"
  unsigned __int8 unkwn0013[12];
  char name[17];
  unsigned __int8 unkwn0030[0x250];
  RFW_HEADER_ENTRY entry[65];
} RFW_HEADER;
#pragma pack()




void extract_block(int i, RFW_HEADER_ENTRY *e, FILE *fh)
{
  printf("block %02u: fofs=%08X, size=%08X, attr=%08X\n", i, e->fofs, e->size, e->unkwn);

  fseek(fh, e->fofs, SEEK_SET);

  char dest[64];
  sprintf(dest, "BLOCK%02X.BIN", i);
  FILE *fd = fopen(dest, "wb");

  unsigned char buf[512];
  for(long sz = e->size; sz > 0; sz -= 512)
  {
    fwrite(buf, fread(buf, 1, (sz > 512)? 512 : sz, fh), 1, fd);
  }

  fclose(fd);
}



void extract(const char *fname)
{
  FILE *fh = fopen(fname, "rb");
  if(fh == NULL) throw "failed to open";

  RFW_HEADER rfwh;
  if(fread(&rfwh, sizeof(rfwh), 1, fh) != 1) {fclose(fh); throw "failed to read header";}

  printf("id: %c%c%c%c%c%c%c%c\n", rfwh.id[0], rfwh.id[1], rfwh.id[2], rfwh.id[3], rfwh.id[4], rfwh.id[5], rfwh.id[6], rfwh.id[7]);
  printf("year: %c%c%c%c\n", rfwh.year[0], rfwh.year[1], rfwh.year[2], rfwh.year[3]);
  printf("version: %s\n", rfwh.ver);
  printf("name: %s\n", rfwh.name);

  for(int i=0; i < 65; i++)
  {
    if(rfwh.entry[i].flag != 0)
    {
      extract_block(i, &rfwh.entry[i], fh);
    }
  }

  fclose(fh);
}




void main()
{
  try
  {
    extract("RockChip_firmware_(PowerPack)\\powerpack.rfw");
  }
  catch(const char *s)
  {
    printf("ERROR: %s\n", s);
  }
  catch(...)
  {
    printf("ERROR: unknown exception\n");
  }
}