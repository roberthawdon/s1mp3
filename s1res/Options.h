//=================================================================================================
// Options.h : class to manage options/settings
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


//-------------------------------------------------------------------------------------------------
#define OPTIONS_ID                      0x65526977

#define OPTIONS_CODEPAGE_DEFAULT        0
#define OPTIONS_CODEPAGE_DEFAULT_URLENC 1

//#define OPTIONS_INI_FILE              _T("s1res.ini")

#define OPTIONS_REG_ROOT                HKEY_CURRENT_USER
#define OPTIONS_REG_KEY                 _T("software\\s1res")
#define OPTIONS_REG_VALUE               _T("settings")


//-------------------------------------------------------------------------------------------------
class Options : private Dialog
{
  //-----------------------------------------------------------------------------------------------
  // typedef (public)
  //-----------------------------------------------------------------------------------------------
  public:
    typedef struct {
      WINDOWPLACEMENT wndpl;        //window position
      INT   nSplitPos;              //split bar in promille (1/1000)
      
      TCHAR szFile[MAX_PATH];       //last file
      DWORD nFilterIndex;           //last filter index

      DWORD nEditorIndex;           //editor index: 0=associated, 1=mspaint, 2=custom
      TCHAR szEditor[MAX_PATH];     //custom editor executable
      TCHAR szEditorArgs[MAX_PATH]; //custom editor arguments

      COLORREF crefMonoForeground;  //monochrome color values
      COLORREF crefMonoBackground;  //
      COLORREF crefCustColors[16];  //the custom colors created inside the ChooseColor-dialog

      DWORD dwId;                   //settings id

      UINT uCodePage;               //codepage used to convert string resources
                                    //see http://msdn2.microsoft.com/de-de/library/ms776446(en-us,VS.85).aspx
    } ATTRIBUTES;

  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    Options() : Dialog(IDD_OPTIONS)
      {::ZeroMemory(&attr, sizeof(attr));}

    bool runDialog(HINSTANCE hInstance, HWND hwndParent)
      {return run(hInstance, hwndParent) > 0;}

    BOOL load();    //load options from registry
    BOOL store();   //store options in registry

  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    VOID setEdtCustomCtrl(HWND hwndDlg, LPCWSTR lpFile, LPCWSTR lpArgs);
    VOID getEdtCustomCtrl(HWND hwndDlg, LPWSTR lpFile, LPWSTR lpArgs);
    VOID parseStringList(LPCWSTR lpString, std::list<CStdString> *lpList);
    BOOL chooseColor(HWND hwndDlg, LPCOLORREF lpColorRef);

  //-----------------------------------------------------------------------------------------------
  // getters (public)
  //-----------------------------------------------------------------------------------------------
  public:
    ATTRIBUTES *get() {return &this->attr;}

  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    ATTRIBUTES attr;

    COLORREF crefMonoForeground;
    COLORREF crefMonoBackground;
    COLORREF crefCustColors[16];
};


// declare the global protection class instance
extern Options g_options;
