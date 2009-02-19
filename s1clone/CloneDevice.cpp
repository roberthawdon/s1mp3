//=================================================================================================
// CloneDevice.cpp : access the s1mp3 player and clone the flash
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#include "common.h"
#include "CloneDevice.h"


//-------------------------------------------------------------------------------------------------
#define CLONEDEVICE_READ_RETRY 3
#define FILE_HEADER_SIGNATURE 0x00656e6f6c633173


//-------------------------------------------------------------------------------------------------
CloneDevice::CloneDevice(void)
{
  this->hThread = NULL;
}


//-------------------------------------------------------------------------------------------------
CloneDevice::~CloneDevice(void)
{
}


//-------------------------------------------------------------------------------------------------
BOOL CloneDevice::displayDeviceList(HWND hwndListCtrl)
{
  ::SendMessage(hwndListCtrl, LB_RESETCONTENT, 0, 0);

  foreach(lstDevDescr, std::list<AdfuSession::DeviceDescriptor>, i)
  {
    ::SendMessageA(hwndListCtrl, LB_ADDSTRING, 0, (LPARAM)i->g_strName.c_str());
  }

  ::SendMessage(hwndListCtrl, LB_SETCURSEL, 0, 0);
  return true;
}


//-------------------------------------------------------------------------------------------------
BOOL CloneDevice::updateDeviceList(HWND hwndDlg)
{
  if(hThread)
  {
    SetLastError(ERROR_BUSY);
    return false;
  }

  this->hwndDlg = hwndDlg;
  this->fCancelThread = false;
  hThread = ::CreateThread(NULL, 0, updateDeviceListThread, (LPVOID)this, 0, NULL);
  return hThread != NULL;
}


//-------------------------------------------------------------------------------------------------
DWORD WINAPI CloneDevice::updateDeviceListThread(LPVOID lpParam)
{
  ((CloneDevice*)lpParam)->updateDeviceListThread();
  ((CloneDevice*)lpParam)->hThread = NULL;
  return NULL;
}


//-------------------------------------------------------------------------------------------------
VOID CloneDevice::updateDeviceListThread()
{
  try
  {
    AdfuSession session;
    session.enumerate(lstDevDescr);
    if(::IsWindow(hwndDlg)) ::SendMessage(hwndDlg, WM_USER, 0, 0);  //success
  }
  catch(AdfuException &e)
  {
    if(::IsWindow(hwndDlg)) ::SendMessage(hwndDlg, WM_USER, 0, e.id());  //error
  }

  this->hThread = NULL;
}




//-------------------------------------------------------------------------------------------------
BOOL CloneDevice::read(HWND hwndDlg, INT nDevSel, INT nChipSel, CStdString &strFile, BOOL fFormat)
{
  if(this->hThread) 
  {
    SetLastError(ERROR_BUSY);
    return false;
  }

  this->lpDevDescr = NULL;
  foreach(lstDevDescr, std::list<AdfuSession::DeviceDescriptor>, i) if(nDevSel-- == 0)
  {
    this->lpDevDescr = &*i;
    break;
  }

  if(!this->lpDevDescr)
  {
    SetLastError(ERROR_DEVICE_REMOVED);
    return false;
  }

  this->hwndDlg = hwndDlg;
  this->nChipSel = nChipSel;
  this->strFile = strFile;
  this->fFormat = fFormat;
  this->fWrite = false;
  this->fVerify = false;

  this->fCancelThread = false;

  this->hThread = CreateThread(NULL, 0, readThread, (LPVOID)this, 0, NULL);
  return this->hThread != NULL;
}


