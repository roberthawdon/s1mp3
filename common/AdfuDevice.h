//=================================================================================================
// AdfuDevice
//
// descr.   adfu device interface (general interface for all AdfuDeviceXxx classes)
// author   wiRe _at_ gmx _dot_ net
// source   http://www.s1mp3.de/
//
// copyright (c)2005-2007 wiRe.
// this file is licensed under the GNU GPL.
// no warranty of any kind is explicitly or implicitly stated.
//=================================================================================================
#pragma once


//-------------------------------------------------------------------------------------------------
class AdfuDevice {
public:
  virtual void enumerate(std::list<std::string> &lstDevice) =0;
  virtual void open(std::string &strName) =0;
  virtual void close() =0;

  enum DIR {IO_READ, IO_WRITE};
  virtual unsigned int io(DIR nDir, const void *lpCdb, unsigned char uCdbLength,
                          void *lpData, unsigned int uDataLength, unsigned int uTimeout) =0;

  unsigned int write(const void *lpCdb, unsigned char uCdbLength, 
                     const void *lpData =NULL, unsigned int uDataLength =0, unsigned int uTimeout =3)
                     {return io(IO_WRITE, lpCdb, uCdbLength, (void*)lpData, uDataLength, uTimeout);}

  unsigned int read(const void *lpCdb, unsigned char uCdbLength, 
                    void *lpData =NULL, unsigned int uDataLength =0, unsigned int uTimeout =3)
                    {return io(IO_READ, lpCdb, uCdbLength, lpData, uDataLength, uTimeout);}
};
