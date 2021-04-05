// 232usb Copyright (c) 03-04,06 Zoroyoshi, Japan
// See source.txt for detail

#include <windows.h>
#include "usbdi.h"
#include "common.h"
#include "file.h"
#include "device.h"
#include "cdc.h"
#include "ftdi.h"
#include "rcs.h"
#include "wmc.h"

#define CLIENTNAME L"RS232_USB"

BOOL APIENTRY
DllMain(
    HANDLE module, DWORD code, void *resv)
{
  switch (code)
  {
  case DLL_PROCESS_ATTACH:
  { //DisableThreadLibraryCalls exists after CE3.0
    DEBUGREGISTER((HINSTANCE)module);
    zmesg(ZM_DRIVER, L"processattach\n");
    int (*proc)(...) = (int (*)(...))GetProcAddress(GetModuleHandle(L"coredll"), L"DisableThreadLibraryCalls");
    if (proc)
      (*proc)(module);
  };
  break;
  case DLL_PROCESS_DETACH:
    zmesg(ZM_DRIVER, L"processdetach\n");
    break;
  default:
    break;
  };
  return TRUE;
};

extern "C" BOOL
USBInstallDriver(
    PCWCH dllname)
{
  zmesg(ZM_DRIVER, L"installdriver\n");

  HKEY hk;
  DWORD rd;

  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Drivers\\USB\\LoadClients\\Default\\Default\\Default\\" CLIENTNAME, 0, 0, 0, 0, 0, &hk, &rd))
    hk = 0;
  RegSetValueEx(hk, L"Dll", 0, REG_SZ, (BYTE *)dllname, (wcslen(dllname) + 1) * sizeof(WCHAR));
  DWORD d = GetTickCount();
  RegSetValueEx(hk, L"InstallTime", 0, REG_DWORD, (BYTE *)&d, sizeof(d));
  RegCloseKey(hk);

  return TRUE;
};

extern "C" BOOL
USBUnInstallDriver(void)
{
  //never called
  zmesg(ZM_DRIVER, L"uninstalldriver\n");
  return TRUE;
};

BOOL WINAPI
USBDeviceNotify(
    PVOID parm, DWORD code,
    PDWORD *d1, PDWORD *d2, PDWORD *d3, PDWORD *d4)
{
  zmesg(ZM_DRIVER, L"devicenotify\n");
  device_extension *dx = (device_extension *)parm;
  if (code == USB_CLOSE_DEVICE)
  {
    if (dx == 0)
      return FALSE;
    if (dx->ud)
    {
      DeactivateDevice(dx->ud); //stop new api..calling COM_Deinit
      dx->deinitbase();
    };
    delete dx;
    zmesg(ZM_ALLOC, L"<free-dx%x>\n", dx);
    return TRUE;
  };
  return FALSE;
};

