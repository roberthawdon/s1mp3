//=================================================================================================
// MainApp.cpp : class for the main application window
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
#include "MainApp.h"
#include "AboutDlg.h"
#include "FwPropDlg.h"
#include "Options.h"
#include "ResItemImage.h"
#include "WizDlgImport.h"
#include "WizDlgExport.h"



//-------------------------------------------------------------------------------------------------
#define WM_USER_UPDATE_VIEW   WM_USER+0x00


//-------------------------------------------------------------------------------------------------
// constructs a MainApp object
//-------------------------------------------------------------------------------------------------
MainApp::MainApp()
{
  hInstance = NULL;
  strAppName.LoadStringW(IDS_APP_NAME);
  hMenu = NULL;
  hImgList = NULL;
  hwndMain = hwndLeftPane = hwndRightPane = hwndSpacer = hwndStatusBar = hwndFocus = NULL;
  hIdleThread = NULL;
}


//-------------------------------------------------------------------------------------------------
// destructs
//-------------------------------------------------------------------------------------------------
MainApp::~MainApp()
{
  if(hImgList) ImageList_Destroy(hImgList);
}




//-------------------------------------------------------------------------------------------------
// clean up when your application terminates
//-------------------------------------------------------------------------------------------------
VOID MainApp::close()
{
  if(::IsWindow(hwndMain))
  {
    g_options.get()->wndpl.length = sizeof(WINDOWPLACEMENT);
    ::GetWindowPlacement(hwndMain, &g_options.get()->wndpl);
  }
  g_options.store();

  this->fIdleThreadExit = true;   //let thread terminate
  while(hIdleThread) Sleep(100);  //...

  if(::IsWindow(hwndMain)) ::DestroyWindow(hwndMain);               hwndMain = NULL;
  /*if(::IsWindow(hwndLeftPane)) ::DestroyWindow(hwndLeftPane);*/   hwndLeftPane = NULL;
  /*if(::IsWindow(hwndRightPane)) ::DestroyWindow(hwndRightPane);*/ hwndRightPane = NULL;
  /*if(::IsWindow(hwndSpacer)) ::DestroyWindow(hwndSpacer);*/       hwndSpacer = NULL;
  /*if(::IsWindow(hwndStatusBar)) ::DestroyWindow(hwndStatusBar);*/ hwndStatusBar = NULL;
}


//-------------------------------------------------------------------------------------------------
// Windows instance initialization
// (creating window objects and loads profile settings or calls FirstRun)
//-------------------------------------------------------------------------------------------------
BOOL MainApp::init(HINSTANCE hInstance, LPTSTR lpCmdLine, INT nCmdShow)
{
  g_options.load();

  //init vars
  this->hInstance = hInstance;
  nSizeDragState = SDS_COLD;

  // detect application filename and extract path
  strAppPath.clear();
  TCHAR szAppPath[4096];
  if(::GetModuleFileName(NULL, szAppPath, 4096))
  {
    CStdString strTemp( szAppPath );
    for(INT n; (n = strTemp.FindOneOf(_T("\\/:"))) >= 0; strTemp.Delete(0, n+1))
      strAppPath.append( strTemp.substr(0, n+1) );
  }

  //register class
  WNDCLASS wc;
  wc.style = CS_DBLCLKS|CS_BYTEALIGNCLIENT|CS_BYTEALIGNWINDOW;
  wc.lpfnWndProc = (WNDPROC)WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAIN));
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL; //(HBRUSH)(COLOR_APPWORKSPACE+1);
  wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN);
  wc.lpszClassName = MAINAPP_GUID;
  if(!::RegisterClass(&wc)) {doError(); return false;}

  // create main window
  hwndMain = ::CreateWindowEx(
    WS_EX_ACCEPTFILES, MAINAPP_GUID, strAppName,
    WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
    GetSystemMetrics(SM_CXFULLSCREEN)/4, GetSystemMetrics(SM_CYFULLSCREEN)/4,
    GetSystemMetrics(SM_CXFULLSCREEN)/2, GetSystemMetrics(SM_CYFULLSCREEN)/2,
    NULL, NULL, hInstance, NULL);
  if(!hwndMain) {doError(); return false;}
  ::SetWindowLong(hwndMain, GWL_USERDATA, PtrToLong(this));

  // retrieve menu
  hMenu = ::GetMenu(hwndMain);

  // create image list
  hImgList = ImageList_Create(RESITM_IMAGE_WIDTH, RESITM_IMAGE_HEIGHT, ILC_COLOR32|ILC_MASK, 64, 512);
  if(!hImgList) {doError(); return false;}

  // create left pane
  hwndLeftPane = ::CreateWindowEx(
    WS_EX_LEFTSCROLLBAR,
    WC_TREEVIEW, _T(""), WS_CHILD|WS_VISIBLE|TVS_HASLINES|TVS_HASBUTTONS|
    TVS_LINESATROOT|TVS_FULLROWSELECT|TVS_SHOWSELALWAYS, //|TVS_EDITLABELS,
    0, 0, 0, 0, hwndMain, NULL, hInstance, NULL);
  if(!hwndLeftPane) {doError(); return false;}

  // create right pane
  hwndRightPane = ::CreateWindowEx(
    0, WC_LISTVIEW, _T(""), WS_CHILD|WS_VISIBLE,
    0, 0, 0, 0, hwndMain, NULL, hInstance, NULL);
  if(!hwndRightPane) {doError(); return false;}
  ListView_SetImageList(hwndRightPane, hImgList, LVSIL_NORMAL);

  // create spacer
  hwndSpacer = ::CreateWindowEx(
    0, WC_STATIC, _T(""),
    WS_CHILD|WS_VISIBLE|WS_BORDER, 0, 0, 0, 0, hwndMain, NULL, hInstance, NULL);
  if(!hwndSpacer) {doError(); return false;}

  // create statusbar
  hwndStatusBar = ::CreateWindowEx(
    0, STATUSCLASSNAME, _T(""),
    WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, hwndMain, NULL, hInstance, NULL);
  if(!hwndStatusBar) {doError(); return false;}

  RECT r;
  nStatusBarHeight = GetClientRect(hwndStatusBar, &r)? r.bottom : 16;

  // show main window
  ::SetWindowPlacement(hwndMain, &g_options.get()->wndpl);
  ::ShowWindow(hwndMain, nCmdShow);
  ::SetFocus(hwndFocus = hwndLeftPane);

  if(!updateView()) doError();
  if(!updateCodePage()) doError();

  // initialize idle thread
  fIdleThreadExit = false;
  hIdleThread = ::CreateThread(NULL, 0, threadIdle, (LPVOID)this, 0, NULL);
  if(hIdleThread) ::SetThreadPriority(hIdleThread, THREAD_PRIORITY_IDLE);

  // open first file from commandline arguments, if any...
  INT nCmdArgs = 0;
  LPWSTR *lpCmdArgs = CommandLineToArgvW(lpCmdLine, &nCmdArgs);
  if(lpCmdArgs && nCmdArgs > 1 && lpCmdArgs[1][0] && !fileOpen(lpCmdArgs[1])) doError();

  return true;
}


//-------------------------------------------------------------------------------------------------
// runs the message loop
//-------------------------------------------------------------------------------------------------
UINT MainApp::run()
{
  //load accelerators from resource
  HACCEL hAccTable = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MAIN));

  //run message loop
  for(MSG lpMsg;;)
  {
    switch(::GetMessage(&lpMsg, NULL, 0, 0))
    {
      case -1:  //doError
        doError();
        return(0);

      case 0:   //WM_QUIT
        return((UINT)lpMsg.wParam);

      default:  //WM_XXX
        if( !::TranslateAccelerator(hwndMain, hAccTable, &lpMsg) )
        //if( !IsDialogMessage(lpMsg.hwnd, &lpMsg) )
        if(::IsWindow(lpMsg.hwnd))
        { 
          ::TranslateMessage(&lpMsg);
          ::DispatchMessage(&lpMsg);
        }
    }
  }
}




