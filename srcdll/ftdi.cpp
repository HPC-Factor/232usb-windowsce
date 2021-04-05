// 232usb Copyright (c) 03-04,06 Zoroyoshi, Japan
// See source.txt for detail

#include <windows.h>
#include "usbdi.h"
#include "common.h"
#include "file.h"
#include "device.h"
#include "ftdi.h"

int ftdi_extension::
recvdata(BYTE* src, DWORD len)
{
  //ftdi: every packet(64bytes) have two status header bytes
  while(len>=2) {
    //signal line
    DWORD err;
    err= src[1]&(CE_OVERRUN|CE_RXPARITY|CE_FRAME|CE_BREAK);
    DWORD in;
    in= src[0]&(MS_CTS_ON|MS_DSR_ON|MS_RING_ON|MS_RLSD_ON)|src[1]>>4&1;
    linestatein(in, err);
    //receive data
    DWORD sz= len-2;
    if(sz>=62) sz= 62;
    if(sz>0) device_extension::recvdata(src+2, sz);
    len-= (sz+2);
    src+= (sz+2);
  };
  return 1;
};

BOOL ftdi_extension::
init()
{
  if(recvendp==0||sendendp==0) return FALSE;

  if(device_extension::init()==0) return FALSE;

  USB_DEVICE_REQUEST req;
  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_VENDOR|USB_REQUEST_FOR_DEVICE;
  req.bRequest= 0x0; //SIO_RESET
  req.wValue= 0; req.wIndex= 0; req.wLength= 0;
  USB_TRANSFER ut;
  ut= uf->lpIssueVendorTransfer(uh, 0, 0
  , USB_SEND_TO_DEVICE|USB_OUT_TRANSFER
  , &req, 0, 0);
  if(ut) uf->lpCloseTransfer(ut);

  return TRUE;
};
BOOL ft100_extension::
init()
{
  if(ftdi_extension::init()==0) return FALSE;

  sendpkt= (sendendp->Descriptor.wMaxPacketSize&0x7ff)-2;

  return TRUE;
};

USB_TRANSFER ftdi_extension::
issuelinestate(int nandcode, int orcode, HANDLE event)
{
  USB_DEVICE_REQUEST req;
  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_VENDOR|USB_REQUEST_FOR_DEVICE;
  req.bRequest= 0x1; //MODEM_CTRL
  req.wValue= (WORD)(nandcode<<8|orcode); req.wIndex= 0; req.wLength= 0;
  return uf->lpIssueVendorTransfer(uh, notifyevent, event
  , USB_SEND_TO_DEVICE|USB_OUT_TRANSFER
  , &req, 0, 0);
};

USB_TRANSFER ftdi_extension::
issuebreak(int code)
{
  USB_DEVICE_REQUEST req;
  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_VENDOR|USB_REQUEST_FOR_DEVICE;
  req.bRequest= 0x4; //SET_DATA
  req.wValue= dcb.ByteSize|(WORD)dcb.Parity<<8|(WORD)dcb.StopBits<<11|(code?1<<14:0);
  req.wIndex= 0; req.wLength= 0;
  return uf->lpIssueVendorTransfer(uh, 0, 0
  , USB_SEND_TO_DEVICE|USB_OUT_TRANSFER
  , &req, 0, 0);
};

BOOL ftdi_extension::
applydcb()
{
  device_extension::applydcb();
  USB_TRANSFER ut;
  USB_DEVICE_REQUEST req;
  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_VENDOR|USB_REQUEST_FOR_DEVICE;
  req.bRequest= 0x3; //SET_BAUDRATE
  int baud= dcb.BaudRate; if(baud==0) baud= 9600;
  int div= (usbmode&MODE_12MHZ)?6000000:24000000;
  div= div/baud;
  req.wValue= div>>3|(div&4?0x4000:(div&2?0x8000:(div&1?0xc000:0)));
  req.wIndex= 0; req.wLength= 0;
  ut= uf->lpIssueVendorTransfer(uh, 0, 0
  , USB_SEND_TO_DEVICE|USB_OUT_TRANSFER
  , &req, 0, 0);
  if(ut) uf->lpCloseTransfer(ut);
  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_VENDOR|USB_REQUEST_FOR_DEVICE;
  req.bRequest= 0x4; //SET_DATA
  req.wValue= dcb.ByteSize|(WORD)dcb.Parity<<8|(WORD)dcb.StopBits<<11;
  req.wIndex= 0; req.wLength= 0;
  ut= uf->lpIssueVendorTransfer(uh, 0, 0
  , USB_SEND_TO_DEVICE|USB_OUT_TRANSFER
  , &req, 0, 0);
  if(ut) uf->lpCloseTransfer(ut);
  return TRUE;
};

BOOL ft100_extension::
applydcb()
{
  device_extension::applydcb();
  USB_TRANSFER ut;
  USB_DEVICE_REQUEST req;
  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_VENDOR|USB_REQUEST_FOR_DEVICE;
  req.bRequest= 0x3; //SET_BAUDRATE
  switch(dcb.BaudRate) {
    case 300: req.wValue= 0; break;
    case 600: req.wValue= 1; break;
    case 1200: req.wValue= 2; break;
    case 2400: req.wValue= 3; break;
    case 4800: req.wValue= 4; break;
    case 9600: default: req.wValue= 5; break;
    case 19200: req.wValue= 6; break;
    case 38400: req.wValue= 7; break;
    case 57600: req.wValue= 8; break;
    case 115200: req.wValue= 9; break;
  };
  req.wIndex= 0; req.wLength= 0;
  ut= uf->lpIssueVendorTransfer(uh, 0, 0
  , USB_SEND_TO_DEVICE|USB_OUT_TRANSFER
  , &req, 0, 0);
  if(ut) uf->lpCloseTransfer(ut);
  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_VENDOR|USB_REQUEST_FOR_DEVICE;
  req.bRequest= 0x4; //SET_DATA
  req.wValue= dcb.ByteSize|(WORD)dcb.Parity<<8|(WORD)dcb.StopBits<<11;
  req.wIndex= 0; req.wLength= 0;
  ut= uf->lpIssueVendorTransfer(uh, 0, 0
  , USB_SEND_TO_DEVICE|USB_OUT_TRANSFER
  , &req, 0, 0);
  if(ut) uf->lpCloseTransfer(ut);
  return TRUE;
};

int ft100_extension::
sentlen(DWORD *lenp)
{
  if(!*lenp==0) (*lenp)--;
  return 0;
};

USB_TRANSFER ft100_extension::
issuedata(BYTE* buf, BYTE* data, ULONG len, HANDLE ev)
{
  buf[0]= (BYTE)(len<<2|1);
  memcpy(buf+1, data, len);
  return uf->lpIssueBulkTransfer(sendpipe, ev?notifyevent:0, ev
  , USB_OUT_TRANSFER, len+1, buf, 0);
};

USB_TRANSFER ft100_extension::
issuesend(BYTE* data, ULONG len)
{
  if(len>(DWORD)sendpkt) len= sendpkt;
  if(!(usbmode&MODE_NO1132)) if(len%16==14) len--;
  return issuedata(sendbuf, data, len, writeevent);
};

USB_TRANSFER ft100_extension::
issuechar(BYTE *p)
{
  BYTE buf[2];
  return issuedata(buf, p, 1, 0);
};

USB_TRANSFER ft100_extension::
issuexon(BYTE *p)
{
  return issuedata(sendxonbuf, p, 1, serialevent);
};
