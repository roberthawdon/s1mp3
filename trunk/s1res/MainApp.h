//=================================================================================================
// MainApp.h : header file for MainApp class
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
#include "FileIO.h"
#include "Editor.h"
#include "ResItem.h"
#include "ResItemImage.h"
#include "ResItemString.h"


//-------------------------------------------------------------------------------------------------
// definitions
//-------------------------------------------------------------------------------------------------
#define MAINAPP_GUID          _T("s1res-{451D5B93-8831-47ce-A570-5A3145A7A570}")
#define MAINAPP_HELP_FILE     _T("s1res.chm")

#define MAINAPP_SPACER_WIDTH 2


//-------------------------------------------------------------------------------------------------
// class MainApp
//-------------------------------------------------------------------------------------------------
class MainApp {
  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    HINSTANCE hInstance;    //identifies the current instance of the application
    CStdString strAppName;  //the application name
    CStdString strAppPath;  //the path of the executable

    HMENU hMenu;            //handle to the main menu
    HIMAGELIST hImgList;    //handle to the image list for the listview

    HWND hwndMain;          //handle of the main window
    HWND hwndLeftPane;      //handle of the left pane
    HWND hwndRightPane;     //handle of the right pane
    HWND hwndSpacer;        //handle of the spacer between left and right pane
    HWND hwndStatusBar;     //handle of the status bar
    HWND hwndFocus;         //handle of the window with the focus

    FileIO fio;             //support for fileaccess

    LONG nStatusBarHeight;  //height of the status bar
    enum {SDS_COLD, SDS_WARM, SDS_HOT} nSizeDragState;

    HANDLE hIdleThread;     //handle to the idle thread
    BOOL fIdleThreadExit;   //if TRUE the idle thread will terminate

    Editor editor;          //edit items

  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    MainApp();
    ~MainApp();

    // [ core ]
    VOID close();
    BOOL init(HINSTANCE hInstance, LPTSTR lpCmdLine, INT nCmdShow);
    UINT run();

    // [ utils ]
    VOID doError(LPCWSTR lpcwError);
    VOID doError(DWORD dwError = ERROR_SUCCESS);

  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    // [ message handlers ]
    static LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT onMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL onCommand(WORD nCmdId);
    INT  onNotify(LPNMHDR lpNotify);
    BOOL onSize(LONG h, LONG v);
    BOOL onSetCursor(HWND hwndOwner);
    BOOL onLButton(BOOL fDown);
    BOOL onMouseMove(WORD x, WORD y, DWORD flags);
    BOOL onDropFiles(HDROP hDrop);

    // [ utils ]
    VOID getErrorText(CStdString &strResult, DWORD dwError);

    ResItem *getLeftPaneItem();

    VOID setStatusText(UINT nId)
      {CStdString str(MAKEINTRESOURCE(nId)); ::SetWindowText(hwndStatusBar, str);}

    VOID enableMenuItem(UINT nId, BOOL fState)
      {::EnableMenuItem(hMenu, nId, fState? MF_ENABLED : MF_DISABLED|MF_GRAYED);}

    VOID checkMenuItem(UINT nId, BOOL fState)
    {::CheckMenuItem(hMenu, nId, fState? MF_CHECKED : MF_UNCHECKED);}

    BOOL fileClose();
    BOOL fileSave(BOOL fSaveAs);
    BOOL fileOpen(LPWSTR lpwFile = NULL);

    BOOL editOpen();
    BOOL editClose();
    BOOL editCloseAll();

    BOOL updateView(BOOL fEntireRedraw = true);
    BOOL updateViewImages(ResItem *lpItem);
    BOOL updateViewStrings(ResItem *lpItem);
    BOOL updateViewStringsStatusText(ResItemString *lpItemString);
    BOOL updateViewFirmware(ResItem *lpItem);
    BOOL updateViewAFI(ResItem *lpItem);

    BOOL updateCodePage(LONG nCodePage = -1);

    // [ idle thread ]
    DWORD threadIdle();
    static DWORD WINAPI threadIdle(LPVOID lpParam)
      {return lpParam? ((MainApp*)lpParam)->threadIdle() : NULL;}
};
