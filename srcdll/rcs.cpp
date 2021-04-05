// 232usb Copyright (c) 03-04,06 Zoroyoshi, Japan
// See source.txt for detail

#include <windows.h>
#include "usbdi.h"
#include "common.h"
#include "file.h"
#include "device.h"
#include "rcs.h"

BOOL rcs_extension::
init()
{
  if(recvendp==0) return FALSE;

  if(device_extension::init()==0) return FALSE;

  sendpkt= 261; //maximum packet size

  //in sigmarion3, multiple receive queue over interrupt transfer
  //causes illegal length... so no change in recvpkt
  //(rcs310 can send additional zero packet for just multiple of maxpacket)

  return TRUE;
};

BOOL rcs_extension::
deinit()
{
  uf->lpClosePipe(recvpipe); recvpipe= 0;

  return TRUE;
};

int rcs_extension::
sentlen(DWORD *lenp)
{
  return 0; //no need to add more packet
};

USB_TRANSFER rcs_extension::
issuesend(BYTE* data, ULONG len)
{
  if(!(usbmode&MODE_ALLSEND)) {
    if(len>(DWORD)sendpkt) len= sendpkt;
    //no need for 1132 bug workaround cuz maxPacketsize=8
    //(although rc-s310 can use multiple control transfers for one data-packet)
  };
  USB_DEVICE_REQUEST req;
  req.bmRequestType= USB_REQUEST_HOST_TO_DEVICE|USB_REQUEST_VENDOR|USB_REQUEST_FOR_INTERFACE;
  req.bRequest= 0x00; //SEND_DATA
  req.wValue= 0;
  req.wIndex= 0; req.wLength= (WORD)len;
  return uf->lpIssueVendorTransfer(uh, notifyevent, writeevent
  , USB_SEND_TO_INTERFACE|USB_OUT_TRANSFER
  , &req, data, 0);
};

USB_TRANSFER rcs_extension::
issuechar(BYTE *p)
{
  return 0; //do not send urgent data
};

USB_TRANSFER rcs_extension::
issuexon(BYTE *p)
{
  return 0; //do not send xon-xoff
};
