//=================================================================================================
// ResItemString.cpp : handle resource strings
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#include "common.h"
#include "ResItemString.h"
#include "Options.h"


//-------------------------------------------------------------------------------------------------
// retrieve the string with the given index using an unicode character set
//-------------------------------------------------------------------------------------------------
BOOL ResItemString::get(CStdString &str, INT nIndex)
{
  str.clear();

  LPCSTR lpStr = (LPCSTR)getData();
  if(!lpStr || !getDataSize()) return error(ERROR_NO_DATA);

  // search index if we have a multi-string
  if(getType() == TYPE_MSTRING)
  {
    for(; nIndex > 0; nIndex--, lpStr++) for(; *lpStr; lpStr++)
    {
      if(lpStr >= (LPCSTR)&getData()[getDataSize()]) return error(ERROR_NOT_FOUND);
    }
  }
  else if(nIndex > 0)
  {
    return error(ERROR_NOT_FOUND);
  }

  // string found, now detect the string length including the zero-terminator
  DWORD dwStrLen = 0;
  do {
    if(&lpStr[dwStrLen] >= (LPCSTR)&getData()[getDataSize()]) return error(ERROR_NOT_FOUND);
  } while(lpStr[dwStrLen++]);

  // convert the given multi-byte string to unicode
  // depending on the configured code-page inside our options/settings
  if(g_options.get()->uCodePage > OPTIONS_CODEPAGE_DEFAULT_URLENC)
  {
    INT nSize = ::MultiByteToWideChar(g_options.get()->uCodePage, MB_PRECOMPOSED, lpStr, dwStrLen, NULL, 0);
    if(nSize > 0)
    {
      LPWSTR lpWStr = new WCHAR[nSize];
      ::ZeroMemory(lpWStr, sizeof(WCHAR)*nSize);
      ::MultiByteToWideChar(g_options.get()->uCodePage, MB_PRECOMPOSED, lpStr, dwStrLen, lpWStr, nSize);
      str = lpWStr;
      delete lpWStr;
    }
  }
  else  //otherwise use default code page
  {
    str = lpStr;
  }

  return success();
}


//-------------------------------------------------------------------------------------------------
// write the changed string back to the given index, and update the whole data.
// unicode characters get converted back to multibyte, using the selected code-set.
// NOTE: fill remaining memory with 0xFF instead 0x00!
//-------------------------------------------------------------------------------------------------
BOOL ResItemString::set(CStdString &str, INT nIndex)
{
  if(!getData() || !getDataSize()) return error(ERROR_NO_DATA);

  // get size of multibyte string
  UINT nCodePage = (g_options.get()->uCodePage > OPTIONS_CODEPAGE_DEFAULT_URLENC)?
    g_options.get()->uCodePage : CP_ACP;
  INT nSize = ::WideCharToMultiByte(nCodePage, 0, str.c_str(), -1, NULL, 0, "_", NULL);
  if(nSize <= 0) return false;

  // alloc memory and convert string
  LPSTR lpStr = new CHAR[nSize];
  ::FillMemory(lpStr, nSize, -1);
  ::WideCharToMultiByte(nCodePage, 0, str.c_str(), -1, lpStr, nSize, "_", NULL);

  // write string back
  if(getType() == TYPE_MSTRING) //multiple-strings?
  {
    // get ptr/size of all previous strings
    LPBYTE lpPrev = getData();
    DWORD dwPrevSize = 0;
    while(nIndex > 0)
    {
      if(dwPrevSize >= getDataSize()) {
        delete lpStr;
        return error(ERROR_NOT_FOUND);
      }
      if(lpPrev[dwPrevSize++] == 0) nIndex--;
    }

    // get ptr/size of all following strings
    LPBYTE lpFollow = &lpPrev[dwPrevSize];
    DWORD dwFollowSize = 0;
    while(lpFollow < &getData()[getDataSize()] && *lpFollow) lpFollow++;
    if(&lpFollow[1] < &getData()[getDataSize()])
    {
      lpFollow++;
      dwFollowSize = getSizeOfStrings() - (DWORD)(lpFollow - getData());
    }

    // copy string into data, if the string exceeds any given limits: cut it!
    LPBYTE lpNewData = new BYTE[getDataSize()];
    ::FillMemory(lpNewData, getDataSize(), -1);
    if(dwPrevSize) ::CopyMemory(lpNewData, lpPrev, dwPrevSize);

    DWORD dwMaxStrSize = getDataSize() - dwPrevSize - dwFollowSize;
    if(dwMaxStrSize < (DWORD)nSize)  //cut string?
    {
      if(dwMaxStrSize > 1) ::CopyMemory(&lpNewData[dwPrevSize], lpStr, dwMaxStrSize-1);
      if(dwMaxStrSize > 0) lpNewData[dwPrevSize + dwMaxStrSize - 1] = 0;
      if(dwFollowSize) ::CopyMemory(&lpNewData[dwPrevSize + dwMaxStrSize], lpFollow, dwFollowSize);
    }
    else  //string fits, just copy...
    {
      if(nSize > 0) ::CopyMemory(&lpNewData[dwPrevSize], lpStr, nSize);
      if(dwFollowSize) ::CopyMemory(&lpNewData[dwPrevSize + nSize], lpFollow, dwFollowSize);
    }

    ::CopyMemory(getData(), lpNewData, getDataSize());
    delete lpNewData;
  }
  else  //otherwise just copy single string
  {
    if(nIndex > 0) return error(ERROR_NOT_FOUND);

    ::FillMemory(getData(), getDataSize(), -1);
    ::CopyMemory(getData(), lpStr, ((DWORD)nSize <= getDataSize())? nSize : getDataSize());
  }

  // free allocated memory and return
  delete lpStr;
  return success();
}


