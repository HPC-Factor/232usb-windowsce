// 232usb Copyright (c) 03-04,06 Zoroyoshi, Japan
// See source.txt for detail

#include <windows.h>
#include <winioctl.h>
#include "usbdi.h"
#include "common.h"
#include "file.h"
#include "device.h"

DWORD WINAPI
notifyevent(void* ev)
{
  SetEvent((HANDLE)ev);
  return 0;
};

//in Win2.xx, there is no ClientInfo key when COM_Init was called.
//so device initialize routines -> USBDeviceAttach,
//       fetching of ClientInfo -> COM_Open.
extern "C" WCHAR*
COM_Init(WCHAR* keyname)
{
  zmesg(ZM_DRIVER,L"com_init %s\n", keyname);
  WCHAR *s= (WCHAR*)LocalAlloc(0, (wcslen(keyname)+1)*sizeof(WCHAR));
  zmesg(ZM_ALLOC,L"<alloc-key %x>\n", s);
  wcscpy(s, keyname);
  return(s);
};

extern "C" BOOL
COM_Deinit(WCHAR* keyname)
{
  zmesg(ZM_DRIVER,L"com_deinit %s\n", keyname);
  HKEY key;
  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0, 0, &key)) return 0;
  DWORD rt;
  device_extension *dx;
  DWORD rd= sizeof(dx);
  if(RegQueryValueEx(key, L"ClientInfo", 0, &rt, (BYTE*)&dx, &rd)) rt= 0;
  RegCloseKey(key);
  if(rt==REG_DWORD&&dx) {
    file_extension* fx;
    //unblock threads
    int retry= 0;
    for(;;) {
      int ref= 0;
      for(fx= dx->list; fx; fx= fx->list) {
        fx->dx= 0;
        SetEvent(fx->waitevent);
	ref+= fx->ref;
      };
      dx->unblock();
      if(ref==0) break;
      if(++retry>5) break;
      Sleep(retry*20);
    };
    //free
    fx= dx->list;
    while(fx) {
      void* ff= fx;
      fx= fx->list;
      LocalFree(ff);
      zmesg(ZM_ALLOC,L"<*free-fx%x>\n", ff);
    };
  };
  LocalFree(keyname);
  zmesg(ZM_ALLOC,L"<free-key %x>\n", keyname);
  return 1;
};

extern "C" BOOL
COM_PowerUp(WCHAR* keyname)
{
//  zmesg(ZM_DRIVER,L"com_powerup\n");
  return 1;
};

extern "C" BOOL
COM_PowerDown(WCHAR* keyname)
{
//  zmesg(ZM_DRIVER,L"com_powerdown\n"); never call file functions when power-down
  return 1;
};

extern "C" HANDLE
COM_Open(WCHAR* keyname, DWORD access, DWORD share)
{
  zmesg(ZM_DRIVER,L"com_open %s x%x x%x\n", keyname, access, share);
  if(keyname==0) return 0;

  HKEY key;
  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0, 0, &key)) return 0;
  DWORD rt;
  device_extension *dx;
  DWORD rd= sizeof(dx);
  if(RegQueryValueEx(key, L"ClientInfo", 0, &rt, (BYTE*)&dx, &rd)) rt= 0;
  RegCloseKey(key);
  if(rt!=REG_DWORD) return 0;
  if(dx==0) return 0;
  file_extension *fx= (file_extension*)LocalAlloc(LPTR, sizeof(file_extension));
  zmesg(ZM_ALLOC,L"<alloc-fx%x>\n", fx);
  if(fx==0) return 0;
  fx->dx= dx;
  //fx prologue
  fx->access= access;
  fx->ref= 0;
  fx->waitevent= CreateEvent(0, 0, 0, 0);
  fx->waitbit= 0;
  fx->waituse= 0;

  EnterCriticalSection(&dx->listlock);
  file_extension* oldlist= dx->list;
  dx->list= fx; fx->list= oldlist;
  LeaveCriticalSection(&dx->listlock);

  if(oldlist==0) dx->open();
  return fx;
};

void
closeFile(file_extension* fx)
{
  device_extension *dx= (device_extension*)InterlockedExchange((long*)&fx->dx, 0);
  if(dx==0) return; //already closed
  if(dx->parent) dx= dx->parent;

  //wait for other thread to finish...
  int retry= 0;
  for(;;) {
    if(fx->ref==0) break;
    SetEvent(fx->waitevent);
    dx->unblock();
    if(++retry>5) break;
    Sleep(retry*20);
  };

  EnterCriticalSection(&dx->listlock);
  file_extension** listp= &dx->list;
  file_extension* li;
  while(li= *listp) {
    if(li==fx) {
      *listp= fx->list;
      break;
    };
    listp= &li->list;
  };
  LeaveCriticalSection(&dx->listlock);

  if(li&&dx->list==0) dx->close(); //last close
  return;
};

