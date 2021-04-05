// 232usb Copyright (c) 03-04 Zoroyoshi, Japan
// See source.txt for detail

#include <windows.h>
#include "usbdi.h"
#include "common.h"
#include "file.h"
#include "device.h"

BOOL device_extension::
init()
{
  zmesg(ZM_DRIVER,L"device::init\n");
  //if(recvendp==0||sendendp==0) return FALSE; //no error fall

  sendpkt= 63; recvpkt= 64; recvlap= 4; //fall-slip
  if(sendendp) sendpkt= (sendendp->Descriptor.wMaxPacketSize&0x7ff)-1;
  if(recvendp) recvpkt= recvendp->Descriptor.wMaxPacketSize&0x7ff;

  linein= MS_CTS_ON|MS_DSR_ON;

  recvpipe= 0; sendpipe= 0;
  if(recvendp) {
    recvpipe= uf->lpOpenPipe(uh, &recvendp->Descriptor);
    USB_TRANSFER ut= uf->lpClearFeature(uh, 0, 0
    , USB_SEND_TO_ENDPOINT, USB_FEATURE_ENDPOINT_STALL, recvendp->Descriptor.bEndpointAddress);
    if(ut) uf->lpCloseTransfer(ut);
  };
  if(sendendp) {
    sendpipe= uf->lpOpenPipe(uh, &sendendp->Descriptor);
    USB_TRANSFER ut= uf->lpClearFeature(uh, 0, 0
    , USB_SEND_TO_ENDPOINT, USB_FEATURE_ENDPOINT_STALL, sendendp->Descriptor.bEndpointAddress);
    if(ut) uf->lpCloseTransfer(ut);
  };
  //?confirm w/real system
  //in ohcd2(ce4)/uhcd, resetpipe also sends ClearFEATURE-stall
  //in ohcd(ce3), resetpipe does not send ClearFeature (Data-Toggle may be mismatched)

  return TRUE;
};

BOOL device_extension::
initbase()
{
  if(init()==0) return FALSE;

  list= 0;
  InitializeCriticalSection(&listlock);
  InitializeCriticalSection(&flowcs);
  readevent= CreateEvent(0, 0, 0, 0);
  writeevent= CreateEvent(0, 0, 0, 0);
  serialevent= CreateEvent(0, 0, 0, 0);

  if(recvlap>MAX_LAP) recvlap= MAX_LAP; //guard

  recvbuf= (BYTE*)LocalAlloc(0, recvsize);
  zmesg(ZM_ALLOC,L"<alloc-recvbuf>%x\n", recvbuf);

  recvin= 0;
  recvstep= (recvpkt+15)&~15; //16byte alignment
  //recvvirt= AllocPhysMem(recvlap*recvstep, PAGE_READWRITE|PAGE_NOCACHE, 0, 0, &recvphys);
  recvvirt= (BYTE*)LocalAlloc(0, recvlap*recvstep);
  zmesg(ZM_ALLOC,L"<alloc-lap>%x\n", recvvirt);
  recvcur= 0;
  recvnum= 0;
  for(int i= 0; i<recvlap; i++) recvut[i]= 0;
  utflowc= 0;
  utflows= 0;
  reset();

  int (*proc)(...)= (int (*)(...))GetProcAddress
  (GetModuleHandle(L"coredll"), L"CeGetThreadPriority");
  if(proc) {
    attachprio= (*proc)(GetCurrentThread());
  } else {
    attachprio= GetThreadPriority(GetCurrentThread());
    if(attachprio<8) attachprio+= 248;
  };
  dispatchthread= CreateThread(0, 0, dispatchwrap, this, 0, 0);
  return TRUE;
};

BOOL device_extension::
deinit()
{
  uf->lpClosePipe(recvpipe); recvpipe= 0;
  uf->lpClosePipe(sendpipe); sendpipe= 0;
  return TRUE;
};

