//=================================================================================================
// Hyperlink.h : class to create static text controls with hyperlink feature
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#include <windows.h>
#include <shellapi.h>


//-------------------------------------------------------------------------------------------------
class Hyperlink {
  //-----------------------------------------------------------------------------------------------
  // methods
  //-----------------------------------------------------------------------------------------------
  public:
    //---------------------------------------------------------------------------------------------
    // this constructor turns a dialog's static text control into an underlined hyperlink
    // you can call it in your WM_INITDIALOG, or anywhere
    //---------------------------------------------------------------------------------------------
    Hyperlink(HWND hwndDlg, UINT uId, LPCWSTR lpUrl)
    {
      // first we'll subclass the edit control
      HWND hCtrl = GetDlgItem(hwndDlg, uId);
      SetWindowText(hCtrl, lpUrl);

      HGLOBAL hOld = (HGLOBAL)GetProp(hCtrl, _T("href_dat"));
      if(hOld) //if it had been subclassed before, we merely need to tell it the new lpUrl
      {
        URLDATA *ud = (URLDATA*)GlobalLock(hOld);
        wcscpy_s<MAX_PATH>(ud->wcUrl, lpUrl);
        GlobalUnlock(hOld);
      }
      else
      {
        HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, sizeof(URLDATA));
        URLDATA *ud = (URLDATA*)GlobalLock(hGlob);
        wcscpy_s<MAX_PATH>(ud->wcUrl, lpUrl);
        ud->wpOldProc = (WNDPROC)(LONG_PTR)GetWindowLongPtr(hCtrl, GWLP_WNDPROC);
        ud->hf = 0;
        ud->hb = 0;
        GlobalUnlock(hGlob);
        SetProp(hCtrl, _T("href_dat"), hGlob);
        SetWindowLongPtr(hCtrl, GWLP_WNDPROC, (LONG)(LONG_PTR)urlCtrlProc);
      }

      // now we could subclass the dialog
      if(!GetProp(hwndDlg, _T("href_dlg")))
      {
        HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, sizeof(URLDATA));
        URLDATA *ud = (URLDATA*)GlobalLock(hGlob);
        ud->wcUrl[0] = _T('\0');
        ud->wpOldProc = (WNDPROC)(LONG_PTR)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
        ud->hb = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        ud->hf = 0; //the font will be created lazilly, the first time WM_CTLCOLORSTATIC gets called
        GlobalUnlock(hGlob);
        SetProp(hwndDlg, _T("href_dlg"), hGlob);
        SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG)(LONG_PTR)urlDlgProc);
      }
    }

  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    //---------------------------------------------------------------------------------------------
    // urlCtrlProc: this is the subclass procedure for the static control
    //---------------------------------------------------------------------------------------------
    static LRESULT CALLBACK urlCtrlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      HGLOBAL hGlob = (HGLOBAL)GetProp(hWnd, _T("href_dat"));
      if(!hGlob) return DefWindowProc(hWnd, uMsg, wParam, lParam);
      URLDATA *ud = (URLDATA*)GlobalLock(hGlob);
      WNDPROC wpOldProc = ud->wpOldProc;

      //process message
      switch(uMsg)
      {
      case WM_DESTROY:
        RemoveProp(hWnd, _T("href_dat"));
        GlobalUnlock(hGlob);
        GlobalFree(hGlob);
        break;

      case WM_LBUTTONDOWN:
        ShellExecute(
          GetParent(hWnd)? GetParent(hWnd) : hWnd,
          _T("open"), ud->wcUrl, NULL, NULL, SW_RESTORE );
        break;
        
      case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
        return true;

      case WM_NCHITTEST:
        return HTCLIENT; //normally a static returns HTTRANSPARENT, disabling WM_SETCURSOR
      }

      GlobalUnlock(hGlob);
      return CallWindowProc(wpOldProc, hWnd, uMsg, wParam, lParam);
    }
      
    //---------------------------------------------------------------------------------------------
    // urlDlgProc: this is the subclass procedure for the dialog
    //---------------------------------------------------------------------------------------------
    static LRESULT CALLBACK urlDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
      HGLOBAL hGlob = (HGLOBAL)GetProp(hWnd, _T("href_dlg"));
      if(!hGlob) return DefWindowProc(hWnd, uMsg, wParam, lParam);
      URLDATA *ud = (URLDATA*)GlobalLock(hGlob);
      WNDPROC wpOldProc = ud->wpOldProc;

      //process message
      switch(uMsg)
      {
      case WM_DESTROY:
        RemoveProp(hWnd, _T("href_dlg"));
        if(ud->hb) DeleteObject(ud->hb);
        if(ud->hf) DeleteObject(ud->hf);
        GlobalUnlock(hGlob);
        GlobalFree(hGlob);
        break;
        
      case WM_CTLCOLORSTATIC:
        {
          HDC hdc = (HDC)wParam;
          HWND hCtrl = (HWND)lParam;
          // to check whether to handle this control, we look for its subclassed property!
          if(!GetProp(hCtrl, _T("href_dat")))
            return CallWindowProc(ud->wpOldProc, hWnd, uMsg, wParam, lParam);

          // change the static text control color
          // (1) leave the text opaque. this will allow us to change the text without make it become looking wrong.
          // (2) SetBkColor. this background colour will be used underneath each character cell.
          // (3) return HBRUSH. This background colour will be used where there's no text.
          SetTextColor(hdc, RGB(0, 0, 255));
          SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
          if(!ud->hf)
          {
            // we use lazy creation of the font
            TEXTMETRIC tm;
            GetTextMetrics(hdc, &tm);
            
            LOGFONT lf;
            lf.lfHeight = tm.tmHeight;
            lf.lfWidth = 0;
            lf.lfEscapement = 0;
            lf.lfOrientation = 0;
            lf.lfWeight = tm.tmWeight;
            lf.lfItalic = tm.tmItalic;
            lf.lfUnderline = true;
            lf.lfStrikeOut = tm.tmStruckOut;
            lf.lfCharSet = tm.tmCharSet;
            lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
            lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
            lf.lfQuality = DEFAULT_QUALITY;
            lf.lfPitchAndFamily = tm.tmPitchAndFamily;
            GetTextFace(hdc, LF_FACESIZE, lf.lfFaceName);
            ud->hf = CreateFontIndirect(&lf);
          }

          SelectObject(hdc, ud->hf);
          return (BOOL)(LONG_PTR)ud->hb;
        }
      }

      GlobalUnlock(hGlob);
      return CallWindowProc(wpOldProc, hWnd, uMsg, wParam, lParam);
    }

  //-----------------------------------------------------------------------------------------------
  // typedefs (private)
  //-----------------------------------------------------------------------------------------------
  private:
    //used to build the hyperlink static control
    typedef struct {
      WCHAR wcUrl[MAX_PATH];
      WNDPROC wpOldProc;
      HFONT hf;
      HBRUSH hb;
    } URLDATA;
};