//=================================================================================================
// ResItemImage.cpp : handle resource images
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#include "common.h"
#include "ResItemImage.h"
#include "Options.h"


//-------------------------------------------------------------------------------------------------
ResItemImage::ResItemImage(TYPE nType, LPCWSTR lpName, LPBYTE lpData, DWORD dwDataSize,
      LPBYTE lpImage, DWORD dwImageSize, UINT nWidth, UINT nHeight, IMGFMT nImgFmt)
      : ResItem(nType, lpName, lpImage, dwImageSize)
{
  this->lpImage = lpImage;
  this->dwImageSize = dwImageSize;
  this->nWidth = nWidth;
  this->nHeight = nHeight;
  this->nImgFmt = nImgFmt;
  this->hBitmap = NULL;
  this->rgbMonoFront = this->rgbMonoBack = -1;
}


//-------------------------------------------------------------------------------------------------
ResItemImage::~ResItemImage()
{
  freeBitmap();
}




//-------------------------------------------------------------------------------------------------
BOOL ResItemImage::freeBitmap()
{
  if(this->hBitmap)
  {
    ::DeleteObject(this->hBitmap);
    this->hBitmap = NULL;
  }
  return success();
}


//-------------------------------------------------------------------------------------------------
HBITMAP ResItemImage::getBitmap()
{
  return hBitmap? hBitmap
    : hBitmap = createBitmap(RESITM_IMAGE_WIDTH, RESITM_IMAGE_HEIGHT);
}




//-------------------------------------------------------------------------------------------------
HBITMAP ResItemImage::createBitmap(UINT nWidth, UINT nHeight)
{
  if(!nWidth || !nHeight) return NULL;

  uint32 *lpBmpData = NULL;

  // initalize monochrome colors if needed
  if(rgbMonoFront == -1) rgbMonoFront = RESITM_IMAGE_RGB_FRONT;
  if(rgbMonoBack == -1) rgbMonoBack = RESITM_IMAGE_RGB_BACK;

  // create bitmap
  BITMAPINFOHEADER bih;
  ::ZeroMemory(&bih, sizeof(BITMAPINFOHEADER));
  bih.biSize = sizeof(BITMAPINFOHEADER);
  bih.biWidth = nWidth;
  bih.biHeight = nHeight;
  bih.biSizeImage = nWidth*nHeight*4;
  bih.biBitCount = 32;
  bih.biPlanes = 1;
  bih.biCompression = BI_RGB;
  HBITMAP hBmp = ::CreateDIBSection(NULL,
    (BITMAPINFO*)&bih, DIB_RGB_COLORS, (void**)&lpBmpData, NULL, 0);
  if(!hBmp) return NULL;

  // insert image
  if(lpBmpData)
  {
    DWORD dwScale = (this->nWidth > this->nHeight)?
      (((DWORD)this->nWidth) << 12) / nWidth : (((DWORD)this->nHeight) << 12) / nHeight;

    //DWORD dwScale = ((((DWORD)this->nWidth) << 12) / nWidth + (((DWORD)this->nHeight) << 12) / nHeight) >> 1;

    if(!dwScale) dwScale = 1;

    LONG nShiftX = ( ((LONG)nWidth) - ((LONG)((((DWORD)this->nWidth) << 12) / dwScale)) ) / 2;
    LONG nShiftY = ( ((LONG)nHeight) - ((LONG)((((DWORD)this->nHeight) << 12) / dwScale)) ) / 2;

    for(LONG nY=nHeight-1; nY >= 0; nY--)
    {
      LONG nYS = (LONG)(((nY - nShiftY) * dwScale) >> 12);
      for(LONG nX=0; nX < (LONG)nWidth; nX++)
      {
        LONG nXS = (LONG)(((nX - nShiftX) * dwScale) >> 12);
        *lpBmpData++ = getPixel(nXS, nYS);
      }
    }
  }

  return hBmp;
}


