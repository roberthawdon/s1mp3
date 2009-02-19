//=================================================================================================
// s1clone
//
// description  wizard to clone s1mp3 players flash chip content
// created      05/27/2007
// last change  05/09/2008
// author       wiRe <http://www.w1r3.de/>
// source       http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================


//-------------------------------------------------------------------------------------------------
#include "common.h"
#include "MyWizDlg.h"
#include "CloneDevice.h"


//-------------------------------------------------------------------------------------------------
MyWizDlg myWizDlg;
CloneDevice clone;


//=================================================================================================
// WINMAIN
//=================================================================================================
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, INT nCmdShow)
{
  InitCommonControls();
  LoadLibrary(L"RICHED20.DLL");

  try
  {
    return myWizDlg.run(hInstance, NULL, nCmdShow);
  }
  catch(std::bad_alloc &)
  {
    myWizDlg.doError(ERROR_OUTOFMEMORY);
  }
  catch(...)
  {
    myWizDlg.doError(ERROR_UNHANDLED_ERROR);
  }

  return(0);
}