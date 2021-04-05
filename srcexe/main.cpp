// 232usb Copyright (c) 03-04 Zoroyoshi, Japan
// See source.txt for detail

#include <windows.h>
#include "resource.h"

WCHAR ent[16][_MAX_PATH];
int nent;
int cent;
int valid;

#define TITLE L"232usb - USB Serial Driver"

void settext(HWND w)
{
  int cursel= SendDlgItemMessage(w, IDC_COMBO2, CB_GETCURSEL, 0, 0);
  switch(cursel) {
  case 1: SetDlgItemText(w, IDC_CHECK1, L"PL2303"); break;
  case 3: SetDlgItemText(w, IDC_CHECK1, L"12MHz"); break;
  default: SetDlgItemText(w, IDC_CHECK1, L"-"); break;
  };
  switch(cursel) {
  case 3: SetDlgItemText(w, IDC_CHECK2, L"FT8U232"); break;
  default: SetDlgItemText(w, IDC_CHECK2, L"No Send 0"); break;
  };
};

void setup(HWND w)
{
  valid= 0;
  HKEY rk= 0;
  WCHAR value[_MAX_PATH];
  if(cent>=0&&cent<nent) RegOpenKeyEx(HKEY_LOCAL_MACHINE, ent[cent], 0, 0, &rk);
  if(rk) {
    int usbmode, classaddr, recvaddr, sendaddr, devicetype, priority, recvsize;
    DWORD rd, rt;
    rd= sizeof(usbmode); rt= 0;
    if(RegQueryValueEx(rk, L"UsbMode", 0, &rt, (BYTE*)&usbmode, &rd)) rt= 0;
    if(rt!=REG_DWORD) usbmode= 1;
    if(usbmode==0x100) usbmode= 1; //old-mode
    rd= sizeof(classaddr); rt= 0;
    if(RegQueryValueEx(rk, L"ClassEndp", 0, &rt, (BYTE*)&classaddr, &rd)) rt= 0;
    if(rt!=REG_DWORD) classaddr= 0;  
    rd= sizeof(recvaddr); rt= 0;
    if(RegQueryValueEx(rk, L"ReceiveEndp", 0, &rt, (BYTE*)&recvaddr, &rd)) rt= 0;
    if(rt!=REG_DWORD) recvaddr= 0;  
    rd= sizeof(sendaddr); rt= 0;
    if(RegQueryValueEx(rk, L"SendEndp", 0, &rt, (BYTE*)&sendaddr, &rd)) rt= 0;
    if(rt!=REG_DWORD) sendaddr= 0;
    rd= sizeof(devicetype); rt= 0;
    if(RegQueryValueEx(rk, L"DeviceType", 0, &rt, (BYTE*)&devicetype, &rd)) rt= 0;
    if(rt!=REG_DWORD) devicetype= 1;
    rd= sizeof(priority); rt= 0;
    if(RegQueryValueEx(rk, L"Priority256", 0, &rt, (BYTE*)&priority, &rd)) rt= 0;
    if(rt!=REG_DWORD) priority= 0;
    rd= sizeof(recvsize); rt= 0;
    if(RegQueryValueEx(rk, L"ReceiveBuffer", 0, &rt, (BYTE*)&recvsize, &rd)) rt= 0;
    if(rt!=REG_DWORD) recvsize= 0;
    RegCloseKey(rk);
    SendDlgItemMessage(w, IDC_COMBO2, CB_SETCURSEL, (usbmode&255), 0);
    SendDlgItemMessage(w, IDC_CHECK1, BM_SETCHECK, (usbmode&0x100)!=0, 0);
    SendDlgItemMessage(w, IDC_CHECK2, BM_SETCHECK, (usbmode&0x200)!=0, 0);
    SendDlgItemMessage(w, IDC_CHECK3, BM_SETCHECK, (usbmode&0x400)!=0, 0);
    SendDlgItemMessage(w, IDC_CHECK4, BM_SETCHECK, (usbmode&0x800)!=0, 0);
    SendDlgItemMessage(w, IDC_CHECK5, BM_SETCHECK, (usbmode&0x1000)!=0, 0);
    SendDlgItemMessage(w, IDC_CHECK6, BM_SETCHECK, devicetype==0, 0);
    wsprintf(value, L"%02x", recvaddr); SetDlgItemText(w, IDC_EDIT1, value);
    wsprintf(value, L"%02x", sendaddr); SetDlgItemText(w, IDC_EDIT2, value);
    wsprintf(value, L"%02x", classaddr); SetDlgItemText(w, IDC_EDIT3, value);
    wsprintf(value, L"%d", priority); SetDlgItemText(w, IDC_EDIT4, value);
    wsprintf(value, L"%d", recvsize); SetDlgItemText(w, IDC_EDIT5, value);
    wcscpy(value, ent[cent]+24);
    *wcschr(value, '\\')= 0;
    SetDlgItemText(w, IDC_STATIC1, value);
    settext(w);
    valid= 1;
  };
};

