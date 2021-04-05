// 232usb Copyright (c) 03-04 Zoroyoshi, Japan
// See source.txt for detail

#include <windows.h>
#include "usbdi.h"
#include "common.h"
#include "file.h"
#include "device.h"
#include "cdc.h"

int cdc_extension::
classissue()
{
  if(classut) return -1; //issued
  int pkt= classendp->Descriptor.wMaxPacketSize&0x7ff;
  classut= uf->lpIssueInterruptTransfer(classpipe, notifyevent, serialevent
  , USB_IN_TRANSFER|USB_SHORT_TRANSFER_OK, pkt, classbuf, 0);
  //zmesg(ZM_USB,L"  classissue=x%x\n", classut);
  if(classut==0) { //?error
    zmesg(ZM_ERROR,L"  classissue err=%d\n", GetLastError());
    return 0;
  };
  return 1;
};

int cdc_extension::
classwait()
{
  if(classut==0) return 0; //nothing queued
  DWORD rc= ~0;
  DWORD len= 0;
  int rt= uf->lpIsTransferComplete(classut);
  if(uf->lpGetTransferStatus(classut, &len, &rc)) {
    if(rt==0) return 0; //waiting
    uf->lpCloseTransfer(classut);
  };
  zmesg(ZM_USB,L"  classwait=x%x len=%d\n", classut, len);
  classut= 0;
  if(rc!=0) { //?error
    zmesg(ZM_ERROR,L"  classwait=x%x rc=%d\n", classut, rc);
  };
  if(len==10&&classbuf[0]==0xa1&&classbuf[1]==0x20) { //SERIAL_STATE notify
    DWORD state= classbuf[8]|classbuf[9]<<8;
    DWORD err;
    err= 0;
    if(state&0x10) err|= CE_FRAME;
    if(state&0x20) err|= CE_RXPARITY;
    if(state&0x40) err|= CE_OVERRUN;
    if(state&4) err|= CE_BREAK;
    DWORD in;
    in= state>>3&MS_CTS_ON|state<<4&MS_DSR_ON|state<<3&MS_RING_ON|state<<7&MS_RLSD_ON|state>>2&1;
    if(!(usbmode&MODE_2303)) in|= MS_CTS_ON;
    linestatein(in, err);
  } else { //other ignored
  };
  return 1;
};


int cdc_extension::
dispatchdo()
{
  int rt;
  rt= device_extension::dispatchdo();
  classwait();
  classissue();
  if(rt==0&&classut==0) return 0;
  return 1;
};

BOOL cdc_extension::
init()
{
  zmesg(ZM_DRIVER,L"cdc::init\n");
  if(classendp==0||recvendp==0||sendendp==0) return FALSE;

  if(device_extension::init()==0) return FALSE;

  lineout= 0;
  classut= 0;

  classpipe= uf->lpOpenPipe(uh, &classendp->Descriptor);

  USB_TRANSFER ut;
  ut= uf->lpClearFeature(uh, 0, 0
  , USB_SEND_TO_ENDPOINT, USB_FEATURE_ENDPOINT_STALL, classendp->Descriptor.bEndpointAddress);
  if(ut) uf->lpCloseTransfer(ut);

  USB_DEVICE_REQUEST req;
  WORD abs;

  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_CLASS|USB_REQUEST_FOR_INTERFACE;
  req.bRequest= 0x2; //SET_COMM_FEATURE
  req.wValue= 1; req.wIndex= classif; req.wLength= sizeof(abs);
  abs= 2; //enable multiplexing
  ut= uf->lpIssueVendorTransfer(uh, 0, 0
  , USB_SEND_TO_INTERFACE|USB_OUT_TRANSFER
  , &req, (BYTE*)&abs, 0);
  if(ut) uf->lpCloseTransfer(ut);

  return TRUE;
};

BOOL cdc_extension::
deinit()
{
  uf->lpClosePipe(classpipe); classpipe= 0;
  device_extension::deinit();

  return TRUE;
};

USB_TRANSFER cdc_extension::
issuebreak(int code)
{
  USB_TRANSFER ut;
  USB_DEVICE_REQUEST req;
  int duration= 0;
  if(code) duration=0xffff;
  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_CLASS|USB_REQUEST_FOR_INTERFACE;
  req.bRequest= 0x23; //SEND_BREAK;
  req.wValue= duration; req.wIndex= classif; req.wLength= 0;
  ut= uf->lpIssueVendorTransfer(uh, 0, 0
  , USB_SEND_TO_INTERFACE|USB_OUT_TRANSFER
  , &req, 0, 0);
  zmesg(ZM_USB,L"  BREAK(x%x) ut=x%x\n", duration, ut);
  if(ut) uf->lpCloseTransfer(ut);
  return(0); //dummy
};

USB_TRANSFER cdc_extension::
issuelinestate(int nandcode, int orcode, HANDLE event)
{
  USB_TRANSFER ut;
  USB_DEVICE_REQUEST req;
  HANDLE ev= event;
  if(event==0) {
    ev= CreateEvent(0, 0, 0, 0);
    EnterCriticalSection(&flowcs);
  };
  lineout= lineout&~nandcode|orcode;
  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_CLASS|USB_REQUEST_FOR_INTERFACE;
  req.bRequest= 0x22; //SET_CONTROL_LINE_STATE
  req.wValue= (WORD)lineout; req.wIndex= classif; req.wLength= 0;
  ut= uf->lpIssueVendorTransfer(uh, notifyevent, ev
  , USB_SEND_TO_INTERFACE|USB_OUT_TRANSFER
  , &req, 0, 0);
  if(event==0) {
    LeaveCriticalSection(&flowcs);
    if(ut) {
      WaitForSingleObject(ev, INFINITE);
      uf->lpCloseTransfer(ut);
      ut= 0;
    };
    CloseHandle(ev);
  };
  zmesg(ZM_USB,L"  LINE_STATE(x%x) ut=x%x\n", lineout, ut);
  return(ut);
};

BOOL cdc_extension::
applydcb()
{
  device_extension::applydcb();
  if(usbmode&MODE_NOBAUD) return TRUE;
  USB_TRANSFER ut;
  USB_DEVICE_REQUEST req;
  BYTE buf[7];
  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_CLASS|USB_REQUEST_FOR_INTERFACE;
  req.bRequest= 0x20; //SET_LINE_CODING
  req.wValue= 0; req.wIndex= classif; req.wLength= 7;
  *(DWORD*)buf= dcb.BaudRate;
  buf[4]= dcb.StopBits;
  buf[5]= dcb.Parity;
  buf[6]= dcb.ByteSize;
  ut= uf->lpIssueVendorTransfer(uh, 0, 0
  , USB_SEND_TO_INTERFACE|USB_OUT_TRANSFER
  , &req, buf, 0);
  zmesg(ZM_USB,L"  APPLYDCB ut=x%x\n", ut);
  if(ut) uf->lpCloseTransfer(ut);
  return TRUE;
};