BOOL device_extension::
deinitbase()
{
  zmesg(ZM_DRIVER,L"deinit\n");
  deinit();

  //wait for thread to abone...
  SetEvent(serialevent);
  if(WaitForSingleObject(dispatchthread, 1000)!=WAIT_OBJECT_0) {
    TerminateThread(dispatchthread, 0);
    zmesg(ZM_ERROR,L"dispatchthread timeout\n");
  };

  CloseHandle(readevent);
  CloseHandle(writeevent);
  CloseHandle(serialevent);
  DeleteCriticalSection(&listlock);
  zmesg(ZM_ALLOC,L"<free-recvbuf>%x\n", recvbuf);
  LocalFree(recvbuf);
  zmesg(ZM_ALLOC,L"<free-lap>%x\n", recvvirt);
  LocalFree(recvvirt);
  return TRUE;
};

void device_extension::
unblock()
{
  SetEvent(readevent);
  SetEvent(writeevent);
};

BOOL device_extension::
reset()
{
  readuse= 0;
  writeuse= 0;
  sendxoff= 0;
  recvflow= 0;
  errors= 0;
  int a= recvin;
  if(a==0) a= recvsize;
  recvout= a; //Serialevent is set by calling functions
  //timeout moved from init..maybe it's fine
  memset(&timeout, 0, sizeof(timeout));
  timeout.ReadIntervalTimeout= 250;
  timeout.ReadTotalTimeoutMultiplier= 10;
  timeout.ReadTotalTimeoutConstant= 100;
  memset(&dcb, 0, sizeof(dcb));
  dcb.DCBlength= sizeof(dcb);
  dcb.BaudRate= 9600;
  dcb.ByteSize= 8; dcb.Parity= NOPARITY; dcb.StopBits= ONESTOPBIT;
  dcb.fDtrControl= DTR_CONTROL_DISABLE;
  dcb.fRtsControl= RTS_CONTROL_DISABLE;
  dcb.fBinary= 1; dcb.XonChar= 0x11; dcb.XoffChar= 0x13;
  return TRUE;
};

BOOL device_extension::
open()
{
  zmesg(ZM_DRIVER,L"open\n");
  reset();
  //dcb.fDtrControl= DTR_CONTROL_ENABLE;
  //dcb.fRtsControl= DTR_CONTROL_ENABLE;
  SetEvent(serialevent);
  applydcb();
  return TRUE;
};

ULONG device_extension::
close()
{
  zmesg(ZM_DRIVER,L"close\n");
  dcb.fInX= 0;
  dcb.fDtrControl= DTR_CONTROL_DISABLE;
  dcb.fRtsControl= RTS_CONTROL_DISABLE;
  SetEvent(serialevent);
  issuebreak(0);
  issuelinestate(3, 0, 0);
  return 0;
};

DWORD device_extension::
comstat(COMSTAT *ce)
{
  DWORD rc= InterlockedExchange((long*)&errors, 0);
  if(linein&1) rc|= CE_BREAK; //once and continues

  memset(ce, 0, sizeof(COMSTAT));
  if(dcb.fOutxCtsFlow&&!(linein&MS_CTS_ON)) ce->fCtsHold= 1;
  if(dcb.fOutxDsrFlow&&!(linein&MS_DSR_ON)) ce->fDsrHold= 1;
  //fRlsdHold= 0;
  if(dcb.fOutX&&sendxoff) ce->fXoffHold= 1;
  if(recvflow) {
    if(dcb.fInX&&!dcb.fTXContinueOnXoff) ce->fXoffSent= 1;
  };
  //fEof= 0;
  //fTxim= 0?
  int sz= recvin-recvout;
  if(sz<0) sz+= recvsize;
  ce->cbInQue= sz;
  ce->cbOutQue= 0;
  return rc;
};

DWORD device_extension::
escapecomm(file_extension* fx, int func)
{
  if(parent) return parent->escapecomm(fx, func);
  if(func==CLRIR) return TRUE;
  return FALSE;
};

BOOL device_extension::
evaluateevent(DWORD bit)
{
  //list-read-lock
  EnterCriticalSection(&listlock);
  file_extension *fx= list;
  while(fx) {
    if(fx->dx==this) {
      DWORD curbit= fx->waitbit;
      for(int i= 0; i<10; i++) {
        DWORD newbit= curbit|curbit>>16&bit;
        if(curbit==newbit) break;
        DWORD oldbit= curbit;
        curbit= InterlockedTestExchange((long*)&fx->waitbit, curbit, newbit);
        if(oldbit==curbit) {
          SetEvent(fx->waitevent);
	  break;
        };
        if(i>=9) zmesg(ZM_ERROR,L"evalover\n");
      };
    };
    fx= fx->list;
  };
  //list-read-unlock
  LeaveCriticalSection(&listlock);
  return TRUE;
};