//-------------------------------------------------------------------------------------------------
BOOL CloneDevice::write(HWND hwndDlg, INT nDevSel, INT nChipSel, CStdString &strFile, 
                        BOOL fWrite, BOOL fVerify)
{
  if(this->hThread) 
  {
    SetLastError(ERROR_BUSY);
    return false;
  }

  this->lpDevDescr = NULL;
  foreach(lstDevDescr, std::list<AdfuSession::DeviceDescriptor>, i) if(nDevSel-- == 0)
  {
    this->lpDevDescr = &*i;
    break;
  }

  if(!this->lpDevDescr)
  {
    SetLastError(ERROR_DEVICE_REMOVED);
    return false;
  }

  this->hwndDlg = hwndDlg;
  this->nChipSel = nChipSel;
  this->strFile = strFile;
  this->fFormat = false;
  this->fWrite = fWrite;
  this->fVerify = fVerify;

  this->fCancelThread = false;

  this->hThread = CreateThread(NULL, 0, writeThread, (LPVOID)this, 0, NULL);
  return this->hThread != NULL;
}




//-------------------------------------------------------------------------------------------------
DWORD WINAPI CloneDevice::readThread(LPVOID lpParam)
{
  ((CloneDevice*)lpParam)->readThread();
  ((CloneDevice*)lpParam)->hThread = NULL;
  return NULL;
}


