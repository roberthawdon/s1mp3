//=================================================================================================
// AboutDlg.h : class to manage about dialog
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#include "resource.h"
#include "Dialog.h"
#include "Hyperlink.h"


class AboutDlg : private Dialog
{
  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    AboutDlg() : Dialog(IDD_ABOUT)
    {
    }

    bool runDialog(HINSTANCE hInstance, HWND hwndParent)
    {
      return run(hInstance, hwndParent) > 0;
    }

  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      switch(uMsg)
      {
      case WM_INITDIALOG:
        Hyperlink(hwndDlg, IDC_TXT_HTTP, URL_MAIN);
        ::ShowWindow(hwndDlg, SW_SHOW);
        return false;

      case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
          ::EndDialog(hwndDlg, LOWORD(wParam));
          return true;
        }
      }
      return false;
    }
};
