232usb - RS232 USB driver for WindowsCE
         Copyright (c) 03-04,06 Zoroyoshi, Japan
         http://www.softclub.jp/zoro/ce/

06-7-27 updated

■概要
USBシリアルデバイスドライバです。
CDC ACM、PL2303、FTDI等に対応しています。
USBモデムやUSBシリアルケーブルなどで動作するといいなあと思います。
WindowsCE2.11以降 USB搭載機種でのみ動作します。
BSDライセンスに従います。

■使い方
(1)CPU名に対応したディレクトリ(XScaleはARMです)にある232usb.dllを
   CE機の\Windowsディレクトリにコピーしてください。これがドライバの本体です。
   232usb.exeは232usb.dllの動作設定用のレジストリ変更プログラムです。
   CE機上の適当な場所にコピーしておいてください。
(1a)バージョン変更のため232usb.dllを入れ替えた場合は、
   232usb.exeを起動して[Del]でレジストリを削除しておいてください。
(2)使いたいシリアルデバイスをUSBに接続します。
   ドライバ名を聞かれますので、232usb と入力してください。
(3)W-ZERO3esなど、ドライバ名を聞かれない機種の場合は、
   232usb.exeを起動して[New]ボタンを押し、ダイアログ表示中に
   シリアルデバイスを接続してください。
   なお登録後は一度デバイスを接続しなおしてください。
(4)COMポートあるいはモデム一覧に現れます。
   ダイアルアップネットワークや24termから利用できます。

■232usb.exe
232usb.exeを利用して232usb.dllの設定を確認・変更できます。
通常の使用においては変更する必要はありません。
変更後はデバイスの抜き差しあるいはリセットが必要になります。
[New]
  デバイスの手動登録に用います。
[Nam]
  デバイスの名前を変更します。設定後はリセットが必要です。
　ダイアルアップネットワークのエントリも再作成してください。
[Del]
  当該デバイスのレジストリを削除します。
　232usb.dllを入れ替えた場合には全デバイスのレジストリを削除してください。
▽BASIC   (確認機種:Sig3のUSBクライアント)
  単なるバルクパイプ入出力をそのままCOMポートとする、汎用ドライバです。
  制御線は使えません。
  たとえばCE機自身をUSBクライアントとしてつなぐことができます。
▽CDC ACM (確認機種:ALEXON TD-480,USB-RSAQ2/PL2303)
  いわゆる「USBモデム」です。BASICに制御線を加えたものです。
  旧版でサポートしているものと同等です。
▽CDC WMC (確認機種:AH-K3001V)
  ファイル交換用のエンドポイントを持つACMクラスの拡張です。
  send/recvのbit15-8がファイル交換用エンドポイント番号となります。
  デバイスオープン後SETIRを発行するとファイル交換用になります。
▽FTDI    (確認機種:SRC06-USB)
  FTDI USBシリアル変換チップです。
  自動認識に失敗してBASICになることが多々あるので、
  その場合にはFTDIに設定してFT8U232にチェックを入れてください。
▽RCS310  (確認機種:RC-S310/ED3)
  RC-S310専用です。
□PL2303  (CDC ACMのみ)
  PL2303の場合に指定します。CTS制御線が使えます。
□12MHz   (FTDIのみ)
  FTDIで基準クロックが48MHzではなく12MHzの場合にチェックします。
□FT8U232 (FTDIのみ)
  FT8U232AM/232BMなどの場合に指定します。
  古いFTDIチップ以外ではチェックしてください。
□No Send 0
  0バイトのバルク出力を行うとバグが発生する場合にチェックします。
  0バイトのWriteFileでもバルク出力は行われません。
□Send Whole
  通常、送信はパケットサイズ-1に分割しておこなわれます。
  Send Wholeの場合はこの分割を行いません。
  また、最後のパケットが丁度の場合には追加の0バイト転送が行われます。
  No Send 0と組み合わせた場合は追加の0バイト転送はありませんが、
  0バイトのWriteFileを行った場合には0バイトの送信が行われます。
□No 1132 Bug
　MediaQ MQ1132を利用したNPD-20JWL等で16n+14バイトの送信を行うと
  データが化けるバグを、分割転送することにより回避する機能を無効にします。
  なおSend Whole設定状態ではバグ回避できません。
□No Baudrate
  SET_LINE_CODING(ボーレート設定)を行うとバグが発生する場合にチェックします。
□Serial Cable
  ダイアルアップネットワークでモデムではなく、
  ケーブル接続として扱います。設定後リセットしてください。
Recv[ ]
  受信用エンドポイントを16進数で指定します。
  bit15-8はCDC WMCでのファイル転送用です。
Send[ ]
  送信用エンドポイントを16進数で指定します。
  bit15-8はCDC WMCでのファイル転送用です。
Class[ ]
  制御線用エンドポイントを16進数で指定します。
Priority[ ]
  受信スレッドのプライオリティを指定します。0で自動です。
Buf.Size[ ]
  受信バッファの大きさを指定します。0で自動(4096)です。

■留意点
・232usb.dllは以下のレジストリを用います。
  \HKEY_LOCAL_MACHINE\Drivers\USB\LoadClients\Default\Default\Default\RS232_USB
    デバイスドライバ新規登録時にフックとして一時的に使われます。
  \HKEY_LOCAL_MACHINE\Drivers\USB\LoadClients\255_2_0\Default\Default\RS232_USB
    各デバイス毎に作成されるエントリです。
    255_2_0の部分は各デバイスのベンダ_プロダクト_リリース番号となります。
・うまく動作しないデバイスやCE本体がたくさんあります。
・新たなデバイスを刺してもドライバ名を聞かれない時はすでに別のドライバで
  動作しているか、電流制限にひっかかっているか、ケーブルが異常です。
・Sig3は電流制限がきついですし、信号の劣化に対してシビアです。
  なおAH-K3001Vは標準のFOMAドライバで動作します。
・W-ZERO3esなどデバイスドライバ名を指定できない機種のために手動登録機能を
  つけました。デバイスドライバの機能には変更ありません。
・ケーブル抜きや電源断やプロセス強制終了などの異常事態に対する
  ハンドリングやフロー制御まわりはかなりいいかげんです。
・旧版に対し受信スレッドの扱いを変えているので、スピードが出ない
  かもしれません。その場合は03-9-30版をお使いください。
・232usb.exeは232usb.dllと同じWindowsディレクトリに入れておいてもかまいません。
・動作確認はSig2、Sig3、NPD-20JWL、HPW-600JC、W-ZERO3esで行っています。

ソースに関してはsrcディレクトリの中のsource.txtを参照してください。
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