//-------------------------------------------------------------------------------------------------
VOID CloneDevice::readThread()
{
  CStdString strText;
  HANDLE hFile = INVALID_HANDLE_VALUE;

  AdfuSession session;
  GiveIO gio(session);
  FlashIO fio(gio);

  try
  {
    ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETRANGE, NULL, MAKELPARAM(0, 1024));
    ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 0, NULL);


    // format disk?
    if(fFormat)
    {
      strText.LoadStringW(IDS_FORMAT);
      println(strText);
      
      try
      {
        hFile = ::CreateFileA(lpDevDescr->g_strName.c_str(), GENERIC_READ|GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, 0, NULL);
        if(hFile == INVALID_HANDLE_VALUE) throw AdfuException();

        struct {
          ULONG Version;
          ULONG Size;
          ULONG BlockLength;
          LARGE_INTEGER NumberOfBlocks;
          LARGE_INTEGER DiskLength;
        } cap;

        ::ZeroMemory(&cap, sizeof(cap));
        cap.Version = sizeof(cap);
        DWORD dwBytesReturned;
        if(!DeviceIoControl(hFile, IOCTL_STORAGE_READ_CAPACITY, NULL, 0, &cap, sizeof(cap), &dwBytesReturned, NULL)) throw AdfuException();
        if(dwBytesReturned < sizeof(cap)) throw AdfuException(ERROR_FUNCTION_FAILED);
        if(cap.NumberOfBlocks.HighPart || !cap.NumberOfBlocks.LowPart || !cap.BlockLength) throw AdfuException(ERROR_UNSUPPORTED_TYPE);

        strText.Format(IDS_FORMAT_SIZE, cap.NumberOfBlocks.LowPart, cap.BlockLength);
        println(strText);

        uint8 format_buffer[256*1024];
        memset(format_buffer, -1, sizeof(format_buffer));
        double fApproxTime = 0.0f;
        DWORD dwBufBlocks = sizeof(format_buffer) / cap.BlockLength;
        DWORD dwRemain = cap.NumberOfBlocks.LowPart;
        for(DWORD dwPos = 0; dwPos < cap.NumberOfBlocks.LowPart; dwPos += dwBufBlocks, dwRemain -= dwBufBlocks)
        {
          DWORD dwLastTick = ::GetTickCount();

          // erase some blocks
          try {writeFile(hFile, format_buffer, ((dwRemain >= dwBufBlocks)?dwBufBlocks : dwRemain) * cap.BlockLength);}
          catch(AdfuException e) {println(e.what());}

          // update status
          fApproxTime += ((double)(::GetTickCount() - dwLastTick)) / 60000.0f;
          if((dwPos & 0x3FF) == 0)
          {
            UINT uApproxRemain = (UINT)((fApproxTime * dwRemain) / (dwPos+1));
            strText.Format(IDS_TIME_APPROX, uApproxRemain, uApproxRemain==1?L"":L"s");
            ::SetDlgItemText(hwndDlg, IDC_TXT_STATUS, strText);
            ::SetThreadExecutionState(ES_SYSTEM_REQUIRED);
          }
          ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, ((ULONG)(dwPos+1)*1024)/cap.NumberOfBlocks.LowPart, NULL);

          if(fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?
        }

        strText.LoadStringW(IDS_FORMAT_DONE);
        println(strText);
      }
      catch(AdfuException e)
      {
        println(e.what());
        strText.LoadStringW(IDS_FORMAT_FAILED);
        println(strText);
      }
      catch(...)
      {
        strText.LoadStringW(IDS_FORMAT_FAILED);
        println(strText);
      }

      if(hFile != INVALID_HANDLE_VALUE)
      {
        ::CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
      }

      ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 0, NULL);
      ::SetDlgItemText(hwndDlg, IDC_TXT_STATUS, L"");
    }
    if(fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?


    // open session
    strText.LoadStringW(IDS_DEVICE_OPEN);
    strText += lpDevDescr->g_strName.c_str();
    println(strText);
    session.open(*lpDevDescr);
    strText.LoadStringW(IDS_DEVICE_RECOVERY);
    if(session.isRecoveryMode()) println(strText);


    // init giveio
    gio.init();
    strText.LoadStringW(IDS_DEVICE_V9);
    if(gio.isV9Device()) println(strText);
    strText.Format(IDS_GIVEIO_VER, gio.version()>>4, gio.version()&15);
    println(strText);


    // read brom (twice for verification)
    FILE_HEADER hdr;
    ::ZeroMemory(&hdr, sizeof(hdr));

    gio.out(1, 0);
    gio.out(2, 0);
    gio.ld(0x8000, hdr.brom, sizeof(hdr.brom));

    gio.out(1, 0);
    gio.out(2, 0);
    uint8 brom[sizeof(hdr.brom)];
    gio.ld(0x8000, brom, sizeof(brom));
    if(memcmp(brom, hdr.brom, sizeof(brom)) != 0) {
      strText.LoadStringW(IDS_BROM_FAULT);
      println(strText);
    }


    // open flash chip
    strText.Format(IDS_FLASH_OPEN, nChipSel);
    println(strText);
    if(!fio.open(nChipSel))
    {
      strText.LoadStringW(IDS_FLASH_NOT_FOUND);
      println(strText);
      throw AdfuException(ERROR_OPERATION_ABORTED);
    }
    CStdString strDescr = fio.get_device_descr();
    strText.Format(IDS_FLASH_CHIP, fio.get_device_id(), strDescr, fio.is_protected());
    println(strText);


    // create file
    strText.LoadStringW(IDS_FILE_CREATE);
    strText += strFile;
    println(strText);
    hFile = ::CreateFile(strFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE) throw AdfuException(::GetLastError());


    // fillup header and write
    hdr.signature = FILE_HEADER_SIGNATURE;
    hdr.flash_id = fio.get_device_id();
    hdr.flash_size = fio.get_pages();
    hdr.block_size = fio.get_block_size();
    hdr.page_size = FlashIO::REAL_PAGE_SIZE;

    writeFile(hFile, &hdr, sizeof(hdr));


    // read flash image
    strText.LoadStringW(IDS_FLASH_READ);
    println(strText);

    uint8 buf[FlashIO::REAL_PAGE_SIZE];
    uint8 enc[FlashIO::REAL_PAGE_SIZE*2];
    double fApproxTime = 0.0f;
    ULONG ulDataSize = 0;
    ULONG ulCompressedSize = sizeof(hdr);
    for(uint32 page = 0; page < fio.get_pages(); page++)
    {
      DWORD dwLastTick = ::GetTickCount();

      // read next page from flash
      for(int retry = 1; true; retry++)
      {
        memset(buf, -1, sizeof(buf));
        if(fio.read_page(page, 0, buf, sizeof(buf)) && fio.validateECC(buf)) break;

        if(retry > CLONEDEVICE_READ_RETRY)
        {
          strText.Format(IDS_FLASH_READ_FAULT, page);
          println(strText);
          break;
        }
      }
      ulDataSize += sizeof(buf);

      // encode page and write to disk
      uint32 enc_size = encodePage(enc, sizeof(enc), buf, sizeof(buf));
      if(enc_size > sizeof(enc)) throw AdfuException(ERROR_BUFFER_OVERFLOW);
      FILE_SECTION sec = {calcChkSum(buf, sizeof(buf)), enc_size};
      writeFile(hFile, &sec, sizeof(sec));
      writeFile(hFile, enc, enc_size);
      ulCompressedSize += sizeof(sec) + enc_size;

      // update status
      fApproxTime += ((double)(::GetTickCount() - dwLastTick)) / 60000.0f;
      if((page & 0x3FF) == 0)
      {
        UINT uApproxRemain = (UINT)((fApproxTime * (fio.get_pages() - page)) / (page+1));
        INT nCompressionRatio = 100 - (INT)((ulCompressedSize*100) / ulDataSize);
        if(nCompressionRatio < 0) nCompressionRatio = 0;
        strText.Format(IDS_TIME_APPROX_COMPR, uApproxRemain, uApproxRemain==1?L"":L"s", nCompressionRatio);
        ::SetDlgItemText(hwndDlg, IDC_TXT_STATUS, strText);
        ::SetThreadExecutionState(ES_SYSTEM_REQUIRED);
      }
      ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 
        ((ULONG)(page+1)*1024) / fio.get_pages(), NULL);

      if(fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?
    }

    if(fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?


    // done
    strText.Format(IDS_REPORT, (float)(ulDataSize/1048576.0f), (float)(ulCompressedSize/1048576.0f));
    println(strText);
    strText.LoadStringW(IDS_DONE);
    println(strText);

    ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 1024, NULL);
    ::SendMessage(hwndDlg, WM_USER, 0, ERROR_SUCCESS);  //success
  }
  catch(AdfuException &e)
  {
    println(e.what());
    ::SendMessage(hwndDlg, WM_USER, 0, e.id());  //error
  }

  ::SetDlgItemText(hwndDlg, IDC_TXT_STATUS, L"");

  // reset and close
  this->hThread = NULL;
  try {fio.close(); gio.reset(); session.close();} catch(...) {}
  if(hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
}




