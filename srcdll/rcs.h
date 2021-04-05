#pragma once

#include "device.h"

struct rcs_extension
:device_extension
{
  virtual BOOL init();
  virtual BOOL deinit();
  virtual USB_TRANSFER issuesend(BYTE*, DWORD);
  virtual USB_TRANSFER issuechar(BYTE*);
  virtual USB_TRANSFER issuexon(BYTE*);
  virtual int sentlen(DWORD*);
};
