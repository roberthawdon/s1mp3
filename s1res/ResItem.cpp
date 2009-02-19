//=================================================================================================
// ResItem.cpp : describe a single resource item and a list of its children
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#include "common.h"
#include "ResItem.h"


//-------------------------------------------------------------------------------------------------
ResItem::ResItem(TYPE nType, LPCWSTR lpName, LPBYTE lpData, DWORD dwDataSize)
{
  this->nType = nType;
  if(lpName) this->strName = lpName;
  else this->strName.clear();
  this->lpData = lpData;
  this->dwDataSize = dwDataSize;
  this->lstItems.clear(); 
  this->hTreeItem = NULL;
}


//-------------------------------------------------------------------------------------------------
ResItem::~ResItem()
{
  // free all child items
  for(; !lstItems.empty(); lstItems.pop_front())
    if(lstItems.front()) delete lstItems.front();
}


//-------------------------------------------------------------------------------------------------
// validate data address space of this item against some child item parameters
//-------------------------------------------------------------------------------------------------
BOOL ResItem::validate(LPCBYTE lpChildData, DWORD dwChildDataSize)
{
  if(!lpChildData && !dwChildDataSize) return true;
  return (lpChildData >= this->lpData)
    && (&lpChildData[dwChildDataSize] <= &this->lpData[this->dwDataSize]);
}


//-------------------------------------------------------------------------------------------------
// add this item and all children to a win32 treeview control
//-------------------------------------------------------------------------------------------------
BOOL ResItem::updateTreeView(HWND hwndTree, HTREEITEM hParentTreeItem)
{
  BOOL fResult = true;
  TVINSERTSTRUCT tvi;

  if(!hParentTreeItem) //first time?
  {
    TreeView_DeleteAllItems(hwndTree);

    ::ZeroMemory(&tvi, sizeof(tvi));
    tvi.hParent = TVI_ROOT;
    tvi.hInsertAfter = TVI_SORT;
    tvi.item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_STATE;
    tvi.item.state = tvi.item.stateMask = TVIS_BOLD|TVIS_EXPANDED;
    tvi.item.pszText = (LPWSTR)this->strName.c_str();
    tvi.item.lParam = (LPARAM)this;
    hParentTreeItem = TreeView_InsertItem(hwndTree, &tvi);
    if(!hParentTreeItem) fResult = false;
    else TreeView_SelectItem(hwndTree, hParentTreeItem);
  }

  this->hTreeItem = hParentTreeItem;

  FOREACH(this->lstItems, std::list<ResItem*>, i) if(*i)
  {
    ZeroMemory(&tvi, sizeof(tvi));
    tvi.hParent = this->hTreeItem;
    tvi.hInsertAfter = TVI_SORT;
    tvi.item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_STATE;
    tvi.item.pszText = (LPWSTR)(*i)->strName.c_str();
    tvi.item.lParam = (LPARAM)(*i);
    switch((*i)->nType)
    {
      case TYPE_FILE_AFI:
      case TYPE_FILE_FW:
        tvi.item.state = tvi.item.stateMask = TVIS_EXPANDED;
      case TYPE_FILE_RES:
      case TYPE_STRING:
      case TYPE_MSTRING:
      //default:
        HTREEITEM hNewTreeItem = TreeView_InsertItem(hwndTree, &tvi);
        if(!hNewTreeItem || !(*i)->updateTreeView(hwndTree, hNewTreeItem)) fResult = false;
    }
  }

  return fResult;
}


//-------------------------------------------------------------------------------------------------
// do another validate before parsing and finally adding the child item to our list
//-------------------------------------------------------------------------------------------------
BOOL ResItem::add(ResItem *lpItem)
{
  if(!lpItem) return error(ERROR_INVALID_PARAMETER);

  // validate new child item and add it to the list
  if(!this->validate(lpItem->lpData, lpItem->dwDataSize))
  {
    lpItem->nType = TYPE_INVALID_DATA;
    lpItem->lpData = NULL;
    lpItem->dwDataSize = 0;
  }

  this->lstItems.push_back(lpItem);
  return success();
}