//-------------------------------------------------------------------------------------------------
// our static window message handler
//-------------------------------------------------------------------------------------------------
LRESULT MainApp::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  MainApp *lp = (MainApp*)LongToPtr(::GetWindowLong(hWnd, GWL_USERDATA));
  return( lp? lp->onMsg(hWnd, uMsg, wParam, lParam) : ::DefWindowProc(hWnd, uMsg, wParam, lParam) );
}


//-------------------------------------------------------------------------------------------------
// handles the windows messages
//-------------------------------------------------------------------------------------------------
LRESULT MainApp::onMsg(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_COMMAND:
      if(onCommand(LOWORD(wParam))) return(0);
      break;
    case WM_NOTIFY:
      switch(onNotify((LPNMHDR)lParam))
      {
      case true: return(0);
      case false: return(1);
      }
      break;

    case WM_SIZE:
      if(onSize(LOWORD(lParam), HIWORD(lParam))) return(0);
      break;

    case WM_SETCURSOR:    
      if(onSetCursor((HWND)wParam)) return(0);
      break;
    case WM_LBUTTONDOWN:
      if(onLButton(true)) return(0);
      break;
    case WM_LBUTTONUP:
      if(onLButton(false)) return(0);
      break;
    case WM_MOUSEMOVE:
      if(onMouseMove(LOWORD(lParam), HIWORD(lParam), (DWORD)wParam)) return(0);
      break;

    case WM_SETFOCUS:
      SetFocus(hwndFocus);  //set our focus manually
      return(0);

    case WM_DROPFILES:
      if(!onDropFiles((HDROP)wParam)) doError();
      return(0);

    case WM_CLOSE:    
      if(fileClose()) close();
      return(0);
    case WM_DESTROY:
      ::PostQuitMessage(NULL);
      return(0);

    case WM_USER_UPDATE_VIEW:
      if(!updateView()) {} //doError();
      return(0);
  }

  return( ::DefWindowProc(hWnd, uMsg, wParam, lParam) );
}


//-------------------------------------------------------------------------------------------------
// received command id
//-------------------------------------------------------------------------------------------------
BOOL MainApp::onCommand(WORD nCmdId)
{
  switch(nCmdId)
  {
    case ID_FILE_OPEN:
      if(!fileOpen()) doError();
      return true;
    case ID_FILE_CLOSE:
      fileClose();
      return true;
    case ID_FILE_SAVE:
      if(!fileSave(false)) doError();
      return true;
    case ID_FILE_SAVEAS:
      if(!fileSave(true)) doError();
      return true;
    case ID_FILE_IMPORT:
      if(fileClose())
      {
        setStatusText(IDS_STATUS_IMPORT);
        WizDlgImport imp;
        if(imp.run(hInstance, hwndMain))  //imp.run(...) already displays any occured errors
        {
          LPBYTE lpData = NULL;
          DWORD dwDataSize = 0;
          if(!imp.getFirmware(lpData, dwDataSize)) doError();
          else if(!fio.load(lpData, dwDataSize)) doError();
          else
          {
            CStdString strUnnamed(MAKEINTRESOURCE(IDS_UNNAMED));
            ::wcscpy_s<MAX_PATH>(g_options.get()->szFile, strUnnamed);
            fio.wasAltered(true);
            fio.getRootItem()->updateTreeView(hwndLeftPane);
            if(!updateView()) doError();
          }
          if(lpData) delete lpData;   //free data
        }
      }
      setStatusText(IDS_STATUS_READY);
      return true;
    case ID_FILE_EXPORT:
      if(fio.isOpen())
      {          
        setStatusText(IDS_STATUS_EXPORT);

        if(editor.refresh())  //refresh all resources
        {
          fio.wasAltered(true);
          if(!updateView()) doError();
        }

        if(!fio.update()) //update changes (needed to update checksum's)
        {
          doError();
        }
        else //export firmware
        {
          WizDlgExport exp;
          if(!exp.setFirmware(fio.getData(), fio.getDataSize())) doError();
          else exp.run(hInstance, hwndMain);  //exp.run(...) already displays any occured errors
        }
      }
      setStatusText(IDS_STATUS_READY);
      return true;
    case ID_FILE_EXIT:
      ::SendMessage(hwndMain, WM_CLOSE, 0, 0);
      return true;

    case ID_EDIT_OPEN:
      if(!editOpen()) doError();
      return true;
    case ID_EDIT_CLOSE:
      if(!editClose()) doError();
      return true;
    case ID_EDIT_CLOSEALL:
      if(!editCloseAll()) doError();
      return true;
    case ID_EDIT_REFRESH:
      if(editor.refresh()) fio.wasAltered(true);
      if(!updateView()) doError();
      return true;
    case ID_EDIT_SELECTALL:
      ListView_SetItemState(hwndRightPane, -1, LVIS_SELECTED, LVIS_SELECTED);
      return true;
    case ID_EDIT_OPTIONS:
      if(!g_options.runDialog(hInstance, hwndMain)) doError();
      return true;

    // set codepage...
    case ID_CODEPAGE_DEFAULT:         return updateCodePage(OPTIONS_CODEPAGE_DEFAULT);
    case ID_CODEPAGE_DEFAULT_URLENC:  return updateCodePage(OPTIONS_CODEPAGE_DEFAULT_URLENC);

    case ID_CODEPAGE_ISO_LATIN1:      return updateCodePage(28591);
    case ID_CODEPAGE_ISO_LATIN2:      return updateCodePage(28592);
    case ID_CODEPAGE_ISO_LATIN3:      return updateCodePage(28593);
    case ID_CODEPAGE_ISO_BALTIC:      return updateCodePage(28594);
    case ID_CODEPAGE_ISO_CYRILLIC:    return updateCodePage(28595);
    case ID_CODEPAGE_ISO_ARABIC:      return updateCodePage(28596);
    case ID_CODEPAGE_ISO_GREEK:       return updateCodePage(28597);
    case ID_CODEPAGE_ISO_HEBREW:      return updateCodePage(28598);
    case ID_CODEPAGE_ISO_TURKISH:     return updateCodePage(28599);
    case ID_CODEPAGE_ISO_ESTONIAN:    return updateCodePage(28603);
    case ID_CODEPAGE_ISO_LATIN9:      return updateCodePage(28605);

    case ID_CODEPAGE_WIN_EUROPEAN:    return updateCodePage(1250);
    case ID_CODEPAGE_WIN_CYRILLIC:    return updateCodePage(1251);
    case ID_CODEPAGE_WIN_LATIN:       return updateCodePage(1252);
    case ID_CODEPAGE_WIN_GREEK:       return updateCodePage(1253);
    case ID_CODEPAGE_WIN_TURKISH:     return updateCodePage(1254);
    case ID_CODEPAGE_WIN_HEBREW:      return updateCodePage(1255);
    case ID_CODEPAGE_WIN_ARABIC:      return updateCodePage(1256);
    case ID_CODEPAGE_WIN_BALTIC:      return updateCodePage(1257);
    case ID_CODEPAGE_WIN_VIETNAMESE:  return updateCodePage(1258);

    case ID_CODEPAGE_JAPANESE:        return updateCodePage(932);
    case ID_CODEPAGE_KOREAN:          return updateCodePage(949);
    case ID_CODEPAGE_SCHINESE:        return updateCodePage(936);
    case ID_CODEPAGE_TCHINESE:        return updateCodePage(950);
    //

    case ID_HELP_OPEN:
      {
        CStdString strHelpFile = strAppPath + MAINAPP_HELP_FILE;
        if(!::HtmlHelp(hwndMain, strHelpFile.c_str(), HH_DISPLAY_TOPIC, NULL))
          doError(MAKEINTRESOURCE(IDS_OPEN_HELP_FAILED));
      }
      return true;
    case ID_HELP_ABOUT:    
      if(!AboutDlg().runDialog(hInstance, hwndMain)) doError();
      return true;
    case ID_HELP_DONATE:
      OpenURL(URL_DONATE);
      return true;
    case ID_HELP_UPDATE:
      OpenURL(URL_UPDATE);
      return true;

    case ID_KEY_TAB:
    case ID_KEY_SH_TAB:
      hwndFocus = (::GetFocus() == hwndLeftPane)? hwndRightPane : hwndLeftPane;
      ::SetFocus(hwndFocus);
      return true;

    case ID_KEY_DEL:  //remove string if one is selected
      if(GetWindowLong(hwndRightPane, GWL_STYLE)&LVS_REPORT)  //do we have a string listview?
      {
        CStdString strEmpty;

        // get selected resource item and detect if it's a string
        ResItem *lpItem = getLeftPaneItem();
        if(!lpItem) break;
        if(lpItem->getType() != ResItem::TYPE_STRING
          && lpItem->getType() != ResItem::TYPE_MSTRING) break;
        ResItemString *lpItemString = (ResItemString*)lpItem;
        
        // get item index and clear string
        INT nItemIndex = ListView_GetNextItem(hwndRightPane, -1, LVNI_SELECTED);
        if(!lpItemString->set(strEmpty, nItemIndex)) break;

        // clear text of listview item
        LVITEM lvi;
        ::ZeroMemory(&lvi, sizeof(lvi));
        lvi.iItem = nItemIndex;
        lvi.mask = LVIF_TEXT;
        lvi.pszText = (LPWSTR)strEmpty.c_str();
        ListView_SetItem(hwndRightPane, &lvi);

        // show all other changes
        fio.wasAltered(true);
        updateView(false);
        updateViewStringsStatusText(lpItemString);

        return true;
      }      
      return false;
  }
  return false;
}


