//=================================================================================================
// DeviceIO.h : access the s1mp3 player and read/write the firmware to/from flash
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once
#include <richedit.h>

#include "../common/AdfuSession.h"
#include "../common/FirmwareIO.h"


//-------------------------------------------------------------------------------------------------
class DeviceIO
{
  //-----------------------------------------------------------------------------------------------
  // typedefs
  //-----------------------------------------------------------------------------------------------
  private:
    class FWIOCB : public FirmwareIO_CALLBACK
    {
    public:
      FWIOCB(HWND hwndProgress) {this->hwndProgress = hwndProgress;}
      void progress(unsigned char uPercent) {::SendMessage(hwndProgress, PBM_SETPOS, uPercent, NULL);}
    private:
      HWND hwndProgress;
    };

  //-----------------------------------------------------------------------------------------------
  // methods
  //-----------------------------------------------------------------------------------------------
  public:
    DeviceIO(void);
    ~DeviceIO(void);

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

    BOOL read(HWND hwndDlg, INT nDevSel, INT nChipSel, BOOL fGiveIO);
    BOOL write(HWND hwndDlg, INT nDevSel, INT nChipSel, BOOL fGiveIO);

    static DWORD WINAPI readThread(LPVOID lpParam);
    VOID readThread();

    static DWORD WINAPI writeThread(LPVOID lpParam);
    VOID writeThread();

    BOOL displayFirmware(HWND hwndDlg);
    BOOL getFirmware(LPBYTE &lpData, DWORD &dwDataSize);  //returns a new allocated object
    BOOL setFirmware(LPBYTE lpData, DWORD dwDataSize);

  //-----------------------------------------------------------------------------------------------
  // methods (private)
  //-----------------------------------------------------------------------------------------------
  private:
    BOOL setAfiEntryFromFirmware(LP_AFI_HEAD_ENTRY lpAfiEntry, LPBYTE lpFwData, DWORD dwFwOfs);

    VOID print(LPCSTR lp)
    {
      CHARRANGE cr = {-1, -1};
      ::SendDlgItemMessage(hwndDlg, IDC_STATUS, EM_EXSETSEL, 0, (LPARAM)&cr); //move caret to end
      ::SendDlgItemMessageA(hwndDlg, IDC_STATUS, EM_REPLACESEL, FALSE, (LPARAM)lp); //"append" text
      ::SendDlgItemMessage(hwndDlg, IDC_STATUS, EM_SCROLLCARET, 0, 0);  //scrolls the caret into view
    }

    VOID print(LPCWSTR lp)
    {
      CHARRANGE cr = {-1, -1};
      ::SendDlgItemMessage(hwndDlg, IDC_STATUS, EM_EXSETSEL, 0, (LPARAM)&cr); //move caret to end
      ::SendDlgItemMessageW(hwndDlg, IDC_STATUS, EM_REPLACESEL, FALSE, (LPARAM)lp); //"append" text
      ::SendDlgItemMessage(hwndDlg, IDC_STATUS, EM_SCROLLCARET, 0, 0);  //scrolls the caret into view
    }

    VOID println(LPCSTR lp)
    {
      print(lp);
      print("\n");
    }

    VOID println(LPCWSTR lp)
    {
      print(lp);
      print("\n");
    }

    VOID free()
    {
      if(lpBREC) delete lpBREC;
      if(lpFW) delete lpFW;
      lpBREC = lpFW = NULL;
      dwBRECSize = dwFWSize = 0;
    }

    VOID trunc(CStdString &str, LPCSTR lpInput, DWORD dwMaxLen)
    {
      str.clear();
      for(; dwMaxLen && lpInput && *lpInput; dwMaxLen--) str += *(lpInput++);
      str = str.Trim();
    }

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
    BOOL fGiveIO;

    LPBYTE lpBREC;
    uint32 dwBRECSize;
    LPBYTE lpFW;
    uint32 dwFWSize;
};