extern "C" BOOL
COM_Close(file_extension* fx)
{
  zmesg(ZM_DRIVER,L"com_close x%x\n", fx);
  closeFile(fx);
  //fx epilogue(CloseHandle..)
  CloseHandle(fx->waitevent);
  LocalFree(fx);
  zmesg(ZM_ALLOC,L"<free-fx%x>\n", fx);
  return 1;
};

extern "C" ULONG
COM_Read(file_extension* fx, PUCHAR buf, ULONG len)
{
  zmesg(ZM_FILE,L"com_read len=%d\n", len);
//  zmesg(ZM_PRIO, L"read p=%d\n", CeGetThreadPriority(GetCurrentThread()));
  device_extension *dx= fx->dx;
  if(dx==0) { SetLastError(ERROR_INVALID_HANDLE); return(ULONG)-1; };
  if((fx->access&GENERIC_READ)==0) {
    SetLastError(ERROR_INVALID_ACCESS);
    return(ULONG)-1;
  };
  if(InterlockedTestExchange(&dx->readuse, 0, 1)!=0) {
    zmesg(ZM_ERROR,L"readuse\n");
    SetLastError(ERROR_ACCESS_DENIED);
    return(ULONG)-1;
  };
  InterlockedIncrement(&fx->ref);
  DWORD tt= dx->timeout.ReadTotalTimeoutMultiplier;
  if(tt==MAXDWORD)  tt= 0;
  DWORD ti= dx->timeout.ReadIntervalTimeout;
  if(ti==MAXDWORD) ti= 0;
  else if(ti==0) ti= MAXDWORD;
  else ti+= tt*8; //fifo size.. true for usb?
  tt= tt*len+dx->timeout.ReadTotalTimeoutConstant;
  if(tt==0&&ti!=0) tt= MAXDWORD;
  DWORD tc= GetTickCount();
  //tt: total    ; 0=immediate/.../MAXDWORD=infinite
  //ti: interval ; 0=immediate/.../MAXDWORD=infinite
  ULONG rc= 0; //read bytes

  while(len>0) {
    if(fx->dx==0) break; //closed
    if(dx->readuse!=1) break; //abort
    int sz= dx->recvin-dx->recvout;
    if(sz<0) sz+= dx->recvsize;
    if(sz==0) { //wait for data..
      DWORD to;
      if(tt!=0&&tt!=MAXDWORD) {
        to= GetTickCount()-tc;
        if(to>tt) to= 0; else to= tt-to;
      } else to= tt;
      if(rc&&ti<to) to= ti;
      if(to==0) break; //timeout
      if(WaitForSingleObject(dx->readevent, to)==WAIT_TIMEOUT) break; //timeout
    } else {
      if(sz>(int)len) sz= (int)len;
      int a= dx->recvsize-dx->recvout;
      if(sz<=a) {
        memcpy(buf, dx->recvbuf+dx->recvout, sz);
        dx->recvout+= sz;
      } else {
        memcpy(buf, dx->recvbuf+dx->recvout, a);
	memcpy(buf+a, dx->recvbuf, sz-a);
	dx->recvout= sz-a;
      };
      buf+= sz; len-= sz; rc+= sz;
      SetEvent(dx->serialevent);
    };
  };
  dx->readuse= 0;
  InterlockedDecrement(&fx->ref);
  return(rc);
};

