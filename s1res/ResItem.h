//=================================================================================================
// ResItem.h : describe a single resource item and a list of its children
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


//-------------------------------------------------------------------------------------------------
class ResItem {
  //-----------------------------------------------------------------------------------------------
  // typedef (public)
  //-----------------------------------------------------------------------------------------------
  public:
    enum TYPE {
      TYPE_UNKNOWN, TYPE_INVALID_DATA,
      TYPE_FILE_AFI, TYPE_FILE_FW, 
      TYPE_FILE_RES, TYPE_FILE_BREC, TYPE_FILE_FONT8, TYPE_FILE_FONT16,
      TYPE_DATA, TYPE_ICON, TYPE_LOGO, TYPE_STRING, TYPE_MSTRING,
    };

  //-----------------------------------------------------------------------------------------------
  // attributes (protected)
  //-----------------------------------------------------------------------------------------------
  protected:
    TYPE nType;           //resource type
    CStdString strName;   //resource/file name
    LPBYTE lpData;        //ptr to data
    DWORD dwDataSize;     //actual size of data

  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    std::list<ResItem*> lstItems;  //child items
    HTREEITEM hTreeItem;

  //-----------------------------------------------------------------------------------------------
  // methods (public)
  //-----------------------------------------------------------------------------------------------
  public:
    ResItem(TYPE nType, LPCWSTR lpName, LPBYTE lpData, DWORD dwDataSize);
    ~ResItem();

    BOOL validate(LPCBYTE lpChildData, DWORD dwChildDataSize);
    BOOL updateTreeView(HWND hwndTree, HTREEITEM hParentTreeItem = NULL);
    BOOL add(ResItem *lpItem);

  //-----------------------------------------------------------------------------------------------
  // getters (public)
  //-----------------------------------------------------------------------------------------------
  public:
    TYPE getType() {return this->nType;}
    LPCTSTR getName() {return this->strName.c_str();}
    LPBYTE getData() {return lpData;}
    DWORD getDataSize() {return dwDataSize;}
    std::list<ResItem*> *getList() {return &this->lstItems;}
    BOOL hasChilds() {return !this->lstItems.empty();}

  //-----------------------------------------------------------------------------------------------
  // utils (protected)
  //-----------------------------------------------------------------------------------------------
  public:
    inline bool success() {return true;}
    inline bool error() {return false;}
    inline bool error(DWORD dwError) {SetLastError(dwError); return false;}
};