//-------------------------------------------------------------------------------------------------
ResItemImage::RGB32 ResItemImage::getPixel(LONG nX, LONG nY)
{
  if(nX >= 0 && nX < (LONG)nWidth && nY >= 0 && nY < (LONG)nHeight) switch(nImgFmt)
  {
  case IMGFMT_MONO:
    {
      BYTE bBitMask = 128 >> (BYTE)(nY % 8);
      DWORD dwByteOfs = nX + (nY / 8)*nWidth;
      if(dwByteOfs >= dwImageSize) break;
      return (lpImage[dwByteOfs] & bBitMask)? rgbMonoFront : rgbMonoBack;
    }

  case IMGFMT_FONT8:
    {
      // font 8x8 and 8x16
      // each letter uses 8*16bits = 8*2bytes, where 1byte defines 8 vertical bits
      // the first 8bytes define the upper part, the following 8bytes the lower part
      BYTE bBitMask = 128 >> (BYTE)(nY % 8);
      DWORD dwLetter = (nWidth/8)*(nY/16) + (nX/8);
      DWORD dwByteOfs = dwLetter*8*2 + (nY&8) + (nX%8);
      if(dwByteOfs >= dwImageSize) break;
      return (lpImage[dwByteOfs] & bBitMask)? rgbMonoFront : rgbMonoBack;
    }

  case IMGFMT_FONT16:
    {
      // font 16x16
      // each letter uses 16*16bits = 16*2bytes, where 1byte defines 8 vertical bits
      // the first 16bytes define the upper part, the following 16bytes the lower part
      BYTE bBitMask = 128 >> (BYTE)(nY % 8);
      DWORD dwLetter = (nWidth/16)*(nY/16) + (nX/16);
      DWORD dwByteOfs = dwLetter*16*2 + (nY&8)*2 + (nX%16);
      if(dwByteOfs >= dwImageSize) break;
      return (lpImage[dwByteOfs] & bBitMask)? rgbMonoFront : rgbMonoBack;
    }

  case IMGFMT_OLED:
    {
      DWORD dwWordOfs = nX + (nHeight-1-nY)*nWidth;
      if((dwWordOfs*2+1) >= dwImageSize) break;
      return colorFromOLED( &((LPWORD)lpImage)[dwWordOfs] );
    }
  }

  return RESITM_IMAGE_RGB_BORDER;
}

//-------------------------------------------------------------------------------------------------
BOOL ResItemImage::setPixel(LONG nX, LONG nY, RGB32 clrRef)
{
  if(nX >= 0 && nX < (LONG)nWidth && nY >= 0 && nY < (LONG)nHeight) switch(nImgFmt)
  {
  case IMGFMT_MONO:
    {
      BYTE bBitMask = 128 >> (BYTE)(nY % 8);
      DWORD dwByteOfs = nX + (nY / 8)*nWidth;
      if(dwByteOfs >= dwImageSize) break;
      int nFrontDiff = abs(colorToGrayscale(clrRef) - colorToGrayscale(rgbMonoFront));
      int nBackDiff  = abs(colorToGrayscale(clrRef) - colorToGrayscale(rgbMonoBack));
      if(nFrontDiff <= nBackDiff) lpImage[dwByteOfs] |= bBitMask;
      else lpImage[dwByteOfs] &= 0xFF ^ bBitMask;
      return success();
    }

  case IMGFMT_FONT8:
    {
      // font 8x8 and 8x16
      // each letter uses 8*16bits = 8*2bytes, where 1byte defines 8 vertical bits
      // the first 8bytes define the upper part, the following 8bytes the lower part
      BYTE bBitMask = 128 >> (BYTE)(nY % 8);
      DWORD dwLetter = (nWidth/8)*(nY/16) + (nX/8);
      DWORD dwByteOfs = dwLetter*8*2 + (nY&8) + (nX%8);
      if(dwByteOfs >= dwImageSize) break;
      int nFrontDiff = abs(colorToGrayscale(clrRef) - colorToGrayscale(rgbMonoFront));
      int nBackDiff  = abs(colorToGrayscale(clrRef) - colorToGrayscale(rgbMonoBack));
      if(nFrontDiff <= nBackDiff) lpImage[dwByteOfs] |= bBitMask;
      else lpImage[dwByteOfs] &= 0xFF ^ bBitMask;
      return success();
    }

  case IMGFMT_FONT16:
    {
      // font 16x16
      // each letter uses 16*16bits = 16*2bytes, where 1byte defines 8 vertical bits
      // the first 16bytes define the upper part, the following 16bytes the lower part
      BYTE bBitMask = 128 >> (BYTE)(nY % 8);
      DWORD dwLetter = (nWidth/16)*(nY/16) + (nX/16);
      DWORD dwByteOfs = dwLetter*16*2 + (nY&8)*2 + (nX%16);
      if(dwByteOfs >= dwImageSize) break;
      int nFrontDiff = abs(colorToGrayscale(clrRef) - colorToGrayscale(rgbMonoFront));
      int nBackDiff  = abs(colorToGrayscale(clrRef) - colorToGrayscale(rgbMonoBack));
      if(nFrontDiff <= nBackDiff) lpImage[dwByteOfs] |= bBitMask;
      else lpImage[dwByteOfs] &= 0xFF ^ bBitMask;
      return success();
    }

  case IMGFMT_OLED:
    {
      DWORD dwWordOfs = nX + (nHeight-1-nY)*nWidth;
      if((dwWordOfs*2+1) >= dwImageSize) break;
      colorToOLED(&((LPWORD)lpImage)[dwWordOfs], clrRef);
      return success();
    }

  default:
    return error(ERROR_INVALID_COLORINDEX);
  }

  return error(ERROR_INVALID_PARAMETER);
}



