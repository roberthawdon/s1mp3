//=================================================================================================
// Mutex.h : class to create a mutex
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#include <windows.h>


//-------------------------------------------------------------------------------------------------
class Mutex {
  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    HANDLE hMutex;

  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    Mutex() {hMutex = ::CreateMutex(NULL, FALSE, NULL);}
    ~Mutex() {if(hMutex) ::CloseHandle(hMutex);}
    VOID lock() {if(hMutex) ::WaitForSingleObject(hMutex, INFINITE);}
    VOID release() {if(hMutex) ::ReleaseMutex(hMutex);}
    HANDLE getHandle() {return hMutex;}
};