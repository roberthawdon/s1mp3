//=================================================================================================
// CloneDevice.h : access the s1mp3 player and clone the flash
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once

#include "../common/FlashIO.h"
//#include "../common/FirmwareIO.h"
//#include "../common/structures.h"


//-------------------------------------------------------------------------------------------------
#define SIZE_OF_BROM 16*1024


//-------------------------------------------------------------------------------------------------
class CloneDevice
{
  //-----------------------------------------------------------------------------------------------
  // typedefs
  //-----------------------------------------------------------------------------------------------
    typedef struct {
      uint64 signature;         //"s1clone",0
      uint32 flash_id;          //flash chip device id
      uint32 flash_size;        //flash size in pages
      uint32 block_size;        //block size in pages
      uint32 page_size;         //size of one page (should be FlashIO::REAL_PAGE_SIZE)
      uint8 brom[SIZE_OF_BROM]; //the brom image
    } FILE_HEADER;

    typedef struct {
      uint16 chksum;        //checksum of the decoded section
      uint16 size;          //size of encoded section
    } FILE_SECTION;

  //-----------------------------------------------------------------------------------------------
  // methods
  //-----------------------------------------------------------------------------------------------
  public:
    CloneDevice(void);
    ~CloneDevice(void);

    BOOL isThreadRunning() {return hThread != NULL;}
    VOID cancelThread() {fCancelThread = true;}
    BOOL isThreadCanceled() {return fCancelThread;};

    BOOL killThread() //shouldn't get used
    {
      if(!hThread) return true;
      if(!::TerminateThread(hThread, 0)) return false;
      hThread = NULL;
      return true;
    }

    BOOL displayDeviceList(HWND hwndListCtrl);
    BOOL updateDeviceList(HWND hwndDlg);
    static DWORD WINAPI updateDeviceListThread(LPVOID lpParam);
    VOID updateDeviceListThread();

    BOOL read(HWND hwndDlg, INT nDevSel, INT nChipSel, CStdString &strFile, BOOL fFormat);
    BOOL write(HWND hwndDlg, INT nDevSel, INT nChipSel, CStdString &strFile, BOOL fWrite, BOOL fVerify);

    static DWORD WINAPI readThread(LPVOID lpParam);
    VOID readThread();

    static DWORD WINAPI writeThread(LPVOID lpParam);
    VOID writeThread();

  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    VOID println(LPCSTR lp)
    {
      ::SendDlgItemMessage(hwndDlg, IDC_LIST, LB_SETTOPINDEX,
        ::SendDlgItemMessageA(hwndDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)lp), 0);
    }
    VOID println(LPCWSTR lp)
    {
      ::SendDlgItemMessage(hwndDlg, IDC_LIST, LB_SETTOPINDEX,
        ::SendDlgItemMessageW(hwndDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)lp), 0);
    }
    VOID writeFile(HANDLE hFile, LPCVOID lpBuf, DWORD dwSize)
    {
      DWORD dwWritten = 0;
      if(!::WriteFile(hFile, lpBuf, dwSize, &dwWritten, NULL)) throw AdfuException(::GetLastError());
      if(dwWritten != dwSize) throw AdfuException(ERROR_WRITE_FAULT);
    }
    VOID readFile(HANDLE hFile, LPVOID lpBuf, DWORD dwSize)
    {
      DWORD dwRead = 0;
      if(!::ReadFile(hFile, lpBuf, dwSize, &dwRead, NULL)) throw AdfuException(::GetLastError());
      if(dwRead != dwSize) throw AdfuException(ERROR_READ_FAULT);
    }
    uint16 calcChkSum(LPBYTE lpBuf, DWORD dwSize);
    uint32 encodePage(LPBYTE lpDest, DWORD dwDestSize, LPBYTE lpSrc, DWORD dwSrcSize);
    uint32 decodePage(LPBYTE lpDest, DWORD dwDestSize, LPBYTE lpSrc, DWORD dwSrcSize);

  //-----------------------------------------------------------------------------------------------
  // attributes (private)
  //-----------------------------------------------------------------------------------------------
  private:
    std::list<AdfuSession::DeviceDescriptor> lstDevDescr;
    AdfuSession::DeviceDescriptor *lpDevDescr;

    HANDLE hThread;
    BOOL fCancelThread;

    HWND hwndDlg;
    INT nChipSel;
    CStdString strFile;
    BOOL fFormat;
    BOOL fWrite;
    BOOL fVerify;
};


//-------------------------------------------------------------------------------------------------
extern CloneDevice clone;