extern "C" BOOL
USBDeviceAttach(
    USB_HANDLE uh,
    PCUSB_FUNCS uf,
    PCUSB_INTERFACE iface,
    PCWCH clientname,
    PBOOL accept,
    PCUSB_DRIVER_SETTINGS sets,
    DWORD resv)
{
  zmesg(ZM_DRIVER, L"deviceattach\n");
  *accept = FALSE;

  USB_DEVICE const *info = uf->lpGetDeviceInfo(uh); //static storage; needless to free
#if DEBUG
  infodump(uh, uf, info);
#endif

  WCHAR ckey[256]; //vendor-specific client registry
  WCHAR *sp;
  sp = ckey;
  sp += wsprintf(sp, L"Drivers\\USB\\LoadClients");
  if (sets->dwVendorId != USB_NO_INFO)
  {
    sp += wsprintf(sp, L"\\%u", sets->dwVendorId);
    if (sets->dwProductId != USB_NO_INFO)
    {
      sp += wsprintf(sp, L"_%u", sets->dwProductId);
      if (sets->dwReleaseNumber != USB_NO_INFO)
        sp += wsprintf(sp, L"_%u", sets->dwReleaseNumber);
    };
  }
  else
    sp += wsprintf(sp, L"\\Default");
  if (sets->dwDeviceClass != USB_NO_INFO)
  {
    sp += wsprintf(sp, L"\\%u", sets->dwDeviceClass);
    if (sets->dwDeviceSubClass != USB_NO_INFO)
    {
      sp += wsprintf(sp, L"_%u", sets->dwDeviceSubClass);
      if (sets->dwDeviceProtocol != USB_NO_INFO)
        sp += wsprintf(sp, L"_%u", sets->dwDeviceProtocol);
    };
  }
  else
    sp += wsprintf(sp, L"\\Default");
  if (sets->dwInterfaceClass != USB_NO_INFO)
  {
    sp += wsprintf(sp, L"\\%u", sets->dwInterfaceClass);
    if (sets->dwInterfaceSubClass != USB_NO_INFO)
    {
      sp += wsprintf(sp, L"_%u", sets->dwInterfaceSubClass);
      if (sets->dwInterfaceProtocol != USB_NO_INFO)
        sp += wsprintf(sp, L"_%u", sets->dwInterfaceProtocol);
    };
  }
  else
    sp += wsprintf(sp, L"\\Default");
  sp += wsprintf(sp, L"\\%s", clientname);

  HKEY hk;
  int usbmode, priority, recvsize;
  DWORD classaddr, recvaddr, sendaddr;
  DWORD rt, rd;

  if (sets->dwVendorId == USB_NO_INFO && sets->dwDeviceClass == USB_NO_INFO && sets->dwInterfaceClass == USB_NO_INFO)
  {
    WCHAR dllname[64];
    DWORD instime;
    DWORD instmode;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, ckey, 0, 0, &hk))
      return FALSE;
    rd = sizeof(dllname) - sizeof(WCHAR);
    if (RegQueryValueEx(hk, L"Dll", 0, &rt, (BYTE *)dllname, &rd))
      rt = 0;
    if (rt == REG_SZ)
      dllname[rd / sizeof(WCHAR)] = 0 /*guard*/;
    else
      dllname[0] = 0;
    rd = sizeof(instime);
    rt = 0;
    if (RegQueryValueEx(hk, L"InstallTime", 0, &rt, (BYTE *)&instime, &rd))
      rt = 0;
    if (rt == REG_DWORD)
      instime = GetTickCount() - instime;
    else
      instime = ~0;
    rd = sizeof(instmode);
    rt = 0;
    if (RegQueryValueEx(hk, L"InstallMode", 0, &rt, (BYTE *)&instmode, &rd))
      rt = 0;
    if (rt == REG_DWORD)
    {
    }
    else
      instmode = 0;
    RegDeleteValue(hk, L"Dll");
    RegDeleteValue(hk, L"InstallTime");
    RegDeleteValue(hk, L"InstallMode");
    RegCloseKey(hk);
    if (instmode == 0 && instime >= 3000 || dllname[0] == 0)
      return FALSE; //not loaded

    wsprintf(ckey, L"Drivers\\USB\\LoadClients\\%u_%u_%u\\Default\\Default\\%s", info->Descriptor.idVendor, info->Descriptor.idProduct, info->Descriptor.bcdDevice, clientname);

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, ckey, 0, 0, 0, 0, 0, &hk, &rd))
      return FALSE; //error create

    //initialize registries
    RegSetValueEx(hk, L"Dll", 0, REG_SZ, (BYTE *)dllname, (wcslen(dllname) + 1) * sizeof(WCHAR));
    WCHAR *v;
    v = L"COM";
    RegSetValueEx(hk, L"Prefix", 0, REG_SZ, (BYTE *)v, (wcslen(v) + 1) * sizeof(WCHAR));
    v = L"Unimodem.dll";
    RegSetValueEx(hk, L"Tsp", 0, REG_SZ, (BYTE *)v, (wcslen(v) + 1) * sizeof(WCHAR));
    rd = 1;
    RegSetValueEx(hk, L"DeviceType", 0, REG_DWORD, (BYTE *)&rd, sizeof(rd));

    //analyze descriptor
    int curcdc = 0;
    int classw = 0;
    int recvw = 0;
    int sendw = 0; //probability
    classaddr = 0;
    recvaddr = 0;
    sendaddr = 0;
    int recvw2 = 0;
    int sendw2 = 0;
    int recvaddr2 = 0;
    int sendaddr2 = 0;
    USB_INTERFACE const *ua = info->lpActiveConfig->lpInterfaces;
    for (UINT ia = 0; ia < info->lpActiveConfig->dwNumInterfaces; ia++, ua++)
    {
      if (ua->Descriptor.bAlternateSetting == 0)
      {
        if (ua->Descriptor.bInterfaceClass == 2)
        { //CDC
          curcdc = ua->Descriptor.bInterfaceSubClass;
        };
        USB_ENDPOINT const *ue = ua->lpEndpoints;
        int ee = 0; //error
        USB_ENDPOINT_DESCRIPTOR const *ec = 0;
        USB_ENDPOINT_DESCRIPTOR const *er = 0;
        USB_ENDPOINT_DESCRIPTOR const *es = 0;
        for (UINT ie = 0; ie < ua->Descriptor.bNumEndpoints; ie++, ue++)
        {
          if ((ue->Descriptor.bEndpointAddress & 128) && (ue->Descriptor.bmAttributes & 3) == 3)
          { //interrupt in
            if (ec)
              ee++;
            ec = &ue->Descriptor;
          }
          else if ((ue->Descriptor.bEndpointAddress & 128) && (ue->Descriptor.bmAttributes & 3) == 2)
          { //bulk in
            if (er)
              ee++;
            er = &ue->Descriptor;
          }
          else if ((ue->Descriptor.bEndpointAddress & 128) == 0 && (ue->Descriptor.bmAttributes & 3) == 2)
          { //bulk out
            if (es)
              ee++;
            es = &ue->Descriptor;
          };
        };
        if (ee == 0)
        {
          if (ec)
          {
            int w = 1;
            if ((ec->wMaxPacketSize & 0x7ff) >= 10)
              w += 2;
            if (er && es && ua->Descriptor.bNumEndpoints == 3)
              w += 1;
            if (ua->Descriptor.bNumEndpoints == 1)
              w += 5;
            if (ua->Descriptor.bInterfaceClass == 2)
            { //CDC
              w += 20;
              if (ua->Descriptor.bInterfaceSubClass == 2)
              { //ACM
                w += 10;
              };
            };
            if (classw < w)
            {
              classw = w;
              classaddr = ec->bEndpointAddress;
            };
          };
          if (er)
          {
            int w = 1;
            if (es)
            {
              w += 1;
              if ((er->bEndpointAddress ^ es->bEndpointAddress) == 128)
                w += 1; //same endpoint number
              if (ua->Descriptor.bNumEndpoints == 2)
                w += 2;
            };
            if (ua->Descriptor.bInterfaceClass == 10)
            {
              w += 20; //CDC-data
              if (curcdc == 2)
                w += 10;
              if (curcdc == 11)
                w += 5;
            }
            else if (ua->Descriptor.bInterfaceClass == 2)
              w += 1; //CDC
            if (recvw < w)
            {
              recvw2 = recvw;
              recvaddr2 = recvaddr;
              recvw = w;
              recvaddr = er->bEndpointAddress;
            }
            else if (recvw2 < w)
            {
              recvw2 = w;
              recvaddr2 = er->bEndpointAddress;
            };
          };
          if (es)
          {
            int w = 1;
            if (er)
            {
              w += 1;
              if ((er->bEndpointAddress ^ es->bEndpointAddress) == 128)
                w += 1; //same endpoint number
              if (ua->Descriptor.bNumEndpoints == 2)
                w += 2;
            };
            if (ua->Descriptor.bInterfaceClass == 10)
            {
              w += 20; //CDC-data
              if (curcdc == 2)
                w += 10;
              if (curcdc == 11)
                w += 5;
            }
            else if (ua->Descriptor.bInterfaceClass == 2)
              w += 1; //CDC
            if (sendw < w)
            {
              sendw2 = sendw;
              sendaddr2 = sendaddr;
              sendw = w;
              sendaddr = es->bEndpointAddress;
            }
            else if (sendw2 < w)
            {
              sendw2 = w;
              sendaddr2 = es->bEndpointAddress;
            };
          };
        };
      };
    };
    if (recvaddr == 0)
    {
      recvaddr = classaddr;
      classaddr = 0;
    };
    usbmode = MODE_BASIC;
    if (classw >= 20 && recvw2 >= 20 && sendw2 >= 20)
    { //data port found...
      usbmode = MODE_WMC;
      sendaddr |= sendaddr2 << 8;
      recvaddr |= recvaddr2 << 8;
    }
    else if (classaddr)
    {
      usbmode = MODE_CDC;
      if (info->lpActiveConfig->dwNumInterfaces == 1 && classaddr == 129 && recvaddr == 131 && sendaddr == 2)
        usbmode |= MODE_2303;
    };
    if (info->Descriptor.idVendor == 0x0403)
    {
      if (info->Descriptor.idProduct == 0x8372)
      {
        usbmode = MODE_FTDI;
      }
      else
      {
        usbmode = MODE_FTDI | MODE_8U232;
      };
    };
    if (info->Descriptor.idVendor == 0x054c && info->Descriptor.idProduct == 0x006c)
      usbmode = MODE_RCS;
    //AH-K3001V sometimes stall when SET_LINE_CODING is sent
    if (info->Descriptor.idVendor == 0x0482 && info->Descriptor.idProduct == 0x0203)
      usbmode |= MODE_NOBAUD;

    RegSetValueEx(hk, L"ClassEndp", 0, REG_DWORD, (BYTE *)&classaddr, sizeof(classaddr));
    RegSetValueEx(hk, L"ReceiveEndp", 0, REG_DWORD, (BYTE *)&recvaddr, sizeof(recvaddr));
    RegSetValueEx(hk, L"SendEndp", 0, REG_DWORD, (BYTE *)&sendaddr, sizeof(sendaddr));
    RegSetValueEx(hk, L"UsbMode", 0, REG_DWORD, (BYTE *)&usbmode, sizeof(usbmode));

    //get friendly name
    rt = info->Descriptor.iProduct;
    if (rt == 0)
      rt = info->Descriptor.iManufacturer;
    if (rt != 0)
    {
      //HPW-600JC does not allow USB_SHORT_TRANSFER in IssueVendorTransfer,
      //so we get size first.
#if 1
      USB_TRANSFER ut = uf->lpGetDescriptor(uh, 0, 0, 0, USB_STRING_DESCRIPTOR_TYPE, (BYTE)rt, 0x409, 2, dllname);
#else
      USB_DEVICE_REQUEST req;
      req.bmRequestType = USB_REQUEST_DEVICE_TO_HOST | USB_REQUEST_STANDARD | USB_REQUEST_FOR_DEVICE;
      req.bRequest = USB_REQUEST_GET_DESCRIPTOR;
      req.wValue = (WORD)(USB_STRING_DESCRIPTOR_TYPE << 8 | rt);
      req.wIndex = 0x0409;
      req.wLength = 2; //get size&type
      USB_TRANSFER ut = uf->lpIssueVendorTransfer(uh, 0, 0, USB_IN_TRANSFER | USB_SEND_TO_DEVICE | USB_SHORT_TRANSFER_OK, &req, dllname, 0);
#endif
      if (ut)
      {
        rd = 0;
        uf->lpGetTransferStatus(ut, &rd, 0);
        uf->lpCloseTransfer(ut);
        if (!(rd == 2 && *(BYTE *)dllname <= sizeof(dllname) - sizeof(WCHAR)))
          rt = 0;
      }
      else
        rt = 0;
      if (rt)
      {
#if 1
        ut = uf->lpGetDescriptor(uh, 0, 0, 0, USB_STRING_DESCRIPTOR_TYPE, (BYTE)rt, 0x409, *(BYTE *)dllname, dllname);
#else
        req.wLength = *(BYTE *)dllname;
        ut = uf->lpIssueVendorTransfer(uh, 0, 0, USB_IN_TRANSFER | USB_SEND_TO_DEVICE | USB_SHORT_TRANSFER_OK, &req, dllname, 0);
#endif
        if (ut)
        {
          rd = 0;
          uf->lpGetTransferStatus(ut, &rd, 0);
          uf->lpCloseTransfer(ut);
          if (*(BYTE *)dllname == rd)
          {
            dllname[*(BYTE *)dllname / sizeof(WCHAR)] = 0;
          }
          else
            rt = 0;
        }
        else
          rt = 0;
      };
    };
    if (rt == 0)
      wcscpy(dllname + 1, L"RS232 USB");
    WCHAR *rs = 0;
    if ((usbmode & 0xff) == MODE_RCS)
    {
      rs = L",rcs310";
    }
    else if ((usbmode & 0xff) == MODE_FTDI)
    {
      rs = L",ftdi";
    }
    else if ((usbmode & 0xff) == MODE_CDC)
    {
      if (usbmode & MODE_2303)
        rs = L",pl2303";
      else
        rs = L",cdc";
    }
    else if ((usbmode & 0xff) == MODE_WMC)
    {
      rs = L",wmc";
    }
    else
    {
      rs = L",basic";
    };
    rt = wcslen(dllname + 1) + 1;
    if (rs && rt + wcslen(rs) < numof(dllname))
      wcscpy(dllname + rt, rs);
    RegSetValueEx(hk, L"FriendlyName", 0, REG_SZ, (BYTE *)(dllname + 1), (wcslen(dllname + 1) + 1) * sizeof(WCHAR));

    RegCloseKey(hk);
    return FALSE;
  };

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, ckey, 0, 0, &hk))
    return FALSE;
  rd = sizeof(usbmode);
  rt = 0;
  if (RegQueryValueEx(hk, L"UsbMode", 0, &rt, (BYTE *)&usbmode, &rd))
    rt = 0;
  if (usbmode == 0x100)
    usbmode = MODE_CDC; //old-mode

  rd = sizeof(classaddr);
  rt = 0;
  if (RegQueryValueEx(hk, L"ClassEndp", 0, &rt, (BYTE *)&classaddr, &rd))
    rt = 0;
  if (rt != REG_DWORD)
    classaddr = 0;
  rd = sizeof(recvaddr);
  rt = 0;
  if (RegQueryValueEx(hk, L"ReceiveEndp", 0, &rt, (BYTE *)&recvaddr, &rd))
    rt = 0;
  if (rt != REG_DWORD)
    recvaddr = 0;
  rd = sizeof(sendaddr);
  rt = 0;
  if (RegQueryValueEx(hk, L"SendEndp", 0, &rt, (BYTE *)&sendaddr, &rd))
    rt = 0;
  if (rt != REG_DWORD)
    sendaddr = 0;
  rd = sizeof(priority);
  rt = 0;
  if (RegQueryValueEx(hk, L"Priority256", 0, &rt, (BYTE *)&priority, &rd))
    rt = 0;
  if (rt != REG_DWORD)
    priority = 0;
  rd = sizeof(recvsize);
  rt = 0;
  if (RegQueryValueEx(hk, L"ReceiveBuffer", 0, &rt, (BYTE *)&recvsize, &rd))
    rt = 0;
  if (rt != REG_DWORD)
    recvsize = 0;

  RegCloseKey(hk);