//-------------------------------------------------------------------------------------------------
BOOL ResItemImage::exportBitmap(LPCWSTR lpcwName)
{
  CONST UINT nByteWidth = (nWidth*3+3)&(3^-1);

  // remember actual monochrome colors used for export
  rgbMonoFront = RESITM_IMAGE_RGB_FRONT;
  rgbMonoBack = RESITM_IMAGE_RGB_BACK;

  // create file
  HANDLE hFile = ::CreateFile(lpcwName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if(hFile == INVALID_HANDLE_VALUE) return error();

  BMP bmp;
  ZeroMemory(&bmp, sizeof(bmp));
  bmp.hdr.id = BMP_HEADER_ID;
  bmp.hdr.size = sizeof(bmp) + nByteWidth * nHeight;
  bmp.hdr.offset = sizeof(bmp);

  bmp.inf.size = sizeof(bmp.inf);
  bmp.inf.bits = 24;
  bmp.inf.compression = 0;
  bmp.inf.width = nWidth;
  bmp.inf.height = nHeight;
  bmp.inf.imagesize = nByteWidth * nHeight;
  bmp.inf.xresolution = bmp.inf.yresolution = 3780;
  bmp.inf.planes = 1;
  if(!write(hFile, &bmp, sizeof(bmp)))
  {
    ::CloseHandle(hFile);
    return error();
  }

  LPBYTE lpRowBuffer = new BYTE[nByteWidth+1];  //+1 because we write 32bit values into 24bit slots
  for(UINT nY = nHeight; nY; nY--)
  {
    ::ZeroMemory(lpRowBuffer, nByteWidth);
    for(UINT nX = 0; nX < nWidth; nX++)
    {
      *((LPRGB32)&lpRowBuffer[3*nX]) = getPixel(nX, nY-1);
    }
    if(!write(hFile, lpRowBuffer, nByteWidth))
    {
      ::CloseHandle(hFile);
      delete lpRowBuffer;
      return error();
    }
  }
  ::CloseHandle(hFile);
  delete lpRowBuffer;
  return success();
}


//-------------------------------------------------------------------------------------------------
BOOL ResItemImage::importBitmap(LPCWSTR lpcwName)
{
  HANDLE hFile = ::CreateFile(lpcwName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
    OPEN_EXISTING, 0, NULL);
  if(hFile == INVALID_HANDLE_VALUE) return error();

  BMP bmp;
  if(!read(hFile, &bmp, sizeof(BMP))) return error();
  if(bmp.hdr.id != BMP_HEADER_ID || bmp.inf.width < 1 || bmp.inf.height < 1 ||
    bmp.inf.bits != 24 || bmp.inf.compression != 0 || bmp.inf.planes != 1)
  {
    ::CloseHandle(hFile);
    return error(ERROR_BAD_FORMAT);
  }
  if(::SetFilePointer(hFile, bmp.hdr.offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
  {
    ::CloseHandle(hFile);
    return error();
  }

  freeBitmap(); //free old bitmap to force update on redraw

  CONST UINT nByteWidth = (bmp.inf.width*3+3)&(3^-1);
  LPBYTE lpRowBuffer = new BYTE[nByteWidth+1];  //+1 because we read 32bit values from 24bit slots
  UINT nY = (nHeight < (UINT)bmp.inf.height)? nHeight : (UINT)bmp.inf.height;
  while(nY-- > 0)
  {
    ::ZeroMemory(lpRowBuffer, nByteWidth+1);

    if(!read(hFile, lpRowBuffer, nByteWidth)) {
      ::CloseHandle(hFile);
      delete lpRowBuffer;
      return error();
    }

    for(UINT nX = 0; nX < nWidth && nX < (UINT)bmp.inf.width; nX++)
    {
      setPixel(nX, nY, *((LPRGB32)&lpRowBuffer[3*nX]));
    }
  }
  delete lpRowBuffer;

  CloseHandle(hFile);
  return success();
}
