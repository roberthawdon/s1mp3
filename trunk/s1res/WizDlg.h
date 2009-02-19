//=================================================================================================
// WizDlg.h : abstract class to implement wizard dialogs with multiple pages
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once


//-------------------------------------------------------------------------------------------------
// class WizDlg
//-------------------------------------------------------------------------------------------------
class WizDlg
{
  //-----------------------------------------------------------------------------------------------
  // typedefs (protected)
  //-----------------------------------------------------------------------------------------------
  protected:
    enum GETID {GETID_DLG_MAIN, GETID_ICO_MAIN, GETID_BTN_NEXT, GETID_BTN_PREV, GETID_BTN_CANCEL};

  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    HINSTANCE hInstance;
    HWND hwndParent;
    HWND hwndMain;
    HWND hwndChild;

    UINT nCmdShow;
    UINT nCurrentStage;
    HICON hPrevIcon;

  //-----------------------------------------------------------------------------------------------
  // methods
  //-----------------------------------------------------------------------------------------------
  public:
    WizDlg(void);
    ~WizDlg(void);

    BOOL run(HINSTANCE hInstance, HWND hwndParent, UINT nCmdShow = SW_SHOW);
    VOID doError(DWORD dwError = ERROR_SUCCESS);

  //-----------------------------------------------------------------------------------------------
  // methods (protected)
  //-----------------------------------------------------------------------------------------------
  protected:
    static INT_PTR CALLBACK StaticDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    UINT getStage();
    BOOL setStage(UINT nNewStage);

    VOID enableNext(BOOL fEnable);
    VOID enablePrev(BOOL fEnable);
    VOID enableCancel(BOOL fEnable);

    VOID close();

  //-----------------------------------------------------------------------------------------------
  // getters (protected)
  //-----------------------------------------------------------------------------------------------
  protected:
    HINSTANCE getInstance() {return hInstance;}
    HWND getParentWindow() {return hwndParent;}
    HWND getMainWindow() {return hwndMain;}
    HWND getChildWindow() {return hwndChild;}

  //-----------------------------------------------------------------------------------------------
  // methods (virtual, protected)
  //-----------------------------------------------------------------------------------------------
  protected:
    virtual UINT getID(GETID nGetId) =0;  //return the requested resource id
    virtual UINT getIDD(UINT nStage) =0;  //return the dialog id for the given stage

    virtual BOOL onInit() =0;             //perform global init and goto the first wizard page
    virtual BOOL onClose() = 0;           //return false to cancel close operation
    virtual BOOL onNext() =0;             //goto next wizard page
    virtual BOOL onPrev() =0;             //goto last wizard page

    virtual INT_PTR DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) =0;
};