//-------------------------------------------------------------------------------------------------
// control notification (return -1 if we don't handle this notification)
//-------------------------------------------------------------------------------------------------
INT MainApp::onNotify(LPNMHDR lpNotify)
{
  if(lpNotify->hwndFrom == hwndLeftPane) switch(lpNotify->code)
  {
    case TVN_SELCHANGED:
      if(!updateView()) doError();
      return true;
  }
  else if(lpNotify->hwndFrom == hwndRightPane) switch(lpNotify->code)
  {
    case LVN_BEGINLABELEDIT:
    {
      LV_DISPINFO *lpDispInf = (LV_DISPINFO *)lpNotify;
      if(!(lpDispInf->item.mask & LVIF_PARAM)) return false;
      ResItem *lpItem = (ResItem*)lpDispInf->item.lParam;
      if(!lpItem) return true;  //no item given
      switch(lpItem->getType())
      {
      case ResItem::TYPE_STRING:
      case ResItem::TYPE_MSTRING:
        break;
      default:
        return true;  //no string resource
      }
      ResItemString *lpItemString = (ResItemString*)lpItem;
      HWND hwndEdit = ListView_GetEditControl(hwndRightPane);
      if(!hwndEdit) return false;

      //::SendMessage(hwndEdit, EM_SETLIMITTEXT, 0, NULL);

      CStdString strText;
      if(!lpItemString->get(strText, lpDispInf->item.iItem)) return false;
      if(!lpItemString->encode(strText)) return false;
      ::SetWindowText(hwndEdit, strText.c_str());

      strText.Format(IDS_STATUS_EDIT, lpDispInf->item.iItem);
      ::SetWindowText(hwndStatusBar, strText.c_str());
      return true;
    }

    case LVN_ENDLABELEDIT:
    {
      LV_DISPINFO *lpDispInf = (LV_DISPINFO *)lpNotify;
      ResItem *lpItem = (ResItem*)lpDispInf->item.lParam;
      if(!lpItem) return true;  //no item given
      switch(lpItem->getType())
      {
      case ResItem::TYPE_STRING:
      case ResItem::TYPE_MSTRING:
        break;
      default:
        return true;  //no string resource
      }
      ResItemString *lpItemString = (ResItemString*)lpItem;

      if(lpDispInf->item.pszText) for(;;)   //only if edit wasn't canceled
      {
        CStdString strText(lpDispInf->item.pszText);
        if(!lpItemString->decode(strText)) break;
        if(!lpItemString->set(strText, lpDispInf->item.iItem)) break;

        LVITEM lvi;
        ::ZeroMemory(&lvi, sizeof(lvi));
        lvi.iItem = lpDispInf->item.iItem;
        lvi.mask = LVIF_TEXT;      
        lpItemString->get(strText, lpDispInf->item.iItem);
        lvi.pszText = (LPWSTR)strText.c_str();
        ListView_SetItem(hwndRightPane, &lvi);

        fio.wasAltered(true);
        updateView(false);
        break;
      }

      // update status bar
      updateViewStringsStatusText(lpItemString);
      return true;
    }

    case LVN_GETINFOTIP:
    {
      LPNMLVGETINFOTIP lpInfTip = (LPNMLVGETINFOTIP)lpNotify;

      LVITEM lvi;
      ::ZeroMemory(&lvi, sizeof(lvi));
      lvi.iItem = lpInfTip->iItem;
      lvi.iSubItem = lpInfTip->iSubItem;
      lvi.mask = LVIF_PARAM;
      if(ListView_GetItem(hwndRightPane, &lvi) && lvi.lParam)
      {
        ResItem *lpItem = (ResItem*)lvi.lParam;
        if(lpItem->getType() == ResItem::TYPE_ICON || lpItem->getType() == ResItem::TYPE_LOGO)
        {
          ResItemImage *lpItemImage = (ResItemImage*)lpItem;

          CStdString strTip;
          strTip.Format(
            (lpItemImage->getImgFmt() == ResItemImage::IMGFMT_MONO)?
              IDS_TOOLTIP_IMAGE_MONO : IDS_TOOLTIP_IMAGE_OLED,
            lpItemImage->getName(),
            lpItemImage->getWidth(),
            lpItemImage->getHeight(),
            lpItemImage->getImageSize()
            );

          if(strTip.length() < (std::string::size_type)lpInfTip->cchTextMax)
          {
            #pragma warning(push)
            #pragma warning(disable:4996)
            ::wcscpy(lpInfTip->pszText, strTip.c_str());
            #pragma warning(pop)
          }
        }
      }

      return true;
    }

    case NM_RETURN:
    case NM_DBLCLK:
      if(!editOpen()) doError();
      return true;
  }

  return -1;
}


//-------------------------------------------------------------------------------------------------
// called when window is resized (also minimized)
//-------------------------------------------------------------------------------------------------
BOOL MainApp::onSize(LONG h, LONG v)
{
  register LONG h1 = (h * g_options.get()->nSplitPos)/1000;
  register LONG h2 = MAINAPP_SPACER_WIDTH;
  register LONG h3 = h - h1 - MAINAPP_SPACER_WIDTH;

  register LONG v2 = nStatusBarHeight;
  register LONG v1 = v - v2;

  HDWP hdwp = ::BeginDeferWindowPos(4);
  ::DeferWindowPos(hdwp, hwndLeftPane, HWND_TOP, 0, 0, h1, v1, SWP_NOACTIVATE|SWP_NOZORDER);
  ::DeferWindowPos(hdwp, hwndSpacer, HWND_TOP, h1, 0, h2, v1, SWP_NOACTIVATE|SWP_NOZORDER);
  ::DeferWindowPos(hdwp, hwndRightPane, HWND_TOP, h1+h2, 0, h3, v1, SWP_NOACTIVATE|SWP_NOZORDER);
  ::DeferWindowPos(hdwp, hwndStatusBar, HWND_TOP, 0, v1, h, v2, SWP_NOACTIVATE|SWP_NOZORDER);
  ::EndDeferWindowPos(hdwp);

  return true;
}


