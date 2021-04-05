#pragma once

struct wmc_extension
:cdc_extension
{
  device_extension obex; //obex port

  virtual BOOL init();
  virtual BOOL deinit();
  virtual void unblock();
  virtual DWORD escapecomm(file_extension*, int);
};
