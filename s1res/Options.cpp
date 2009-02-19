//=================================================================================================
// Options.cpp : class to manage options/settings
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
#include "Options.h"


//-------------------------------------------------------------------------------------------------
INT_PTR CALLBACK Options::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
  case WM_INITDIALOG:
    ::SendDlgItemMessage(hwndDlg, IDC_BTN_CUSTOM, BM_SETIMAGE, IMAGE_ICON, 
      (LPARAM)::LoadIcon(getInstance(), MAKEINTRESOURCE(IDI_OPEN)) );

    crefMonoForeground = attr.crefMonoForeground;
    crefMonoBackground = attr.crefMonoBackground;
    ::CopyMemory(crefCustColors, attr.crefCustColors, sizeof(crefCustColors));

    setEdtCustomCtrl(hwndDlg, attr.szEditor, attr.szEditorArgs);

    if(attr.nEditorIndex > 2) attr.nEditorIndex = 0;
    EnableDlgItem(hwndDlg, IDC_EDT_CUSTOM, attr.nEditorIndex == 2);
    EnableDlgItem(hwndDlg, IDC_BTN_CUSTOM, attr.nEditorIndex == 2);
    SetDlgRadioButton(hwndDlg, IDC_RAD_IMGEDT_DEFAULT, attr.nEditorIndex == 0);
    SetDlgRadioButton(hwndDlg, IDC_RAD_IMGEDT_MSPAINT, attr.nEditorIndex == 1);
    SetDlgRadioButton(hwndDlg, IDC_RAD_IMGEDT_CUSTOM, attr.nEditorIndex == 2);

    ::ShowWindow(hwndDlg, SW_SHOW);
    return false;

  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDC_BTN_CUSTOM:
      {
        CStdString strCaption(MAKEINTRESOURCE(IDS_BROWSE_CAPTION));
        CStdString strFilter(MAKEINTRESOURCE(IDS_BROWSE_FILTER));
        strFilter.Replace(_T('\n'), _T('\0'));
        
        //parse filename
        WCHAR wcFile[MAX_PATH];
        WCHAR wcArgs[MAX_PATH];
        getEdtCustomCtrl(hwndDlg, wcFile, wcArgs);
        for(UINT n=0; wcFile[n]; n++) if(wcFile[n] == _T('/')) wcFile[n] = _T('\\'); //fix openfilename bug

        //browse for file
        ::OPENFILENAME ofn;
        ZeroMemory(&ofn, sizeof(::OPENFILENAME));
        ofn.lStructSize = sizeof(::OPENFILENAME);
        ofn.hwndOwner = hwndDlg;
        ofn.hInstance = getInstance();
        ofn.lpstrFilter = strFilter.c_str();
        //ofn.lpstrCustomFilter = NULL;
        //ofn.nMaxCustFilter = 0;
        //ofn.nFilterIndex = 0;
        ofn.lpstrFile = wcFile;
        ofn.nMaxFile = MAX_PATH;
        //ofn.lpstrFileTitle = NULL;
        //ofn.nMaxFileTitle = 0;
        //ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = strCaption.c_str();
        ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_LONGNAMES|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
        if(GetOpenFileName(&ofn)) setEdtCustomCtrl(hwndDlg, wcFile, wcArgs);
      }
      return true;

    case IDC_BTN_CHOOSE_FG:
      chooseColor(hwndDlg, &crefMonoForeground);
      return true;

    case IDC_BTN_CHOOSE_BG:
      chooseColor(hwndDlg, &crefMonoBackground);
      return true;

    case IDC_EDT_CUSTOM:
      SetDlgRadioButton(hwndDlg, IDC_RAD_IMGEDT_DEFAULT, false);
      SetDlgRadioButton(hwndDlg, IDC_RAD_IMGEDT_MSPAINT, false);
      SetDlgRadioButton(hwndDlg, IDC_RAD_IMGEDT_CUSTOM, true);
      return true;

    case IDC_RAD_IMGEDT_DEFAULT:
    case IDC_RAD_IMGEDT_MSPAINT:
    case IDC_RAD_IMGEDT_CUSTOM:
      EnableDlgItem(hwndDlg, IDC_EDT_CUSTOM, LOWORD(wParam) == IDC_RAD_IMGEDT_CUSTOM);
      EnableDlgItem(hwndDlg, IDC_BTN_CUSTOM, LOWORD(wParam) == IDC_RAD_IMGEDT_CUSTOM);
      return true;

    case IDOK:
      if(GetDlgRadioButton(hwndDlg, IDC_RAD_IMGEDT_DEFAULT)) attr.nEditorIndex = 0;
      else if(GetDlgRadioButton(hwndDlg, IDC_RAD_IMGEDT_MSPAINT)) attr.nEditorIndex = 1;
      else if(GetDlgRadioButton(hwndDlg, IDC_RAD_IMGEDT_CUSTOM)) attr.nEditorIndex = 2;
      getEdtCustomCtrl(hwndDlg, attr.szEditor, attr.szEditorArgs);
      attr.crefMonoForeground = crefMonoForeground;
      attr.crefMonoBackground = crefMonoBackground;
      ::CopyMemory(attr.crefCustColors, crefCustColors, sizeof(attr.crefCustColors));
    case IDCANCEL:
      ::EndDialog(hwndDlg, LOWORD(wParam));
      return true;
    }
    break;

  case WM_CTLCOLORSTATIC:
    if(lParam == (LPARAM)::GetDlgItem(hwndDlg, IDC_TXT_SAMPLE)) //change color of the static text ctrl
    {
      HDC hdc = (HDC)wParam;
      SetTextColor(hdc, crefMonoForeground);
      SetBkColor(hdc, crefMonoBackground);
      return (INT_PTR)(::CreateSolidBrush(crefMonoBackground));
    }
    break;
  }
  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL Options::load()
{
  LONG lResult;
  HKEY hKey;
  DWORD dwType;
  DWORD cbData;

  //load default options
  ::ZeroMemory(&attr, sizeof(attr));
  attr.nSplitPos = 250;
  attr.crefMonoForeground = RGB(0, 0, 192);
  attr.crefMonoBackground = RGB(255, 255, 255);

  //load options
  lResult = ::RegOpenKeyEx(OPTIONS_REG_ROOT, OPTIONS_REG_KEY, 0, KEY_QUERY_VALUE, &hKey);
  if(lResult == ERROR_SUCCESS)
  {
    cbData = sizeof(attr);
    lResult = ::RegQueryValueEx(hKey, OPTIONS_REG_VALUE, NULL, &dwType, (LPBYTE)&attr, &cbData);
    if((lResult == ERROR_SUCCESS || lResult == ERROR_MORE_DATA) && dwType == REG_BINARY && attr.dwId == OPTIONS_ID)
    {
      ::RegCloseKey(hKey);
      return TRUE;  //valid data loaded, return success
    }
    ::RegCloseKey(hKey);
  }

  //no valid options found, return error
  ::SetLastError((lResult != ERROR_SUCCESS)? lResult : ERROR_READ_FAULT);
  return FALSE;


  /* LOAD-DATA-FROM-INI-FILE *
  CStdString strIniFile(lpcwAppPath);
  strIniFile.append(OPTIONS_INI_FILE);

  //load options
  if(
    ! ::GetPrivateProfileStruct(
        OPTIONS_INI_SECTION_OPTIONS,
        OPTIONS_INI_KEY_OPTIONS,
        &attr, sizeof(attr),
        strIniFile.c_str()     )
    || (attr.dwId != OPTIONS_ID)
    )
  {
    //no valid options found, load default values
    ::ZeroMemory(&attr, sizeof(attr));
    attr.nSplitPos = 250;
    return false;
  }

  return true;
  */
}


