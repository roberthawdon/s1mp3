//=================================================================================================
// FwPropDlg.cpp : class to manage the firmware property sheet dialog
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
#include "FwPropDlg.h"


//-------------------------------------------------------------------------------------------------
INT_PTR CALLBACK FwPropDlg::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    {
      setEditUINT(hwndDlg, IDC_EDT_VER_A, lpFwHead->version_id[0]>>4, 1);
      setEditUINT(hwndDlg, IDC_EDT_VER_B, lpFwHead->version_id[0]&15, 1);
      setEditUINT(hwndDlg, IDC_EDT_VER_C, (lpFwHead->version_id[1]>>4)*10 + (lpFwHead->version_id[1]&15), 2);

      SYSTEMTIME st = {
        (lpFwHead->date.year[0]>>4)*1000 + (lpFwHead->date.year[0]&15)*100 +
        (lpFwHead->date.year[1]>>4)*10 + (lpFwHead->date.year[1]&15),
        (lpFwHead->date.month>>4)*10 + (lpFwHead->date.month&15),
        0,
        (lpFwHead->date.day>>4)*10 + (lpFwHead->date.day&15),
        0, 0, 0, 0
      };
      DateTime_SetSystemtime(::GetDlgItem(hwndDlg, IDC_EDT_DATE), GDT_VALID, &st);

      char szHex[32];
      sprintf_s<sizeof(szHex)>(szHex, "%04X", lpFwHead->vendor_id);
      setEditText(hwndDlg, IDC_EDT_VID, szHex, 4);
      sprintf_s<sizeof(szHex)>(szHex, "%04X", lpFwHead->product_id);
      setEditText(hwndDlg, IDC_EDT_PID, szHex, 4);

      setEditText(hwndDlg, IDC_EDT_GENERAL,       lpFwHead->info.general, 32);
      setEditText(hwndDlg, IDC_EDT_MANUFACTURER,  lpFwHead->info.manufacturer, 32);
      setEditText(hwndDlg, IDC_EDT_DEV_NAME,      lpFwHead->info.device, 32);
      setEditText(hwndDlg, IDC_EDT_VENDOR_INF,    lpFwHead->info.vendor, 8);
      setEditText(hwndDlg, IDC_EDT_PROD_INF,      lpFwHead->info.product, 16);
      setEditText(hwndDlg, IDC_EDT_PROD_REV,      lpFwHead->info.revision, 4);
      setEditText(hwndDlg, IDC_EDT_USB_DESC,      (LPCWSTR)lpFwHead->info.usb_desc, 23);

      setComboBox(hwndDlg, IDC_CMB_LANG,    IDS_FW_PROP_LANG,     lpFwHead->comval.language_id);
      setComboBox(hwndDlg, IDC_CMB_BATTERY, IDS_FW_PROP_BATTERY,  lpFwHead->comval.battery_type);
      setComboBox(hwndDlg, IDC_CMB_ONLINE,  IDS_FW_PROP_ONLINE,   lpFwHead->comval.online_mode);
      setComboBox(hwndDlg, IDC_CMB_REPLAY,  IDS_FW_PROP_REPLAY,   lpFwHead->comval.replay_mode);

      setSlider(hwndDlg, IDC_SLD_DISP,    IDC_TXT_DISP,     lpFwHead->comval.display_contrast,  0, 255);
      setSlider(hwndDlg, IDC_SLD_LIGHT,   IDC_TXT_LIGHT,    lpFwHead->comval.light_time,        0, 255);
      setSlider(hwndDlg, IDC_SLD_SLEEP,   IDC_TXT_SLEEP,    lpFwHead->comval.sleep_time,        0, 255);
      setSlider(hwndDlg, IDC_SLD_STANDBY, IDC_TXT_STANDBY,  lpFwHead->comval.standby_time,      0, 255);

      if(lpFwHead->mtp.manufacturer_sz > 0 && lpFwHead->mtp.manufacturer_sz <= 32)
      {
        if(lpFwHead->mtp.manufacturer_sz < 32) lpFwHead->mtp.manufacturer[lpFwHead->mtp.manufacturer_sz] = 0;
        setEditText(hwndDlg, IDC_EDT_MTP_MANUFACTURER, lpFwHead->mtp.manufacturer, 32);
        if(lpFwHead->mtp.info_sz < 32) lpFwHead->mtp.info[lpFwHead->mtp.info_sz] = 0;
        setEditText(hwndDlg, IDC_EDT_MTP_INFO,         lpFwHead->mtp.info,         32);
        if(lpFwHead->mtp.version_sz < 16) lpFwHead->mtp.version[lpFwHead->mtp.version_sz] = 0;
        setEditText(hwndDlg, IDC_EDT_MTP_VERSION,      lpFwHead->mtp.version,      16);

        if(lpFwHead->mtp.serial_sz == 6)
        {
          sprintf_s<sizeof(szHex)>(szHex, "%02X%02X%02X%02X%02X%02X", 
            (BYTE)lpFwHead->mtp.serial[0], (BYTE)lpFwHead->mtp.serial[1], (BYTE)lpFwHead->mtp.serial[2],
            (BYTE)lpFwHead->mtp.serial[3], (BYTE)lpFwHead->mtp.serial[4], (BYTE)lpFwHead->mtp.serial[5]);
          setEditText(hwndDlg, IDC_EDT_MTP_SERIAL, szHex, 12);
        }
        else
        {
          EnableDlgItem(hwndDlg, IDC_EDT_MTP_SERIAL, false);
        }

        sprintf_s<sizeof(szHex)>(szHex, "%04X", lpFwHead->mtp.vendor_id);
        setEditText(hwndDlg, IDC_EDT_MTP_VID, szHex, 4);
        sprintf_s<sizeof(szHex)>(szHex, "%04X", lpFwHead->mtp.product_id);
        setEditText(hwndDlg, IDC_EDT_MTP_PID, szHex, 4);
      }
      else
      {
        EnableDlgItem(hwndDlg, IDC_EDT_MTP_MANUFACTURER, false);
        EnableDlgItem(hwndDlg, IDC_EDT_MTP_INFO, false);
        EnableDlgItem(hwndDlg, IDC_EDT_MTP_VERSION, false);
        EnableDlgItem(hwndDlg, IDC_EDT_MTP_SERIAL, false);
        EnableDlgItem(hwndDlg, IDC_EDT_MTP_VID, false);
        EnableDlgItem(hwndDlg, IDC_EDT_MTP_PID, false);
      }

      SetDlgRadioButton(hwndDlg, IDC_CHK_RADIO, (lpFwHead->comval.radio_disable & 1) != 0);

      ::ShowWindow(hwndDlg, SW_SHOW);
    }
    return false;

  case WM_HSCROLL:
  case WM_VSCROLL:
    if(LOWORD(wParam) != SB_ENDSCROLL && ::IsWindow((HWND)lParam))
    {
      UINT uPos = (UINT)::SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
      if((HWND)lParam == ::GetDlgItem(hwndDlg, IDC_SLD_DISP))
        setSlider(hwndDlg, IDC_SLD_DISP, IDC_TXT_DISP, uPos);
      else if((HWND)lParam == ::GetDlgItem(hwndDlg, IDC_SLD_LIGHT))
        setSlider(hwndDlg, IDC_SLD_LIGHT, IDC_TXT_LIGHT, uPos);
      else if((HWND)lParam == ::GetDlgItem(hwndDlg, IDC_SLD_SLEEP))
        setSlider(hwndDlg, IDC_SLD_SLEEP, IDC_TXT_SLEEP, uPos);
      else if((HWND)lParam == ::GetDlgItem(hwndDlg, IDC_SLD_STANDBY))
        setSlider(hwndDlg, IDC_SLD_STANDBY, IDC_TXT_STANDBY, uPos);
    }
    break;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDOK:
      {
        lpFwHead->version_id[0] =
          ((::GetDlgItemInt(hwndDlg, IDC_EDT_VER_A, NULL, FALSE)%10)<<4) +
           (::GetDlgItemInt(hwndDlg, IDC_EDT_VER_B, NULL, FALSE)%10);
        lpFwHead->version_id[1] =
          (((::GetDlgItemInt(hwndDlg, IDC_EDT_VER_C, NULL, FALSE)/10)%10)<<4) +
            (::GetDlgItemInt(hwndDlg, IDC_EDT_VER_C, NULL, FALSE)%10);

        SYSTEMTIME st;
        DateTime_GetSystemtime(::GetDlgItem(hwndDlg, IDC_EDT_DATE), &st);
        lpFwHead->date.year[0] = (((st.wYear/1000)%10)<<4) + ((st.wYear/100)%10);
        lpFwHead->date.year[1] = (((st.wYear/10)%10)<<4) + (st.wYear%10);
        lpFwHead->date.month = (((st.wMonth/10)%10)<<4) + (st.wMonth%10);
        lpFwHead->date.day = (((st.wDay/10)%10)<<4) + (st.wDay%10);

        char szHex[32];
        ::GetDlgItemTextA(hwndDlg, IDC_EDT_VID, szHex, sizeof(szHex));
        lpFwHead->vendor_id = (uint16)strtoul(szHex, NULL, 16);
        ::GetDlgItemTextA(hwndDlg, IDC_EDT_PID, szHex, sizeof(szHex));
        lpFwHead->product_id = (uint16)strtoul(szHex, NULL, 16);

        getEditText(hwndDlg, IDC_EDT_GENERAL,       lpFwHead->info.general, 32);
        getEditText(hwndDlg, IDC_EDT_MANUFACTURER,  lpFwHead->info.manufacturer, 32);
        getEditText(hwndDlg, IDC_EDT_DEV_NAME,      lpFwHead->info.device, 32);
        getEditText(hwndDlg, IDC_EDT_VENDOR_INF,    lpFwHead->info.vendor, 8);
        getEditText(hwndDlg, IDC_EDT_PROD_INF,      lpFwHead->info.product, 16);
        getEditText(hwndDlg, IDC_EDT_PROD_REV,      lpFwHead->info.revision, 4);
        getEditText(hwndDlg, IDC_EDT_USB_DESC,      (LPWSTR)lpFwHead->info.usb_desc, 23);

        getComboBox(hwndDlg, IDC_CMB_LANG,    &lpFwHead->comval.language_id);
        getComboBox(hwndDlg, IDC_CMB_BATTERY, &lpFwHead->comval.battery_type);
        getComboBox(hwndDlg, IDC_CMB_ONLINE,  &lpFwHead->comval.online_mode);
        getComboBox(hwndDlg, IDC_CMB_REPLAY,  &lpFwHead->comval.replay_mode);

        lpFwHead->comval.display_contrast = (uint8)::SendDlgItemMessage(hwndDlg, IDC_SLD_DISP,    TBM_GETPOS, 0, 0);
        lpFwHead->comval.light_time       = (uint8)::SendDlgItemMessage(hwndDlg, IDC_SLD_LIGHT,   TBM_GETPOS, 0, 0);
        lpFwHead->comval.sleep_time       = (uint8)::SendDlgItemMessage(hwndDlg, IDC_SLD_SLEEP,   TBM_GETPOS, 0, 0);
        lpFwHead->comval.standby_time     = (uint8)::SendDlgItemMessage(hwndDlg, IDC_SLD_STANDBY, TBM_GETPOS, 0, 0);

        if(lpFwHead->mtp.manufacturer_sz > 0 && lpFwHead->mtp.manufacturer_sz <= 32)
        {
          lpFwHead->mtp.manufacturer_sz = getEditText(hwndDlg, IDC_EDT_MTP_MANUFACTURER, lpFwHead->mtp.manufacturer, 32);
          lpFwHead->mtp.info_sz         = getEditText(hwndDlg, IDC_EDT_MTP_INFO,         lpFwHead->mtp.info,         32);
          lpFwHead->mtp.version_sz      = getEditText(hwndDlg, IDC_EDT_MTP_VERSION,      lpFwHead->mtp.version,      16);

          if(lpFwHead->mtp.serial_sz == 6)
          {
            ::ZeroMemory(szHex, sizeof(szHex));
            ::GetDlgItemTextA(hwndDlg, IDC_EDT_MTP_SERIAL, szHex, sizeof(szHex));
            lpFwHead->mtp.serial[5] = (uint8)strtoul(&szHex[5*2], NULL, 16); szHex[5*2] = 0;
            lpFwHead->mtp.serial[4] = (uint8)strtoul(&szHex[4*2], NULL, 16); szHex[4*2] = 0;
            lpFwHead->mtp.serial[3] = (uint8)strtoul(&szHex[3*2], NULL, 16); szHex[3*2] = 0;
            lpFwHead->mtp.serial[2] = (uint8)strtoul(&szHex[2*2], NULL, 16); szHex[2*2] = 0;
            lpFwHead->mtp.serial[1] = (uint8)strtoul(&szHex[1*2], NULL, 16); szHex[1*2] = 0;
            lpFwHead->mtp.serial[0] = (uint8)strtoul(&szHex[0*2], NULL, 16);
          }

          ::GetDlgItemTextA(hwndDlg, IDC_EDT_MTP_VID, szHex, sizeof(szHex));
          lpFwHead->mtp.vendor_id = (uint16)strtoul(szHex, NULL, 16);
          ::GetDlgItemTextA(hwndDlg, IDC_EDT_MTP_PID, szHex, sizeof(szHex));
          lpFwHead->mtp.product_id = (uint16)strtoul(szHex, NULL, 16);
        }

        lpFwHead->comval.radio_disable = (lpFwHead->comval.radio_disable & 0xFE)
          | (GetDlgRadioButton(hwndDlg, IDC_CHK_RADIO)? 1 : 0);
      }
    case IDCANCEL:
      ::EndDialog(hwndDlg, LOWORD(wParam));
      return true;
    }
    break;
  }
  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL FwPropDlg::setComboBox(HWND hwndDlg, UINT uCtrlId, UINT uInitStrId, UINT uSelect)
{
  CStdString strItem;
  CStdString strInit(MAKEINTRESOURCE(uInitStrId));

  HWND hwndCombo = ::GetDlgItem(hwndDlg, uCtrlId);
  ::SendMessage(hwndCombo, CB_RESETCONTENT, NULL, NULL);

  while(!strInit.IsEmpty())
  {
    TCHAR chr = strInit.at(0);
    strInit.Delete(0);
    if(chr == _T('\t'))
    {
      ::SendMessage(hwndCombo, CB_ADDSTRING, NULL, (LPARAM)strItem.c_str());
      strItem.clear();
    }
    else
    {
      strItem += chr;
    }
  }
  if(!strItem.IsEmpty()) ::SendMessage(hwndCombo, CB_ADDSTRING, NULL, (LPARAM)strItem.c_str());
  return ::SendMessage(hwndCombo, CB_SETCURSEL, uSelect, NULL) != CB_ERR;
}


