//=================================================================================================
// FwPropDlg.h : class to manage the firmware property sheet dialog
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
#include "resource.h"
#include "Dialog.h"

#include "../common/s1def.h"


//-------------------------------------------------------------------------------------------------
class FwPropDlg : private Dialog
{
  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    FwPropDlg(LP_FW_HEAD lpFwHead) : Dialog(IDD_FW_PROP)
      {this->lpFwHead = lpFwHead;}

    bool runDialog(HINSTANCE hInstance, HWND hwndParent)
      {return run(hInstance, hwndParent) == IDOK;}

  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL setComboBox(HWND hwndDlg, UINT uCtrlId, UINT uInitStrId, UINT uSelect =0);
    BOOL getComboBox(HWND hwndDlg, UINT uCtrlId, uint8 *lpValue);
    BOOL setEditUINT(HWND hwndDlg, UINT uCtrlId, UINT uNumber, UINT uMaxChar =0);
    BOOL setEditText(HWND hwndDlg, UINT uCtrlId, LPCSTR lpText, UINT uMaxChar =0);
    UINT getEditText(HWND hwndDlg, UINT uCtrlId, LPSTR lpText, UINT uMaxChar);
    BOOL setEditText(HWND hwndDlg, UINT uCtrlId, LPCWSTR lpText, UINT uMaxChar =0);
    UINT getEditText(HWND hwndDlg, UINT uCtrlId, LPWSTR lpText, UINT uMaxChar);
    BOOL setSlider(HWND hwndDlg, UINT uCtrlId, UINT uTextId, UINT uValue, UINT uMin =0, UINT uMax =0);

  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    LP_FW_HEAD lpFwHead;
};