extern "C" ULONG
COM_Write(file_extension* fx, PUCHAR buf, ULONG len)
{
  zmesg(ZM_FILE,L"com_write len=%d\n", len);
//  zmesg(ZM_PRIO, L"write p=%d\n", CeGetThreadPriority(GetCurrentThread()));
  device_extension *dx= fx->dx;
  if(dx==0) { SetLastError(ERROR_INVALID_HANDLE); return(ULONG)-1; };
  if((fx->access&GENERIC_WRITE)==0) {
    SetLastError(ERROR_INVALID_ACCESS);
    return(ULONG)-1;
  };
  if(InterlockedTestExchange(&dx->writeuse, 0, 1)!=0) {
    zmesg(ZM_ERROR,L"writeuse\n");
    SetLastError(ERROR_ACCESS_DENIED);
    return(ULONG)-1;
  };
  InterlockedIncrement(&fx->ref);

  DWORD tt= dx->timeout.WriteTotalTimeoutConstant+dx->timeout.WriteTotalTimeoutMultiplier*len;
  if(tt==0)  tt= MAXDWORD;
  DWORD tc= GetTickCount();
  ULONG rc= 0; //wrote bytes
  int need0= 0; //need additional 0 packet
  if(len==0&&(dx->usbmode&(MODE_NO0SEND|MODE_ALLSEND))!=MODE_NO0SEND) need0= 1;

  while(len!=0||need0) {
    EnterCriticalSection(&dx->flowcs);
    if(len!=0&&
    ( dx->dcb.fOutxCtsFlow&&!(dx->linein&MS_CTS_ON)
    ||dx->dcb.fOutxDsrFlow&&!(dx->linein&MS_DSR_ON)
    ||dx->dcb.fOutX&&dx->sendxoff
    ||dx->dcb.fInX&&!dx->dcb.fTXContinueOnXoff&&dx->recvflow)) { //flow controlled -- do not send
      LeaveCriticalSection(&dx->flowcs);
      DWORD to;
      if(tt!=MAXDWORD) {
        to= GetTickCount()-tc;
        if(to>tt) to= 0; else to= tt-to;
      } else to= tt;
      if(to==0) len= 0; //timeout
      else if(WaitForSingleObject(dx->writeevent, to)==WAIT_TIMEOUT) len= 0; //timeout
      if(fx->dx==0||dx->writeuse!=1) len= 0; //closed/abort
      continue; //continue to next event...
    };
    USB_TRANSFER ut= dx->issuesend(buf, len);
    LeaveCriticalSection(&dx->flowcs);

    if(ut==0) break; //error

    DWORD rf= 0;
    DWORD sz= 0;
    DWORD rt= ~0;
    for(;;) {
      DWORD to;
      if(tt!=MAXDWORD) {
        to= GetTickCount()-tc;
        if(to>tt) to= 0; else to= tt-to;
	to+= 20; //additional time to send packet
      } else to= tt;
      if(to==0) break; //timeout occurs
      if(WaitForSingleObject(dx->writeevent, to)==WAIT_TIMEOUT) break; //timeout occurs
      if(fx->dx==0||dx->writeuse!=1) break; //closed/abort
      if(rf= dx->uf->lpIsTransferComplete(ut)) break; //transfer complete
    };
    if(rf==0) dx->uf->lpAbortTransfer(ut, 0);
    dx->uf->lpGetTransferStatus(ut, &sz, &rt);
    dx->uf->lpCloseTransfer(ut);
    need0= dx->sentlen(&sz);
    if(len==0||(dx->usbmode&MODE_NO0SEND)) need0= 0;
    if(sz>len) sz= len; //guard
    buf+= sz; len-= sz; rc+= sz;
    if(rf==0||rt!=0) len= 0; //timeout/closed/abort//end transfer
  };

  dx->writeuse= 0;
  InterlockedDecrement(&fx->ref);
  return(rc);
};

extern "C" ULONG
COM_Seek(file_extension* fx, LONG pos, DWORD mode)
{
  zmesg(ZM_FILE,L"com_seek\n");
  return -1;
};

BOOL
fxGetCommMask(file_extension* fx, DWORD* pmask)
{
  zmesg(ZM_FILE,L"GetCommMask\n");
  *pmask= fx->waitbit>>16;
  return TRUE;
};

BOOL
fxSetCommMask(file_extension* fx, DWORD mask)
{
  zmesg(ZM_FILE,L"SetCommMask x%x\n", mask);
  mask&= 0xffff;
  DWORD curbit= fx->waitbit;
  for(int i= 0; i<10; i++) {
    DWORD oldbit= curbit;
    curbit= InterlockedTestExchange((long*)&fx->waitbit, curbit, curbit&mask|mask<<16);
    if(oldbit==curbit) break;
    if(i>=9) zmesg(ZM_ERROR,L"maskover\n");
  };
  InterlockedTestExchange(&fx->waituse, 1, 2);
  SetEvent(fx->waitevent);
  return TRUE;
};