//-------------------------------------------------------------------------------------------------
BOOL FwPropDlg::getComboBox(HWND hwndDlg, UINT uCtrlId, uint8 *lpValue)
{
  LRESULT l = ::SendDlgItemMessage(hwndDlg, uCtrlId, CB_GETCURSEL, 0, 0);
  if(l == CB_ERR) return FALSE;
  if(lpValue) *lpValue = (uint8)l;
  return TRUE;
}


//-------------------------------------------------------------------------------------------------
BOOL FwPropDlg::setEditUINT(HWND hwndDlg, UINT uCtrlId, UINT uNumber, UINT uMaxChar)
{
  if(uMaxChar) ::SendDlgItemMessage(hwndDlg, uCtrlId, EM_LIMITTEXT, uMaxChar, NULL);
  return ::SetDlgItemInt(hwndDlg, uCtrlId, uNumber, FALSE);
}


//-------------------------------------------------------------------------------------------------
BOOL FwPropDlg::setEditText(HWND hwndDlg, UINT uCtrlId, LPCSTR lpText, UINT uMaxChar)
{
  HWND hwndCtrl = ::GetDlgItem(hwndDlg, uCtrlId);
  if(uMaxChar) ::SendMessage(hwndCtrl, EM_LIMITTEXT, uMaxChar, NULL);

  CHAR *lpBuf = new CHAR[uMaxChar+1];
  strncpy_s(lpBuf, uMaxChar+1, lpText, uMaxChar);
  ::SendMessageA(hwndCtrl, WM_SETTEXT, NULL, (LPARAM)lpBuf);
  delete lpBuf;

  return TRUE;
}


