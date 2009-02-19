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
// access folders: http://www.codeproject.com/file/Folder_Utility.asp
//=================================================================================================
// TODO
//  try to close the editor
//  don't know how todo, maybe we search any window caption containing the filename
//  a more complex way would be to detect which app accessed our file
//  but how to close a single bmp if all bmp's are opened inside the same app?
//=================================================================================================
#include "common.h"
#include "Editor.h"
#include "Options.h"


//-------------------------------------------------------------------------------------------------
#ifdef _DEBUG
  #define EDITOR_DEBUG(_fmt_, _arg_) \
    {CStdString str; str.Format(_fmt_, _arg_); OutputDebugString(str.c_str());}
#else
  #define EDITOR_DEBUG(_fmt_, _arg_)
#endif


//-------------------------------------------------------------------------------------------------
Editor::Editor()
{
  //create unique sub-folder inside the system temporary folder
  WCHAR wcTmpPath[MAX_PATH] = _T("\0");
  ::GetTempPath(MAX_PATH, wcTmpPath);

  strTmpPath.assign(wcTmpPath);
  strTmpPath.append(TMP_PATH_GUID);
  ::CreateDirectory(strTmpPath.c_str(), NULL);

  //test if we could create a file
  HANDLE hFile = ::CreateFile(TMP_PATH_GUID,
    GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
  if(hFile != INVALID_HANDLE_VALUE) ::CloseHandle(hFile);
    else strTmpPath.assign(wcTmpPath);
}

//-------------------------------------------------------------------------------------------------
Editor::~Editor()
{
  //always try to remove our temp folder on exit
  ::RemoveDirectory(strTmpPath.c_str());
}




//-------------------------------------------------------------------------------------------------
BOOL Editor::open(ResItemImage *lpItemImage, HWND hwndParent)
{
  lock();
  BOOL fResult = _open(lpItemImage, hwndParent);
  release();
  return fResult;
}

//-------------------------------------------------------------------------------------------------
BOOL Editor::close(ResItemImage *lpItemImage)
{
  lock();
  BOOL fResult = _close(lpItemImage);
  release();
  return fResult;
}

//-------------------------------------------------------------------------------------------------
BOOL Editor::refresh(ResItemImage *lpItemImage)
{
  lock();
  BOOL fResult = _refresh(lpItemImage);
  release();
  return fResult;
}




//-------------------------------------------------------------------------------------------------
// open resource for edit
//-------------------------------------------------------------------------------------------------
BOOL Editor::_open(ResItemImage *lpItemImage, HWND hwndParent)
{
  if(!lpItemImage)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return false;
  }

  //update and close if item was already opened
  _refresh(lpItemImage);
  _close(lpItemImage);

  //create temp file from resource and insert item into list
  FILETIME ft;
  CStdString strName = strTmpPath + _T('\\') + lpItemImage->getName() + _T(".BMP");
  if(!lpItemImage->exportBitmap(strName.c_str())) return false;
  getFileTime(strName.c_str(), &ft);
  lstItems.push_back( EditorItem(lpItemImage, strName.c_str(), &ft) );

  EDITOR_DEBUG(_T("s1res::Editor::open(%s)"), strName)

  //try to open temp file
  switch(g_options.get()->nEditorIndex)
  {
  case 2: //custom editor
    {
      CStdString strExec = g_options.get()->szEditor;
      CStdString strArgs = g_options.get()->szEditorArgs;
      if(strArgs.empty()) strArgs = strName;
      else
      {
        //insert strName for each "%1" or append, if not included before
        if(strArgs.Replace(CStdString(_T("%1")), strName) <= 0)
          strArgs += _T(" ") + strName;
      }
      if(exec(hwndParent, _T("open"), strExec.c_str(), strArgs.c_str(), NULL, SW_SHOW)) return true;
      _close(lpItemImage);
      return false;
    }

  case 1: //mspaint.exe
    {
      if(exec(hwndParent, _T("open"), EDITOR_MSPAINT_EXE, strName.c_str(), NULL, SW_SHOW)) return true;
      _close(lpItemImage);
      return false;
    }

  default:  //default/assoicated editor
    {
      if(exec(hwndParent, _T("edit"), strName.c_str(), NULL, NULL, SW_SHOW)) return true;
      if(exec(hwndParent, _T("open"), strName.c_str(), NULL, NULL, SW_SHOW)) return true;
      _close(lpItemImage);
      return false;
    }
  }
}