//-------------------------------------------------------------------------------------------------
BOOL Options::store()
{
  //store options
  attr.dwId = OPTIONS_ID; //set id

  HKEY hKey = NULL;
  LONG lResult = ::RegCreateKey(OPTIONS_REG_ROOT, OPTIONS_REG_KEY, &hKey);
  if(lResult == ERROR_SUCCESS)
  {
    lResult = ::RegSetValueEx(hKey, OPTIONS_REG_VALUE, NULL, REG_BINARY, (LPBYTE)&attr, sizeof(attr));
    ::RegCloseKey(hKey);
    if(lResult == ERROR_SUCCESS) return TRUE;
  }

  ::SetLastError(lResult);
  return FALSE;


  /* WRITE-DATA-TO-INI-FILE *
  CStdString strIniFile(lpcwAppPath);
  strIniFile.append(OPTIONS_INI_FILE);
  attr.dwId = OPTIONS_ID; //set id

  return ::WritePrivateProfileStruct(OPTIONS_INI_SECTION_OPTIONS, OPTIONS_INI_KEY_OPTIONS,
    &attr, sizeof(attr), strIniFile.c_str());
  */
}


//-------------------------------------------------------------------------------------------------
VOID Options::setEdtCustomCtrl(HWND hwndDlg, LPCWSTR lpFile, LPCWSTR lpArgs)
{
  CStdString strText;

  if((lpFile && lpFile[0]) || (lpArgs && lpArgs[0]))
  {
    strText = _T('\"');
    if(lpFile) strText += lpFile;
    strText += _T("\" \"");
    if(lpArgs && lpArgs[0]) strText += lpArgs;
    else strText += _T("%1");
    strText += _T('\"');
  }

  ::SetDlgItemText(hwndDlg, IDC_EDT_CUSTOM, strText);
}


