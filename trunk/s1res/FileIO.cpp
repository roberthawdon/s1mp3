//=================================================================================================
// FileIO.cpp : access firmware and resource files (AFI, FWI, RES)
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

#include "../common/s1def.h"
#include "../common/Sum16.h"
#include "../common/Sum32.h"

#include "FileIO.h"
#include "ResItem.h"
#include "ResItemImage.h"
#include "ResItemString.h"


//-------------------------------------------------------------------------------------------------
FileIO::FileIO()
{
  lpRootItem = NULL;
  lpData = NULL;
  dwDataSize = 0;
  fAltered = false;
}

FileIO::~FileIO()
{
  close();
}


//-------------------------------------------------------------------------------------------------
VOID FileIO::close()
{
  SAFE_DELETE(lpRootItem)
  SAFE_DELETE(lpData)
  dwDataSize = 0;
  fAltered = false;
}


//-------------------------------------------------------------------------------------------------
// load file with other data
//-------------------------------------------------------------------------------------------------
BOOL FileIO::load(LPBYTE lpData, DWORD dwDataSize)
{
  close();
  if(!lpData || !dwDataSize)
  {
    ::SetLastError(ERROR_INVALID_PARAMETER);
    return false;
  }

  this->dwDataSize = dwDataSize;
  this->lpData = new BYTE[dwDataSize];
  ::CopyMemory(this->lpData, lpData, dwDataSize);

  return parse();
}


