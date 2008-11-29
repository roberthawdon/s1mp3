//=================================================================================================
// AdfuException
//
// descr.   adfu exception class
// author   wiRe _at_ gmx _dot_ net
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2007 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#include <windows.h>
#include <tchar.h>


class AdfuException {
public:
  AdfuException(DWORD dwError = 0)
  {
    if(dwError == 0) dwError = GetLastError();
    g_dwError = dwError;

    LPVOID pTextBuffer = NULL;
    if(FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM, 
      NULL, dwError, 0, (LPSTR)&pTextBuffer, 0, NULL) != 0)
    {
      g_strError = (LPCSTR)pTextBuffer;
      LocalFree(pTextBuffer);

      //remove tabs and new/lines from error string
      for(std::string::size_type st; (st = g_strError.find_first_of('\t')) != -1; g_strError.replace(st, 1, 1, ' '));
      for(std::string::size_type st; (st = g_strError.find_first_of('\r')) != -1; g_strError.replace(st, 1, 1, ' '));
      for(std::string::size_type st; (st = g_strError.find_first_of('\n')) != -1; g_strError.replace(st, 1, 1, ' '));
    }
    else
    {    
      g_strError = "unknown error code";
    }

    #ifdef _DEBUG
      std::string strDebug = "AdfuException: ";
      strDebug += g_strError;
      ::OutputDebugStringA(strDebug.c_str());
    #endif
  }

  DWORD id() {return g_dwError;}
  const char *what() {return g_strError.c_str();}

private:
  DWORD g_dwError;
  std::string g_strError;
};
