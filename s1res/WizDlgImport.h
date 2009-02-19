//=================================================================================================
// WizDlgImport.h : custom implementation of an import wizard dialog
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#include "WizDlg.h"
#include "DeviceIO.h"


//-------------------------------------------------------------------------------------------------
class WizDlgImport : public WizDlg
{
  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    WizDlgImport() : WizDlg()
    {
      LoadLibrary(_T("RICHED20.DLL"));
      fGiveIO = true;
    }

    BOOL getFirmware(LPBYTE &lpData, DWORD &dwDataSize) //redirect getFirmware-method
    {
      return device.getFirmware(lpData, dwDataSize);
    }

  //-----------------------------------------------------------------------------------------------
  // methods (protected)
  //-----------------------------------------------------------------------------------------------
  protected:
    UINT getID(GETID nGetId);     //return the requested resource id
    UINT getIDD(UINT nStage);     //return the dialog id for the given stage

    BOOL onInit();                //perform global init and goto the first wizard page
    BOOL onClose();               //return false to cancel close operation
    BOOL onNext();                //goto next wizard page
    BOOL onPrev();                //goto last wizard page

    INT_PTR DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL onInitDialog(HWND hwndDlg);
    BOOL onCommand(HWND hwndDlg, WORD wCmd, WORD wEvent);
    BOOL onDestroy(HWND hwndDlg);
    BOOL onUserMsg(HWND hwndDlg, DWORD lParam);

  //-----------------------------------------------------------------------------------------------
  // attributes (protected)
  //-----------------------------------------------------------------------------------------------
  protected:
    DeviceIO device;              //give decoupled access to the device

    BOOL fGiveIO;                 //use giveio or adfu access mode
    INT nDevSel;                  //the selected device
};