//-------------------------------------------------------------------------------------------------
VOID Options::getEdtCustomCtrl(HWND hwndDlg, LPWSTR lpFile, LPWSTR lpArgs)
{
  if(lpFile) *lpFile = _T('\0');
  if(lpArgs) *lpArgs = _T('\0');

  WCHAR wcBuf[MAX_PATH*4];
  if(::GetDlgItemText(hwndDlg, IDC_EDT_CUSTOM, wcBuf, MAX_PATH*4))
  {
    std::list<CStdString> lstStr;
    parseStringList(wcBuf, &lstStr);
    if(lstStr.empty()) return;
    
    if(lpFile) wcscpy_s(lpFile, MAX_PATH, lstStr.front().c_str());
    lstStr.pop_front();
    
    if(lpArgs)
    {      
      CStdString strArgs;
      while(!lstStr.empty())
      {
        strArgs += lstStr.front();
        lstStr.pop_front();
        if(!lstStr.empty()) strArgs += _T(' ');
      }
      wcscpy_s(lpArgs, MAX_PATH, strArgs.c_str());
    }
  }
}


//-------------------------------------------------------------------------------------------------
VOID Options::parseStringList(LPCWSTR lpString, std::list<CStdString> *lpList)
{
  if(!lpList) return;
  lpList->clear();
  if(!lpString) return;

  while(*lpString)
  {
    while(*lpString == _T(' ')) lpString++;   //skip whitespaces
    if(!*lpString) return;

    LPCWSTR lpEndSeq = wcschr(lpString, _T(' '));
    if(*lpString == _T('"')) lpEndSeq = wcschr(++lpString, _T('"'));

    if(lpEndSeq)
    {
      *(LPWSTR)lpEndSeq = _T('\0');
      lpList->push_back( CStdString( lpString ) );
      lpString = &lpEndSeq[1];
    }
    else
    {
      lpList->push_back( CStdString( lpString ) );
      return;
    }
  }
}


//-------------------------------------------------------------------------------------------------
BOOL Options::chooseColor(HWND hwndDlg, LPCOLORREF lpColorRef)
{
  if(lpColorRef != NULL)
  {
    CHOOSECOLOR cc;
    ::ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwndDlg;
    cc.lpCustColors = crefCustColors;
    cc.Flags = CC_RGBINIT|CC_ANYCOLOR;
    cc.rgbResult = *lpColorRef;
    if(!ChooseColor(&cc)) return FALSE;
    *lpColorRef = cc.rgbResult;
  }

  ::RedrawWindow(::GetDlgItem(hwndDlg, IDC_TXT_SAMPLE), NULL, NULL, RDW_INVALIDATE|RDW_ERASENOW);
  return TRUE;
}




//-------------------------------------------------------------------------------------------------
// the one and only instance of the options class
//-------------------------------------------------------------------------------------------------
Options g_options;