//-------------------------------------------------------------------------------------------------
// set cursor
//-------------------------------------------------------------------------------------------------
BOOL MainApp::onSetCursor(HWND hwndOwner)
{
  if(nSizeDragState == SDS_HOT) return false;
  nSizeDragState = SDS_COLD;
  if(hwndOwner != hwndMain) return false;

  RECT r;
  POINT p;
  if(!::GetCursorPos(&p)) return false;
  if(!::GetWindowRect(hwndSpacer, &r)) return false;

  if(p.x >= r.left && p.x <= r.right && p.y >= r.top && p.y <= r.bottom)
  {
    ::SetCursor( LoadCursor(NULL, IDC_SIZEWE) );
    nSizeDragState = SDS_WARM;
    return true;
  }
  else
  {
    return false;
  }
}


//-------------------------------------------------------------------------------------------------
// called on WM_LBUTTONXXX
//-------------------------------------------------------------------------------------------------
BOOL MainApp::onLButton(BOOL fDown)
{
  if(fDown)
  {
    if(nSizeDragState != SDS_WARM) return false;
    nSizeDragState = SDS_HOT;
    ::SetCapture(hwndMain);
  }
  else
  {
    if(nSizeDragState != SDS_HOT) return false;
    nSizeDragState = SDS_COLD;
    ::ReleaseCapture();
  }

  return true;
}


//-------------------------------------------------------------------------------------------------
// mouse move
//-------------------------------------------------------------------------------------------------
BOOL MainApp::onMouseMove(WORD x, WORD y, DWORD flags)
{
  if(nSizeDragState != SDS_HOT) return false;

  RECT r;
  POINT p;
  if(!::GetWindowRect(hwndMain, &r)) return false;
  if(!::GetCursorPos(&p)) return false;

  g_options.get()->nSplitPos = ((p.x - r.left + MAINAPP_SPACER_WIDTH/2)*1000) / (r.right-r.left);
  if(g_options.get()->nSplitPos < 100) g_options.get()->nSplitPos = 100;
  if(g_options.get()->nSplitPos > 900) g_options.get()->nSplitPos = 900;

  if(!::GetClientRect(hwndMain, &r)) return false;
  onSize(r.right, r.bottom);
  ::UpdateWindow(hwndMain);

  return true;
}


//-------------------------------------------------------------------------------------------------
// accept dropped files
//-------------------------------------------------------------------------------------------------
BOOL MainApp::onDropFiles(HDROP hDrop)
{
  BOOL fResult = true;
  BOOL fUpdate = false;
  CStdString strError(MAKEINTRESOURCE(IDS_DRAGNDROP_FAILED));

  UINT nFiles = ::DragQueryFile(hDrop, -1, NULL, 0);
  for(UINT n = 0; n < nFiles; n++)
  {
    WCHAR wcFile[MAX_PATH];
    if(::DragQueryFile(hDrop, n, wcFile, MAX_PATH))
    {
      BOOL fError = false;

      LPCWSTR lpName = wcFile;
      if(wcsrchr(wcFile, _T('\\')) >= lpName) lpName = wcsrchr(wcFile, _T('\\'))+1;
      if(wcsrchr(wcFile, _T('/')) >= lpName) lpName = wcsrchr(wcFile, _T('/'))+1;
      if(wcsrchr(wcFile, _T(':')) >= lpName) lpName = wcsrchr(wcFile, _T(':'))+1;

      LPWSTR lpExt = (LPWSTR)wcsrchr(lpName, _T('.'));
      if(lpExt && _wcsicmp(lpExt, _T(".bmp")) == 0)  //replace equal-named icons by bitmaps
      {
        if(!(GetWindowLong(hwndRightPane, GWL_STYLE)&LVS_REPORT))  //do we have an image listview?
        {
          LVFINDINFO fi;
          fi.flags = LVFI_STRING|LVFI_PARTIAL;
          fi.psz = lpName;
          lpExt[0] = _T('\0');
          INT nIndex = ListView_FindItem(hwndRightPane, -1, &fi);
          lpExt[0] = _T('.');
          if(nIndex >= 0)
          {
            LV_ITEM lvi;
            ::ZeroMemory(&lvi, sizeof(lvi));
            lvi.iItem = nIndex;
            lvi.mask = LVIF_PARAM;
            if(ListView_GetItem(hwndRightPane, &lvi) && lvi.lParam)
            {
              ResItem *lpItem = (ResItem*)lvi.lParam;
              if(lpItem->getType() == ResItem::TYPE_ICON || lpItem->getType() == ResItem::TYPE_LOGO)
              {
                ResItemImage *lpItemImage = (ResItemImage*)lpItem;
                if(!lpItemImage->importBitmap(wcFile)) fError = true;
                else
                {
                  fio.wasAltered(true);
                  fUpdate = true;
                }
              }
              else
              {
                ::SetLastError(ERROR_NOT_FOUND);
                fError = true;
              }
            }
            else
            {
              ::SetLastError(ERROR_NOT_FOUND);
              fError = true;
            }
          }
          else
          {
            ::SetLastError(ERROR_NOT_FOUND);
            fError = true;
          }
        }
        else
        {
          ::SetLastError(ERROR_UNSUPPORTED_TYPE);
          fError = true;
        }
      }
      //else if(nFiles == 1)  //try to open file only if just one file was dropped
      //{
      //  return fileOpen(wcFile);        
      //}
      else
      {
        ::SetLastError(ERROR_UNSUPPORTED_TYPE);
        fError = true;
      }

      if(fError)  //error occured above? then append it to the final error string
      {
        fResult = false;
        CStdString strErrorText;
        getErrorText(strErrorText, GetLastError());
        strErrorText.Replace(_T('\n'), _T(' '));
        strErrorText.Replace(_T('\r'), _T(' '));
        strError.append(lpName);
        strError += _T(": ") + strErrorText + _T('\n');
      }
    }
  }
  ::DragFinish(hDrop);

  if(!fResult) doError(strError);
  return fUpdate? updateView() : true;
}




//-------------------------------------------------------------------------------------------------
// load error string from system
//-------------------------------------------------------------------------------------------------
VOID MainApp::getErrorText(CStdString &strResult, DWORD dwError)
{
  LPTSTR lpszText = NULL;
  if(::FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
    NULL, dwError, 0, (LPTSTR)&lpszText, 0, NULL) && lpszText)
  {
    strResult.assign(lpszText);
    ::LocalFree(lpszText);
  }
  else
  {
    strResult.LoadStringW(IDS_ERROR_UNKNOWN);
  }
}

//-------------------------------------------------------------------------------------------------
// get current selection on the left pane
//-------------------------------------------------------------------------------------------------
ResItem *MainApp::getLeftPaneItem()
{
  TVITEM tvi;
  ::ZeroMemory(&tvi, sizeof(tvi));
  tvi.mask = TVIF_PARAM|TVIF_HANDLE;
  tvi.hItem = TreeView_GetSelection(hwndLeftPane);
  if(!TreeView_GetItem(hwndLeftPane, &tvi)) return NULL;
  if(!tvi.lParam) {SetLastError(ERROR_INTERNAL_ERROR); return NULL;}
  return (ResItem*)tvi.lParam;
}

//-------------------------------------------------------------------------------------------------
// shows an error dialog
//-------------------------------------------------------------------------------------------------
VOID MainApp::doError(LPCWSTR lpcwError)
{
  CStdString strCaption(MAKEINTRESOURCE(IDS_ERROR));
  CStdString strText(lpcwError);
  ::MessageBox(hwndMain, strText, strCaption, MB_OK | MB_ICONERROR);
}

