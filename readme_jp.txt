232usb - RS232 USB driver for WindowsCE
         Copyright (c) 03-04,06 Zoroyoshi, Japan
         http://www.softclub.jp/zoro/ce/

06-7-27 updated

���T�v
USB�V���A���f�o�C�X�h���C�o�ł��B
CDC ACM�APL2303�AFTDI���ɑΉ����Ă��܂��B
USB���f����USB�V���A���P�[�u���Ȃǂœ��삷��Ƃ����Ȃ��Ǝv���܂��B
WindowsCE2.11�ȍ~ USB���ڋ@��ł̂ݓ��삵�܂��B
BSD���C�Z���X�ɏ]���܂��B

���g����
(1)CPU���ɑΉ������f�B���N�g��(XScale��ARM�ł�)�ɂ���232usb.dll��
   CE�@��\Windows�f�B���N�g���ɃR�s�[���Ă��������B���ꂪ�h���C�o�̖{�̂ł��B
   232usb.exe��232usb.dll�̓���ݒ�p�̃��W�X�g���ύX�v���O�����ł��B
   CE�@��̓K���ȏꏊ�ɃR�s�[���Ă����Ă��������B
(1a)�o�[�W�����ύX�̂���232usb.dll�����ւ����ꍇ�́A
   232usb.exe���N������[Del]�Ń��W�X�g�����폜���Ă����Ă��������B
(2)�g�������V���A���f�o�C�X��USB�ɐڑ����܂��B
   �h���C�o���𕷂���܂��̂ŁA232usb �Ɠ��͂��Ă��������B
(3)W-ZERO3es�ȂǁA�h���C�o���𕷂���Ȃ��@��̏ꍇ�́A
   232usb.exe���N������[New]�{�^���������A�_�C�A���O�\������
   �V���A���f�o�C�X��ڑ����Ă��������B
   �Ȃ��o�^��͈�x�f�o�C�X��ڑ����Ȃ����Ă��������B
(4)COM�|�[�g���邢�̓��f���ꗗ�Ɍ���܂��B
   �_�C�A���A�b�v�l�b�g���[�N��24term���痘�p�ł��܂��B

��232usb.exe
232usb.exe�𗘗p����232usb.dll�̐ݒ���m�F�E�ύX�ł��܂��B
�ʏ�̎g�p�ɂ����Ă͕ύX����K�v�͂���܂���B
�ύX��̓f�o�C�X�̔����������邢�̓��Z�b�g���K�v�ɂȂ�܂��B
[New]
  �f�o�C�X�̎蓮�o�^�ɗp���܂��B
[Nam]
  �f�o�C�X�̖��O��ύX���܂��B�ݒ��̓��Z�b�g���K�v�ł��B
�@�_�C�A���A�b�v�l�b�g���[�N�̃G���g�����č쐬���Ă��������B
[Del]
  ���Y�f�o�C�X�̃��W�X�g�����폜���܂��B
�@232usb.dll�����ւ����ꍇ�ɂ͑S�f�o�C�X�̃��W�X�g�����폜���Ă��������B
��BASIC   (�m�F�@��:Sig3��USB�N���C�A���g)
  �P�Ȃ�o���N�p�C�v���o�͂����̂܂�COM�|�[�g�Ƃ���A�ėp�h���C�o�ł��B
  ������͎g���܂���B
  ���Ƃ���CE�@���g��USB�N���C�A���g�Ƃ��ĂȂ����Ƃ��ł��܂��B
��CDC ACM (�m�F�@��:ALEXON TD-480,USB-RSAQ2/PL2303)
  ������uUSB���f���v�ł��BBASIC�ɐ���������������̂ł��B
  ���łŃT�|�[�g���Ă�����̂Ɠ����ł��B
��CDC WMC (�m�F�@��:AH-K3001V)
  �t�@�C�������p�̃G���h�|�C���g������ACM�N���X�̊g���ł��B
  send/recv��bit15-8���t�@�C�������p�G���h�|�C���g�ԍ��ƂȂ�܂��B
  �f�o�C�X�I�[�v����SETIR�𔭍s����ƃt�@�C�������p�ɂȂ�܂��B
��FTDI    (�m�F�@��:SRC06-USB)
  FTDI USB�V���A���ϊ��`�b�v�ł��B
  �����F���Ɏ��s����BASIC�ɂȂ邱�Ƃ����X����̂ŁA
  ���̏ꍇ�ɂ�FTDI�ɐݒ肵��FT8U232�Ƀ`�F�b�N�����Ă��������B
��RCS310  (�m�F�@��:RC-S310/ED3)
  RC-S310��p�ł��B
��PL2303  (CDC ACM�̂�)
  PL2303�̏ꍇ�Ɏw�肵�܂��BCTS��������g���܂��B
��12MHz   (FTDI�̂�)
  FTDI�Ŋ�N���b�N��48MHz�ł͂Ȃ�12MHz�̏ꍇ�Ƀ`�F�b�N���܂��B
��FT8U232 (FTDI�̂�)
  FT8U232AM/232BM�Ȃǂ̏ꍇ�Ɏw�肵�܂��B
  �Â�FTDI�`�b�v�ȊO�ł̓`�F�b�N���Ă��������B
��No Send 0
  0�o�C�g�̃o���N�o�͂��s���ƃo�O����������ꍇ�Ƀ`�F�b�N���܂��B
  0�o�C�g��WriteFile�ł��o���N�o�͍͂s���܂���B
��Send Whole
  �ʏ�A���M�̓p�P�b�g�T�C�Y-1�ɕ������Ă����Ȃ��܂��B
  Send Whole�̏ꍇ�͂��̕������s���܂���B
  �܂��A�Ō�̃p�P�b�g�����x�̏ꍇ�ɂ͒ǉ���0�o�C�g�]�����s���܂��B
  No Send 0�Ƒg�ݍ��킹���ꍇ�͒ǉ���0�o�C�g�]���͂���܂��񂪁A
  0�o�C�g��WriteFile���s�����ꍇ�ɂ�0�o�C�g�̑��M���s���܂��B
��No 1132 Bug
�@MediaQ MQ1132�𗘗p����NPD-20JWL����16n+14�o�C�g�̑��M���s����
  �f�[�^��������o�O���A�����]�����邱�Ƃɂ��������@�\�𖳌��ɂ��܂��B
  �Ȃ�Send Whole�ݒ��Ԃł̓o�O����ł��܂���B
��No Baudrate
  SET_LINE_CODING(�{�[���[�g�ݒ�)���s���ƃo�O����������ꍇ�Ƀ`�F�b�N���܂��B
��Serial Cable
  �_�C�A���A�b�v�l�b�g���[�N�Ń��f���ł͂Ȃ��A
  �P�[�u���ڑ��Ƃ��Ĉ����܂��B�ݒ�ナ�Z�b�g���Ă��������B
Recv[ ]
  ��M�p�G���h�|�C���g��16�i���Ŏw�肵�܂��B
  bit15-8��CDC WMC�ł̃t�@�C���]���p�ł��B
Send[ ]
  ���M�p�G���h�|�C���g��16�i���Ŏw�肵�܂��B
  bit15-8��CDC WMC�ł̃t�@�C���]���p�ł��B
Class[ ]
  ������p�G���h�|�C���g��16�i���Ŏw�肵�܂��B
Priority[ ]
  ��M�X���b�h�̃v���C�I���e�B���w�肵�܂��B0�Ŏ����ł��B
Buf.Size[ ]
  ��M�o�b�t�@�̑傫�����w�肵�܂��B0�Ŏ���(4096)�ł��B

�����ӓ_
�E232usb.dll�͈ȉ��̃��W�X�g����p���܂��B
  \HKEY_LOCAL_MACHINE\Drivers\USB\LoadClients\Default\Default\Default\RS232_USB
    �f�o�C�X�h���C�o�V�K�o�^���Ƀt�b�N�Ƃ��Ĉꎞ�I�Ɏg���܂��B
  \HKEY_LOCAL_MACHINE\Drivers\USB\LoadClients\255_2_0\Default\Default\RS232_USB
    �e�f�o�C�X���ɍ쐬�����G���g���ł��B
    255_2_0�̕����͊e�f�o�C�X�̃x���__�v���_�N�g_�����[�X�ԍ��ƂȂ�܂��B
�E���܂����삵�Ȃ��f�o�C�X��CE�{�̂��������񂠂�܂��B
�E�V���ȃf�o�C�X���h���Ă��h���C�o���𕷂���Ȃ����͂��łɕʂ̃h���C�o��
  ���삵�Ă��邩�A�d�������ɂЂ��������Ă��邩�A�P�[�u�����ُ�ł��B
�ESig3�͓d�������������ł����A�M���̗򉻂ɑ΂��ăV�r�A�ł��B
  �Ȃ�AH-K3001V�͕W����FOMA�h���C�o�œ��삵�܂��B
�EW-ZERO3es�Ȃǃf�o�C�X�h���C�o�����w��ł��Ȃ��@��̂��߂Ɏ蓮�o�^�@�\��
  ���܂����B�f�o�C�X�h���C�o�̋@�\�ɂ͕ύX����܂���B
�E�P�[�u��������d���f��v���Z�X�����I���Ȃǂُ̈펖�Ԃɑ΂���
  �n���h�����O��t���[����܂��͂��Ȃ肢��������ł��B
�E���łɑ΂���M�X���b�h�̈�����ς��Ă���̂ŁA�X�s�[�h���o�Ȃ�
  ��������܂���B���̏ꍇ��03-9-30�ł����g�����������B
�E232usb.exe��232usb.dll�Ɠ���Windows�f�B���N�g���ɓ���Ă����Ă����܂��܂���B
�E����m�F��Sig2�ASig3�ANPD-20JWL�AHPW-600JC�AW-ZERO3es�ōs���Ă��܂��B

�\�[�X�Ɋւ��Ă�src�f�B���N�g���̒���source.txt���Q�Ƃ��Ă��������B
-------------------------------------------------------------------------
232usb - RS232 USB driver for WindowsCE
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
