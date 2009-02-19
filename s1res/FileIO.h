//=================================================================================================
// FileIO.h : access firmware and resource files (AFI, FWI, RES)
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

#include "../common/s1def.h"


//-------------------------------------------------------------------------------------------------
class FileIO
{
  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    ResItem *lpRootItem;
    LPBYTE lpData;
    DWORD dwDataSize;
    BOOL fAltered;

  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    FileIO();
    ~FileIO();

    VOID close();

    BOOL load(LPBYTE lpData, DWORD dwDataSize);
    BOOL load(LPCWSTR lpFileName);
    BOOL store(LPCWSTR lpFileName);

    BOOL update();

    LPBYTE getData() {return lpData;}
    DWORD getDataSize() {return dwDataSize;}

    BOOL isOpen() {return(lpRootItem && lpData && dwDataSize);}
    BOOL wasAltered() {return fAltered;}
    BOOL wasAltered(BOOL fAltered) {return(this->fAltered = fAltered);}

  //-----------------------------------------------------------------------------------------------
  // getters (public)
  //-----------------------------------------------------------------------------------------------
  public:
    ResItem *getRootItem() {return this->lpRootItem;}

  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    BOOL parse();
    BOOL parse(ResItem *lpItemParent);

    BOOL update(ResItem *lpItemParent);

    BOOL isAFI(LPBYTE lpb, DWORD dwSize);
    BOOL isFW(LPBYTE lpb, DWORD dwSize);
    BOOL isRES(LPBYTE lpb, DWORD dwSize);
    BOOL isBREC(LPBYTE lpb, DWORD dwSize);
    BOOL isFONT8(LP_FW_HEAD_ENTRY lpe);
    BOOL isFONT16(LP_FW_HEAD_ENTRY lpe);

    VOID convertString(CStdString &strName, const char *lpStr, INT nLength);
    VOID convertFilename(CStdString &strName, const char *lpFile8, const char *lpExt3);
};