//-------------------------------------------------------------------------------------------------
DWORD WINAPI CloneDevice::writeThread(LPVOID lpParam)
{
  ((CloneDevice*)lpParam)->writeThread();
  ((CloneDevice*)lpParam)->hThread = NULL;
  return NULL;
}


//-------------------------------------------------------------------------------------------------
VOID CloneDevice::writeThread()
{
  CStdString strText;
  HANDLE hFile = INVALID_HANDLE_VALUE;

  AdfuSession session;
  GiveIO gio(session);
  FlashIO fio(gio);

  try
  {
    ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETRANGE, NULL, MAKELPARAM(0, 1024));
    ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 0, NULL);


    // open session
    strText.LoadStringW(IDS_DEVICE_OPEN);
    strText += lpDevDescr->g_strName.c_str();
    println(strText);
    session.open(*lpDevDescr);
    strText.LoadStringW(IDS_DEVICE_RECOVERY);
    if(session.isRecoveryMode()) println(strText);


    // init giveio
    gio.init();
    strText.LoadStringW(IDS_DEVICE_V9);
    if(gio.isV9Device()) println(strText);
    strText.Format(IDS_GIVEIO_VER, gio.version()>>4, gio.version()&15);
    println(strText);


    // read brom
    uint8 brom[SIZE_OF_BROM];
    gio.out(1, 0);
    gio.out(2, 0);
    gio.ld(0x8000, brom, sizeof(brom));


    // open flash chip
    strText.Format(IDS_FLASH_OPEN, nChipSel);
    println(strText);
    if(!fio.open(nChipSel))
    {
      strText.LoadStringW(IDS_FLASH_NOT_FOUND);
      println(strText);
      throw AdfuException(ERROR_OPERATION_ABORTED);
    }
    CStdString strDescr = fio.get_device_descr();
    strText.Format(IDS_FLASH_CHIP, fio.get_device_id(), strDescr, fio.is_protected());
    println(strText);


    // open file
    strText.LoadStringW(IDS_FILE_OPEN);
    strText += strFile;
    println(strText);
    hFile = ::CreateFile(strFile, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE) throw AdfuException(::GetLastError());


    // read and verify file header
    FILE_HEADER hdr;
    UINT uDifferences = 0;
    ::ZeroMemory(&hdr, sizeof(hdr));
    readFile(hFile, &hdr, sizeof(hdr));
    if(hdr.signature != FILE_HEADER_SIGNATURE) throw AdfuException(ERROR_UNSUPPORTED_TYPE);
    if(hdr.page_size != FlashIO::REAL_PAGE_SIZE) throw AdfuException(ERROR_UNSUPPORTED_TYPE);

    if(memcmp(brom, hdr.brom, sizeof(brom)) != 0)
    {
      uDifferences++;
      strText.LoadStringW(IDS_DIFF_BROM);
      println(strText);

      //do some debug output
      #ifdef _DEBUG
        FILE *fh;
        fopen_s(&fh, "brom.desired.bin", "wb");
        fwrite(hdr.brom, sizeof(hdr.brom), 1, fh);
        fclose(fh);
        fopen_s(&fh, "brom.actual.bin", "wb");
        fwrite(brom, sizeof(brom), 1, fh);
        fclose(fh);
      #endif
    }

    if(hdr.flash_id != fio.get_device_id())
    {
      uDifferences++;
      strText.LoadStringW(IDS_DIFF_FLASH_ID);
      println(strText);
    }

    if(hdr.flash_size != fio.get_pages())
    {
      uDifferences++;
      strText.LoadStringW(IDS_DIFF_FLASH_SIZE);
      println(strText);
    }

    if(hdr.block_size != fio.get_block_size())
    {
      uDifferences++;
      strText.LoadStringW(IDS_DIFF_FLASH_BLOCK_SIZE);
      println(strText);
    }


    // any differences in write-mode? ask user to stop operation
    if(fWrite && uDifferences > 0)
    {
      CStdString strCaption(MAKEINTRESOURCE(IDS_WARNING));
      CStdString strText(MAKEINTRESOURCE(IDS_DIFF_WARNING));
      if(::MessageBox(hwndDlg, strText, strCaption, MB_ICONEXCLAMATION|MB_YESNO) != IDYES)
        throw AdfuException(ERROR_OPERATION_ABORTED);
    }


    // if we are in write-mode, then format entire flash now
    if(fWrite)
    {
      strText.LoadStringW(IDS_FLASH_FORMAT);
      println(strText);

      for(uint32 page = 0; page < fio.get_pages(); page += fio.get_block_size())
      {
        if(!fio.erase_block(page))
        {
          strText.Format(IDS_FLASH_FORMAT_FAILED, page / fio.get_block_size());
          println(strText);
        }

        ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 
          ((ULONG)(page+1)*1024) / fio.get_pages(), NULL);
      }
    }


    // start to verify or write flash image
    ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 0, NULL);
    if(fWrite) strText.LoadStringW(fVerify? IDS_FLASH_WRITE_AND_VERIFY : IDS_FLASH_WRITE);
    else strText.LoadStringW(IDS_VERIFY);
    println(strText);

    uint8 buf[FlashIO::REAL_PAGE_SIZE];
    uint8 cmp[FlashIO::REAL_PAGE_SIZE];
    uint8 enc[FlashIO::REAL_PAGE_SIZE*2];
    double fApproxTime = 0.0f;
    for(uint32 page = 0; page < fio.get_pages(); page++)
    {
      DWORD dwLastTick = ::GetTickCount();

      // read and decode page
      FILE_SECTION sec;
      readFile(hFile, &sec, sizeof(sec));
      if(sec.size > sizeof(enc)) throw AdfuException(ERROR_INVALID_DATA);
      readFile(hFile, enc, sec.size);
      uint32 dec_size = decodePage(buf, sizeof(buf), enc, sec.size);
      if(dec_size != sizeof(buf)) throw AdfuException(ERROR_INVALID_DATA);
      if(calcChkSum(buf, sizeof(buf)) != sec.chksum)
      {
        strText.Format(IDS_CHKSUM_INVALID, page);
        println(strText);
      }


      // verify if flash was formatted successfully
      #ifdef _DEBUG
        uint8 prev[FlashIO::REAL_PAGE_SIZE];
        if(fWrite && fVerify)
        {
          for(int retry = 1; true; retry++)
          {
            memset(prev, -1, sizeof(prev));
            if(fio.read_page(page, 0, prev, sizeof(prev)) && fio.validateECC(cmp)) break;

            if(retry > CLONEDEVICE_READ_RETRY)
            {
              strText.Format(IDS_FLASH_READ_FAULT, page);
              println(strText);
              break;
            }
          }

          uint16 offs = 0;
          while(offs < sizeof(prev) && prev[offs] == 0xFF) offs++;
          if(offs < sizeof(prev))
          {
            strText.Format(L"error: page 0x%08X wasn't formatted.", page);
            println(strText);
          }
        }
      #endif


      // write page to flash (but skip initial 0xFFs)
      if(fWrite)
      {
        uint16 offs = 0;
        while(offs < sizeof(buf) && buf[offs] == 0xFF) offs++;
        if(offs < sizeof(buf))  //write the remaining data without the initial 0xFFs
        {
          if(!fio.write_page(page, offs, &buf[offs], sizeof(buf)-offs))
          {
            strText.Format(IDS_FLASH_WRITE_FAULT, page);
            println(strText);
          }
        }
      }

      //verify with flash
      if(fVerify)
      {
        for(int retry = 1; true; retry++)
        {
          memset(cmp, -1, sizeof(cmp));
          if(fio.read_page(page, 0, cmp, sizeof(cmp)) && fio.validateECC(cmp)) break;

          if(retry > CLONEDEVICE_READ_RETRY)
          {
            strText.Format(IDS_FLASH_READ_FAULT, page);
            println(strText);
            break;
          }
        }

        if(memcmp(buf, cmp, sizeof(buf)) != 0)
        {
          strText.Format(IDS_VERIFY_DIFF, page);
          println(strText);
          uDifferences++;

          //do some debug output
          #ifdef _DEBUG
            FILE *fh;
            char sz[256];
            if(fWrite)
            {
              sprintf_s<sizeof(sz)>(sz, "%08X.previous.bin", page);
              fopen_s(&fh, sz, "wb");
              fwrite(prev, sizeof(prev), 1, fh);
              fclose(fh);
            }
            sprintf_s<sizeof(sz)>(sz, "%08X.desired.bin", page);            
            fopen_s(&fh, sz, "wb");
            fwrite(buf, sizeof(buf), 1, fh);
            fclose(fh);
            sprintf_s<sizeof(sz)>(sz, "%08X.actual.bin", page);
            fopen_s(&fh, sz, "wb");
            fwrite(cmp, sizeof(cmp), 1, fh);
            fclose(fh);
          #endif
          //
        }
      }

      // update status
      fApproxTime += ((double)(::GetTickCount() - dwLastTick)) / 60000.0f;
      if((page & 0x3FF) == 0)
      {
        UINT uApproxRemain = (UINT)((fApproxTime * (fio.get_pages() - page)) / (page+1));
        strText.Format(IDS_TIME_APPROX, uApproxRemain, uApproxRemain==1?L"":L"s");
        ::SetDlgItemText(hwndDlg, IDC_TXT_STATUS, strText);
        ::SetThreadExecutionState(ES_SYSTEM_REQUIRED);
      }
      ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 
        ((ULONG)(page+1)*1024) / fio.get_pages(), NULL);

      if(fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?
    }

    if(fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?


    // done
    strText.LoadStringW(IDS_DONE);
    println(strText);

    // any differences on verify? notify user
    if(fVerify && uDifferences > 0)
    {
      CStdString strCaption(MAKEINTRESOURCE(IDS_WARNING));
      strText.Format(IDS_VERIFY_DIFF_FIN, uDifferences);
      ::MessageBox(hwndDlg, strText, strCaption, MB_ICONEXCLAMATION|MB_OK);
    }

    ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 1024, NULL);
    ::SendMessage(hwndDlg, WM_USER, 0, ERROR_SUCCESS);  //success
  }
  catch(AdfuException &e)
  {
    println(e.what());
    ::SendMessage(hwndDlg, WM_USER, 0, e.id());  //error
  }

  ::SetDlgItemText(hwndDlg, IDC_TXT_STATUS, L"");

  // reset and close
  this->hThread = NULL;
  try {fio.close(); gio.reset(); session.close();} catch(...) {}
  if(hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
}








