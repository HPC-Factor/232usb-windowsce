struct device_extension;


struct file_extension {
  file_extension* list; //next file_extension
  device_extension* dx; //parent device extension .. set 0 if closing
  DWORD access;
//  DWORD share;
  long ref; //reference count ; blocks freeing memory

  HANDLE waitevent;
  long waituse; //0=normal/1=using/2=abort
  DWORD waitbit; //hiword:1=mask/loword:1=raised
};

DWORD WINAPI notifyevent(void*);