//-------------------------------------------------------------------------------------------------
UINT FwPropDlg::getEditText(HWND hwndDlg, UINT uCtrlId, LPSTR lpText, UINT uMaxChar)
{
  CHAR *lpBuf = new CHAR[uMaxChar+1];
  ::FillMemory(lpBuf, sizeof(CHAR)*(uMaxChar+1), 0x76);
  UINT uLength = ::GetDlgItemTextA(hwndDlg, uCtrlId, lpBuf, uMaxChar+1);
  ::CopyMemory(lpText, lpBuf, sizeof(CHAR)*uMaxChar);
  delete lpBuf;

  return (uLength < uMaxChar)? uLength : uMaxChar;
}


//-------------------------------------------------------------------------------------------------
BOOL FwPropDlg::setEditText(HWND hwndDlg, UINT uCtrlId, LPCWSTR lpText, UINT uMaxChar)
{
  HWND hwndCtrl = ::GetDlgItem(hwndDlg, uCtrlId);
  if(uMaxChar) ::SendMessage(hwndCtrl, EM_LIMITTEXT, uMaxChar, NULL);

  WCHAR *lpBuf = new WCHAR[uMaxChar+1];
  wcsncpy_s(lpBuf, uMaxChar+1, lpText, uMaxChar);
  ::SendMessageW(hwndCtrl, WM_SETTEXT, NULL, (LPARAM)lpBuf);
  delete lpBuf;

  return TRUE;
}


