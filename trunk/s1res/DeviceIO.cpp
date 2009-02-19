//=================================================================================================
// DeviceIO.cpp : access the s1mp3 player and read/write the firmware to/from flash
//
// author   wiRe <http://www.w1r3.de/>
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2009 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#include "common.h"
#include "resource.h"
#include "DeviceIO.h"

#include "../common/s1def.h"
#include "../common/FlashIO.h"
#include "../common/Sum16.h"
#include "../common/Sum32.h"


//-------------------------------------------------------------------------------------------------
DeviceIO::DeviceIO(void)
{
  hThread = NULL;
  lpBREC = lpFW = NULL;
  dwBRECSize = dwFWSize = 0;
}


//-------------------------------------------------------------------------------------------------
DeviceIO::~DeviceIO(void)
{
  free();
}


//-------------------------------------------------------------------------------------------------
BOOL DeviceIO::displayDeviceList(HWND hwndListCtrl)
{
  ::SendMessage(hwndListCtrl, LB_RESETCONTENT, 0, 0);

  FOREACH(this->lstDevDescr, std::list<AdfuSession::DeviceDescriptor>, i)
  {
    ::SendMessageA(hwndListCtrl, LB_ADDSTRING, 0, (LPARAM)i->g_strName.c_str());
  }

  ::SendMessage(hwndListCtrl, LB_SETCURSEL, 0, 0);
  return true;
}


//-------------------------------------------------------------------------------------------------
BOOL DeviceIO::updateDeviceList(HWND hwndDlg)
{
  if(this->hThread)
  {
    SetLastError(ERROR_BUSY);
    return false;
  }

  this->fCancelThread = false;
  this->hwndDlg = hwndDlg;
  this->hThread = ::CreateThread(NULL, 0, updateDeviceListThread, (LPVOID)this, 0, NULL);
  return this->hThread != NULL;
}




//-------------------------------------------------------------------------------------------------
DWORD WINAPI DeviceIO::updateDeviceListThread(LPVOID lpParam)
{
  ((DeviceIO*)lpParam)->updateDeviceListThread();
  ((DeviceIO*)lpParam)->hThread = NULL;
  return NULL;
}


//-------------------------------------------------------------------------------------------------
VOID DeviceIO::updateDeviceListThread()
{
  try
  {
    AdfuSession session;
    session.enumerate(this->lstDevDescr);
    if(::IsWindow(hwndDlg)) ::SendMessage(hwndDlg, WM_USER, 0, 0);  //success
  }
  catch(AdfuException &e)
  {
    if(::IsWindow(hwndDlg)) ::SendMessage(hwndDlg, WM_USER, 0, e.id());  //error
  }

  this->hThread = NULL;
}




//-------------------------------------------------------------------------------------------------
BOOL DeviceIO::read(HWND hwndDlg, INT nDevSel, INT nChipSel, BOOL fGiveIO)
{
  if(this->hThread) 
  {
    SetLastError(ERROR_BUSY);
    return false;
  }

  this->lpDevDescr = NULL;
  FOREACH(this->lstDevDescr, std::list<AdfuSession::DeviceDescriptor>, i) if(nDevSel-- == 0)
  {
    this->lpDevDescr = &*i;
    break;
  }

  if(!this->lpDevDescr)
  {
    SetLastError(ERROR_DEVICE_REMOVED);
    return false;
  }

  this->fCancelThread = false;

  this->hwndDlg = hwndDlg;
  this->nChipSel = nChipSel;
  this->fGiveIO = fGiveIO;

  this->hThread = CreateThread(NULL, 0, readThread, (LPVOID)this, 0, NULL);
  return this->hThread != NULL;
}


