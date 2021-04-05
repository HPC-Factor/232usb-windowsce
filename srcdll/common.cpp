// 232usb Copyright (c) 03-04 Zoroyoshi, Japan
// See source.txt for detail

#include <windows.h>
#include "usbdi.h"
#include "common.h"

#if DEBUG
DBGPARAM dpCurSettings = {
  L"232usb",
  L"Test", L"Alloc", L"Driver", L"File",
  L"Init", L"Usb", L"Dump", L"Prio",
  L"", L"", L"", L"",
  L"", L"", L"", L"Error",
  ZM_TEST|ZM_ERROR
};

void zmesg(ULONG b, WCHAR const* fmt, ...)
{
  static DWORD base= 0;
  static char lc= '\n';
  DWORD cur= GetTickCount();
  if(base==0) base= cur;
  if(b!=(ULONG)-1&&(b&dpCurSettings.ulZoneMask)==0) return;
  cur-= base;
  va_list va;
  va_start(va, fmt);
  static int inited= 0;
  static CRITICAL_SECTION cs;
  char buf[1000];
  int n= 0;
  if(lc=='\n') n= swprintf((WCHAR*)buf, L"%06d ", cur%1000000);
  n+= vswprintf((WCHAR*)buf+n, fmt, va);
  char*p= buf;
  for(int i= 0; i<n; i++) {
    lc= buf[i*2];
    if(lc=='\n') *p++= '\r';
    *p++= lc;
  };
  if(inited==0) {
    InitializeCriticalSection(&cs);
    inited= 1;
  };
  EnterCriticalSection(&cs);
  HANDLE fh= CreateFile(L"\\hoe.txt", GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  SetFilePointer(fh, 0, 0, FILE_END);
  DWORD rd;
  WriteFile(fh, buf, p-buf, &rd, 0);
  CloseHandle(fh);
  LeaveCriticalSection(&cs);
};

void infodump(
  USB_HANDLE uh,
  USB_FUNCS const* uf,
  USB_DEVICE const* ui)
{
  if(ui==0) return;
  zmesg(-1,L"count=%d configs=x%x active=x%x\n", ui->dwCount, ui->lpConfigs, ui->lpActiveConfig);
  zmesg(-1,L"Descriptor=%d %d x%x\n", ui->Descriptor.bLength, ui->Descriptor.bDescriptorType, ui->Descriptor.bcdUSB);
  zmesg(-1,L"  device=%d %d %d pkt=%d\n", ui->Descriptor.bDeviceClass, ui->Descriptor.bDeviceSubClass, ui->Descriptor.bDeviceProtocol, ui->Descriptor.bMaxPacketSize0);
  zmesg(-1,L"  vendor=x%x x%x x%x\n", ui->Descriptor.idVendor, ui->Descriptor.idProduct, ui->Descriptor.bcdDevice);
  zmesg(-1,L"  manu='%d product='%d serial='%d numconfig=%d\n", ui->Descriptor.iManufacturer, ui->Descriptor.iProduct, ui->Descriptor.iSerialNumber, ui->Descriptor.bNumConfigurations);
  zmesg(-1,L"configvalue=%d wTotal=%d interfaces=%d attr=x%x power=%d ext=x%x\n"
  , ui->lpActiveConfig->Descriptor.bConfigurationValue, ui->lpActiveConfig->Descriptor.wTotalLength
  , ui->lpActiveConfig->Descriptor.bNumInterfaces, ui->lpActiveConfig->Descriptor.bmAttributes, ui->lpActiveConfig->Descriptor.MaxPower
  , ui->lpActiveConfig->lpvExtended);
  zmesg(-1,L"numinterfaces*numalt=%d count=%d\n", ui->lpActiveConfig->dwNumInterfaces, ui->lpActiveConfig->dwCount);
  USB_INTERFACE const* ua= ui->lpActiveConfig->lpInterfaces;
  for(UINT i= 0; i<ui->lpActiveConfig->dwNumInterfaces; i++) {
    zmesg(-1,L"num=%d alt=%d endpoints=%d\n", ua->Descriptor.bInterfaceNumber, ua->Descriptor.bAlternateSetting, ua->Descriptor.bNumEndpoints);
    zmesg(-1,L"  interface=%d %d %d\n", ua->Descriptor.bInterfaceClass, ua->Descriptor.bInterfaceSubClass, ua->Descriptor.bInterfaceProtocol);
    zmesg(-1,L"  intstr='%d ext=x%x\n", ua->Descriptor.iInterface, ua->lpvExtended);
    if(ua->lpvExtended) {
      int sz= _msize((void*)ua->lpvExtended);
      BYTE* e= (BYTE*)ua->lpvExtended;
      zmesg(-1,L"  [%d]", sz);
      for(int i= 0; i<sz; i++) zmesg(-1,L" %02x", e[i]);
      zmesg(-1,L"\n");
    };
    USB_ENDPOINT const* ue= ua->lpEndpoints;
    for(UINT j= 0; j<ua->Descriptor.bNumEndpoints; j++) {
      zmesg(-1,L"  endpoint=%d count=%d addr=%d attr=x%x pkt=%d interval=%d ext=x%x\n", j, ue->dwCount, ue->Descriptor.bEndpointAddress, ue->Descriptor.bmAttributes, ue->Descriptor.wMaxPacketSize, ue->Descriptor.bInterval
      , ue->lpvExtended);
      if(ue->lpvExtended) {
        int sz= _msize((void*)ue->lpvExtended);
        BYTE* e= (BYTE*)ue->lpvExtended;
	zmesg(-1,L"  [%d]", sz);
	for(int i= 0; i<sz; i++) zmesg(-1,L" %02x", e[i]);
	zmesg(-1,L"\n");
      };
      ue++;
    };
    ua++;
  };
};
#endif