BOOL
fxWaitCommEvent(file_extension* fx, DWORD* pbit)
{
  zmesg(ZM_FILE,L"WaitCommEvent\n");
//  zmesg(ZM_PRIO, L"wait p=%d\n", CeGetThreadPriority(GetCurrentThread()));
  if(InterlockedTestExchange(&fx->waituse, 0, 1)!=0) {
    zmesg(ZM_ERROR,L"waituse\n");
    SetLastError(ERROR_ACCESS_DENIED);
    *pbit= 0; return FALSE;
  };
  ResetEvent(fx->waitevent);
  DWORD curbit= fx->waitbit;
  int r= 0;
  for(;;) {
    if(fx->dx==0) { r= -2; break; };
    if(fx->waituse!=1) { r= -1; break; };
    if(curbit==0) break;
    if(curbit&0xffff) {
      DWORD oldbit= curbit;
      curbit= InterlockedTestExchange((long*)&fx->waitbit, curbit, curbit&~0xffff);
      if(oldbit==curbit) break;
      if(++r==10) { zmesg(ZM_ERROR,L"waitover\n"); break; };//retry over...guardian
      continue;
    };
    WaitForSingleObject(fx->waitevent, INFINITE);
    curbit= fx->waitbit; r= 0;
  };
  fx->waituse= 0;
  if(r==-2) { //handle closed
    SetLastError(ERROR_INVALID_HANDLE);
    *pbit= 0; return FALSE;
  };
  if(r==-1) { //abort by setcommevent
    *pbit= 0; return TRUE;
  };
  if(curbit==0) { //no event mask
    SetLastError(ERROR_INVALID_PARAMETER);
    *pbit= 0; return FALSE;
  };
  *pbit= curbit&0xffff;
  return TRUE;
};

