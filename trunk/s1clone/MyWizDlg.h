//=================================================================================================
// MyWizDlg.h : custom implementation of a wizard dialog
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


//-------------------------------------------------------------------------------------------------
class MyWizDlg : public WizDlg
{
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
    BOOL onTimer(HWND hwndDlg);
    BOOL onUserMsg(HWND hwndDlg, DWORD lParam);
    BOOL onDestroy(HWND hwndDlg);

  //-----------------------------------------------------------------------------------------------
  // attributes (protected)
  //-----------------------------------------------------------------------------------------------
  protected:
    std::list<UINT> lstHistory;
    INT nFilterIndex;   //the last filter index of the open-dialog
    INT nCounter;       //our countdown counter
    INT nOperation;     //the current user operation (IDC_RADIO_read/write/verify/pay)
    INT nDevSel;        //the id of the selected device
    CStdString strFile; //the image filename
    BOOL fFormat;       //format? (read operations)
    BOOL fVerify;       //verify? (write operations)
};
