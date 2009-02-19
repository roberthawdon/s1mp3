//=================================================================================================
// WizDlgExport.cpp : custom implementation of an export wizard dialog
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#include "common.h"
#include "resource.h"
#include "RCData.h"
#include "WizDlgExport.h"


//-------------------------------------------------------------------------------------------------
UINT WizDlgExport::getID(GETID nGetId)
{
  switch(nGetId)
  {
  case GETID_DLG_MAIN: return IDD_WIZ_EXPORT;
  case GETID_ICO_MAIN: return IDI_WIZARD;
  case GETID_BTN_NEXT: return IDC_BTN_NEXT;
  case GETID_BTN_PREV: return IDC_BTN_PREV;
  case GETID_BTN_CANCEL: return IDCANCEL;
  default: return 0;
  }
}


//-------------------------------------------------------------------------------------------------
UINT WizDlgExport::getIDD(UINT nStage)
{
  return nStage;  //use dialog-id as stage-id only
}


//-------------------------------------------------------------------------------------------------
BOOL WizDlgExport::onInit()
{
  return setStage(IDD_WIZ_PG_DEVICE);
}


//-------------------------------------------------------------------------------------------------
BOOL WizDlgExport::onClose()
{
  if(device.isThreadRunning())
  {
    ::SetLastError(ERROR_NOT_READY);
    return false;
  }

  return true;
}


//-------------------------------------------------------------------------------------------------
BOOL WizDlgExport::onNext()
{
  switch(getStage())
  {
  case IDD_WIZ_PG_DEVICE:
    return setStage(IDD_WIZ_PG_ACCESS);
  case IDD_WIZ_PG_ACCESS:
    return setStage(IDD_WIZ_PG_FINISH);
  case IDD_WIZ_PG_FINISH:
    close();
    return true;
  }

  ::SetLastError(ERROR_NOT_FOUND);
  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL WizDlgExport::onPrev()
{
  return setStage(IDD_WIZ_PG_DEVICE);
}


//-------------------------------------------------------------------------------------------------
INT_PTR WizDlgExport::DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG: return onInitDialog(hwndDlg);
  case WM_COMMAND: return onCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam));
  case WM_DESTROY: return onDestroy(hwndDlg);
  case WM_USER: return onUserMsg(hwndDlg, (DWORD)lParam);
  }

  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL WizDlgExport::onInitDialog(HWND hwndDlg)
{
  enableCancel(true);
  enablePrev(false);
  enableNext(false);

  switch(getStage())
  {
  case IDD_WIZ_PG_DEVICE:
    ::SetDlgItemTextA(hwndDlg, IDC_TEXT, (LPCSTR)RCData().load(getInstance(), IDR_RTF_PG_DEVICE));
    ::SendDlgItemMessage(hwndDlg, IDC_BTN_RELOAD, BM_SETIMAGE,
      IMAGE_ICON, (LPARAM)::LoadIcon(getInstance(), MAKEINTRESOURCE(IDI_RELOAD)));
    ::SendDlgItemMessage(hwndDlg, IDC_LIST, LB_RESETCONTENT, 0, 0);
    if(!device.updateDeviceList(hwndDlg)) doError();
    else
    {
      enableCancel(false);
      EnableDlgItem(hwndDlg, IDC_BTN_RELOAD, false);
    }
    SetDlgRadioButton(hwndDlg, IDC_RAD_ADFU, true);
    ::SetFocus(::GetDlgItem(hwndDlg, IDC_LIST));
    return false;

  case IDD_WIZ_PG_ACCESS:
    ::SetDlgItemTextA(hwndDlg, IDC_TEXT, (LPCSTR)RCData().load(getInstance(), IDR_RTF_PG_ACCESS_WR));
    if(!device.write(hwndDlg, nDevSel, 1, fGiveIO))
    {
      doError();
      enablePrev(true);
    }
    else
    {
      enableCancel(false);
    }
    ::SetFocus(::GetDlgItem(hwndDlg, IDC_STATUS));
    return false;

  case IDD_WIZ_PG_FINISH:
    ::SetDlgItemTextA(hwndDlg, IDC_TEXT, (LPCSTR)RCData().load(getInstance(), IDR_RTF_PG_FINISH_WR));
    EnableDlgItem(hwndDlg, IDC_STATUS, false);
    ::ShowWindow(::GetDlgItem(hwndDlg, IDC_STATUS), SW_HIDE);
    enablePrev(true);
    enableNext(true);
    return true;
  }

  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL WizDlgExport::onCommand(HWND hwndDlg, WORD wCmd, WORD wEvent)
{
  switch(getStage())
  {
  case IDD_WIZ_PG_DEVICE:
    if(wCmd == IDC_BTN_RELOAD)
    {
      enableNext(false);
      ::SendDlgItemMessage(hwndDlg, IDC_LIST, LB_RESETCONTENT, 0, 0);
      if(!device.updateDeviceList(hwndDlg)) doError();
      else
      {
        enableCancel(false);
        enablePrev(false);
        EnableDlgItem(hwndDlg, IDC_BTN_RELOAD, false);
      }
    }
    return true;
  }

  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL WizDlgExport::onDestroy(HWND hwndDlg)
{
  switch(getStage())
  {
  case IDD_WIZ_PG_DEVICE:
    fGiveIO = GetDlgRadioButton(hwndDlg, IDC_RAD_GIVEIO);
    nDevSel = (INT)::SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
    break;
  }

  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL WizDlgExport::onUserMsg(HWND hwndDlg, DWORD lParam)
{
  switch(getStage())
  {
  case IDD_WIZ_PG_DEVICE:             // device list update has finished
    enableCancel(true);
    EnableDlgItem(hwndDlg, IDC_BTN_RELOAD, true);
    if(lParam == ERROR_SUCCESS) 
    {
      if(!device.displayDeviceList(::GetDlgItem(hwndDlg, IDC_LIST))) doError();
      else enableNext( ::SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0) != LB_ERR );
    }
    else
    {
      SetLastError(lParam);
      doError();
    }
    return true;

  case IDD_WIZ_PG_ACCESS:            // device access has finished
    enableCancel(true);
    if(lParam == ERROR_SUCCESS) 
    {
      enablePrev(true);
      enableNext(true);
    }
    else
    {
      SetLastError(lParam);
      doError();

      enablePrev(true);
    }
    return true;
  }

  return false;
}