void changed(HWND w)
{
  if(valid==0) return;
  int usbmode= 0;
  int devicetype= 1;
  usbmode= SendDlgItemMessage(w, IDC_COMBO2, CB_GETCURSEL, 0, 0);
  if(usbmode!=0) if(SendDlgItemMessage(w, IDC_CHECK1, BM_GETCHECK, 0, 0)) usbmode|= 0x100;
  if(SendDlgItemMessage(w, IDC_CHECK2, BM_GETCHECK, 0, 0)) usbmode|= 0x200;
  if(SendDlgItemMessage(w, IDC_CHECK3, BM_GETCHECK, 0, 0)) usbmode|= 0x400;
  if(SendDlgItemMessage(w, IDC_CHECK4, BM_GETCHECK, 0, 0)) usbmode|= 0x800;
  if(SendDlgItemMessage(w, IDC_CHECK5, BM_GETCHECK, 0, 0)) usbmode|= 0x1000;
  if(SendDlgItemMessage(w, IDC_CHECK6, BM_GETCHECK, 0, 0)) devicetype= 0;
  int recvaddr, sendaddr, classaddr, priority, recvsize;
  WCHAR value[_MAX_PATH];
  GetDlgItemText(w, IDC_EDIT1, value, _MAX_PATH); recvaddr= wcstol(value, 0, 16);
  GetDlgItemText(w, IDC_EDIT2, value, _MAX_PATH); sendaddr= wcstol(value, 0, 16);
  GetDlgItemText(w, IDC_EDIT3, value, _MAX_PATH); classaddr= wcstol(value, 0, 16);
  GetDlgItemText(w, IDC_EDIT4, value, _MAX_PATH); priority= wcstol(value, 0, 0);
  GetDlgItemText(w, IDC_EDIT5, value, _MAX_PATH); recvsize= wcstol(value, 0, 0);
  HKEY rk= 0;
  RegOpenKeyEx(HKEY_LOCAL_MACHINE, ent[cent], 0, 0, &rk);
  if(rk) {
    int oldmode;
    DWORD rd, rt;
    rd= sizeof(oldmode); rt= 0;
    if(RegQueryValueEx(rk, L"UsbMode", 0, &rt, (BYTE*)&oldmode, &rd)) rt= 0;
    if(rt!=REG_DWORD) usbmode= 1;
    if(oldmode==0x100) oldmode= 1; //old-mode
    usbmode|= oldmode&~0x1fff;
    RegSetValueEx(rk, L"UsbMode", 0, REG_DWORD, (BYTE*)&usbmode, 4);
    RegSetValueEx(rk, L"ReceiveEndp", 0, REG_DWORD, (BYTE*)&recvaddr, 4);
    RegSetValueEx(rk, L"SendEndp", 0, REG_DWORD, (BYTE*)&sendaddr, 4);
    RegSetValueEx(rk, L"ClassEndp", 0, REG_DWORD, (BYTE*)&classaddr, 4);
    RegSetValueEx(rk, L"DeviceType", 0, REG_DWORD, (BYTE*)&devicetype, 4);
    if(priority==0) {
      RegDeleteValue(rk, L"Priority256");
    } else {
      RegSetValueEx(rk, L"Priority256", 0, REG_DWORD, (BYTE*)&priority, 4);
    };
    if(recvsize==0) {
      RegDeleteValue(rk, L"ReceiveBuffer");
    } else {
      RegSetValueEx(rk, L"ReceiveBuffer", 0, REG_DWORD, (BYTE*)&recvsize, 4);
    };
    RegCloseKey(rk);
  };
};

void deletekey(WCHAR const* kn)
{
  WCHAR value[_MAX_PATH];
  wcscpy(value, kn);
  for(;;) {
    DEBUGMSG(1, (L"%s\n", value));
    RegDeleteKey(HKEY_LOCAL_MACHINE, value);
    //delete tail key
    WCHAR *p= wcsrchr(value, '\\');
    if(p==0||p-value<=23) break; //do not delete loadclients
    *p= 0;
    HKEY rk= 0;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, value, 0, 0, &rk);
    if(rk==0) break;
    DWORD c0= 100, c1= 100;
    RegQueryInfoKey(rk, 0, 0, 0, &c0, 0, 0, &c1, 0, 0, 0, 0);
    RegCloseKey(rk);
    if(c0||c1) break;
  };
};