//-------------------------------------------------------------------------------------------------
BOOL DeviceIO::write(HWND hwndDlg, INT nDevSel, INT nChipSel, BOOL fGiveIO)
{
  if(this->hThread) 
  {
    SetLastError(ERROR_BUSY);
    return false;
  }

  this->lpDevDescr = NULL;
  FOREACH(this->lstDevDescr, std::list<AdfuSession::DeviceDescriptor>, i) if(nDevSel-- == 0)
  {
    this->lpDevDescr = &*i;
    break;
  }

  if(!this->lpDevDescr)
  {
    SetLastError(ERROR_DEVICE_REMOVED);
    return false;
  }

  this->fCancelThread = false;

  this->hwndDlg = hwndDlg;
  this->nChipSel = nChipSel;
  this->fGiveIO = fGiveIO;

  this->hThread = CreateThread(NULL, 0, writeThread, (LPVOID)this, 0, NULL);
  return this->hThread != NULL;
}




//-------------------------------------------------------------------------------------------------
DWORD WINAPI DeviceIO::readThread(LPVOID lpParam)
{
  ((DeviceIO*)lpParam)->readThread();
  ((DeviceIO*)lpParam)->hThread = NULL;
  return NULL;
}


//-------------------------------------------------------------------------------------------------
VOID DeviceIO::readThread()
{
  CStdString strText;
  AdfuSession session;
  GiveIO gio(session);
  FlashIO fio(gio);  

  FWIOCB fwioCB(::GetDlgItem(this->hwndDlg, IDC_PROGRESS));

  try
  {
    ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETRANGE, NULL, MAKELPARAM(0, 100));
    ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 0, NULL);

    if(this->fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?


    // open session
    strText.LoadStringW(IDS_DIO_DEVICE_OPEN);
    strText += this->lpDevDescr->g_strName.c_str();
    println(strText);
    session.open(*this->lpDevDescr);
    strText.LoadStringW(IDS_DIO_DEVICE_RECOVERY);
    if(session.isRecoveryMode()) println(strText);

    if(this->fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?


    if(this->fGiveIO) //use giveio to read firmware
    {


      // init giveio
      gio.init();
      strText.LoadStringW(IDS_DIO_DEVICE_V9);
      if(gio.isV9Device()) println(strText);
      strText.Format(IDS_DIO_GIVEIO_VER, gio.version()>>4, gio.version()&15);
      println(strText);

      if(this->fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?


      // open flash chip
      strText.Format(IDS_DIO_FLASH_OPEN, this->nChipSel);
      println(strText);
      if(!fio.open(this->nChipSel))
      {
        strText.LoadStringW(IDS_DIO_FLASH_NOT_FOUND);
        println(strText);
        throw AdfuException(ERROR_OPERATION_ABORTED);
      }
      CStdString strDescr = fio.get_device_descr();
      strText.Format(IDS_DIO_FLASH_CHIP, fio.get_device_id(), strDescr, fio.is_protected());
      println(strText);

      if(this->fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?


      // read firmware
      strText.LoadStringW(IDS_DIO_READ);
      println(strText);

      free();
      FirmwareIO(fio).read(lpBREC, dwBRECSize, lpFW, dwFWSize, &fwioCB);


    }
    else //use adfu to read firmware
    {


      // read firmware
      strText.LoadStringW(IDS_DIO_READ);
      println(strText);

      if(session.isRecoveryMode())
      {
        strText.LoadStringW(IDS_DIO_ERROR_RECOVERY);
        println(strText);
      }
      else
      {
        free();
        FirmwareIO(session).read(lpBREC, dwBRECSize, lpFW, dwFWSize, &fwioCB);
      }


    }


    // done
    strText.LoadStringW(IDS_DIO_DONE);
    println(strText);

    try {fio.close(); gio.reset(); session.close();} catch(...) {}  // reset and close

    //::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 100, NULL);
    ::SendMessage(hwndDlg, WM_USER, 0, ERROR_SUCCESS);  //success
  }
  catch(AdfuException &e)
  {    
    try {fio.close(); gio.reset(); session.close();} catch(...) {}  // reset and close

    println(e.what());
    ::SendMessage(hwndDlg, WM_USER, 0, e.id());  //error
  }

  hThread = NULL;
}




//-------------------------------------------------------------------------------------------------
DWORD WINAPI DeviceIO::writeThread(LPVOID lpParam)
{
  ((DeviceIO*)lpParam)->writeThread();
  ((DeviceIO*)lpParam)->hThread = NULL;
  return NULL;
}


//-------------------------------------------------------------------------------------------------
VOID DeviceIO::writeThread()
{
  CStdString strText;

  AdfuSession session;
  GiveIO gio(session);
  FlashIO fio(gio);

  FWIOCB fwioCB(::GetDlgItem(this->hwndDlg, IDC_PROGRESS));

  try
  {
    ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETRANGE, NULL, MAKELPARAM(0, 100));
    ::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 0, NULL);

    if(this->fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?


    // open session
    strText.LoadStringW(IDS_DIO_DEVICE_OPEN);
    strText += this->lpDevDescr->g_strName.c_str();
    println(strText);
    session.open(*this->lpDevDescr);
    strText.LoadStringW(IDS_DIO_DEVICE_RECOVERY);
    if(session.isRecoveryMode()) println(strText);

    if(this->fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?


    if(this->fGiveIO) //use giveio to write firmware
    {


      // init giveio
      gio.init();
      strText.LoadStringW(IDS_DIO_DEVICE_V9);
      if(gio.isV9Device()) println(strText);
      strText.Format(IDS_DIO_GIVEIO_VER, gio.version()>>4, gio.version()&15);
      println(strText);

      if(this->fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?


      // open flash chip
      strText.Format(IDS_DIO_FLASH_OPEN, this->nChipSel);
      println(strText);
      if(!fio.open(this->nChipSel))
      {
        strText.LoadStringW(IDS_DIO_FLASH_NOT_FOUND);
        println(strText);
        throw AdfuException(ERROR_OPERATION_ABORTED);
      }
      CStdString strDescr = fio.get_device_descr();
      strText.Format(IDS_DIO_FLASH_CHIP, fio.get_device_id(), strDescr, fio.is_protected());
      println(strText);

      if(this->fCancelThread) throw AdfuException(ERROR_OPERATION_ABORTED);  // cancel thread?


      // write firmware
      strText.LoadStringW(IDS_DIO_WRITE);
      println(strText);

      if(!lpBREC || !dwBRECSize) {
        strText.LoadStringW(IDS_DIO_NO_BREC);
        println(strText);
      }

      if(!lpFW || !dwFWSize) {
        strText.LoadStringW(IDS_DIO_NO_FW);
        println(strText);
      }

      FirmwareIO(fio).write(lpBREC, dwBRECSize, lpFW, dwFWSize, &fwioCB);


    }
    else
    {


      // write firmware
      strText.LoadStringW(IDS_DIO_WRITE);
      println(strText);

      if(session.isRecoveryMode())
      {
        strText.LoadStringW(IDS_DIO_ERROR_RECOVERY);
        println(strText);
      }
      else
      {
        FirmwareIO(session).write(NULL, 0, lpFW, dwFWSize, &fwioCB);
      }


    }


    // done
    strText.LoadStringW(IDS_DIO_DONE);
    println(strText);

    try {fio.close(); gio.reset(); session.close();} catch(...) {}  // reset and close

    //::SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, 1024, NULL);
    ::SendMessage(hwndDlg, WM_USER, 0, ERROR_SUCCESS);  //success
  }
  catch(AdfuException &e)
  {
    println(e.what());
    try {fio.close(); gio.reset(); session.close();} catch(...) {}  // reset and close
    ::SendMessage(hwndDlg, WM_USER, 0, e.id());  //error
  }

  hThread = NULL;
}




//-------------------------------------------------------------------------------------------------
BOOL DeviceIO::displayFirmware(HWND hwndDlg)
{
  CStdString strText;

  this->hwndDlg = hwndDlg;
  ::SendDlgItemMessage(hwndDlg, IDC_LIST, LB_RESETCONTENT, 0, 0);

  if(lpBREC && dwBRECSize > 11)
  {
    strText.Format(IDS_FWI_FLASH_TYPE, lpBREC[8], lpBREC[9], lpBREC[10], lpBREC[11]);
    println(strText);
  }

  LP_FW_HEAD fwh = (LP_FW_HEAD)lpFW;
  if(lpFW && dwFWSize >= sizeof(uint32))
  {
    strText.Format(IDS_FWI_FILE_ID, fwh->id);
    println(strText);
  }

  if(!lpFW || dwFWSize < sizeof(FW_HEAD) || fwh->id != FW_HEAD_ID)
  {
    ::SetLastError(ERROR_INVALID_DATA);
    return false;
  }

  strText.Format(IDS_FWI_INVALID_HEADER_CHKSUM);
  if(fwh->sum16 != Sum16().generate(fwh, 0x1FE) || fwh->sum32 != Sum32().generate(((uint8*)fwh)+0x200, sizeof(FW_HEAD)-0x200))
    println(strText);

  strText.Format(IDS_FWI_VENDOR_ID, fwh->vendor_id); println(strText);
  strText.Format(IDS_FWI_PRODUCT_ID, fwh->product_id); println(strText);
  strText.Format(IDS_FWI_VERSION, fwh->version_id[0]>>4, fwh->version_id[0]&15, fwh->version_id[1]); println(strText);
  strText.Format(IDS_FWI_DATE, fwh->date.month, fwh->date.day, fwh->date.year[0], fwh->date.year[1]); println(strText);

  trunc(strText, fwh->info.general, 32); strText.Format(IDS_FWI_INFO, strText); println(strText);
  trunc(strText, fwh->info.manufacturer, 32); strText.Format(IDS_FWI_MANUFACTURER, strText); println(strText);
  trunc(strText, fwh->info.device, 32); strText.Format(IDS_FWI_DEVICE_NAME, strText); println(strText);
  trunc(strText, fwh->info.vendor, 8); strText.Format(IDS_FWI_USB_ATTR, strText); println(strText);
  trunc(strText, fwh->info.product, 16); strText.Format(IDS_FWI_USB_ID, strText); println(strText);
  trunc(strText, fwh->info.revision, 4); strText.Format(IDS_FWI_USB_REV, strText); println(strText);

  println(_T(""));

  CStdString strExt;
  CStdString strMissingData(MAKEINTRESOURCE(IDS_FWI_MISSING_DATA));
  CStdString strInvalidChksum(MAKEINTRESOURCE(IDS_FWI_INVALID_CHKSUM));
  for(int n=0; n<AFI_HEAD_ENTRIES; n++) if(fwh->entry[n].filename[0])
  {
    trunc(strText, fwh->entry[n].filename, 8);
    strText += _T('.');    
    trunc(strExt, fwh->entry[n].extension, 3);
    strText += strExt;
    if(dwFWSize < ( (((size_t)fwh->entry[n].fofs_s9)<<9) + fwh->entry[n].fsize )) strText += strMissingData;
    else if(Sum32().generate(((uint8*)fwh) + (((size_t)fwh->entry[n].fofs_s9)<<9), fwh->entry[n].fsize) != fwh->entry[n].sum32) strText += strInvalidChksum;
    println(strText);
  }

  return true;
}


//-------------------------------------------------------------------------------------------------
// setup AfiEntry using informations from the first equal-named firmware file
// the firmware offset could get adjustet by the dwFwOfs parameter, pointing to the fw-start.
//-------------------------------------------------------------------------------------------------
BOOL DeviceIO::setAfiEntryFromFirmware(LP_AFI_HEAD_ENTRY lpAfiEntry,
                                       LPBYTE lpFwData, DWORD dwFwOfs)
{
  if(!lpAfiEntry || !lpFwData) return false;
  LP_FW_HEAD lpFwHdr = (LP_FW_HEAD)lpFwData;

  for(int e=0; e < FW_HEAD_ENTRIES; e++)
  {
    if(_strnicmp(lpFwHdr->entry[e].filename, lpAfiEntry->filename, 8)==0
      && _strnicmp(lpFwHdr->entry[e].extension, lpAfiEntry->extension, 3)==0)
    {
      lpAfiEntry->fofs = dwFwOfs + (lpFwHdr->entry[e].fofs_s9<<9);
      lpAfiEntry->fsize = lpFwHdr->entry[e].fsize;
      lpAfiEntry->sum32 = Sum32().generate(
        &lpFwData[lpFwHdr->entry[e].fofs_s9<<9], lpFwHdr->entry[e].fsize);
      return true;
    }
  }

  return false;
}


//-------------------------------------------------------------------------------------------------
BOOL DeviceIO::getFirmware(LPBYTE &lpData, DWORD &dwDataSize)
{
  lpData = NULL;
  dwDataSize = 0;

  if(!lpFW || dwFWSize < sizeof(FW_HEAD))
  {
    ::SetLastError(ERROR_INVALID_DATA);
    return false;
  }

  // if we don't have a BREC then only return the FW-part
  if(!lpBREC || dwBRECSize < 16)
  {
    lpData = new BYTE[dwDataSize = dwFWSize];
    ::CopyMemory(lpData, lpFW, dwDataSize);
    return true;
  }

  // otherwise generate an AFI file by including the boot record
  // NOTE: this is not really necessary for s1res, but to do a firmware backup
  dwDataSize = sizeof(AFI_HEAD) + ((lpBREC)?dwBRECSize:0) + dwFWSize;
  lpData = new BYTE[dwDataSize];
  ::ZeroMemory(lpData, dwDataSize);

  // fill afi header...
  int i = 0;
  uint32 fofs = sizeof(AFI_HEAD);

  LP_AFI_HEAD lpAFI = (LP_AFI_HEAD)lpData;
  lpAFI->id = AFI_HEAD_ID;
  ::CopyMemory(&lpAFI->vendor_id, "wiRe", 4);
  ::CopyMemory(lpAFI->version_id, ((LP_FW_HEAD)lpFW)->version_id, sizeof(lpAFI->version_id));

  // add boot record...
  lpAFI->entry[i].content = 'B';
  lpAFI->entry[i].type = AFI_ENTRY_TYPE_BREC;
  memcpy(lpAFI->entry[i].desc, &lpBREC[8], 4);
  memcpy(lpAFI->entry[i].filename, "BREC", 4);
  memcpy(lpAFI->entry[i].filename+4, &lpBREC[8], 4);
  memcpy(lpAFI->entry[i].extension, "BIN", 3);
  lpAFI->entry[i].fofs = fofs;
  lpAFI->entry[i].fsize = (uint32)dwBRECSize;
  lpAFI->entry[i].sum32 = Sum32().generate(lpBREC, dwBRECSize);
  ::CopyMemory(&lpData[fofs], lpBREC, dwBRECSize);
  fofs += lpAFI->entry[i++].fsize;

  // add FWSC entry (dummy)
  lpAFI->entry[i].content = 'F';
  lpAFI->entry[i].type = AFI_ENTRY_TYPE_FWSC;
  memcpy(lpAFI->entry[i].desc, &lpBREC[8], 4);
  memcpy(lpAFI->entry[i].filename, "FWSC", 4);
  memcpy(lpAFI->entry[i].filename+4, &lpBREC[8], 4);
  memcpy(lpAFI->entry[i].extension, "BIN", 3);
  setAfiEntryFromFirmware(&lpAFI->entry[i], lpFW, fofs);
  i++;

  // add ADFU entry (dummy)
  if(((LP_FW_HEAD)lpFW)->version_id[0] < 0x90)  //only add for firmwares before v9
  {
    lpAFI->entry[i].content = 'U';
    lpAFI->entry[i].type = AFI_ENTRY_TYPE_ADFU;
    memcpy(lpAFI->entry[i].desc, &lpBREC[8], 4);
    memcpy(lpAFI->entry[i].filename, "ADFU", 4);
    memcpy(lpAFI->entry[i].filename+4, &lpBREC[8], 4);
    memcpy(lpAFI->entry[i].extension, "AP", 3);
    setAfiEntryFromFirmware(&lpAFI->entry[i], lpFW, fofs);
    i++;
  }

  // add ADFUS entry (dummy)
  lpAFI->entry[i].content = 'A';
  lpAFI->entry[i].type = AFI_ENTRY_TYPE_ADFUS;
  memcpy(lpAFI->entry[i].filename, "ADFUS   ", 8);
  memcpy(lpAFI->entry[i].extension, "BIN", 3);
  setAfiEntryFromFirmware(&lpAFI->entry[i], lpFW, fofs);
  i++;

  // add HWSCAN entry (dummy)
  lpAFI->entry[i].content = 'H';
  lpAFI->entry[i].type = AFI_ENTRY_TYPE_HWSCAN;
  memcpy(lpAFI->entry[i].filename, "HWSCAN  ", 8);
  memcpy(lpAFI->entry[i].extension, "BIN", 3);
  setAfiEntryFromFirmware(&lpAFI->entry[i], lpFW, fofs);
  i++;

  // add firmware data
  lpAFI->entry[i].content = 'I';
  lpAFI->entry[i].type = AFI_ENTRY_TYPE_FW;
  memcpy(lpAFI->entry[i].filename, "FWIMAGE ", 8);  
  memcpy(lpAFI->entry[i].extension, "FW ", 3);
  lpAFI->entry[i].fofs = fofs;
  lpAFI->entry[i].fsize = (uint32)dwFWSize;
  lpAFI->entry[i].sum32 = Sum32().generate(lpFW, dwFWSize);
  ::CopyMemory(&lpData[fofs], lpFW, dwFWSize);
  fofs += lpAFI->entry[i++].fsize;

  // calc afi checksum
  lpAFI->eoh.sum32 = Sum32().generate(lpAFI, sizeof(AFI_HEAD)-4);

  return true;
}

//-------------------------------------------------------------------------------------------------
BOOL DeviceIO::setFirmware(LPBYTE lpData, DWORD dwDataSize)
{
  free();   //free any previous firmware data

  if(!lpData || dwDataSize < 16) {
    ::SetLastError(ERROR_INVALID_PARAMETER);
    return false;
  }

  // if we only have a firmware file (no afi) then do only a copy of the fw-part
  if(dwDataSize >= sizeof(FW_HEAD) && ((LP_FW_HEAD)lpData)->id == FW_HEAD_ID) {
    lpFW = new BYTE[dwFWSize = dwDataSize];
    ::CopyMemory(lpFW, lpData, dwFWSize);
    return true;
  }

  // only allow afi files with own id instead
  LP_AFI_HEAD lpAFI = (LP_AFI_HEAD)lpData;
  if(dwDataSize < sizeof(AFI_HEAD) || lpAFI->id != AFI_HEAD_ID || ::memcmp(&lpAFI->vendor_id, "wiRe", 4) != 0)
  {
    ::SetLastError(ERROR_INVALID_DATA);
    return false;
  }

  // we have a valid afi part, now search BRECFxxx.BIN
  for(int i=0; i < AFI_HEAD_ENTRIES; i++)
    if( lpAFI->entry[i].content == 'B'
    && lpAFI->entry[i].type == AFI_ENTRY_TYPE_BREC
    && ::_strnicmp(lpAFI->entry[i].filename, "BREC", 4) == 0
    && ::_strnicmp(lpAFI->entry[i].extension, "BIN", 3) == 0
    && dwDataSize >= (lpAFI->entry[i].fofs + lpAFI->entry[i].fsize) )
  {
    lpBREC = new BYTE[ dwBRECSize = lpAFI->entry[i].fsize ];
    ::CopyMemory(lpBREC, &lpData[lpAFI->entry[i].fofs], dwBRECSize);
    break;
  }

  // and the same for FWIMAGE.FW
  for(int i=0; i < AFI_HEAD_ENTRIES; i++)
    if( lpAFI->entry[i].content == 'I'
    && lpAFI->entry[i].type == AFI_ENTRY_TYPE_FW
    && ::_strnicmp(lpAFI->entry[i].filename, "FWIMAGE ", 8) == 0
    && ::_strnicmp(lpAFI->entry[i].extension, "FW ", 3) == 0
    && dwDataSize >= (lpAFI->entry[i].fofs + lpAFI->entry[i].fsize) )
  {
    lpFW = new BYTE[ dwFWSize = lpAFI->entry[i].fsize ];
    ::CopyMemory(lpFW, &lpData[lpAFI->entry[i].fofs], dwFWSize);
    break;
  }

  return true;
}
