//=================================================================================================
// Editor.h : class to manage all opened files (create and maintenance temporary files)
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
#include "ResItemImage.h"
#include "Mutex.h"


//-------------------------------------------------------------------------------------------------
#define TMP_PATH_GUID _T("s1res-{93C39201-9CE3-45bb-B7EC-36259E8FDAEA}")
#define EDITOR_MSPAINT_EXE _T("mspaint.exe")


//-------------------------------------------------------------------------------------------------
class EditorItem {
  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    ResItemImage *lpItem;       //ptr to the opened resource
    CStdString strFileName;     //the filename of the opened resource
    FILETIME fileTime;          //the time of the last write access

  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    EditorItem(ResItemImage *lpItem, LPCWSTR lpcwName, LPFILETIME lpFileTime)
    {
      this->lpItem = lpItem;
      strFileName.assign(lpcwName);
      this->fileTime = *lpFileTime;
    }

    ResItemImage *getItem() {return lpItem;}
    LPCWSTR getName() {return strFileName.c_str();}
    LPFILETIME getFileTime() {return &fileTime;}
    VOID setFileTime(LPFILETIME lpFileTime) {this->fileTime = *lpFileTime;}
};


//-------------------------------------------------------------------------------------------------
class Editor : private Mutex {
  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    CStdString strTmpPath;
    std::list<EditorItem> lstItems;

  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    Editor();
    ~Editor();

    BOOL open(ResItemImage *lpItemImage, HWND hwndParent =NULL); //open resource for edit
    BOOL close(ResItemImage *lpItemImage = NULL);    //close one/all resources, opened for edit
    BOOL refresh(ResItemImage *lpItemImage = NULL);  //refresh one/all resources, opened for edit

    UINT getNoOfOpenItems() {/*lock();*/ UINT nResult = (UINT)lstItems.size(); /*release();*/ return nResult;}

  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    BOOL _open(ResItemImage *lpItemImage, HWND hwndParent =NULL); //open resource for edit
    BOOL _close(ResItemImage *lpItemImage = NULL);    //close one/all resources, opened for edit
    BOOL _refresh(ResItemImage *lpItemImage = NULL);  //refresh one/all resources, opened for edit

    BOOL exec(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd); //remap ShellExecute function
    BOOL getFileTime(LPCWSTR lpFile, LPFILETIME lpTime);

    inline BOOL cmpFileTime(LPFILETIME lpTime1, LPFILETIME lpTime2)
    {
      return (!lpTime1 || !lpTime2)? false
        : (lpTime1->dwHighDateTime == lpTime2->dwHighDateTime)
          && (lpTime1->dwLowDateTime == lpTime2->dwLowDateTime);
    }
};
