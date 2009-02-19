//=================================================================================================
// MyWizDlg.cpp : custom implementation of a wizard dialog
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#include "common.h"
#include "RCData.h"
#include "MyWizDlg.h"
#include "CloneDevice.h"


//-------------------------------------------------------------------------------------------------
#define URL_DONATE  L"http://www.s1mp3.de/donate.s1clone.html"


//-------------------------------------------------------------------------------------------------
UINT MyWizDlg::getID(GETID nGetId)
{
  switch(nGetId)
  {
  case GETID_DLG_MAIN: return IDD_MAIN;
  case GETID_ICO_MAIN: return IDI_MAIN;
  case GETID_BTN_NEXT: return IDC_BTN_NEXT;
  case GETID_BTN_PREV: return IDC_BTN_PREV;
  case GETID_BTN_CANCEL: return IDCANCEL;
  default: return 0;
  }
}


//-------------------------------------------------------------------------------------------------
UINT MyWizDlg::getIDD(UINT nStage)
{
  return nStage;
}


//-------------------------------------------------------------------------------------------------
BOOL MyWizDlg::onInit()
{
  fFormat = true;
  fVerify = true;
  nFilterIndex = 0;
  nOperation = IDC_RADIO_READ;
  return setStage(IDD_PG_INIT);
}


//-------------------------------------------------------------------------------------------------
BOOL MyWizDlg::onClose()
{
  if(clone.isThreadRunning())
  {
    if(clone.isThreadCanceled())
    {
      ::SetLastError(ERROR_SIGNAL_PENDING);
      return false;
    }

    CStdString strCaption(MAKEINTRESOURCE(IDS_WARNING));
    CStdString strText(MAKEINTRESOURCE(IDS_ASK_TO_QUIT));
    if(::MessageBox(hwndMain, strText, strCaption, MB_ICONEXCLAMATION|MB_YESNO) == IDYES) clone.cancelThread();
    ::SetLastError(ERROR_SUCCESS);
    return false;
  }

  return true;
}