extern "C" BOOL
COM_IOControl(file_extension *fx, DWORD code
, PBYTE ibuf, DWORD ilen, PBYTE obuf, DWORD olen, PDWORD odone)
{
  zmesg(ZM_FILE,L"com_ioctl x%x %d\n", fx, code>>2&0xfff);
  if(code==CTL_CODE(0x103/*FILE_DEVICE_PSL*/, 255, METHOD_NEITHER, FILE_ANY_ACCESS)) {
    if(ilen==sizeof(DWORD)*4&&((DWORD*)ibuf)[0]==sizeof(DWORD)*4
    &&((DWORD*)ibuf)[1]==DLL_PROCESS_EXITING) {
      zmesg(ZM_DRIVER,L"ioctl_psl_notify\n");
      closeFile(fx);
    };
    return TRUE;
  };
  device_extension *dx= fx->dx;
  if(dx==0) { SetLastError(ERROR_INVALID_HANDLE); return(ULONG)-1; };
  if((code&~(0xfff<<2))!=CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0,METHOD_BUFFERED,FILE_ANY_ACCESS)) return FALSE;
  InterlockedIncrement(&fx->ref);
  int rc= FALSE;
  switch(code>>2&0xfff) {
  case 1: {//EscapeCommFunction(SETBREAK)==SetCommBreak
    rc= (int)dx->issuebreak(1);
  } break;
  case 2: {//EscapeCommFunction(CLRBREAK)==ClearCommBreak
    rc= (int)dx->issuebreak(0);
  } break;
  case 3: {//EscapeCommFunction(SETDTR)
    if(dx->dcb.fDtrControl==DTR_CONTROL_HANDSHAKE) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    rc= (int)dx->issuelinestate(1, 1, 0);
  } break;
  case 4: {//EscapeCommFunction(CLRDTR)
    if(dx->dcb.fDtrControl==DTR_CONTROL_HANDSHAKE) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    rc= (int)dx->issuelinestate(1, 0, 0);
  } break;
  case 5: {//EscapeCommFunction(SETRTS)
    if(dx->dcb.fRtsControl==RTS_CONTROL_HANDSHAKE) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    rc= (int)dx->issuelinestate(2, 2, 0);
  } break;
  case 6: {//EscapeCommFunction(CLRRTS)
    if(dx->dcb.fRtsControl==RTS_CONTROL_HANDSHAKE) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    rc= (int)dx->issuelinestate(2, 0, 0);
  } break;
  case 7: {//EscapeCommFunction(SETXOFF)
    if(dx->dcb.fOutX) dx->sendxoff= 1;
    rc= TRUE;
  } break;
  case 8: {//EscapeCommFunction(SETXON)
    if(dx->dcb.fOutX) dx->sendxoff= 0;
    SetEvent(dx->writeevent);
    rc= TRUE;
  } break;
  case 9: {//GetCommMask
    if(odone) *odone= 0;
    if(obuf==0||odone==0||olen<sizeof(DWORD)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    rc= fxGetCommMask(fx, (DWORD*)obuf);
    if(rc) *odone= sizeof(DWORD);
  } break;
  case 10:{//SetCommMask
    if(ibuf==0||ilen<sizeof(DWORD)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    rc= fxSetCommMask(fx, *(DWORD*)ibuf);
  } break;
  case 11:{//WaitCommEvent
    if(odone) *odone= 0;
    if(obuf==0||odone==0||olen<sizeof(DWORD)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    rc= fxWaitCommEvent(fx, (DWORD*)obuf);
    if(rc) *odone= sizeof(DWORD);
  } break;
  case 12:{//ClearCommError (GET_COMMSTATUS)
    if(odone) *odone= 0;
    if(obuf==0||odone==0||olen<sizeof(DWORD)+sizeof(COMSTAT)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    *(DWORD*)obuf= dx->comstat((COMSTAT*)(obuf+sizeof(DWORD)));
    rc= TRUE;
    *odone= sizeof(DWORD)+sizeof(COMSTAT);
  } break;
  case 13:{//GetCommModemStatus
    if(odone) *odone= 0;
    if(obuf==0||odone==0||olen<sizeof(DWORD)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    *(DWORD*)obuf= dx->linein&(MS_CTS_ON|MS_DSR_ON|MS_RING_ON|MS_RLSD_ON);
    rc= TRUE;
    *odone= sizeof(DWORD);
  } break;
  case 14:{//GetCommProperties
    if(odone) *odone= 0;
    if(obuf==0||odone==0||olen<sizeof(COMMPROP)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    memset(obuf, 0, sizeof(COMMPROP));
    COMMPROP *cp= (COMMPROP*)obuf;
    //sets dummy data...
    cp->wPacketLength= ~0; cp->wPacketVersion= ~0;
    cp->dwServiceMask= SP_SERIALCOMM;
    cp->dwMaxBaud= BAUD_USER;
    cp->dwProvSubType= PST_RS232;
    cp->dwProvCapabilities= (DWORD)~0>>1;
    cp->dwSettableParams= (DWORD)~0>>1; 
    cp->dwSettableBaud= (DWORD)~0>>1;
    cp->wSettableData= (WORD)~0>>1;
    cp->wSettableStopParity= (WORD)~0>>1;
    rc= TRUE;
    *odone= sizeof(COMMPROP);
  } break;
  case 15:{//SetCommTimeouts
    if(ibuf==0||ilen<sizeof(COMMTIMEOUTS)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    dx->timeout= *(COMMTIMEOUTS*)ibuf;
    rc= TRUE;
  } break;
  case 16:{//GetCommTimeouts
    if(odone) *odone= 0;
    if(obuf==0||odone==0||olen<sizeof(COMMTIMEOUTS)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    *(COMMTIMEOUTS*)obuf= dx->timeout;
    rc= TRUE;
    *odone= sizeof(COMMTIMEOUTS);
  } break;
  case 17:{//PurgeComm (terminate pending read/write)
    if(ibuf==0||ilen<sizeof(DWORD)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    DWORD f= *(DWORD*)ibuf;
    if(f&PURGE_TXABORT) {
      InterlockedTestExchange(&dx->writeuse, 1, 2);
      SetEvent(dx->writeevent);
    };
    if(f&PURGE_RXABORT) {
      InterlockedTestExchange(&dx->readuse, 1, 2);
      SetEvent(dx->readevent);
    };
    if(f&PURGE_RXCLEAR) {
      int a= dx->recvin;
      if(a==0) a= dx->recvsize;
      dx->recvout= a;
      SetEvent(dx->serialevent);
    };
    rc= TRUE;
  } break;
  case 18:{//SetupComm (SET_QUEUE_SIZE)
    //ignored
    rc= TRUE;
  } break;
  case 19:{//TransmitCommChar
    if(ibuf==0||ilen<sizeof(BYTE)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    USB_TRANSFER ut= dx->issuechar(&ibuf[0]);
    if(ut) {
      DWORD sz= 0;
      dx->uf->lpGetTransferStatus(ut, &sz, 0);
      dx->uf->lpCloseTransfer(ut);
      if(sz==1) rc= TRUE;
    };
  } break;
  case 20:{//GetCommState (GET_DCB)
    if(odone) *odone= 0;
    if(obuf==0||odone==0||olen<sizeof(DCB)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    *(DCB*)obuf= dx->dcb;
    rc= TRUE;
    *odone= sizeof(DCB);
  } break;
  case 21:{//SetCommState (SET_DCB)
    if(ibuf==0||ilen<sizeof(DCB)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    if(((DCB*)ibuf)->DCBlength!=sizeof(DCB)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      break;
    };
    dx->dcb= *(DCB*)ibuf;
    dx->applydcb();
    rc= TRUE;
  } break;
  case 22:{//EscapeCommFunction(SETIR)
    rc= dx->escapecomm(fx, SETIR);
  } break;
  case 23:{//EscapeCommFunction(CLRIR)
    rc= dx->escapecomm(fx, CLRIR);
  } break;
  };
  InterlockedDecrement(&fx->ref);
  return(rc);
};