void listup(HWND w)
{
  SendDlgItemMessage(w, IDC_COMBO1, CB_RESETCONTENT, 0, 0);
  nent= 0;
  DWORD len;
  HKEY rprd;
  RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Drivers\\USB\\LoadClients", 0, 0, &rprd);
  for(int iprd= 0; ; iprd++) {
    WCHAR mprd[_MAX_PATH];
    len= _MAX_PATH;
    if(RegEnumKeyEx(rprd, iprd, mprd, &len, 0, 0, 0, 0)) break;
    HKEY rcls;
    RegOpenKeyEx(rprd, mprd, 0, 0, &rcls);
    for(int icls= 0; ; icls++) {
      WCHAR mcls[_MAX_PATH];
      len= _MAX_PATH;
      if(RegEnumKeyEx(rcls, icls, mcls, &len, 0, 0, 0, 0)) break;
      HKEY rifc;
      RegOpenKeyEx(rcls, mcls, 0, 0, &rifc);
      for(int iifc= 0; ; iifc++) {
        WCHAR mifc[_MAX_PATH];
        len= _MAX_PATH;
        if(RegEnumKeyEx(rifc, iifc, mifc, &len, 0, 0, 0, 0)) break;
        WCHAR value[_MAX_PATH];
	wcscpy(value, mifc);
	wcscat(value, L"\\RS232_USB");
        HKEY rk;
        if(RegOpenKeyEx(rifc, value, 0, 0, &rk)==0) {
	  if(wcsicmp(mprd, L"Default") || wcsicmp(mcls, L"Default")
	  || wcsicmp(mifc, L"Default")) {
	    wsprintf(ent[nent], L"Drivers\\USB\\LoadClients\\%s\\%s\\%s\\RS232_USB"
	    , mprd, mcls, mifc);
	    DWORD type;
	    wcscpy(value, L"No Name");
	    len= sizeof(value);
            RegQueryValueEx(rk, L"FriendlyName", 0, &type, (BYTE*)value, &len);
            SendDlgItemMessage(w, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)value);
	    nent++;
	  };
          RegCloseKey(rk);
        };
      };
      RegCloseKey(rifc);
    };
    RegCloseKey(rcls);
  };
  RegCloseKey(rprd);
  valid= 0;
  RECT r;
  GetWindowRect(GetDlgItem(w, IDC_COMBO1), &r);
  SetWindowPos(GetDlgItem(w, IDC_COMBO1), 0, 0, 0, r.right-r.left, 120, SWP_NOZORDER|SWP_NOMOVE);
  cent= 0;
  SendDlgItemMessage(w, IDC_COMBO1, CB_SETCURSEL, cent, 0);
  setup(w);
};

WCHAR textbuf[_MAX_PATH];
BOOL CALLBACK
textproc(
  HWND w,
  UINT msg,
  WPARAM wp,
  LPARAM lp
) {
  switch(msg) {
  case WM_INITDIALOG: {
    SetDlgItemText(w, IDC_EDIT1, textbuf);
  } break;
  case WM_COMMAND: {
    switch(LOWORD(wp)) {
    case IDOK: {
      GetDlgItemText(w, IDC_EDIT1, textbuf, _MAX_PATH);
      EndDialog(w, TRUE);
      return(TRUE);
    } break;
    case IDCANCEL: {
      EndDialog(w, FALSE);
      return(TRUE);
    } break;
    };
  } break;
  };
  return(FALSE);
};

