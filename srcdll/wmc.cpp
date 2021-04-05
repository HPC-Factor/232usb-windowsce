// 232usb Copyright (c) 03-04,06 Zoroyoshi, Japan
// See source.txt for detail

#include <windows.h>
#include "usbdi.h"
#include "common.h"
#include "file.h"
#include "device.h"
#include "cdc.h"
#include "wmc.h"

BOOL wmc_extension::
init()
{
  zmesg(ZM_DRIVER,L"wmc::init\n");
  if(obex.sendendp==0||obex.recvendp==0) return FALSE;
  if(cdc_extension::init()==0) return FALSE;

  obex.parent= this;
  obex.uh= uh;
  obex.uf= uf;
  obex.usbmode= (usbmode&~255)|MODE_BASIC;
  obex.priority= priority;
  obex.recvsize= recvsize; //same as parent
  obex.classif= 0;
  obex.classendp= 0;
  //obex.recvendp, obex.sendendp are set by USBAttach

  obex.ud= 0;
  obex.initbase();

  return TRUE;
};

BOOL wmc_extension::
deinit()
{
  obex.deinitbase();
  cdc_extension::deinit();  
  return TRUE;
};

void wmc_extension::
unblock()
{
  cdc_extension::unblock();
  SetEvent(obex.readevent);
  SetEvent(obex.writeevent);
};

DWORD wmc_extension::
escapecomm(file_extension* fx, int func)
{
  if(parent) return parent->escapecomm(fx, func);
  if(func==CLRIR) {
    fx->dx= this;
    return TRUE;
  } else if(func==SETIR) {
    fx->dx= &obex;
    return TRUE;
  };
  return FALSE;
};
