#pragma once

#include "device.h"

struct cdc_extension
:device_extension
{
  virtual BOOL init();
  virtual BOOL deinit();
  virtual BOOL applydcb();
  virtual USB_TRANSFER issuelinestate(int, int, HANDLE);
  virtual USB_TRANSFER issuebreak(int);
  virtual int dispatchdo();

  int classissue();
  int classwait();

  DWORD lineout;

  USB_PIPE classpipe;
  USB_TRANSFER classut;
  BYTE classbuf[64];

};