int device_extension::
recvissue()
{
  if(recvnum>=recvlap) return -1; //no more overlap
  int sz= recvout-recvin-1;
  if(sz<0) sz+= recvsize;
  sz-= recvpkt*(recvnum+1); //1 for me
  int c= recvcur+recvnum;
  if(sz<0) { //no more room in buffer
    //check if the pipe is present
    int f= 1;
    int rt= uf->lpIsPipeHalted(recvpipe, &f);
    if(rt&&f==0) return -1; //ok
    zmesg(ZM_ERROR,L"  recvissue[%d] PipeHalt %d %d\n", c, rt, f);
    return 0;
  };
  if(c>=recvlap) c-= recvlap;
  USB_TRANSFER ut= uf->lpIssueBulkTransfer(recvpipe, notifyevent, serialevent
  , USB_IN_TRANSFER|USB_SHORT_TRANSFER_OK, recvpkt, recvvirt+recvstep*c, 0);
  //zmesg(ZM_USB,L"  recvissue[%d]=x%x len=%d\n", c, ut, recvpkt);
  if(ut==0) { //?error
    zmesg(ZM_ERROR,L"  recvissue[%d] err=%d\n", c, GetLastError());
    return 0;
  };
  recvut[c]= ut;
  recvnum++;
  return 1;
};

int device_extension::
recvdata(BYTE* src, DWORD len)
{
  if(dcb.fOutX) { //search xoff/xon char
    int ff= 0;
    BYTE* p= recvbuf+recvin;
    int zend= recvout-1;
    int z= zend-recvin;
    if(z<0) {
      zend= z/*flag*/; z= recvsize-recvin;
    };
    while(len>0&&z>0) {
      do {
        BYTE c= *src++; len--;
	if(c==dcb.XoffChar) ff= -1;
	else if(c==dcb.XonChar) ff= 1;
	else *p++= c, z--;
      } while(len>0&&z>0);
      if(z==0&&zend<0) {
        p= recvbuf; zend= recvout-1; z= zend;
      };
    }; 
    if(len>0) {
      zmesg(ZM_ERROR,L"  recvdata overflow %d\n", len);
      while(len>0) { //this may not happened
        BYTE c= *src++; len--;
        if(c==dcb.XoffChar) ff= -1;
        else if(c==dcb.XonChar) ff= 1;
      };
    };
    if(zend<0) {
      recvin= recvsize-z;
    } else {
      recvin= zend-z;
    };
    if(ff<0) {
      sendxoff= 1; SetEvent(writeevent);
    } else if(ff>0) {
      sendxoff= 0; SetEvent(writeevent);
    };
  } else { //just copy
    int sz= recvout-recvin-1;
    if(sz<0) sz+= recvsize;
    if((int)len>sz) {
      zmesg(ZM_ERROR,L"  recvdata overflow %d>%d\n", len, sz);
      len= sz;
    };
    int a= recvsize-recvin;
    if((int)len>=a) {
      memcpy(recvbuf+recvin, src, a);
      memcpy(recvbuf, src+a, len-a);
      recvin= len-a;
    } else {
      memcpy(recvbuf+recvin, src, len);
      recvin+= len;
    };
  };
  SetEvent(readevent);
  evaluateevent(EV_RXCHAR);
  return 1;
};