//-------------------------------------------------------------------------------------------------
// load error string from sytem
//-------------------------------------------------------------------------------------------------
VOID MainApp::doError(DWORD dwError)
{
  if(dwError == ERROR_SUCCESS)
  {
    dwError = ::GetLastError();
    if(dwError == ERROR_SUCCESS) return;
  }

  CStdString strError;
  getErrorText(strError, dwError);
  doError(strError);
}




//-------------------------------------------------------------------------------------------------
// close file, returns false if user stopped closing
//-------------------------------------------------------------------------------------------------
BOOL MainApp::fileClose()
{
  if(!fio.isOpen()) return true;

  setStatusText(IDS_STATUS_FCLOSE);

  if(fio.wasAltered())
  {
    CStdString strCaption(MAKEINTRESOURCE(IDS_WARNING));
    CStdString strBody(MAKEINTRESOURCE(IDS_CLOSE_WARN));
    if(MessageBox(hwndMain, strBody.c_str(), strCaption.c_str(), 
      MB_ICONEXCLAMATION|MB_YESNOCANCEL) != IDYES)
    {
      setStatusText(IDS_STATUS_READY);
      ::SetLastError(ERROR_SUCCESS);
      return false;
    }
  }

  editor.close();
  fio.close();
  if(!updateView()) doError();

  setStatusText(IDS_STATUS_READY);
  return true;
}


//-------------------------------------------------------------------------------------------------
// save file
//-------------------------------------------------------------------------------------------------
BOOL MainApp::fileSave(BOOL fSaveAs)
{
  if(!fio.isOpen()) return true;

  setStatusText(IDS_STATUS_FSAVE);

  if(editor.refresh())  //refresh all resources
  {
    fio.wasAltered(true);
    if(!updateView()) doError();
  }

  if(fSaveAs || !g_options.get()->szFile[0])
  {
    // browse for filename
    CStdString strFilter(MAKEINTRESOURCE(IDS_FSAVE_FILTER));
    strFilter.Replace(_T('\n'), _T('\0'));

    // bugfix for vista, otherwise the open dialog could fail
    for(WCHAR *wc; wc = wcschr(g_options.get()->szFile, _T('/')); *wc = _T('\\'));

    // now we could open
    ::OPENFILENAME ofn;
    ::ZeroMemory(&ofn, sizeof(::OPENFILENAME));
    ofn.lStructSize = sizeof(::OPENFILENAME);
    ofn.hwndOwner = hwndMain;
    ofn.hInstance = hInstance;
    ofn.lpstrFilter = strFilter.c_str();
    //ofn.lpstrCustomFilter = NULL;
    //ofn.nMaxCustFilter = 0;
    //ofn.nFilterIndex = 0;
    ofn.lpstrFile = g_options.get()->szFile;
    ofn.nMaxFile = MAX_PATH;
    //ofn.lpstrFileTitle = NULL;
    //ofn.nMaxFileTitle = 0;
    //ofn.lpstrInitialDir = NULL;
    //ofn.lpstrTitle = NULL;
    ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_LONGNAMES|OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT;
    if(!::GetSaveFileName(&ofn))
    {
      if(::CommDlgExtendedError() != ERROR_SUCCESS) doError(ERROR_INTERNAL_ERROR);
      setStatusText(IDS_STATUS_READY);
      ::SetLastError(ERROR_SUCCESS);
      return false;
    }

    if(!fio.store(g_options.get()->szFile)) doError();
  }

  setStatusText(IDS_STATUS_READY);
  fio.wasAltered(false);
  updateView(false);
  return true;
}


//-------------------------------------------------------------------------------------------------
// open file
//-------------------------------------------------------------------------------------------------
BOOL MainApp::fileOpen(LPWSTR lpwFile)
{
  if(!fileClose()) return false;

  setStatusText(IDS_STATUS_FOPEN);

  if(!lpwFile)
  {
    // browse for filename
    CStdString strFilter(MAKEINTRESOURCE(IDS_FOPEN_FILTER));
    strFilter.Replace(_T('\n'), _T('\0'));

    // bugfix for vista, otherwise the open dialog could fail
    for(WCHAR *wc; wc = wcschr(g_options.get()->szFile, _T('/')); *wc = _T('\\'));

    // now we could open
    ::OPENFILENAME ofn;
    ::ZeroMemory(&ofn, sizeof(::OPENFILENAME));
    ofn.lStructSize = sizeof(::OPENFILENAME);
    ofn.hwndOwner = hwndMain;
    ofn.hInstance = hInstance;
    ofn.lpstrFilter = strFilter.c_str();
    //ofn.lpstrCustomFilter = NULL;
    //ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = g_options.get()->nFilterIndex;
    ofn.lpstrFile = g_options.get()->szFile;
    ofn.nMaxFile = MAX_PATH;
    //ofn.lpstrFileTitle = NULL;
    //ofn.nMaxFileTitle = 0;
    //ofn.lpstrInitialDir = NULL;
    //ofn.lpstrTitle = NULL;
    ofn.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_LONGNAMES|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    if(!::GetOpenFileName(&ofn))
    {
      if(::CommDlgExtendedError() != ERROR_SUCCESS) doError(ERROR_INTERNAL_ERROR);
      setStatusText(IDS_STATUS_READY);
      ::SetLastError(ERROR_SUCCESS);
      return false;
    }

    g_options.get()->nFilterIndex = ofn.nFilterIndex;
  }
  else
  {
    ::wcscpy_s<MAX_PATH>(g_options.get()->szFile, lpwFile);
  }

  // read new file and update treeview
  if(!fio.load(g_options.get()->szFile))
  {
    setStatusText(IDS_STATUS_READY);
    fio.close();
    return false;
  }
  fio.getRootItem()->updateTreeView(hwndLeftPane);

  return updateView();
}



//-------------------------------------------------------------------------------------------------
// update editor view (eg. activate/disable menu items and controls)
//-------------------------------------------------------------------------------------------------
// extract image to temp folder and open it with external editor
// update image from temp file on each refresh or filesave
// remove temp file only when firmware gets closed
//-------------------------------------------------------------------------------------------------
BOOL MainApp::editOpen()
{
  if(!fio.isOpen()) return true;
  if(!ListView_GetSelectedCount(hwndRightPane)) return true;

  ::SetFocus(hwndRightPane);  //focus the listview

  if(GetWindowLong(hwndRightPane, GWL_STYLE)&LVS_REPORT)  //do we have a string listview?
  {
    ListView_EditLabel(hwndRightPane, 
      ListView_GetNextItem(hwndRightPane, -1, LVNI_SELECTED));
    return true;
  }
  else                                                    //or an image listview?
  {
    CStdString strStatus;
    
    LVITEM lvi;
    BOOL fResult = true;
    CStdString strError(MAKEINTRESOURCE(IDS_OPEN_ITEM_FAILED));
    for(int nSel = -1; (nSel = ListView_GetNextItem(hwndRightPane, nSel, LVNI_SELECTED)) >= 0;)
    {
      ::ZeroMemory(&lvi, sizeof(lvi));
      lvi.iItem = nSel;
      lvi.mask = LVIF_PARAM;
      if(ListView_GetItem(hwndRightPane, &lvi) && lvi.lParam)
      {
        ResItem *lpItem = (ResItem*)lvi.lParam;
        switch(lpItem->getType())
        {
        case ResItem::TYPE_ICON:
        case ResItem::TYPE_LOGO:
        case ResItem::TYPE_FILE_FONT8:
        case ResItem::TYPE_FILE_FONT16:
          {
            ResItemImage *lpItemImage = (ResItemImage*)lpItem;

            strStatus.Format(IDS_STATUS_OPEN, lpItem->getName());
            ::SetWindowText(hwndStatusBar, strStatus.c_str());

            if(editor.open(lpItemImage))  //success? now we have something to close
            {
              enableMenuItem(ID_EDIT_CLOSE, true);
              enableMenuItem(ID_EDIT_CLOSEALL, true);
            }
            else  //error?
            {
              fResult = false;
              CStdString strErrorText;
              getErrorText(strErrorText, GetLastError());
              strErrorText.Replace(_T('\n'), _T(' '));
              strErrorText.Replace(_T('\r'), _T(' '));
              strError.append(lpItem->getName());
              strError += _T(": ") + strErrorText + _T('\n');
            }
          }
          break;

        case ResItem::TYPE_FILE_FW:
          setStatusText(IDS_STATUS_PROPSHEET);          
          if(lpItem->getDataSize() >= sizeof(FW_HEAD)
            && FwPropDlg((LP_FW_HEAD)lpItem->getData()).runDialog(hInstance, hwndMain))
          {
            this->fio.wasAltered(true);
            updateView(false);
          }
          break;

        case ResItem::TYPE_FILE_BREC:
          //setStatusText(IDS_STATUS_BREC);
//TODO: show brec-logo-finder tool
          break;
        }
      }
    } //for

    if(!fResult) doError(strError); //show error?

    setStatusText(IDS_STATUS_READY);
  }

  return true;
}

