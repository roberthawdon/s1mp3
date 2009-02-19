//=================================================================================================
// ResItemString.h : handle resource strings
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
#define RESITM_STRING_MAX_ENTRIES 32


//-------------------------------------------------------------------------------------------------
class ResItemString : public ResItem
{
  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:

  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    ResItemString(TYPE nType, LPCWSTR lpName, LPBYTE lpData, DWORD dwDataSize)
          : ResItem(nType, lpName, lpData, dwDataSize) {}

    BOOL get(CStdString &str, INT nIndex);
    BOOL set(CStdString &str, INT nIndex);

    BOOL encode(CStdString &str);
    BOOL decode(CStdString &str);

  //-----------------------------------------------------------------------------------------------
  // getters (public)
  //-----------------------------------------------------------------------------------------------
  public:
    DWORD getNoOfStrings()
    {
      DWORD dwResult = 0;

      if(getData() && getDataSize())
      {
        switch(getType())
        {
        case TYPE_STRING:
          dwResult = 1;
          break;
        
        case TYPE_MSTRING:
          // count string-terminators (0x00)
          for(LPCSTR lpPos = (LPCSTR)getData(); lpPos < (LPCSTR)&getData()[getDataSize()]; lpPos++)
            if(*lpPos == 0x00) dwResult++;
          break;
        }
      }

      return dwResult;
    }

    DWORD getSizeOfStrings()
    {
      DWORD dwResult = 0;
      
      if(getData() && getDataSize())
      {
        switch(getType())
        {
        case TYPE_STRING: //get size of single string
          while(dwResult < getDataSize() && getData()[dwResult++]);
          break;
        
        case TYPE_MSTRING:  //search backwards for the last zero-terminator
          for(dwResult = getDataSize(); dwResult > 0 && getData()[dwResult-1]; dwResult--);
          break;
        }        
      }

      return dwResult;
    }

    DWORD getSizeRemaining()
    {
      return getDataSize() - getSizeOfStrings();
    }

  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    INT hex2dec(WCHAR wc);
};
