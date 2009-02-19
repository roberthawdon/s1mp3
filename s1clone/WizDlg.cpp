//=================================================================================================
// WizDlg.cpp : abstract class to implement wizard dialogs with multiple pages
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#include <windows.h>
#include "WizDlg.h"


//-------------------------------------------------------------------------------------------------
WizDlg::WizDlg(void)
{
  hInstance = NULL;
  hwndParent = hwndMain = hwndChild = NULL;
  nCurrentStage = 0;
}


//-------------------------------------------------------------------------------------------------
WizDlg::~WizDlg(void)
{
  if(::IsWindow(hwndMain)) ::DestroyWindow(hwndMain);
}


//-------------------------------------------------------------------------------------------------
BOOL WizDlg::run(HINSTANCE hInstance, HWND hwndParent, UINT nCmdShow)
{
  if(::IsWindow(hwndMain)) ::DestroyWindow(hwndMain);

  this->hInstance = hInstance;
  this->hwndParent = hwndParent;
  hwndMain = hwndChild = NULL;
  nCurrentStage = 0;

  //init the main wizard dialog
  if(!CreateDialogParam(hInstance, MAKEINTRESOURCE(getID(GETID_DLG_MAIN)),
    hwndParent, StaticDlgProc, (LPARAM)this))
  {
    doError();
    return false;
  }

  if(!::IsWindow(hwndMain))
  {
    doError(ERROR_INTERNAL_ERROR);
    return false;
  }

  //show wizard
  ShowWindow(hwndMain, nCmdShow);

  //load accelerators from resource
  //HACCEL hAccTable = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MAIN));

  //run message loop
  for(MSG lpMsg;;)
  {
    switch(::GetMessage(&lpMsg, NULL, 0, 0))
    {
      case -1:  //doError
        doError();
        return false;

      case 0:   //WM_QUIT
        return true; //((UINT)lpMsg.wParam);

      default:  //WM_XXX
        //if( !::TranslateAccelerator(hwndMain, hAccTable, &lpMsg) )
        if( !::IsDialogMessage(hwndMain, &lpMsg) )
        { 
          ::TranslateMessage(&lpMsg);
          ::DispatchMessage(&lpMsg);
        }
    }
  }
}


//-------------------------------------------------------------------------------------------------
// show error dialog
//-------------------------------------------------------------------------------------------------
VOID WizDlg::doError(DWORD dwError)
{
  if(dwError == ERROR_SUCCESS)
  {
    dwError = ::GetLastError();
    if(dwError == ERROR_SUCCESS) return;
  }

  HWND hwnd = ::IsWindow(hwndMain)? hwndMain : ::IsWindow(hwndParent)? hwndParent : NULL;
  LPTSTR lpszText = NULL;
  if(::FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
    NULL, dwError, 0, (LPTSTR)&lpszText, 0, NULL))
  {
    ::MessageBox(hwnd, lpszText, L"Error", MB_OK | MB_ICONERROR);
    ::LocalFree(lpszText);
  }
  else
  {
    ::MessageBox(hwnd, L"Unknown error occured.", L"Error", MB_OK | MB_ICONERROR);
  }
}


//-------------------------------------------------------------------------------------------------
INT_PTR CALLBACK WizDlg::StaticDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  WizDlg *lpDlg = NULL;
  if(uMsg == WM_INITDIALOG)
  {
    if(lpDlg = (WizDlg *)lParam)
    {
      if(!lpDlg->hwndMain) lpDlg->hwndMain = hwndDlg;
      else lpDlg->hwndChild = hwndDlg;
    }
    ::SetWindowLong(hwndDlg, GWL_USERDATA, PtrToLong(lpDlg));
  }
  else
  {
    lpDlg = (WizDlg *)LongToPtr(::GetWindowLong(hwndDlg, GWL_USERDATA));
  }

  if(!lpDlg) return false;
  else if(hwndDlg == lpDlg->hwndMain) return lpDlg->MainDlgProc(hwndDlg, uMsg, wParam, lParam);
  else if(hwndDlg == lpDlg->hwndChild) return lpDlg->DlgProc(hwndDlg, uMsg, wParam, lParam);
  else return false;
}


//-------------------------------------------------------------------------------------------------
INT_PTR WizDlg::MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    hwndMain = hwndDlg;
    ::SetClassLongPtr(hwndDlg, GCL_HICON, 
      PtrToLong(::LoadIcon(hInstance, MAKEINTRESOURCE(getID(GETID_ICO_MAIN)))));
    ::SetFocus(::GetDlgItem(hwndMain, this->getID(GETID_BTN_NEXT)));
    if(!this->onInit())
    {
      doError();
      ::DestroyWindow(hwndDlg);
    }
    return false;

  case WM_COMMAND:
    if(LOWORD(wParam) == this->getID(GETID_BTN_NEXT))
    {
      if(!this->onNext()) doError();
      return true;
    }
    else if(LOWORD(wParam) == this->getID(GETID_BTN_PREV))
    {
      if(!this->onPrev()) doError();
      return true;
    }
    else if(LOWORD(wParam) == this->getID(GETID_BTN_CANCEL))
    {
      ::SendMessage(hwndDlg, WM_CLOSE, 0, 0);
      return true;
    }
    break;

  case WM_CLOSE:
    if(!this->onClose()) doError();
    else ::DestroyWindow(hwndDlg);
    return true;

  case WM_DESTROY:
    ::PostQuitMessage(0);
    return true;
  }

  return false;
}


//-------------------------------------------------------------------------------------------------
UINT WizDlg::getStage()
{
  return nCurrentStage;
}

//-------------------------------------------------------------------------------------------------
BOOL WizDlg::setStage(UINT nNewStage)
{
  if(::IsWindow(hwndChild)) ::DestroyWindow(hwndChild);
  hwndChild = NULL;
  nCurrentStage = nNewStage;
  if(!::CreateDialogParam(hInstance, MAKEINTRESOURCE(getIDD(nNewStage)),
    hwndMain, StaticDlgProc, (LPARAM)this)) return false;
  if(!::IsWindow(hwndChild))
  {
    SetLastError(ERROR_INTERNAL_ERROR);
    return false;
  }
  return true;
}


//-------------------------------------------------------------------------------------------------
VOID WizDlg::enableNext(BOOL fEnable)
{
  ::EnableWindow(::GetDlgItem(hwndMain, this->getID(GETID_BTN_NEXT)), fEnable);
}

VOID WizDlg::enablePrev(BOOL fEnable)
{
  ::EnableWindow(::GetDlgItem(hwndMain, this->getID(GETID_BTN_PREV)), fEnable);
}

VOID WizDlg::enableCancel(BOOL fEnable)
{
  ::EnableWindow(::GetDlgItem(hwndMain, this->getID(GETID_BTN_CANCEL)), fEnable);
}
