(translated using Google Translate)

232usb-RS232 USB driver for WindowsCE
         Copyright (c) 03-04,06 Zoroyoshi, Japan
         http://www.softclub.jp/zoro/ce/

06-7-27 updated

■ Outline
USB serial device driver.
Compatible with CDC ACM, PL2303, FTDI, etc.
I think it would be nice to work with a USB modem or USB serial cable.
WindowsCE 2.11 or later Only compatible with models equipped with USB.
Subject to the BSD license.

■ How to use
(1) 232usb.dll in the directory corresponding to the CPU name (XScale is ARM)
   Copy it to the \Windows directory of the CE machine. This is the driver body.
   232usb.exe is a registry change program for setting the operation of 232usb.dll.
   Copy it to a suitable location on the CE machine.
(1a) When 232usb.dll is replaced due to version change,
   Start 232usb.exe and use [Del] to delete the registry.
(2) Connect the serial device you want to use to USB.
   You will be asked for the driver name, so enter 232usb.
(3) For models such as W-ZERO3es that you cannot ask for the driver name,
   Start 232usb.exe and press [New] button.
   Please connect the serial device.
   Please reconnect the device once after registration.
(4) It will appear in the COM port or modem list.
   It can be used from dial-up network or 24term.

■ 232usb.exe
You can use 232usb.exe to check and change the settings of 232usb.dll.
No change is required for normal use.
After changing, it is necessary to insert or remove the device or reset.
[New]
  Used for manual registration of devices.
[Nam]
  Rename the device. Reset is required after setting.
Please recreate the dialup network entry.
[Del]
  Delete the registry for the device.
If you replace 232usb.dll, delete the registry of all devices.
▽ BASIC (Check model: Sig3 USB client)
  This is a general-purpose driver that simply uses bulk pipe input/output as a COM port.
  Control lines cannot be used.
  For example, you can connect the CE machine itself as a USB client.
▽ CDC ACM (Check model: ALEXON TD-480, USB-RSAQ2/PL2303)
  It is a so-called "USB modem". It is a control line added to BASIC.
  It is equivalent to the one supported by the old version.
▽ CDC WMC (Check model: AH-K3001V)
  It is an extension of the ACM class that has an endpoint for file exchange.
  Bit 15-8 of send/recv is the file exchange endpoint number.
  Issuing SETIR after opening the device will be for file exchange.
▽ FTDI (Check model: SRC06-USB)
  FTDI USB serial conversion chip.
  Since it often happens that it fails in automatic recognition and becomes BASIC,
  In that case, set it to FTDI and check FT8U232.
▽ RCS310 (Check model: RC-S310/ED3)
  For RC-S310 only.
□PL2303 (CDC ACM only)
  Specify for PL2303. CTS control line can be used.
□ 12MHz (FTDI only)
  Check if the reference clock in FTDI is 12MHz instead of 48MHz.
□ FT8U232 (FTDI only)
  Specify for FT8U232AM/232BM etc.
  Check it except for old FTDI chips.
□ No Send 0
  Check if a bug occurs when you perform bulk output of 0 bytes.
  Bulk output is not performed even with 0-byte WriteFile.
□ Send Whole
  Usually, the transmission is divided into packet size-1.
  This division is not performed for Send Whole.
  Also, if the last packet is exactly, an additional 0 byte transfer is done.
  There is no additional 0 byte transfer when combined with No Send 0,
  When 0 byte Write File is executed, 0 byte is sent.
□ No 1132 Bug
When sending 16n + 14 bytes with NPD-20JWL etc. using MediaQ MQ1132
  The function to avoid the bug that the data becomes garbled by dividing and transferring is disabled.
  Note that bugs cannot be avoided when Send Whole is set.
□ No Baudrate
  Check if a bug occurs if you do SET_LINE_CODING (set baud rate).
□ Serial Cable
  Dial-up network, not a modem
  Treated as a cable connection. Please reset after setting.
Recv[]
  Specify the receiving endpoint in hexadecimal.
  bit15-8 are for file transfer with CDC WMC.
Send[]
  Specify the transmission endpoint in hexadecimal.
  bit15-8 are for file transfer with CDC WMC.
Class[]
  Specify the end point for control line in hexadecimal.
Priority[]
  Specifies the priority of the receiving thread. 0 is automatic.
Buf.Size[]
  Specify the size of the receive buffer. 0 is automatic (4096).

■ Notes
・232usb.dll uses the following registry.
  \HKEY_LOCAL_MACHINE\Drivers\USB\LoadClients\Default\Default\Default\RS232_USB
    It is temporarily used as a hook when registering a new device driver.
  \HKEY_LOCAL_MACHINE\Drivers\USB\LoadClients\255_2_0\Default\Default\RS232_USB
    This is an entry created for each device.
    255_2_0 is the vendor_product_release number of each device.
-There are many devices and CEs that do not work well.
・If you are not asked for the driver name even if you insert a new device, try another driver.
  It is working, stuck in current limit, or the cable is defective.
・Sig3 has a tight current limit and is strict against signal deterioration.
  The AH-K3001V works with the standard FOMA driver.
-Manual registration function for models such as W-ZERO3es that cannot specify device driver name
  I put it on. There is no change in the device driver function.
・For abnormal situations such as cable disconnection, power cutoff, process forced termination, etc.
  The handling and the flow control are pretty difficult.
・Since the handling of the receiving thread is changed from the old version, the speed does not come out
  maybe. In that case, please use the 03-9-30 version.
-232usb.exe may be put in the same Windows directory as 232usb.dll.
-The operation has been confirmed with Sig2, Sig3, NPD-20JWL, HPW-600JC and W-ZERO3es.

See source.txt in the src directory for the source.
-------------------------------------------------- -----------------------
232usb-RS232 USB driver for WindowsCE
Copyright (c) 03-04,06 Zoroyoshi, Japan
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.