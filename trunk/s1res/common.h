//=================================================================================================
// common.h : common header file for this project
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
#include <commctrl.h>
#include <commdlg.h>
#include <htmlhelp.h>
#include <shellapi.h>
#include <exception>
#include <list>

#include <afxres.h>

#pragma warning(push)
#pragma warning(disable:4311)
#pragma warning(disable:4996)
  #include "StdString.h"
#pragma warning(pop)


//-------------------------------------------------------------------------------------------------
// global definitions
//-------------------------------------------------------------------------------------------------
#define URL_MAIN      _T("http://www.s1mp3.de/")
#define URL_UPDATE    _T("http://www.s1mp3.de/update.s1res.html?4000")
#define URL_DONATE    _T("http://www.s1mp3.de/donate.s1res.html")


//-------------------------------------------------------------------------------------------------
// global typedefs
//-------------------------------------------------------------------------------------------------
typedef __int8  int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;
typedef unsigned __int8  uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;


//-------------------------------------------------------------------------------------------------
// global macros
//-------------------------------------------------------------------------------------------------
#define SAFE_DELETE(_ptr_) {if(_ptr_) {delete(_ptr_); (_ptr_) = NULL;}}

#define FOREACH(_var_, _type_, _iterator_) \
  for(_type_::iterator _iterator_ = (_var_).begin(); _iterator_ != (_var_).end(); _iterator_++)

#define OpenURL(_url_) \
  ((BOOL)(::ShellExecute(NULL, _T("open"), _url_, _T(""), NULL, SW_SHOW) > (HINSTANCE)32))

#define SetDlgRadioButton(_hwndDlg, _uid, _state) \
  ::SendDlgItemMessage(_hwndDlg, _uid, BM_SETCHECK, (_state)?BST_CHECKED:BST_UNCHECKED, 0)
#define GetDlgRadioButton(_hwndDlg, _uid) \
 (::SendDlgItemMessage(_hwndDlg, _uid, BM_GETCHECK, 0, 0) == BST_CHECKED)

#define EnableDlgItem(_hwndDlg, _uid, _state) \
  ::EnableWindow(::GetDlgItem(_hwndDlg, _uid), _state)


//-------------------------------------------------------------------------------------------------
// enable "XP-style"
//-------------------------------------------------------------------------------------------------
#if defined _M_IX86
  #pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