//-------------------------------------------------------------------------------------------------
// encode the string using url encoding scheme
// see http://www.w3schools.com/tags/ref_urlencode.asp
//-------------------------------------------------------------------------------------------------
BOOL ResItemString::encode(CStdString &str)
{
  if(g_options.get()->uCodePage == OPTIONS_CODEPAGE_DEFAULT_URLENC)   //url-encode string?
  {
    const char hex[] = "0123456789ABCDEF";
    CStdString strDest;

    LPCWSTR lpszStr = str.c_str();
    while(*lpszStr)
    {
      if(*lpszStr > 255) strDest += _T('?');  //don't support extended character sets
      else if((*lpszStr > 32) && (*lpszStr < 127) && (*lpszStr != _T('%'))) strDest += (char)*lpszStr;
      else strDest.AppendFormat( _T("%%%c%c"), hex[(*lpszStr >> 4)&15], hex[*lpszStr&15] );
      lpszStr++;
    }

    str = strDest;
  }
  return success();
}


//-------------------------------------------------------------------------------------------------
// decode the string using url encoding scheme
// see http://www.w3schools.com/tags/ref_urlencode.asp
//-------------------------------------------------------------------------------------------------
BOOL ResItemString::decode(CStdString &str)
{
  if(g_options.get()->uCodePage == OPTIONS_CODEPAGE_DEFAULT_URLENC)   //url-decode string?
  {
    std::string strDest;
    for(INT n = 0; n < str.GetLength(); n++)
    {
      WCHAR wc = str.GetAt(n);
      if(wc != _T('%')) strDest += (char)wc;  //truncate wchar to char
      else if((n+2) < str.GetLength())
      {
        INT h = hex2dec(str.GetAt(n+1));
        INT l = hex2dec(str.GetAt(n+2));
        if(h < 0 || l < 0)
        {
          strDest += _T('%');
          continue;
        }
        strDest += (char)(((h&15)<<4)|(l&15));
        n += 2;
      }
      else return error(ERROR_INTERNAL_ERROR);
    }

    str = strDest;
  }
  return success();
}


//-------------------------------------------------------------------------------------------------
INT ResItemString::hex2dec(WCHAR wc)
{
  switch(wc)
  {
    case _T('0'): return(0);
    case _T('1'): return(1);
    case _T('2'): return(2);
    case _T('3'): return(3);
    case _T('4'): return(4);
    case _T('5'): return(5);
    case _T('6'): return(6);
    case _T('7'): return(7);
    case _T('8'): return(8);
    case _T('9'): return(9);
    case _T('a'): case _T('A'): return(10);
    case _T('b'): case _T('B'): return(11);
    case _T('c'): case _T('C'): return(12);
    case _T('d'): case _T('D'): return(13);
    case _T('e'): case _T('E'): return(14);
    case _T('f'): case _T('F'): return(15);
    default: return(-1);
  }
}