//-------------------------------------------------------------------------------------------------
// load file
//-------------------------------------------------------------------------------------------------
BOOL FileIO::load(LPCWSTR lpFileName)
{
  close();

  DWORD dwRet = 0;
  BY_HANDLE_FILE_INFORMATION finf;
  HANDLE hFile = CreateFile(lpFileName, GENERIC_READ, 
    FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if(hFile == INVALID_HANDLE_VALUE) return false;
  if(!GetFileInformationByHandle(hFile, &finf)) {CloseHandle(hFile); return false;}
  if(finf.nFileSizeHigh) {CloseHandle(hFile); SetLastError(ERROR_NOT_SUPPORTED); return false;}
  dwDataSize = finf.nFileSizeLow;
  lpData = new BYTE[dwDataSize];
  if(!ReadFile(hFile, lpData, dwDataSize, &dwRet, NULL)) {CloseHandle(hFile); return false;}
  if(dwRet != dwDataSize) {CloseHandle(hFile); SetLastError(ERROR_READ_FAULT); return false;}
  CloseHandle(hFile);

  return parse();
}


//-------------------------------------------------------------------------------------------------
// store file
//-------------------------------------------------------------------------------------------------
BOOL FileIO::store(LPCWSTR lpFileName)
{
  if(!lpData || !dwDataSize) {::SetLastError(ERROR_INVALID_DATA); return false;}
  if(!update()) return false;

  DWORD dwRet = 0;
  HANDLE hFile = CreateFile(lpFileName, GENERIC_WRITE, 
    FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if(hFile == INVALID_HANDLE_VALUE) return false;
  if(!WriteFile(hFile, lpData, dwDataSize, &dwRet, NULL)) {CloseHandle(hFile); return false;}
  if(dwRet != dwDataSize) {CloseHandle(hFile); SetLastError(ERROR_WRITE_FAULT); return false;}
  CloseHandle(hFile);
  
  return true;
}




//-------------------------------------------------------------------------------------------------
// parse the root item
//-------------------------------------------------------------------------------------------------
BOOL FileIO::parse()
{
  SAFE_DELETE(lpRootItem)

  if(isAFI(lpData, dwDataSize))
  {
    CStdString strName(MAKEINTRESOURCE(IDS_TVROOT_AFI));
    lpRootItem = new ResItem(ResItem::TYPE_FILE_AFI, strName, lpData, dwDataSize);
  }
  else if(isFW(lpData, dwDataSize))
  {
    CStdString strName(MAKEINTRESOURCE(IDS_TVROOT_FW));
    lpRootItem = new ResItem(ResItem::TYPE_FILE_FW, strName, lpData, dwDataSize);
  }
  else if(isRES(lpData, dwDataSize))
  {
    CStdString strName(MAKEINTRESOURCE(IDS_TVROOT_RES));
    lpRootItem = new ResItem(ResItem::TYPE_FILE_RES, strName, lpData, dwDataSize);
  }
  else
  {
    SetLastError(ERROR_UNSUPPORTED_TYPE);  //ERROR_DATATYPE_MISMATCH, ERROR_INVALID_DATA
    return false;
  }

  if(!parse(lpRootItem)) return false;

  if(!lpRootItem->hasChilds())
  {
    SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
    return false;
  }

  return true;
}


//-------------------------------------------------------------------------------------------------
// recursive function to parse the given item for any further resource items
//-------------------------------------------------------------------------------------------------
// HINT: could be optimized if we first push the new ResItem-object on the list and 
//       work with the ((ResItem*)list->back()) pointer instead ((ResItem)newItem)
//-------------------------------------------------------------------------------------------------
BOOL FileIO::parse(ResItem *lpItemParent)
{
  BOOL fResult = true;

  switch(lpItemParent->getType())
  {
  case ResItem::TYPE_FILE_AFI:
    {
      LP_AFI_HEAD lpHead = (LP_AFI_HEAD)lpItemParent->getData();
      for(UINT n=0; n < AFI_HEAD_ENTRIES; n++) if(lpHead->entry[n].filename[0])
      {
        CStdString strName;
        convertFilename(strName, lpHead->entry[n].filename, lpHead->entry[n].extension);
        LPBYTE lpData = &lpItemParent->getData()[ lpHead->entry[n].fofs ];
        DWORD dwDataSize = lpHead->entry[n].fsize;
        ResItem::TYPE nType =
          !lpItemParent->validate(lpData, dwDataSize)? ResItem::TYPE_INVALID_DATA :
          isFW(lpData, dwDataSize)? ResItem::TYPE_FILE_FW :
          isRES(lpData, dwDataSize)? ResItem::TYPE_FILE_RES :
          isBREC(lpData, dwDataSize)? ResItem::TYPE_FILE_BREC :
          ResItem::TYPE_UNKNOWN;

        ResItem *lpNewItem = new ResItem(nType, strName, lpData, dwDataSize);
        if(!parse(lpNewItem) | !lpItemParent->add(lpNewItem)) fResult = false;
      }
    }
    break;

  case ResItem::TYPE_FILE_FW:
    {
      LP_FW_HEAD lpHead = (LP_FW_HEAD)lpItemParent->getData();
      for(UINT n=0; n < FW_HEAD_ENTRIES; n++) if(lpHead->entry[n].filename[0])
      {
        CStdString strName;
        convertFilename(strName, lpHead->entry[n].filename, lpHead->entry[n].extension);
        LPBYTE lpData = &lpItemParent->getData()[ ((DWORD)lpHead->entry[n].fofs_s9)<<9 ];
        DWORD dwDataSize = lpHead->entry[n].fsize;
        ResItem::TYPE nType =
          !lpItemParent->validate(lpData, dwDataSize)? ResItem::TYPE_INVALID_DATA :
          isRES(lpData, dwDataSize)? ResItem::TYPE_FILE_RES :
          isFONT8(&lpHead->entry[n])? ResItem::TYPE_FILE_FONT8 :
          isFONT16(&lpHead->entry[n])? ResItem::TYPE_FILE_FONT16 :
          ResItem::TYPE_UNKNOWN;

        ResItem *lpNewItem;
        switch(nType)
        {
        case ResItem::TYPE_FILE_FONT8:
          lpNewItem = new ResItemImage(nType, strName, lpData, dwDataSize,
            lpData, dwDataSize, 8*64, (dwDataSize+64-1)/64, ResItemImage::IMGFMT_FONT8);
          break;
        case ResItem::TYPE_FILE_FONT16:
          lpNewItem = new ResItemImage(nType, strName, lpData, dwDataSize,
            lpData, dwDataSize, 16*64, (dwDataSize+(2*64)-1)/(2*64), ResItemImage::IMGFMT_FONT16);
          break;
        default:
          lpNewItem = new ResItem(nType, strName, lpData, dwDataSize);
        }

        if(!parse(lpNewItem) | !lpItemParent->add(lpNewItem)) fResult = false;
      }
    }
    break;

  case ResItem::TYPE_FILE_RES:
    {
      LP_RES_HEAD lpHead = (LP_RES_HEAD)lpItemParent->getData();
      LP_RES_HEAD_ENTRY lpEntry = (LP_RES_HEAD_ENTRY)(&lpItemParent->getData()[sizeof(RES_HEAD)]);
      for(UINT n=0; n < lpHead->num_entries; n++) if(lpEntry[n].name[0]) 
      {
        CStdString strName;
        convertString(strName, lpEntry[n].name, 9);
        //strName.AppendFormat(_T(" [%u]"), n);
        LPBYTE lpData = &lpItemParent->getData()[ lpEntry[n].fofs ];
        DWORD dwDataSize = lpEntry[n].size;
        ResItem::TYPE nType = ResItem::TYPE_UNKNOWN;

        if(!lpItemParent->validate(lpData, dwDataSize)) nType = ResItem::TYPE_INVALID_DATA;
        else switch(lpEntry[n].type)
        {
        case RES_ENTRY_TYPE_ICON:
          if(dwDataSize >= sizeof(RES_ICON_HEAD))
          {
            // autodetect "real" icon size
            // and support OLED icons larger than 65535 bytes (pto 256kb)
            ResItemImage::IMGFMT nImgFmt = ResItemImage::IMGFMT_MONO;
            LP_RES_ICON_HEAD lpIconHead = (LP_RES_ICON_HEAD)lpData;
            DWORD dwSizeOLED = lpIconHead->width*lpIconHead->height*2 + 4;
            DWORD dwSizeMONO = lpIconHead->width*((lpIconHead->height+7)/8) + 4;
            if(dwSizeOLED > 0xFFFF && dwSizeMONO > dwDataSize)
            {
              nImgFmt = ResItemImage::IMGFMT_OLED;  //we detected a large OLED resource
              dwDataSize = dwSizeOLED;              //and have to adjust the resource size
            }
            else if(dwSizeOLED <= dwDataSize)
            {
              nImgFmt = ResItemImage::IMGFMT_OLED;
            }

            // create image resource (additional resource data)
            if(lpItemParent->validate(lpData, dwDataSize) && dwDataSize >= sizeof(RES_ICON_HEAD))
            {              
              if(!lpItemParent->add(
                new ResItemImage(
                  ResItem::TYPE_ICON, strName, lpData, dwDataSize,
                  &lpData[ sizeof(RES_ICON_HEAD) ], dwDataSize - sizeof(RES_ICON_HEAD),
                  lpIconHead->width, lpIconHead->height, nImgFmt ))) fResult = false;
              continue;
            }
          }
          nType = ResItem::TYPE_INVALID_DATA;
          break;

        case RES_ENTRY_TYPE_LOGO:
          if(dwDataSize >= 128*64/8)
          {
            if(!lpItemParent->add(
              new ResItemImage(
                ResItem::TYPE_LOGO, strName, lpData, dwDataSize,
                lpData, dwDataSize, 128, 64, ResItemImage::IMGFMT_MONO ))) fResult = false;
            continue;
          }
          else if(dwDataSize >= 128*32/8)
          {
            if(!lpItemParent->add(
              new ResItemImage(
                ResItem::TYPE_LOGO, strName, lpData, dwDataSize,
                lpData, dwDataSize, 128, 32, ResItemImage::IMGFMT_MONO ))) fResult = false;
            continue;
          }
          nType = ResItem::TYPE_INVALID_DATA;
          break;

        case RES_ENTRY_TYPE_MSTRING:
          {
            if(!lpItemParent->add(
              new ResItemString(
                ResItem::TYPE_MSTRING, strName, lpData, dwDataSize ))) fResult = false;
            continue;
          }

        case RES_ENTRY_TYPE_STRING:
          {
            if(!lpItemParent->add(
              new ResItemString(
                ResItem::TYPE_STRING, strName, lpData, dwDataSize ))) fResult = false;
            continue;
          }

        default:
          nType = ResItem::TYPE_DATA;
        }

        // add default resource item
        if(!lpItemParent->add(
          new ResItem(
            nType, strName, lpData, dwDataSize ))) fResult = false;
      }
    }
    break;

  //case ResItem::TYPE_FILE_FONT8:
  //  {
  //  }
  //  break;

  //case ResItem::TYPE_FILE_FONT16:
  //  {
  //  }
  //  break;
  }

  return fResult;
}




//-------------------------------------------------------------------------------------------------
// update data (write-back) before file gets saved
//-------------------------------------------------------------------------------------------------
BOOL FileIO::update()
{
  return update(lpRootItem);
}

//-------------------------------------------------------------------------------------------------
BOOL FileIO::update(ResItem *lpItemParent)
{
  BOOL fResult = true;
  if(!lpItemParent) return true;

  // step through all child resource items (recursive)
  if(lpItemParent->hasChilds())
  {
    FOREACH(*lpItemParent->getList(), std::list<ResItem*>, i)
      if(!update(*i)) fResult = false;
  }

  // update checksums...
  switch(lpItemParent->getType())
  {
  case ResItem::TYPE_FILE_AFI:
    if(isAFI(lpItemParent->getData(), lpItemParent->getDataSize()))
    {
      LP_AFI_HEAD lpAFI = (LP_AFI_HEAD)lpItemParent->getData();
      for(int n = 0; n < AFI_HEAD_ENTRIES; n++) if(lpAFI->entry[n].filename[0])
        if((lpAFI->entry[n].fofs + lpAFI->entry[n].fsize) <= lpItemParent->getDataSize())
          lpAFI->entry[n].sum32 = Sum32().generate(&lpItemParent->getData()[lpAFI->entry[n].fofs], lpAFI->entry[n].fsize);
      lpAFI->eoh.sum32 = Sum32().generate(lpAFI, sizeof(AFI_HEAD)-4);
    }
    break;

  case ResItem::TYPE_FILE_FW:
    if(isFW(lpItemParent->getData(), lpItemParent->getDataSize()))
    {
      LP_FW_HEAD lpFW = (LP_FW_HEAD)lpItemParent->getData();
      for(int n = 0; n < FW_HEAD_ENTRIES; n++) if(lpFW->entry[n].filename[0]) {
        if(((((DWORD)lpFW->entry[n].fofs_s9) << 9) + lpFW->entry[n].fsize) <= lpItemParent->getDataSize()) {
          lpFW->entry[n].sum32 = Sum32().generate(&lpItemParent->getData()[((DWORD)lpFW->entry[n].fofs_s9) << 9], lpFW->entry[n].fsize);
        }
      }
      lpFW->sum32 = Sum32().generate(&lpFW->entry, sizeof(lpFW->entry));
      lpFW->sum16 = Sum16().generate(lpFW, 0x1FE);
    }
    break;
  }

  return fResult;
}




//-------------------------------------------------------------------------------------------------
// some utils
//-------------------------------------------------------------------------------------------------
BOOL FileIO::isAFI(LPBYTE lpb, DWORD dwSize)
{
  if(dwSize < sizeof(AFI_HEAD)) return false;
  return(((LP_AFI_HEAD)lpb)->id == AFI_HEAD_ID);
}

BOOL FileIO::isFW(LPBYTE lpb, DWORD dwSize)
{
  if(dwSize < sizeof(FW_HEAD)) return false;
  return(((LP_FW_HEAD)lpb)->id == FW_HEAD_ID);
}

BOOL FileIO::isRES(LPBYTE lpb, DWORD dwSize)
{
  if(dwSize < sizeof(RES_HEAD)) return false;
  if(((LP_RES_HEAD)lpb)->id != RES_HEAD_ID) return false;
  return(dwSize >= ( sizeof(RES_HEAD) + 
    sizeof(RES_HEAD_ENTRY) * ((LP_RES_HEAD)lpb)->num_entries ));
}

BOOL FileIO::isBREC(LPBYTE lpb, DWORD dwSize)
{
  return (dwSize == 0x4000)&&(memcmp(&lpb[4], "BREC", 4) == 0);
}

BOOL FileIO::isFONT8(LP_FW_HEAD_ENTRY lpe)
{
  return (::_strnicmp(lpe->filename, "ASCII   ", 8) == 0
    && ::_strnicmp(lpe->extension, "BIN", 3) == 0
    && lpe->fsize >= 8 && (lpe->fsize % 8) == 0);
}

BOOL FileIO::isFONT16(LP_FW_HEAD_ENTRY lpe)
{
  return (::_strnicmp(lpe->filename, "FONT", 4) == 0
    && ::strncmp(lpe->extension, "$$$", 3) == 0
    && lpe->fsize >= 16 && (lpe->fsize % 16) == 0);
}


VOID FileIO::convertString(CStdString &strName, const char *lpStr, INT nLength)
{
  strName.clear();
  for(INT n=0; (n < nLength) && (lpStr[n] > 31) && (lpStr[n] < 127); n++) strName += lpStr[n];
}

VOID FileIO::convertFilename(CStdString &strName, const char *lpFile8, const char *lpExt3)
{
  strName.clear();
  for(INT n=0; (n < 8) && (lpFile8[n] > 32) && (lpFile8[n] < 127); n++) strName += lpFile8[n];
  strName += '.';
  for(INT n=0; (n < 3) && (lpExt3[n] >32) && (lpExt3[n] < 127); n++) strName += lpExt3[n];
}
