#pragma once

#include "device.h"

//8u232am
struct ftdi_extension
:device_extension
{
  virtual BOOL init();
  virtual BOOL applydcb();
  virtual USB_TRANSFER issuelinestate(int, int, HANDLE);
  virtual USB_TRANSFER issuebreak(int);
  virtual int recvdata(UCHAR*, DWORD);
};

//old one...need data-length at the top of out packet
struct ft100_extension
:ftdi_extension
{
  virtual BOOL init();
  virtual BOOL applydcb();
  virtual USB_TRANSFER issuesend(BYTE*, DWORD);
  virtual USB_TRANSFER issuechar(BYTE*);
  virtual USB_TRANSFER issuexon(BYTE*);
  virtual int sentlen(DWORD*);
  USB_TRANSFER issuedata(BYTE*, BYTE*, DWORD, HANDLE);
  BYTE sendbuf[64];
  BYTE sendxonbuf[2];
};