BOOL CALLBACK
messageproc(
  HWND w,
  UINT msg,
  WPARAM wp,
  LPARAM lp
) {
  switch(msg) {
  case WM_INITDIALOG: {
    SetWindowText(w, TITLE);
    SendDlgItemMessage(w, IDC_COMBO2, CB_ADDSTRING, 0, (LPARAM)L"BASIC");
    SendDlgItemMessage(w, IDC_COMBO2, CB_ADDSTRING, 0, (LPARAM)L"CDC ACM");
    SendDlgItemMessage(w, IDC_COMBO2, CB_ADDSTRING, 0, (LPARAM)L"CDC WMC");
    SendDlgItemMessage(w, IDC_COMBO2, CB_ADDSTRING, 0, (LPARAM)L"FTDI");
    SendDlgItemMessage(w, IDC_COMBO2, CB_ADDSTRING, 0, (LPARAM)L"RCS310");
    RECT r;
    GetWindowRect(GetDlgItem(w, IDC_COMBO2), &r);
    SetWindowPos(GetDlgItem(w, IDC_COMBO2), 0, 0, 0, r.right-r.left, 200, SWP_NOZORDER|SWP_NOMOVE);
    listup(w);
    return(TRUE);
  } break;
  case WM_COMMAND: {
    switch(LOWORD(wp)) {
    case IDC_COMBO1: {
      if(HIWORD(wp)==CBN_SELCHANGE) {
        cent= SendDlgItemMessage(w, IDC_COMBO1, CB_GETCURSEL, 0, 0);
	setup(w);
      };
    } break;
    case IDC_COMBO2: {
      if(HIWORD(wp)==CBN_SELCHANGE) {
        settext(w);
        changed(w);
      };
    } break;
    case IDC_CHECK1: case IDC_CHECK2: case IDC_CHECK3: case IDC_CHECK4: case IDC_CHECK5:
    case IDC_CHECK6: {
      if(HIWORD(wp)==BN_CLICKED) changed(w);
    } break;
    case IDC_EDIT1: case IDC_EDIT2: case IDC_EDIT3: case IDC_EDIT4: case IDC_EDIT5: {
      if(HIWORD(wp)==EN_CHANGE) changed(w);
    } break;
    case IDC_BUTTON1: {
      if(valid) {
	SendDlgItemMessage(w, IDC_COMBO1, CB_GETLBTEXT, cent, (LPARAM)textbuf);
        if(DialogBox(GetModuleHandle(0), (WCHAR*)IDD_DIALOG2, w, textproc)&&textbuf[0]) {
          HKEY rk= 0;
          RegOpenKeyEx(HKEY_LOCAL_MACHINE, ent[cent], 0, 0, &rk);
          if(rk) {
            RegSetValueEx(rk, L"FriendlyName", 0, REG_SZ, (BYTE*)textbuf, (wcslen(textbuf)+1)*2);
            RegCloseKey(rk);
	    SendDlgItemMessage(w, IDC_COMBO1, CB_DELETESTRING, cent, 0);
	    SendDlgItemMessage(w, IDC_COMBO1, CB_INSERTSTRING, cent, (LPARAM)textbuf);
	    SendDlgItemMessage(w, IDC_COMBO1, CB_SETCURSEL, cent, 0);
	  };
	};
      };
    } break;
    case IDC_BUTTON2: {
      if(HIWORD(wp)==BN_CLICKED) {
        if(valid) {
	  deletekey(ent[cent]);
	  listup(w);
	} else {
	  deletekey(L"Drivers\\USB\\LoadClients\\Default\\Default\\Default\\RS232_USB");
	};
      };
    } break;
    case IDC_BUTTON3: {
      if(HIWORD(wp)==BN_CLICKED) {
        //create force-load key
	HKEY hk= 0;
	DWORD rd;
	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE
        , L"Drivers\\USB\\LoadClients\\Default\\Default\\Default\\RS232_USB"
        , 0, 0, 0, 0, 0, &hk, &rd)) hk= 0;
	WCHAR* dllname= L"232usb";
        RegSetValueEx(hk, L"Dll", 0, REG_SZ, (BYTE*)dllname, (wcslen(dllname)+1)*sizeof(WCHAR));
        DWORD d= 1;
        RegSetValueEx(hk, L"InstallMode", 0, REG_DWORD, (BYTE*)&d, sizeof(d));
        RegCloseKey(hk);
        MessageBox(w, L"Connect device NOW", L"232usb", MB_OK|MB_APPLMODAL);
	deletekey(L"Drivers\\USB\\LoadClients\\Default\\Default\\Default\\RS232_USB");
	listup(w);
      };
    } break;
    case IDOK: {
      EndDialog(w, TRUE);
      return(TRUE);
    } break;
    case IDCANCEL: {
      EndDialog(w, FALSE);
      return(TRUE);
    } break;
    };
  } break;
  };
  return(FALSE);
};


int WINAPI WinMain(	HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPTSTR    lpCmdLine,
					int       nCmdShow)
{
  HWND bw= FindWindow(0, TITLE);
  if(bw) {
    SetForegroundWindow((HWND)((DWORD)bw|1));
    return TRUE;
  };
  DialogBox(hInstance, (WCHAR*)IDD_DIALOG1, 0, messageproc);
	return 0;
}