//-------------------------------------------------------------------------------------------------
BOOL MainApp::editClose()
{
  if(!fio.isOpen()) return true;
  if(!ListView_GetSelectedCount(hwndRightPane)) return true;

  ::SetFocus(hwndRightPane);  //focus the listview

  if(!(GetWindowLong(hwndRightPane, GWL_STYLE)&LVS_REPORT))  //do we have an image listview?
  {
    CStdString strStatus;
    
    LVITEM lvi;
    BOOL fResult = true;
    CStdString strError(MAKEINTRESOURCE(IDS_CLOSE_ITEM_FAILED));
    for(int nSel = -1; (nSel = ListView_GetNextItem(hwndRightPane, nSel, LVNI_SELECTED)) >= 0;)
    {
      ::ZeroMemory(&lvi, sizeof(lvi));
      lvi.iItem = nSel;
      lvi.mask = LVIF_PARAM;
      if(ListView_GetItem(hwndRightPane, &lvi) && lvi.lParam)
      {
        ResItem *lpItem = (ResItem*)lvi.lParam;
        switch(lpItem->getType())
        {
        case ResItem::TYPE_ICON:
        case ResItem::TYPE_LOGO:
        case ResItem::TYPE_FILE_FONT8:
        case ResItem::TYPE_FILE_FONT16:
          {
            ResItemImage *lpItemImage = (ResItemImage*)lpItem;

            strStatus.Format(IDS_STATUS_CLOSE, lpItem->getName());
            ::SetWindowText(hwndStatusBar, strStatus.c_str());

            editor.refresh(lpItemImage);
            if(!editor.close(lpItemImage))  //error?
            {
              fResult = false;
              CStdString strErrorText;
              getErrorText(strErrorText, GetLastError());
              strErrorText.Replace(_T('\n'), _T(' '));
              strErrorText.Replace(_T('\r'), _T(' '));
              strError.append(lpItem->getName());
              strError += _T(": ") + strErrorText + _T('\n');
            }
          }
          break;
        }
      }
    } //for

    if(!fResult) doError(strError); //show error?

    if(!editor.getNoOfOpenItems())  //no more open items
    {
      enableMenuItem(ID_EDIT_CLOSE, false);
      enableMenuItem(ID_EDIT_CLOSEALL, false);
    }
    
    setStatusText(IDS_STATUS_READY);
  }

  return true;
}

//-------------------------------------------------------------------------------------------------
BOOL MainApp::editCloseAll()
{
  setStatusText(IDS_STATUS_CLOSE_ALL);

  editor.refresh();
  if(!editor.close()) doError();
  else 
  {
    enableMenuItem(ID_EDIT_CLOSE, false);
    enableMenuItem(ID_EDIT_CLOSEALL, false);
  }

  setStatusText(IDS_STATUS_READY);
  return true;
}



//-------------------------------------------------------------------------------------------------
// update editor view (eg. activate/disable menu items and controls)
//-------------------------------------------------------------------------------------------------
BOOL MainApp::updateView(BOOL fEntireRedraw)
{
  if(fio.isOpen())
  {
    // enable/disable menu items
    enableMenuItem(ID_FILE_CLOSE, true);
    enableMenuItem(ID_FILE_SAVE, true);
    enableMenuItem(ID_FILE_SAVEAS, true);
    enableMenuItem(ID_FILE_EXPORT, true);

    enableMenuItem(ID_EDIT_OPEN, false);
    enableMenuItem(ID_EDIT_CLOSE, false);
    //enableMenuItem(ID_EDIT_CLOSEALL, true);
    enableMenuItem(ID_EDIT_REFRESH, true);
    enableMenuItem(ID_EDIT_SELECTALL, false);

    // set new window title
    CStdString strTitle = strAppName + _T(" - ");
    strTitle.append(g_options.get()->szFile);
    if(fio.wasAltered()) strTitle += _T('*');
    ::SetWindowText(hwndMain, strTitle.c_str());

    // redraw/update all content?
    if(fEntireRedraw)
    {
      // remember current listview selection
      INT nListViewSel = ListView_GetSelectionMark(hwndRightPane);

      // clear right view
      ListView_DeleteAllItems(hwndRightPane);
      while(ListView_DeleteColumn(hwndRightPane, 0));
      ImageList_RemoveAll(hImgList);

      // get current selection on the left pane
      ResItem *lpItem = getLeftPaneItem();
      if(!lpItem) return true;

      // update right view content
      switch(lpItem->getType())
      {
        case ResItem::TYPE_STRING:
        case ResItem::TYPE_MSTRING:
          if(!updateViewStrings(lpItem)) return false;
          break;

        case ResItem::TYPE_FILE_RES:
          if(!updateViewImages(lpItem)) return false;
          break;

        case ResItem::TYPE_FILE_FW:
          if(!updateViewFirmware(lpItem)) return false;
          break;

        case ResItem::TYPE_FILE_AFI:
          if(!updateViewAFI(lpItem)) return false;
          break;

        default:
          setStatusText(IDS_STATUS_READY);
      }

      // set listview pos
      INT nItemCount = ListView_GetItemCount(hwndRightPane);
      if(nListViewSel < 0) nListViewSel = 0;
      if(nListViewSel >= nItemCount) nListViewSel = nItemCount-1;
      if(nListViewSel >= 0)
      {
        ListView_SetItemState(hwndRightPane, nListViewSel, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
        ListView_EnsureVisible(hwndRightPane, nListViewSel, false);
      }

    }
  }
  else //if no file is opened
  {
    // enable/disable menu items
    enableMenuItem(ID_FILE_CLOSE, false);
    enableMenuItem(ID_FILE_SAVE, false);
    enableMenuItem(ID_FILE_SAVEAS, false);
    enableMenuItem(ID_FILE_EXPORT, false);

    enableMenuItem(ID_EDIT_OPEN, false);
    enableMenuItem(ID_EDIT_CLOSE, false);
    enableMenuItem(ID_EDIT_CLOSEALL, false);
    enableMenuItem(ID_EDIT_REFRESH, false);
    enableMenuItem(ID_EDIT_SELECTALL, false);

    // set default window title
    ::SetWindowText(hwndMain, strAppName.c_str());

    // clear all views
    ListView_DeleteAllItems(hwndRightPane);
    TreeView_DeleteAllItems(hwndLeftPane);
    ImageList_RemoveAll(hImgList);
    ::SetWindowLong(hwndRightPane, GWL_STYLE, WS_CHILD|WS_VISIBLE);
 
    // update statusbar
    setStatusText(IDS_STATUS_READY);
  }

  return true;
}


//-------------------------------------------------------------------------------------------------
BOOL MainApp::updateViewImages(ResItem *lpItem)
{
  // change listview styles
  ::SetWindowLong(hwndRightPane, 
    GWL_STYLE, WS_CHILD|WS_VISIBLE|LVS_ICON|LVS_SHOWSELALWAYS);
  ListView_SetExtendedListViewStyle(hwndRightPane, LVS_EX_INFOTIP);

  // init imagelist
  ImageList_RemoveAll(hImgList);
  HBITMAP hBmp = ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_NO_IMG));
  if(!hBmp) return false;
  INT nNoImage = ImageList_Add(hImgList, hBmp, NULL);
  ::DeleteObject(hBmp);

  // update listview
  LVITEM lvi;
  ::ZeroMemory(&lvi, sizeof(lvi));
  lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
  UINT uImages = 0, uStrings = 0, uErrors = 0, uUnknown = 0;
  FOREACH(*lpItem->getList(), std::list<ResItem*>, i) if(*i) switch((*i)->getType())
  {
    case ResItem::TYPE_ICON:  //add only image resources
    case ResItem::TYPE_LOGO:
    {
      uImages++;
      lvi.pszText = (LPWSTR)(*i)->getName();
      lvi.lParam = (LPARAM)*i;
      ResItemImage *lpItemImage = (ResItemImage*)(*i);
      if((!lpItemImage->getBitmap()) || (lvi.iImage = ImageList_Add(hImgList, lpItemImage->getBitmap(), NULL)) < 0)
        lvi.iImage = nNoImage;
      if(ListView_InsertItem(hwndRightPane, &lvi) >= 0) lvi.iItem++;
      break;
    }

    case ResItem::TYPE_STRING:
    case ResItem::TYPE_MSTRING:
      uStrings++;
      break;

    case ResItem::TYPE_INVALID_DATA:
      uErrors++;
      break;

    default:
      uUnknown++;
  }

  // update status bar
  CStdString strStatus;
  strStatus.Format(IDS_STATUS_ITEMS,
    uImages,  (uImages  != 1)? _T("s") : _T(""),
    uStrings, (uStrings != 1)? _T("s") : _T(""),
    uUnknown, (uUnknown != 1)? _T("s") : _T(""),
    uErrors,  (uErrors  != 1)? _T("s") : _T("") );
  ::SetWindowText(hwndStatusBar, strStatus.c_str());

  // update menu
  enableMenuItem(ID_EDIT_OPEN, (lvi.iItem > 0));
  enableMenuItem(ID_EDIT_CLOSE, (lvi.iItem > 0) && (editor.getNoOfOpenItems() > 0));
  enableMenuItem(ID_EDIT_SELECTALL, (lvi.iItem > 0));

  return true;
}