//-------------------------------------------------------------------------------------------------
uint16 CloneDevice::calcChkSum(LPBYTE lpBuf, DWORD dwSize)
{
  uint16 cs = 0;
  while(dwSize--) cs = ((cs&1)?0x8000:0) + (cs>>1) + *lpBuf++;
  return cs;
}


//-------------------------------------------------------------------------------------------------
// NOTE: for the worst-case, the destination buffer has to be 50% larger than the source buffer
//-------------------------------------------------------------------------------------------------
uint32 CloneDevice::encodePage(LPBYTE lpDest, DWORD dwDestSize, LPBYTE lpSrc, DWORD dwSrcSize)
{
  DWORD dwSize = 0;
  WORD nLastByte = -1;
  while(dwSrcSize--)
  {
    WORD nByte = *lpSrc++;
    if(dwDestSize) {*lpDest++ = (BYTE)nByte; dwDestSize--;}
    dwSize++;

    if(nLastByte == nByte)
    {
      DWORD dwRep = 0;
      while(dwSrcSize && nByte == (int)*lpSrc) {dwSrcSize--; lpSrc++; dwRep++;}
      while(dwRep >= 0x80)
      {
        if(dwDestSize) {*lpDest++ = (BYTE)dwRep | 0x80; dwDestSize--;}
        dwSize++;
        dwRep >>= 7;
      }
      if(dwDestSize) {*lpDest++ = (BYTE)(dwRep & 0x7F); dwDestSize--;}
      dwSize++;
      nByte = -1;
    }

    nLastByte = nByte;
  }
  return dwSize;
}


//-------------------------------------------------------------------------------------------------
uint32 CloneDevice::decodePage(LPBYTE lpDest, DWORD dwDestSize, LPBYTE lpSrc, DWORD dwSrcSize)
{
  DWORD dwSize = 0;
  WORD nLastByte = -1;
  while(dwSrcSize--)
  {
    WORD nByte = *lpSrc++;
    if(dwDestSize) {*lpDest++ = (BYTE)nByte; dwDestSize--;}
    dwSize++;

    if(nLastByte == nByte)
    {
      DWORD dwRep = 0;
      for(DWORD dwShift = 0; dwSrcSize; dwShift+=7)
      {
        dwRep |= ((DWORD)*lpSrc & 0x7F) << dwShift;
        dwSrcSize--;
        if((*lpSrc++ & 0x80) == 0) break;
      }
      while(dwRep--)
      {
        if(dwDestSize) {*lpDest++ = (BYTE)nByte; dwDestSize--;}
        dwSize++;
      }
      nByte = -1;
    }

    nLastByte = nByte;
  }
  return dwSize;
}