//-------------------------------------------------------------------------------------------------
// close one/all resources, opened for edit
//-------------------------------------------------------------------------------------------------
BOOL Editor::_close(ResItemImage *lpItemImage)
{
  if(lpItemImage)  //close one item...
  { 
    FOREACH(lstItems, std::list<EditorItem>, i) if(i->getItem() == lpItemImage)
    {
      EDITOR_DEBUG(_T("s1res::Editor::close(%s)"), i->getName())
      BOOL fResult = ::DeleteFile(i->getName());
      lstItems.erase(i);
      return fResult;
    }

    SetLastError(ERROR_NOT_FOUND);
    return false;
  }
  else  //close all items...
  {
    BOOL fResult = true;

    while(!lstItems.empty())
    {
      EDITOR_DEBUG(_T("s1res::Editor::close(%s)"), lstItems.front().getName())
      if(!::DeleteFile(lstItems.front().getName())) fResult = false;
      lstItems.pop_front();
    }

    return fResult;
  }
}


//-------------------------------------------------------------------------------------------------
// refresh one/all resources, opened for edit
// returns FALSE if no refresh was necessary, or TRUE otherwise
//-------------------------------------------------------------------------------------------------
BOOL Editor::_refresh(ResItemImage *lpItemImage)
{
  if(lpItemImage)  //refresh one item...
  { 
    FOREACH(lstItems, std::list<EditorItem>, i) if(i->getItem() == lpItemImage)
    {
      EDITOR_DEBUG(_T("s1res::Editor::getFileTime(%s)"), i->getName())

      //refresh necessary? compare filetimes first...
      FILETIME ft;
      if(getFileTime(i->getName(), &ft) && cmpFileTime(&ft, i->getFileTime()))
      {
        SetLastError(ERROR_SUCCESS);
        return false;  //still the same, return no refresh was necessary
      }
      i->setFileTime(&ft);  //set new filetime

      //update image
      EDITOR_DEBUG(_T("s1res::Editor::refresh(%s)"), i->getName())

      i->getItem()->importBitmap(i->getName());
      return true;
    }

    SetLastError(ERROR_NOT_FOUND);
    return false;
  }
  else  //refresh all items...
  {
    BOOL fResult = false;
    FOREACH(lstItems, std::list<EditorItem>, i) fResult |= _refresh(i->getItem());
    return fResult;
  }
}


//-------------------------------------------------------------------------------------------------
// remap ShellExecute function
//-------------------------------------------------------------------------------------------------
BOOL Editor::exec(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
  INT nResult = (INT)(INT_PTR)::ShellExecute(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
  if(nResult > 32) return true;
  else switch(nResult) {
  case 0:  ::SetLastError(ERROR_OUTOFMEMORY); return false;
  default: ::SetLastError(nResult);           return false;
  }
}


//-------------------------------------------------------------------------------------------------
BOOL Editor::getFileTime(LPCWSTR lpFile, LPFILETIME lpTime)
{
  if(!lpTime) return false;
  ::ZeroMemory(lpTime, sizeof(FILETIME));
  HANDLE hFile = ::CreateFile(lpFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if(hFile == INVALID_HANDLE_VALUE) return false;
  BOOL fResult = ::GetFileTime(hFile, NULL, NULL, lpTime);
  ::CloseHandle(hFile);
  return fResult;
}
