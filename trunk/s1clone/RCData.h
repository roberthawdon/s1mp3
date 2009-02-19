//=================================================================================================
// RCData.h : support loading of custom RCDATA resources
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once


//-------------------------------------------------------------------------------------------------
class RCData
{
  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    RCData() {lp = NULL;}
    RCData(HINSTANCE hInstance, UINT uId) {load(hInstance, uId);}
    LPBYTE get() {return lp;}
    LPBYTE load(HINSTANCE hInstance, UINT uId)
    {
      HRSRC hrc = FindResource(hInstance, MAKEINTRESOURCE(uId), RT_RCDATA);
      if(!hrc) return(lp = NULL);
      //DWORD dwSize = SizeofResource(hInstance, hrc);
      HGLOBAL hg = LoadResource(hInstance, hrc);
      if(!hg) return(lp = NULL);
      return(lp = (LPBYTE)LockResource(hg));
    }

  //-----------------------------------------------------------------------------------------------
  // argument (private)
  //-----------------------------------------------------------------------------------------------
  private:
    LPBYTE lp;
};