//-------------------------------------------------------------------------------------------------
BOOL MainApp::updateViewStrings(ResItem *lpItem)
{
  // change listview styles
  ::SetWindowLong(hwndRightPane, GWL_STYLE, 
    WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SINGLESEL|LVS_NOCOLUMNHEADER|LVS_EDITLABELS|LVS_SHOWSELALWAYS);
  ListView_SetExtendedListViewStyle(hwndRightPane, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

  // setup columns
  LVCOLUMN lvc;
  ::ZeroMemory(&lvc, sizeof(lvc));
  lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_SUBITEM;
  ListView_InsertColumn(hwndRightPane, 0, &lvc);

  // validate param
  if(lpItem->getType() != ResItem::TYPE_STRING && lpItem->getType() != ResItem::TYPE_MSTRING)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return false;
  }

  // update listview
  ResItemString *lpItemString = (ResItemString*)lpItem;
  
  LVITEM lvi;
  ::ZeroMemory(&lvi, sizeof(lvi));
  lvi.mask = LVIF_TEXT|LVIF_PARAM;
  lvi.lParam = (LPARAM)lpItem;
  for(CStdString strText; lpItemString->get(strText, lvi.iItem); lvi.iItem++)
  {
    lvi.pszText = (LPWSTR)strText.c_str();
    if(ListView_InsertItem(hwndRightPane, &lvi) < 0) return false;      
  }

  // update status bar
  updateViewStringsStatusText(lpItemString);

  // update menu
  //enableMenuItem(ID_EDIT_OPEN, (lvi.iItem > 0));

  // finally adjust each column
  ListView_SetColumnWidth(hwndRightPane, 0, LVSCW_AUTOSIZE_USEHEADER);

  return true;
}


//-------------------------------------------------------------------------------------------------
BOOL MainApp::updateViewStringsStatusText(ResItemString *lpItemString)
{
  CStdString strStatus;
  DWORD dwOccupied = lpItemString->getSizeOfStrings();
  LONG nLeft = lpItemString->getDataSize() - dwOccupied;
  dwOccupied -= lpItemString->getNoOfStrings();
  strStatus.Format(IDS_STATUS_STRINGS,
    lpItemString->getNoOfStrings(), (lpItemString->getNoOfStrings() != 1)? _T("s") : _T(""),
    dwOccupied, (dwOccupied != 1)? _T("s") : _T(""),
    nLeft, (abs(nLeft) != 1)? _T("s") : _T(""));
  ::SetWindowText(hwndStatusBar, strStatus.c_str());

  return true;
}


//-------------------------------------------------------------------------------------------------
BOOL MainApp::updateViewFirmware(ResItem *lpItem)
{
  // change listview styles
  ::SetWindowLong(hwndRightPane, 
    GWL_STYLE, WS_CHILD|WS_VISIBLE|LVS_ICON|LVS_SHOWSELALWAYS);
  ListView_SetExtendedListViewStyle(hwndRightPane, LVS_EX_INFOTIP);

  // init imagelist
  ImageList_RemoveAll(hImgList);
  HBITMAP hBmp[] = {
    ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_PROP_SHEET)),
    ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_PROP_SHEET_MASK)),
    ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FONT_SHEET)),
    ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FONT_SHEET_MASK))
  };
  INT nImageSheet = ImageList_Add(hImgList, hBmp[0], hBmp[1]);
  INT nImageFont = ImageList_Add(hImgList, hBmp[2], hBmp[3]);
  for(int n=0; n < sizeof(hBmp)/sizeof(HBITMAP); n++) if(hBmp[n]) ::DeleteObject(hBmp[n]);

  // update listview
  LVITEM lvi;
  ::ZeroMemory(&lvi, sizeof(lvi));
  lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
  CStdString strFirmwareAttr(MAKEINTRESOURCE(IDS_PROPERTY_SHEET));
  lvi.pszText = (LPWSTR)strFirmwareAttr.c_str();
  lvi.lParam = (LPARAM)lpItem;
  lvi.iImage = nImageSheet;
  if(ListView_InsertItem(hwndRightPane, &lvi) >= 0) lvi.iItem++;  //insert fw-attr item always

  UINT uRes = 0, uFonts = 0, uErrors = 0, uUnknown = 0;
  FOREACH(*lpItem->getList(), std::list<ResItem*>, i) if(*i) switch((*i)->getType())
  {
    case ResItem::TYPE_FILE_FONT8:    //add only font resources
    case ResItem::TYPE_FILE_FONT16:
    {
      uFonts++;
      lvi.pszText = (LPWSTR)(*i)->getName();
      lvi.lParam = (LPARAM)*i;
      lvi.iImage = nImageFont;
      if(ListView_InsertItem(hwndRightPane, &lvi) >= 0) lvi.iItem++;
      break;
    }

    case ResItem::TYPE_FILE_RES:
      uRes++;
      break;

    case ResItem::TYPE_INVALID_DATA:
      uErrors++;
      break;

    default:
      uUnknown++;
  }

  // update status bar
  CStdString strStatus;
  strStatus.Format(IDS_STATUS_FIRMWARE,
    uRes,     (uRes  != 1)? _T("s") : _T(""),
    uFonts,   (uFonts != 1)? _T("s") : _T(""),
    uUnknown, (uUnknown != 1)? _T("s") : _T(""),
    uErrors,  (uErrors  != 1)? _T("s") : _T("") );
  ::SetWindowText(hwndStatusBar, strStatus.c_str());

  // update menu
  enableMenuItem(ID_EDIT_OPEN, (lvi.iItem > 0));
  enableMenuItem(ID_EDIT_CLOSE, (lvi.iItem > 0) && (editor.getNoOfOpenItems() > 0));
  enableMenuItem(ID_EDIT_SELECTALL, (lvi.iItem > 0));

  return true;
}