//-------------------------------------------------------------------------------------------------
BOOL MyWizDlg::onNext()
{
  lstHistory.push_back(getStage());

  switch(getStage())
  {
  case IDD_PG_INIT:
    return setStage(IDD_PG_SELECT);

  case IDD_PG_SELECT:
    if(::SendDlgItemMessage(hwndChild, IDC_RADIO_PAY, BM_GETCHECK, 0, 0))
    {
      OpenURL(URL_DONATE);
      return setStage(IDD_PG_END);
    }
    return setStage(IDD_PG_FILE);

  case IDD_PG_FILE:
    {
      INT nLength = ::GetWindowTextLength(::GetDlgItem(hwndChild, IDC_EDT_FILE));
      if(nLength >= 0)
      {
        TCHAR *lpBuf = new TCHAR[nLength+1];
        ::GetDlgItemText(hwndChild, IDC_EDT_FILE, lpBuf, nLength+1);
        strFile = lpBuf;
        delete lpBuf;
      }

      CStdString strFileExtension(MAKEINTRESOURCE(IDS_FILE_EXTENSION));
      if(nLength < (INT)strFileExtension.length()
        || strFile.ToLower().substr(nLength - strFileExtension.length()).compare(strFileExtension) != 0) 
            strFile.append(strFileExtension);

      ::SetDlgItemText(hwndChild, IDC_EDT_FILE, strFile.c_str());   //write back any changes

      if(nOperation == IDC_RADIO_READ) //check if file exists/gets overwritten
      {
        HANDLE hFile = ::CreateFile(strFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        if(hFile != INVALID_HANDLE_VALUE)
        {
          ::CloseHandle(hFile);

          CStdString strCaption(MAKEINTRESOURCE(IDS_WARNING));
          CStdString strText(MAKEINTRESOURCE(IDS_FILE_OVERWRITE));
          if(::MessageBox(hwndMain, strText, strCaption, MB_ICONEXCLAMATION|MB_YESNO) != IDYES)
          {
            lstHistory.pop_back();
            return true;
          }
        }
      }
    }
    return setStage(IDD_PG_DEVICE);

  case IDD_PG_DEVICE:
    return setStage(IDD_PG_CLONE);

  case IDD_PG_CLONE:
    lstHistory.clear();
    return setStage(IDD_PG_END);

  case IDD_PG_END:
    return setStage(IDD_PG_SELECT);
  }

  lstHistory.pop_back();
  ::SetLastError(ERROR_FILE_NOT_FOUND);
  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL MyWizDlg::onPrev()
{
  UINT nPrevStage = IDD_PG_INIT;
  if(!lstHistory.empty())
  {
    nPrevStage = lstHistory.back();;
    lstHistory.pop_back();
  }
  return setStage(nPrevStage);
}


//-------------------------------------------------------------------------------------------------
INT_PTR MyWizDlg::DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG: return onInitDialog(hwndDlg);
  case WM_COMMAND: return onCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam));
  case WM_TIMER: return onTimer(hwndDlg);
  case WM_USER: return onUserMsg(hwndDlg, (DWORD)lParam);
  case WM_DESTROY: return onDestroy(hwndDlg);
  
  case WM_POWERBROADCAST:
    if(wParam != PBT_APMQUERYSUSPEND && wParam != PBT_APMQUERYSTANDBYFAILED) break;
    return clone.isThreadRunning()? BROADCAST_QUERY_DENY : true;
  }

  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL MyWizDlg::onInitDialog(HWND hwndDlg)
{
  RCData rcRTF;

  enablePrev(!lstHistory.empty());
  enableNext(true);

  switch(getStage())
  {
  case IDD_PG_INIT:
    ::SetDlgItemText(hwndDlg, IDC_RE2, (LPCWSTR)rcRTF.load(hInstance, IDR_RTF_PG_INIT));        
    ::SetDlgItemText(hwndDlg, IDC_RE2_TERMS, (LPCWSTR)rcRTF.load(hInstance, IDR_RTF_TERMS));
    ::SetFocus(::GetDlgItem(hwndDlg, IDC_RE2_TERMS));
    return false;

  case IDD_PG_SELECT:
    ::SetDlgItemText(hwndDlg, IDC_RE2, (LPCWSTR)rcRTF.load(hInstance, IDR_RTF_PG_SELECT));        
    ::SendDlgItemMessage(hwndDlg, nOperation, BM_SETCHECK, BST_CHECKED, 0);
    ::SetFocus(::GetDlgItem(hwndDlg, nOperation));
    return false;

  case IDD_PG_FILE:
    ::SetDlgItemText(hwndDlg, IDC_RE2, (LPCWSTR)rcRTF.load(hInstance, IDR_RTF_PG_FILE));
    ::SendDlgItemMessage(hwndDlg, IDC_BTN_OPEN, BM_SETIMAGE,
      IMAGE_ICON, (LPARAM)::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OPEN)));
    ::SetDlgItemText(hwndDlg, IDC_EDT_FILE, strFile.c_str());
    ::SetFocus(::GetDlgItem(hwndDlg, IDC_EDT_FILE));
    return false;

  case IDD_PG_DEVICE:
    ::SetDlgItemText(hwndDlg, IDC_RE2, (LPCWSTR)rcRTF.load(hInstance, IDR_RTF_PG_DEVICE));
    ::SendDlgItemMessage(hwndDlg, IDC_BTN_RELOAD, BM_SETIMAGE,
      IMAGE_ICON, (LPARAM)::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RELOAD)));
    ::SendDlgItemMessage(hwndDlg, IDC_LIST, LB_RESETCONTENT, 0, 0);
    enableNext(false);
    if(!clone.updateDeviceList(hwndDlg)) doError();
    else
    {
      enableCancel(false);
      enablePrev(false);
      ::EnableWindow(::GetDlgItem(hwndDlg, IDC_BTN_RELOAD), false);
    }
    ::SetFocus(::GetDlgItem(hwndDlg, IDC_LIST));
    switch(nOperation)
    {
      case IDC_RADIO_READ:
        ::SendDlgItemMessage(hwndDlg, IDC_CHKBOX, BM_SETCHECK, fFormat, 0);
        break;

      case IDC_RADIO_VERIFY:
        fVerify = true;
        ::EnableWindow(::GetDlgItem(hwndDlg, IDC_CHKBOX), false);
      case IDC_RADIO_WRITE:
        {
          CStdString strVerify(MAKEINTRESOURCE(IDS_CHKBOX_VERIFY));
          ::SetDlgItemText(hwndDlg, IDC_CHKBOX, strVerify.c_str());
          ::SendDlgItemMessage(hwndDlg, IDC_CHKBOX, BM_SETCHECK, fVerify, 0);
        }
    }
    return false;

  case IDD_PG_CLONE:
    ::SetDlgItemText(hwndDlg, IDC_RE2, (LPCWSTR)rcRTF.load(hInstance, IDR_RTF_PG_CLONE));
    enableNext(false);
    ::SetFocus(::GetDlgItem(hwndMain, getID(GETID_BTN_CANCEL)));
    nCounter = 6;
    ::SendMessage(hwndDlg, WM_TIMER, 0, 0);
    ::SetTimer(hwndDlg, 1, 1000, NULL);
    return false;

  case IDD_PG_END:
    ::SetDlgItemText(hwndDlg, IDC_RE2, (LPCWSTR)rcRTF.load(hInstance, IDR_RTF_PG_END));
    ::SetFocus(::GetDlgItem(hwndDlg, IDC_BTN_DONATE));
    return true;
  }

  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL MyWizDlg::onCommand(HWND hwndDlg, WORD wCmd, WORD wEvent)
{
  switch(getStage())
  {
  case IDD_PG_FILE:
    switch(wCmd)
    {
    case IDC_BTN_OPEN:
      {
        //browse for filename
        CStdString strTitle(MAKEINTRESOURCE(IDS_FOPEN_CAPTION));
        CStdString strFilter(MAKEINTRESOURCE(IDS_FOPEN_FILTER));
        while(strFilter.Replace(_T('\n'), _T('\0')) > 0);

        WCHAR szFile[MAX_PATH] = L"";
        OPENFILENAME ofn;
        ::ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = hwndMain;
        ofn.hInstance = hInstance;
        ofn.lpstrFilter = strFilter.c_str();
        ofn.nFilterIndex = nFilterIndex;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = strTitle.c_str();
        ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_LONGNAMES| 
          ((nOperation != IDC_RADIO_READ)? OFN_FILEMUSTEXIST : 0);
        if(!::GetOpenFileName(&ofn))
        {
          if(::CommDlgExtendedError() != ERROR_SUCCESS) doError(::CommDlgExtendedError());
          return true;
        }
        strFile = szFile;
        ::SetDlgItemText(hwndDlg, IDC_EDT_FILE, szFile);
        nFilterIndex = ofn.nFilterIndex;
      }
      return true;

    case IDC_EDT_FILE:
      if(wEvent == EN_CHANGE)
      {
        //get changed filename
        INT nLength = ::GetWindowTextLength(::GetDlgItem(hwndDlg, IDC_EDT_FILE));
        ::GetDlgItemText(hwndDlg, IDC_EDT_FILE, strFile.GetBuf(nLength), nLength+1);

        //test if we have a valid filename
        if(wcsnlen_s(strFile.Trim(), strFile.GetLength()) <= 0)
        {
          enableNext(false);
        }
        else if(nOperation == IDC_RADIO_READ)
        {
          enableNext(true);
        }
        else
        {
          //verify if file does exist and enable/disable next button
          DWORD dwAttr = GetFileAttributes(strFile.c_str());
          enableNext((dwAttr != INVALID_FILE_ATTRIBUTES) && !(dwAttr&FILE_ATTRIBUTE_DIRECTORY));
        }
      }
      return true;
    }
    return true;

  case IDD_PG_DEVICE:
    if(wCmd == IDC_BTN_RELOAD)
    {
      enableNext(false);
      ::SendDlgItemMessage(hwndDlg, IDC_LIST, LB_RESETCONTENT, 0, 0);
      if(!clone.updateDeviceList(hwndDlg)) doError();
      else
      {
        enableCancel(false);
        enablePrev(false);
        ::EnableWindow(::GetDlgItem(hwndDlg, IDC_BTN_RELOAD), false);
      }
    }
    return true;

  case IDD_PG_END:
    if(wCmd == IDC_BTN_DONATE) OpenURL(URL_DONATE);
    return true;
  }

  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL MyWizDlg::onTimer(HWND hwndDlg)
{
  if(getStage() != IDD_PG_CLONE) return false;

  if(nCounter > 1)
  {
    nCounter--;
    CStdString strStatus;
    strStatus.Format(IDS_COUNTDOWN, nCounter);
    ::SetDlgItemText(hwndDlg, IDC_TXT_STATUS, strStatus.c_str());
  }
  else if(nCounter == 1)
  {
    nCounter = 0;
    ::KillTimer(hwndDlg, 1);
    ::SetDlgItemText(hwndDlg, IDC_TXT_STATUS, L"");
    enablePrev(false);

    switch(nOperation)
    {
    case IDC_RADIO_READ:
      //start reading thread
      if(!clone.read(hwndDlg, nDevSel, 1, strFile, fFormat))
      {
        doError();
        nCounter = -1;
        enablePrev(true);
      }
      break;

    case IDC_RADIO_WRITE:
      //start writing thread
      if(!clone.write(hwndDlg, nDevSel, 1, strFile, true, fVerify))
      {
        doError();
        nCounter = -1;
        enablePrev(true);
      }
      break;

    case IDC_RADIO_VERIFY:
      //start verify thread
      if(!clone.write(hwndDlg, nDevSel, 1, strFile, false, true))
      {
        doError();
        nCounter = -1;
        enablePrev(true);
      }
      break;
    }

  }

  return true;
}


//-------------------------------------------------------------------------------------------------
BOOL MyWizDlg::onUserMsg(HWND hwndDlg, DWORD lParam)
{
  switch(getStage())
  {
  case IDD_PG_DEVICE:           // device update has finished
    enableCancel(true);
    enablePrev(true);
    ::EnableWindow(::GetDlgItem(hwndDlg, IDC_BTN_RELOAD), true);

    if(lParam == ERROR_SUCCESS) 
    {
      if(!clone.displayDeviceList(::GetDlgItem(hwndDlg, IDC_LIST))) doError();
      else enableNext( ::SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0) != LB_ERR );
    }
    else
    {
      SetLastError(lParam);
      doError();
    }

    return true;

  case IDD_PG_CLONE:            // cloning has finished
    ::KillTimer(hwndDlg, 1);
    nCounter = -1;
    enablePrev(true);

    if(lParam == ERROR_SUCCESS) 
    {
      enableNext(true);
    }
    else
    {
      SetLastError(lParam);
      doError();
      enableNext(true);
    }
    
    return true;
  }

  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL MyWizDlg::onDestroy(HWND hwndDlg)
{
  switch(getStage())
  {
  case IDD_PG_SELECT:
    nOperation = 
      (::SendDlgItemMessage(hwndDlg, IDC_RADIO_READ,   BM_GETCHECK, 0, 0) == BST_CHECKED)? IDC_RADIO_READ :
      (::SendDlgItemMessage(hwndDlg, IDC_RADIO_WRITE,  BM_GETCHECK, 0, 0) == BST_CHECKED)? IDC_RADIO_WRITE :
      (::SendDlgItemMessage(hwndDlg, IDC_RADIO_VERIFY, BM_GETCHECK, 0, 0) == BST_CHECKED)? IDC_RADIO_VERIFY :
      IDC_RADIO_PAY;
    break;

  case IDD_PG_FILE:
    {
    }
    break;

  case IDD_PG_DEVICE:
    if(nOperation == IDC_RADIO_READ)
      fFormat = ::SendDlgItemMessage(hwndDlg, IDC_CHKBOX, BM_GETCHECK, 0, 0) == BST_CHECKED;
    else
      fVerify = ::SendDlgItemMessage(hwndDlg, IDC_CHKBOX, BM_GETCHECK, 0, 0) == BST_CHECKED;

    // get first selected item
    nDevSel = (INT)::SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
    
    // get list of selected items
    //LB_GETSELCOUNT
    break;
  }

  return false;
}
