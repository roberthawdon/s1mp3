//=================================================================================================
// ResItemImage.h : handle resource images
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#include "common.h"
#include "ResItem.h"


//-------------------------------------------------------------------------------------------------
#define RESITM_IMAGE_WIDTH 48   //width must be word-aligned
#define RESITM_IMAGE_HEIGHT 48

#define RGB2RGB16(_r, _g, _b) ( ((((RGB16)(_r)&0xFF)>>3)<<11) | ((((RGB16)(_g)&0xFF)>>2)<<5) | ((((RGB16)(_b)&0xFF)>>3)<<0) )
#define RGB2RGB32(_r, _g, _b) ( (((RGB32)(_r)&0xFF)<<16) | (((RGB32)(_g)&0xFF)<<8) | (((RGB32)(_b)&0xFF)<<0) )
#define COLORREF2RGB32(_cref) ( (((RGB32)(_cref)&0xFF)<<16) | ((RGB32)(_cref)&0xFF00) | (((RGB32)((_cref)>>16)&0xFF)<<0) )

#define RESITM_IMAGE_RGB_FRONT  COLORREF2RGB32(g_options.get()->crefMonoForeground)
#define RESITM_IMAGE_RGB_BACK   COLORREF2RGB32(g_options.get()->crefMonoBackground)
#define RESITM_IMAGE_RGB_BORDER COLORREF2RGB32(g_options.get()->crefMonoBackground)


//-------------------------------------------------------------------------------------------------
class ResItemImage : public ResItem
{
  //-----------------------------------------------------------------------------------------------
  // typedefs (public)
  //-----------------------------------------------------------------------------------------------
  public:
    typedef unsigned __int16 RGB16, *LPRGB16;
    typedef unsigned __int32 RGB32, *LPRGB32;
    enum IMGFMT {IMGFMT_MONO, IMGFMT_OLED, IMGFMT_FONT8, IMGFMT_FONT16};

  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    LPBYTE lpImage;     //ptr to real image data
    DWORD dwImageSize;  //the real image size
    UINT nWidth;        //the image width
    UINT nHeight;       //the image height
    IMGFMT nImgFmt;     //the image format

    RGB32 rgbMonoFront; //monochrome colors used to export the last bitmap
    RGB32 rgbMonoBack;  //

    HBITMAP hBitmap;    //the bitmap representing 

  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    ResItemImage(TYPE nType, LPCWSTR lpName, LPBYTE lpData, DWORD dwDataSize,
      LPBYTE lpImage, DWORD dwImageSize, UINT nWidth, UINT nHeight, IMGFMT nImgFmt);
    ~ResItemImage();

    HBITMAP getBitmap();
    BOOL freeBitmap();

    BOOL exportBitmap(LPCWSTR lpcwName);
    BOOL importBitmap(LPCWSTR lpcwName);

  //-----------------------------------------------------------------------------------------------
  // getters (public)
  //-----------------------------------------------------------------------------------------------
  public:
    IMGFMT getImgFmt() {return nImgFmt;}
    DWORD getImageSize() {return dwImageSize;}
    UINT getWidth() {return nWidth;}
    UINT getHeight() {return nHeight;}

  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    HBITMAP createBitmap(UINT nWidth, UINT nHeight);
    RGB32 getPixel(LONG nX, LONG nY);
    BOOL setPixel(LONG nX, LONG nY, RGB32 clrRef);

  //-----------------------------------------------------------------------------------------------
  // inline methods (private)
  //-----------------------------------------------------------------------------------------------
  // convert OLED colors from/to RGB32
  // OLED color is RGB16(5:6:5) stored in big endian
  //-----------------------------------------------------------------------------------------------
  private:
    inline VOID colorToOLED(LPRGB16 lpDest, RGB32 c32)
    {
      RGB16 c16 = RGB2RGB16(c32>>16, c32>>8, c32>>0);
      *lpDest = ((c16 >> 8) & 0x00FF) | ((c16 << 8) & 0xFF00); //little-endian
    }

    inline RGB32 colorFromOLED(LPRGB16 lpSrc)
    {
      RGB16 c16 = ((*lpSrc >> 8) & 0x00FF) | ((*lpSrc << 8) & 0xFF00); //little-endian
      return RGB2RGB32((c16>>11)<<3, (c16>>5)<<2, (c16>>0)<<3);
    }

    inline int colorToGrayscale(RGB32 c32) //return 0..255 grayscale
    {
      return (int)(((c32>>16)&0xFF)/3.2f + ((c32>>8)&0xFF)/1.7f + ((c32>>0)&0xFF)/9.3f);
    }

    inline bool write(HANDLE hFile, LPCVOID lpBuffer, DWORD dwSize)
    {
      DWORD dwWritten = 0;
      if(::WriteFile(hFile, lpBuffer, dwSize, &dwWritten, NULL) && dwWritten == dwSize) return true;
      if(dwWritten != dwSize) ::SetLastError(ERROR_WRITE_FAULT);
      ::CloseHandle(hFile);
      return false;
    }

    inline bool read(HANDLE hFile, LPVOID lpBuffer, DWORD dwSize)
    {
      DWORD dwRead = 0;
      if(::ReadFile(hFile, lpBuffer, dwSize, &dwRead, NULL) && dwRead == dwSize) return true;
      if(dwRead != dwSize) ::SetLastError(ERROR_READ_FAULT);
      ::CloseHandle(hFile);
      return false;
    }

  //-----------------------------------------------------------------------------------------------
  // typedefs (private)
  //-----------------------------------------------------------------------------------------------
  private:
    #pragma pack(1)

    #define BMP_HEADER_ID 0x4D42

    typedef struct {
       unsigned short int id;                   //magic identifier = 'BM'
       unsigned int size;                       //file size in bytes
       unsigned short int reserved1, reserved2;
       unsigned int offset;                     //offset to image data, bytes
    } BMP_HEADER;

    typedef struct {
       unsigned int size;               //header size in bytes
       int width,height;                //width and height of image
       unsigned short int planes;       //number of color planes
       unsigned short int bits;         //bits per pixel
       unsigned int compression;        //compression type
       unsigned int imagesize;          //image size in bytes
       int xresolution,yresolution;     //pixels per meter
       unsigned int ncolours;           //number of colors
       unsigned int importantcolours;   //important colors
    } BMP_INFOHEADER;

    typedef struct {
      BMP_HEADER hdr;
      BMP_INFOHEADER inf;
    } BMP;

    #pragma pack()
};