#if _WIN32_WCE >= 300
  zmesg(ZM_PRIO, L"usbdeviceattach p=%d\n", CeGetThreadPriority(GetCurrentThread()));
#else
  zmesg(ZM_PRIO, L"usbdeviceattach p=%d\n", GetThreadPriority(GetCurrentThread()));
#endif

  device_extension *dx = 0;
  switch (usbmode & 0xff)
  {
  case MODE_BASIC:
    dx = new device_extension;
    break;
  case MODE_CDC:
    dx = new cdc_extension;
    break;
  case MODE_WMC:
    dx = new wmc_extension;
    break;
  case MODE_FTDI:
    if (usbmode & MODE_8U232)
      dx = new ftdi_extension;
    else
      dx = new ft100_extension;
    break;
  case MODE_RCS:
    dx = new rcs_extension;
    break;
  };
  zmesg(ZM_ALLOC, L"<alloc-dx%x>\n", dx);
  if (dx == 0)
    return FALSE;

  dx->parent = 0;
  dx->uh = uh;
  dx->uf = uf;
  dx->uf->lpRegisterNotificationRoutine(dx->uh, USBDeviceNotify, dx);

  {
    dx->usbmode = usbmode;
    dx->priority = priority;
    if (recvsize == 0)
      recvsize = 4096;
    dx->recvsize = recvsize;
    dx->classif = 0;
    dx->classendp = 0;
    dx->recvendp = 0;
    dx->sendendp = 0;
    USB_ENDPOINT const *recvendp2 = 0, *sendendp2 = 0;
    USB_INTERFACE const *ua = info->lpActiveConfig->lpInterfaces;
    for (UINT ia = 0; ia < info->lpActiveConfig->dwNumInterfaces; ia++, ua++)
    {
      if (ua->Descriptor.bAlternateSetting == 0)
      {
        USB_ENDPOINT const *ue = ua->lpEndpoints;
        for (UINT ie = 0; ie < ua->Descriptor.bNumEndpoints; ie++, ue++)
        {
          if (ue->Descriptor.bEndpointAddress == (classaddr & 255))
          {
            dx->classendp = ue;
            dx->classif = ua->Descriptor.bInterfaceNumber;
          };
          if (ue->Descriptor.bEndpointAddress == (recvaddr & 255))
          {
            dx->recvendp = ue;
          };
          if (ue->Descriptor.bEndpointAddress == (sendaddr & 255))
          {
            dx->sendendp = ue;
          };
          if (ue->Descriptor.bEndpointAddress == (recvaddr >> 8 & 255))
          {
            recvendp2 = ue;
          };
          if (ue->Descriptor.bEndpointAddress == (sendaddr >> 8 & 255))
          {
            sendendp2 = ue;
          };
        };
      };
    };
    if (dx->usbmode & MODE_WMC)
    {
      ((wmc_extension *)dx)->obex.recvendp = recvendp2;
      ((wmc_extension *)dx)->obex.sendendp = sendendp2;
    };
  };

  *accept = TRUE;
  dx->ud = 0;

  zmesg(ZM_INIT, L"  calling dx->init()\n");
  if (dx->initbase() == 0)
    return TRUE; //device connected, but do not activate the device
  zmesg(ZM_INIT, L"  calling activatedevice()\n");
  dx->ud = ActivateDevice(ckey, (DWORD)dx);
  zmesg(ZM_INIT, L"  called activatedevice()\n");
  return TRUE;
};
