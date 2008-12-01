#include <stdio.h>
#include <stdlib.h>

#ifdef LINUX
  
#else

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;

#endif

int main(int argc, char *argv[])
{
  if(argc != 3)
  {
    fprintf(stderr, "convert binary data into c header files - copyright (c)2006 wiRe@gmx.net\n");
    fprintf(stderr, "usage: bin2c input.bin name >output.h\n");
    return -1;
  }

  FILE *fh = fopen(argv[1], "rb");
  if(!fh)
  {
    fprintf(stderr, "error: failed to open file \"%s\"\n", argv[1]);
    return -1;
  }

  fprintf(stdout, "static unsigned char %s[] =\n{\n  ", argv[2]);
  int cnt = 7;
  int chr = fgetc(fh);
  if(chr != EOF) while(1)
  {
    fprintf(stdout, "0x%02X", (unsigned char)chr);
    chr = fgetc(fh);
    if(chr == EOF) break;
    if(cnt <= 0)
    {
      cnt = 7;
      fprintf(stdout, ",\n  ");
    }
    else
    {
      cnt--;
      fprintf(stdout, ", ");
    }
  }
  fprintf(stdout, "\n};\n");

  if(ferror(fh))
  {
    fclose(fh);
    fprintf(stderr, "error: failed to read from file \"%s\"\n", argv[1]);
    return -1;
  }

  fclose(fh);
  return 0;
}

