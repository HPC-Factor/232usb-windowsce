#define numof(a) (sizeof(a)/sizeof(*(a)))

#define MODE_BASIC 0
#define MODE_CDC 1
#define MODE_WMC 2
#define MODE_FTDI 3
#define MODE_RCS 4

#define MODE_2303 0x100
#define MODE_12MHZ 0x100
#define MODE_NO0SEND 0x200
#define MODE_8U232 0x200
#define MODE_ALLSEND 0x400
//no bug workaround for MQ1132
#define MODE_NO1132 0x800
#define MODE_NOBAUD 0x1000

#define ZM_TEST   (1u<<0)
#define ZM_ALLOC  (1u<<1)
#define ZM_DRIVER (1u<<2)
#define ZM_FILE   (1u<<3)
#define ZM_INIT   (1u<<4)
#define ZM_USB    (1u<<5)
#define ZM_DUMP   (1u<<6)
#define ZM_PRIO   (1u<<7)
#define ZM_ERROR  (1u<<15)

#if DEBUG
void infodump(
  USB_HANDLE uh,
  USB_FUNCS const* uf,
  USB_DEVICE const* ui);
void zmesg(ULONG, WCHAR const*,...);
void infodump(
  USB_HANDLE uh,
  USB_FUNCS const* uf,
  USB_DEVICE const* ui);
#else
#define zmesg 1?(void)0:
#endif
