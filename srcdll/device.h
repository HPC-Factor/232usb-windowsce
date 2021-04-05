#pragma once

//device functions
struct file_extension;

#define MAX_LAP 8

struct device_extension {
  // functions
  BOOL initbase();
  virtual BOOL init();
  BOOL deinitbase();
  virtual BOOL deinit();
  virtual void unblock();
  BOOL reset(); //reset device_extension
  BOOL open();
  ULONG close();
  virtual BOOL applydcb();
  virtual USB_TRANSFER issuesend(UCHAR*, DWORD);
  virtual USB_TRANSFER issuechar(UCHAR*);
  virtual USB_TRANSFER issuexon(UCHAR*);
  virtual int sentlen(DWORD*); //returns whether extra zero length transfer needed
  virtual USB_TRANSFER issuebreak(int);
  virtual USB_TRANSFER issuelinestate(int, int, HANDLE);

  DWORD comstat(COMSTAT* buf);
  virtual DWORD escapecomm(file_extension*, int);

  BOOL evaluateevent(DWORD);

  int recvissue();
  int recvwait();
  virtual int recvdata(UCHAR*, DWORD);
  int recvline();
  int linestatein(DWORD, DWORD);
  int dispatch();
  static DWORD WINAPI dispatchwrap(void*);
  virtual int dispatchdo();

  device_extension* parent; //parent desc.
  file_extension* list;
  CRITICAL_SECTION listlock;

  //initialize at DeviceAttach
  HANDLE ud; //returned by activate_device
  USB_HANDLE uh;
  USB_FUNCS const *uf;
  int priority;
  int attachprio; //priority of deviceattach process
  HANDLE dispatchthread;
  HANDLE serialevent;

  int usbmode;
  int classif; //interface number
  const USB_ENDPOINT *classendp, *recvendp, *sendendp;

  USB_PIPE recvpipe;
  int recvsize;
  BYTE* recvbuf; //receive buffer
  int recvin;   //data-in pointer 0..RECVSIZE-1
  int recvout;  //data-out pointer 1..RECVSIZE

  USB_PIPE sendpipe;

  int recvlap; //overlapped receive
  int recvpkt; //receive issue size
  int recvstep; //aligned recvpkt
  //DWORD recvphys; //physical address(not used)
  BYTE* recvvirt; //virtual address
  int recvcur; //current queue position
  int recvnum; //current waiting number
  USB_TRANSFER recvut[MAX_LAP];
  USB_TRANSFER utflowc, utflows;

  int sendpkt;

  //initialize at COM_Init
  COMMTIMEOUTS timeout;
  //initialize at first COM_Open
  DCB dcb;
  DWORD errors; //all or of errors
  DWORD linein; //current state, 1=MS_BREAK_ON

  HANDLE writeevent;
  long writeuse; //0=normal/1=using
  HANDLE readevent; //event recvthread->read
  long readuse; //0=normal/1=using

  long sendxoff;  //1=xoffchar received
  long recvflow;  //1=recv flow stopped
  CRITICAL_SECTION flowcs; //for lineout/VendorTransfer and recvflow/Ctrl-S
};
