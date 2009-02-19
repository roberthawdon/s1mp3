//=================================================================================================
// Dialog.h : class to create modal dialog boxes
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


//-------------------------------------------------------------------------------------------------
class Dialog {
  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    //---------------------------------------------------------------------------------------------
    Dialog(UINT uId)
    {
      this->uId = uId;
      this->hInstance = NULL;
    }

    //---------------------------------------------------------------------------------------------
    INT_PTR run(HINSTANCE hInstance, HWND hwndParent) //return <= 0 on errors
    {
      this->hInstance = hInstance;
      return ::DialogBoxParam(hInstance, MAKEINTRESOURCE(uId), 
        hwndParent, StaticDialogProc, (LPARAM)this);
    }

    //---------------------------------------------------------------------------------------------
    HWND create(HINSTANCE hInstance, HWND hwndParent) //return <= 0 on errors
    {
      this->hInstance = hInstance;
      return ::CreateDialogParam(hInstance, MAKEINTRESOURCE(uId), 
        hwndParent, StaticDialogProc, (LPARAM)this);
    }


  //-----------------------------------------------------------------------------------------------
  // methods (protected)
  //-----------------------------------------------------------------------------------------------
  protected:
    //---------------------------------------------------------------------------------------------
    virtual INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      if(uMsg == WM_COMMAND) switch(LOWORD(wParam))
      {
      case IDOK:
      case IDCANCEL:
        ::EndDialog(hwndDlg, LOWORD(wParam));
      }
      return false;
    }

    //---------------------------------------------------------------------------------------------
    UINT getDialogId()
    {
      return uId;
    }

    //---------------------------------------------------------------------------------------------
    HINSTANCE getInstance()
    {
      return hInstance;
    }


  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    //---------------------------------------------------------------------------------------------
    static INT_PTR CALLBACK StaticDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      register Dialog *lpDlg;

      switch(uMsg)
      {
      case WM_INITDIALOG:
        ::SetWindowLongPtr(hwndDlg, GWLP_USERDATA, PtrToLong(lParam));
        if(!lParam) return false;
        return ((Dialog*)lParam)->DialogProc(hwndDlg, uMsg, wParam, lParam);
      
      case WM_DESTROY:
        lpDlg = (Dialog *)LongToPtr(::GetWindowLongPtr(hwndDlg, GWLP_USERDATA));
        if(!lpDlg) return false;
        ::SetWindowLongPtr(hwndDlg, GWLP_USERDATA, NULL);
        return lpDlg->DialogProc(hwndDlg, uMsg, wParam, lParam);

      default:
        lpDlg = (Dialog *)LongToPtr(::GetWindowLongPtr(hwndDlg, GWLP_USERDATA));
        if(!lpDlg) return false;
        return lpDlg->DialogProc(hwndDlg, uMsg, wParam, lParam);
      }
    }


  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    UINT uId;
    HINSTANCE hInstance;
};