//-------------------------------------------------------------------------------------------------
UINT FwPropDlg::getEditText(HWND hwndDlg, UINT uCtrlId, LPWSTR lpText, UINT uMaxChar)
{
  WCHAR *lpBuf = new WCHAR[uMaxChar+1];
  ::FillMemory(lpBuf, sizeof(WCHAR)*(uMaxChar+1), 0x76);
  UINT uLength = ::GetDlgItemTextW(hwndDlg, uCtrlId, lpBuf, uMaxChar+1);
  ::CopyMemory(lpText, lpBuf, sizeof(WCHAR)*uMaxChar);
  delete lpBuf;

  return (uLength < uMaxChar)? uLength : uMaxChar;
}


//-------------------------------------------------------------------------------------------------
BOOL FwPropDlg::setSlider(HWND hwndDlg, UINT uCtrlId, UINT uTextId, UINT uValue,
                          UINT uMin, UINT uMax)
{
  HWND hwndCtrl = ::GetDlgItem(hwndDlg, uCtrlId);
  if(uMin || uMax) ::SendMessage(hwndCtrl, TBM_SETRANGE, FALSE, MAKELONG(uMin, uMax));
  ::SendMessage(hwndCtrl, TBM_SETPOS, TRUE, uValue);
  ::SetDlgItemInt(hwndDlg, uTextId, uValue, FALSE);
  return TRUE;
}