int device_extension::
recvwait()
{
  if(recvnum==0) return 0; //nothing queued
  DWORD rc= ~0;
  DWORD len= 0;
  int rt= uf->lpIsTransferComplete(recvut[recvcur]);
  if(uf->lpGetTransferStatus(recvut[recvcur], &len, &rc)) {
    if(rt==0) return 0; //waiting
    uf->lpCloseTransfer(recvut[recvcur]);
  };
#if DEBUG
  if((usbmode&255)!=MODE_FTDI||len>2) 
    zmesg(ZM_USB,L"  recvwait[%d]=x%x len=%d\n", recvcur, recvut[recvcur], len);
#endif
  recvut[recvcur]= 0;
  if(rc!=0) { //?error
    zmesg(ZM_ERROR,L"  recvwait[%d]=x%x rc=%d\n", recvcur, recvut[recvcur], rc);
    //do nothing(len maybe 0)
  };
#if DEBUG
  if((dpCurSettings.ulZoneMask&ZM_DUMP)&&((usbmode&255)!=MODE_FTDI||len>2)) {
    WCHAR z[140], *p;
    DWORD n= 0;
    do {
      DWORD m= n+64; if(m>len) m= len;
      for(p= z; n<m; n++) {
        DWORD d= recvvirt[recvstep*recvcur+n];
        *p++= L"0123456789ABCDEF"[d>>4];
        *p++= L"0123456789ABCDEF"[d&15];
      };
      *p= 0;
      zmesg(ZM_DUMP,L"%04d>%s\n",len,z);
    } while(n<len);
  };
#endif
  recvdata(recvvirt+recvstep*recvcur, len);

  int c= recvcur+1;
  if(c>=recvlap) c-= recvlap;
  recvcur= c; recvnum--;
  return 1;
};

int device_extension::
recvline()
{
  if(utflowc) {
    DWORD rc= ~0;
    int rt= uf->lpIsTransferComplete(utflowc);
    if(uf->lpGetTransferStatus(utflowc, 0, &rc)) {
      if(rt) {
        uf->lpCloseTransfer(utflowc);
        utflowc= 0;
      };
    } else utflowc= 0;
    if(rc) zmesg(ZM_ERROR, L"utflowc rc=x%x\n", rc);
  };
  if(utflows) {
    DWORD rc= ~0;
    int rt= uf->lpIsTransferComplete(utflows);
    if(uf->lpGetTransferStatus(utflows, 0, &rc)) {
      if(rt) {
        uf->lpCloseTransfer(utflows);
        utflows= 0;
      };
    } else utflows= 0;
    if(rc) zmesg(ZM_ERROR, L"utflows rc=x%x\n", rc);
  };
  if(utflowc!=0||utflows!=0) return 0;

  int sz= recvin-recvout;
  if(sz<0) sz+= recvsize;
  if(recvflow==0&&sz>recvsize*3/4) {
    int code= 0;
    if(dcb.fRtsControl==RTS_CONTROL_HANDSHAKE) code|= 2;
    if(dcb.fDtrControl==DTR_CONTROL_HANDSHAKE) code|= 1;
    if(code||dcb.fInX) {
      EnterCriticalSection(&flowcs);
      if(code) utflowc= issuelinestate(code, 0, serialevent);
      if(dcb.fInX) {
        utflows= issuexon((BYTE*)&dcb.XoffChar);
      };
      recvflow= 1;
      LeaveCriticalSection(&flowcs);
    } else recvflow= 1;
  } else if(recvflow==1&&sz<recvsize/4) {
    int code= 0;
    if(dcb.fRtsControl==RTS_CONTROL_HANDSHAKE) code|= 2;
    if(dcb.fDtrControl==DTR_CONTROL_HANDSHAKE) code|= 1;
    if(code||dcb.fInX) {
      EnterCriticalSection(&flowcs);
      if(code) utflowc= issuelinestate(code, code, serialevent);
      if(dcb.fInX) {
        utflows= issuexon((BYTE*)&dcb.XonChar);
      };
      recvflow= 0;
      LeaveCriticalSection(&flowcs);
    } else recvflow= 0;
  };
  return 0;
};

int device_extension::
sentlen(DWORD *lenp)
{
  if(sendendp&&*lenp!=0
  &&*lenp%(sendendp->Descriptor.wMaxPacketSize&0x7ff)==0) return 1;
  return 0;
};

USB_TRANSFER device_extension::
issuesend(BYTE* buf, ULONG len)
{
  if(!(usbmode&MODE_ALLSEND)) {
    if(len>(DWORD)sendpkt) len= sendpkt;
    if(!(usbmode&MODE_NO1132)) if(len%16==15) len--;
  };
#if DEBUG
  if(dpCurSettings.ulZoneMask&ZM_DUMP) {
    WCHAR z[140], *p;
    DWORD n= 0;
    do {
      DWORD m= n+64; if(m>len) m= len;
      for(p= z; n<m; n++) {
        DWORD d= buf[n];
        *p++= L"0123456789ABCDEF"[d>>4];
        *p++= L"0123456789ABCDEF"[d&15];
      };
      *p= 0;
      zmesg(ZM_DUMP,L"%04d<%s\n",len,z);
    } while(n<len);
  };
#endif
  zmesg(ZM_USB,L"  issuesend len=%d\n", len);
  return uf->lpIssueBulkTransfer(sendpipe, notifyevent, writeevent
  , USB_OUT_TRANSFER, len, buf, 0);
};

