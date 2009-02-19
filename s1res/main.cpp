//=================================================================================================
// s1res - resource editor for s1mp3/s1mp4 players
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
// [TODO]
//
// - free all bitmaps (ResItemImage::freeBitmap) after options changed color and refresh listview
// - add wizard to merge with other resource files
// - BREC image extractor dialog to extract/edit "hourglass"-logo
// - advanced: allow to drag'n'drop and/or copy/paste images/files/...
//=================================================================================================
// [TO CHANGE VERSION INFO]
//
// - common.h : #define URL_UPDATE
// - resource : IDD_ABOUT dialog
// - resource : VS_VERSION_INFO
// - installer.nsi : Caption
// - installer.nsi : VIProductVersion
//=================================================================================================
// [HISTORY]
//
// v4.0
//   -released sources under the GNU GPL
// v3.6
//   -firmware import invalid-brec bugfix (!)
// v3.5
//   -improoved firmware import
//   -monochrome picture bugfix
//   -single string resource bugfix
//   -support for more character code-pages
// v3.4
//   -improoved firmware property sheet
//   -support for different character code-pages
//   -clear strings with a single press of the delete-key
// v3.3
//   -improoved firmware import
//   -firmware-import produces afi file, now containing the boot-record part
//   -firmware-import inserts dummy-place-holders for the original update-tool
//   -added property sheet to change firmware properties/attributes
//   -now supports font resources
//   -select colors used to display monochrome images
//   -added support for alternative payment method
// v3.2
//   -improoved firmware import
//   -updated help file
// v3.1
//   -improoved firmware import
// v3.0
//   -new GUI from scratch
//   -better edit support for icons/images (external tools and auto-refresh)
//   -support own image editors by configuring custom exe-path and arguments (eg. '"MyEditor.exe" /file="%1"')
//   -string-editor now accepts url encoded characters to support different char-sets
//   -import and export mechanism to directly access mp3 player's firmware
//   -drop bitmap files into the application to replace equal-named icons/images
// v2.1
//   -fixed a bug with old b/w icons, caused by the patch for large 2" images
// v2.0
//   -fixed a bug with large 2" images
//   -removed overflow warning bug
// v1.9
//   -fixed a bug inside the text edit control
// v1.8
//   -drag'n'drop import-feature for images
// v1.7
//   -larger icon preview for OLED 128x128 displays
// v1.6
//   -fixed bug inside import function for OLED/colored bitmap files
// v1.5
//   -support 128x64 logos
// v1.4
//   -removed bug, all icons get displayed now
//   -changed text control because language ids change from firmware to firmware
// v1.3
//   -support OLED/multicolor icons
//   -data overflow warning on saving
// v1.2
//   -open/save for firmware images
//   -correct firmware files checksums
//   -autodetect input file
//   -image edit commands: import/export/clear/invert/move
//   -support fonts
//   -load fonts inside fwimages
//=================================================================================================
// [ DOCS ]
//
// http://www.w3schools.com/tags/ref_urlencode.asp - url encoding
// http://www.codeguru.com/cpp/controls/listview/editingitemsandsubitem/article.php/c1077/ - listviews
// http://www.codeproject.com/useritems/HTMLHelp.asp - writing html help files
//=================================================================================================
#include "common.h"
#include "MainApp.h"


//-------------------------------------------------------------------------------------------------
// the one and only instance of class MainApp
//-------------------------------------------------------------------------------------------------
MainApp mainApp;


//-------------------------------------------------------------------------------------------------
// the windows entry point
//-------------------------------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, INT nCmdShow)
{
  try
  {
    InitCommonControls();
    //LoadLibrary("RICHED32.DLL");

    // init the main application
    if( !mainApp.init(hInstance, GetCommandLine(), nCmdShow) ) return(0);

    // run the main application
    return( mainApp.run() );
  }
  catch(std::bad_alloc &)
  {
    mainApp.doError(ERROR_OUTOFMEMORY);
  }
  catch(...)
  {
    mainApp.doError(ERROR_UNHANDLED_ERROR);
  }
  return(0);
}