//-------------------------------------------------------------------------------------------------
BOOL MainApp::updateViewAFI(ResItem *lpItem)
{
  // change listview styles
  ::SetWindowLong(hwndRightPane, 
    GWL_STYLE, WS_CHILD|WS_VISIBLE|LVS_ICON|LVS_SHOWSELALWAYS);
  ListView_SetExtendedListViewStyle(hwndRightPane, LVS_EX_INFOTIP);

  // init imagelist
  ImageList_RemoveAll(hImgList);
  HBITMAP hBmp[] = {
    ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_IMAGE_SHEET)),
    ::LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_IMAGE_SHEET_MASK))
  };
  INT nImageSheet = ImageList_Add(hImgList, hBmp[0], hBmp[1]);
  for(int n=0; n < sizeof(hBmp)/sizeof(HBITMAP); n++) if(hBmp[n]) ::DeleteObject(hBmp[n]);

  // update listview
  LVITEM lvi;
  ::ZeroMemory(&lvi, sizeof(lvi));
  lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
  lvi.iImage = nImageSheet;

  UINT uBootRecords = 0, uFwFiles = 0, uUnknown = 0, uErrors = 0;
  FOREACH(*lpItem->getList(), std::list<ResItem*>, i) if(*i) switch((*i)->getType())
  {
    //case ResItem::TYPE_FILE_BREC: //add an brec-image-tool for every brec entry
    //  uBootRecords++;
    //  lvi.pszText = (LPWSTR)(*i)->getName();
    //  lvi.lParam = (LPARAM)*i;
    //  if(ListView_InsertItem(hwndRightPane, &lvi) >= 0) lvi.iItem++;
    //  break;

    case ResItem::TYPE_FILE_FW:
      uFwFiles++;
      break;

    case ResItem::TYPE_INVALID_DATA:
      uErrors++;
      break;

    default:
      uUnknown++;
  }

  // update status bar
  CStdString strStatus;
  strStatus.Format(IDS_STATUS_AFI,
    uBootRecords, (uBootRecords != 1)? _T("s") : _T(""),
    uFwFiles,     (uFwFiles     != 1)? _T("s") : _T(""),
    uUnknown,     (uUnknown     != 1)? _T("s") : _T(""),
    uErrors,      (uErrors      != 1)? _T("s") : _T("") );
  ::SetWindowText(hwndStatusBar, strStatus.c_str());

  // update menu
  enableMenuItem(ID_EDIT_OPEN, (lvi.iItem > 0));
  enableMenuItem(ID_EDIT_CLOSE, (lvi.iItem > 0) && (editor.getNoOfOpenItems() > 0));
  enableMenuItem(ID_EDIT_SELECTALL, (lvi.iItem > 0));

  return true;
}




//-------------------------------------------------------------------------------------------------
BOOL MainApp::updateCodePage(LONG nCodePage)
{
  //if we are inside a string view, then stop any editing by clearing the whole right view now... 
  if(GetWindowLong(hwndRightPane, GWL_STYLE)&LVS_REPORT) ListView_DeleteAllItems(hwndRightPane);

  //set new codepage value inside options
  if(nCodePage >= 0) g_options.get()->uCodePage = (UINT)nCodePage;
    else nCodePage = g_options.get()->uCodePage;

  //check newly selected menu-item and uncheck all others
  checkMenuItem(ID_CODEPAGE_DEFAULT, nCodePage == OPTIONS_CODEPAGE_DEFAULT);
  checkMenuItem(ID_CODEPAGE_DEFAULT_URLENC, nCodePage == OPTIONS_CODEPAGE_DEFAULT_URLENC);

  checkMenuItem(ID_CODEPAGE_ISO_LATIN1,      nCodePage == 28591);
  checkMenuItem(ID_CODEPAGE_ISO_LATIN2,      nCodePage == 28592);
  checkMenuItem(ID_CODEPAGE_ISO_LATIN3,      nCodePage == 28593);
  checkMenuItem(ID_CODEPAGE_ISO_BALTIC,      nCodePage == 28594);
  checkMenuItem(ID_CODEPAGE_ISO_CYRILLIC,    nCodePage == 28595);
  checkMenuItem(ID_CODEPAGE_ISO_ARABIC,      nCodePage == 28596);
  checkMenuItem(ID_CODEPAGE_ISO_GREEK,       nCodePage == 28597);
  checkMenuItem(ID_CODEPAGE_ISO_HEBREW,      nCodePage == 28598);
  checkMenuItem(ID_CODEPAGE_ISO_TURKISH,     nCodePage == 28599);
  checkMenuItem(ID_CODEPAGE_ISO_ESTONIAN,    nCodePage == 28603);
  checkMenuItem(ID_CODEPAGE_ISO_LATIN9,      nCodePage == 28605);

  checkMenuItem(ID_CODEPAGE_WIN_EUROPEAN,    nCodePage == 1250);
  checkMenuItem(ID_CODEPAGE_WIN_CYRILLIC,    nCodePage == 1251);
  checkMenuItem(ID_CODEPAGE_WIN_LATIN,       nCodePage == 1252);
  checkMenuItem(ID_CODEPAGE_WIN_GREEK,       nCodePage == 1253);
  checkMenuItem(ID_CODEPAGE_WIN_TURKISH,     nCodePage == 1254);
  checkMenuItem(ID_CODEPAGE_WIN_HEBREW,      nCodePage == 1255);
  checkMenuItem(ID_CODEPAGE_WIN_ARABIC,      nCodePage == 1256);
  checkMenuItem(ID_CODEPAGE_WIN_BALTIC,      nCodePage == 1257);
  checkMenuItem(ID_CODEPAGE_WIN_VIETNAMESE,  nCodePage == 1258);

  checkMenuItem(ID_CODEPAGE_JAPANESE, nCodePage == 932);
  checkMenuItem(ID_CODEPAGE_KOREAN,   nCodePage == 949);
  checkMenuItem(ID_CODEPAGE_SCHINESE, nCodePage == 936);
  checkMenuItem(ID_CODEPAGE_TCHINESE, nCodePage == 950);

  //if we are inside a string view, then update the view completly
  if(GetWindowLong(hwndRightPane, GWL_STYLE)&LVS_REPORT) updateView();

  return true;
}




//-------------------------------------------------------------------------------------------------
DWORD MainApp::threadIdle()
{
  while(!this->fIdleThreadExit)
  {
    #ifdef _DEBUG
      OutputDebugString(_T("s1res::MainApp:threadIdle()"));
    #endif

    if(this->fio.isOpen() && this->editor.refresh())
    {
      this->fio.wasAltered(true);
      ::SendMessage(this->hwndMain, WM_USER_UPDATE_VIEW, 0, 0);
    }

    if(!this->fIdleThreadExit) ::Sleep(500);
    if(!this->fIdleThreadExit) ::Sleep(500);
    if(!this->fIdleThreadExit) ::Sleep(500);
    if(!this->fIdleThreadExit) ::Sleep(500);
    if(!this->fIdleThreadExit) ::Sleep(500);
    if(!this->fIdleThreadExit) ::Sleep(500);
  }

  this->hIdleThread = NULL;
  return true;
}