USB_TRANSFER device_extension::
issuechar(BYTE *p)
{
  zmesg(ZM_USB,L"  issuechar x%02x", *p);
  return uf->lpIssueBulkTransfer(sendpipe, 0, 0
  , USB_OUT_TRANSFER, 1, p, 0);
};

USB_TRANSFER device_extension::
issuexon(BYTE *p)
{
  zmesg(ZM_USB,L"  issuexon x%02x", *p);
  return uf->lpIssueBulkTransfer(sendpipe, notifyevent, serialevent
  , USB_OUT_TRANSFER, 1, p, 0);
};

USB_TRANSFER device_extension::
issuebreak(int code)
{
  return 0;
};

USB_TRANSFER device_extension::
issuelinestate(int nandcode, int orcode, HANDLE ev)
{
  return 0;
};

int device_extension::
linestatein(DWORD in, DWORD err)
{
  DWORD ev= 0;
  if(in) ev|= EV_ERR;
  DWORD curerr= errors;
  for(int i= 0; i<10; i++) {
    DWORD olderr= curerr;
    curerr= InterlockedTestExchange((long*)&errors, curerr, curerr|err);
    if(olderr==curerr) break;
  };
  DWORD oldin= linein;
  linein= in; in^= oldin;
  if(in&1) ev|= EV_BREAK;
  if(in&MS_CTS_ON) ev|= EV_CTS;
  if(in&MS_DSR_ON) ev|= EV_DSR;
  if(in&MS_RING_ON) ev|= EV_RING;
  if(in&MS_RLSD_ON) ev|= EV_RLSD;
  if(ev&(EV_CTS|EV_DSR)) SetEvent(writeevent);
  if(ev) evaluateevent(ev);
  return 1;
};

BOOL device_extension::
applydcb()
{
  if(dcb.fOutX==0) sendxoff= 0;
  int mode= 0;
  if(dcb.fDtrControl==DTR_CONTROL_ENABLE) mode|= 1;
  else if(dcb.fDtrControl==DTR_CONTROL_HANDSHAKE&&recvflow==0) mode|= 1;
  if(dcb.fRtsControl==RTS_CONTROL_ENABLE) mode|= 2;
  else if(dcb.fRtsControl==RTS_CONTROL_HANDSHAKE&&recvflow==0) mode|= 2;
  issuelinestate(3, mode, 0);
  return TRUE;
};

int device_extension::
dispatchdo()
{
  while(recvwait()==1) ;
  int rti; //not to exit when full of buffer
  while((rti= recvissue())==1) ;
  recvline();
  if(rti==0&&recvnum==0&&utflows==0&&utflowc==0) return 0;
  return 1;
};

int device_extension::
dispatch()
{
  zmesg(ZM_DRIVER,L"dispatch\n");
  int prio= priority;
  if(priority<=0) prio= attachprio;
  int (*proc)(...)= (int (*)(...))GetProcAddress
  (GetModuleHandle(L"coredll"), L"CeSetThreadPriority");
  if(proc) {
    (*proc)(GetCurrentThread(), prio);
  } else {
    SetThreadPriority(GetCurrentThread(), prio<248?0:prio-248);
  };
#if _WIN32_WCE>=300
  zmesg(ZM_PRIO, L"dispatch p=%d\n", CeGetThreadPriority(GetCurrentThread()));
#else
  zmesg(ZM_PRIO, L"dispatch p=%d\n", GetThreadPriority(GetCurrentThread()));
#endif
  for(;;) {
    if(dispatchdo()==0) break;
    WaitForSingleObject(serialevent, INFINITE);
  };
  zmesg(ZM_DRIVER,L"dispatch end\n");
  return 0;
};

DWORD WINAPI device_extension::
dispatchwrap(void *dx)
{
  return ((device_extension*)dx)->dispatch();
};
