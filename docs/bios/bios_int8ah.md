<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# PC-88VA BIOS解析資料 — INT 8AH / コミュニケーションBIOS

テクマニ6.10相当。対象: VA / VA2 ROM、PC-Engine 1.1。

## 読み方と検証範囲

[文書仕様] はテクマニの公開インターフェースの要約、[ROM確認] はVA・VA2の静的なコード／データ参照、[未確認] は実装照合や起動後測定が未完了の事項。
BNN資料は補助照合に使用したが、OCRの表配置が不明な箇所を確定根拠にはしていない。
これは**静的解析の初版**であり、各機能の全パラメータ・全分岐を確定した完成版ではない。
PC-Engine 1.1起動後の最終IVT、RAM内容、実機での戻り値は未測定。VA3 ROMは独立検証していない。
未掲載の番号が安全に呼び出せることを意味しない。内部フックは公開APIではない。

数値は特記しない限り16進。Conventional memoryの物理アドレスは `segment × 10H + offset`。
ROMバンク番号とRAMのsegmentは別概念。TVRAM・GVRAM・サウンドメモリは通常RAMのワークとは分離する。
初期IVTのROM入口は、OS・拡張BIOSによる差し替え後の入口を保証しない。
NEC拡張0FH命令は命令長を確認し、現代x86逆アセンブラの表示をそのまま採用しない。

## 機能一覧 [文書仕様]

選択レジスタ: **AH**。以下は機能識別と主要な用途であり、省略されたパラメータを任意値で代用してはいけない。

| 番号 | 機能 |
|---|---|
| 00H | CINIT — UART／通信環境初期化 |
| 01H | SENDCH — 文字送信 |
| 02H | RETRY — 送信できなかった文字の再送 |
| 03H | GETRX — 受信文字を取り出して消費 |
| 04H | SNSRX — 受信文字を消費せず照会 |
| 05H | PEEPCQ — native受信キューを参照 |
| 06H | COMSTS — 通信status取得 |
| 07H | BREAK — break信号送出 |
| 08H | BRATE — 通信速度設定 |
| 09H | SETKJ — 漢字コード設定 |
| 0AH | SUP — 通信方法設定 |
| 0BH | CQSTS — 受信buffer状態 |
| 0CH | CLRCQ — 受信buffer消去 |
| 0DH | GETWT — timeout付き文字受信 |
| 0EH | COMBYE — 通信環境解除 |
| 0FH | XOPEN — XMODEM開始 |
| 10H | XCLOSE — XMODEM終了 |
| 11H | XGET — XMODEM block受信 |
| 12H | XPUT — XMODEM block送信 |
| 13H | CTRAP — 受信trap vector設定 |
| 14H | ABTK1 — ABORTKEY1設定 |
| 15H | ABTK2 — ABORTKEY2設定 |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:0353 → bank 3, E000:C703 → ROMバンク内offset C773 |
| VA2 | F000:04B3 → bank 0, E000:E803 → ROMバンク内offset E872 |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `0040:005E` | `0045EH` | 1 byte | [文書仕様] ARTMODE: 8251 mode |
| `0040:005F` | `0045FH` | 1 byte | [文書仕様] ARTCMD: 8251 command shadow |
| `0040:08B0` | `00CB0H` | 4 bytes | [文書仕様] CTRAPV: 受信trap far pointer |
| `0040:08B4` | `00CB4H` | 2 bytes | [文書仕様] ABORTKEY: 中断key |
| `0040:08B6` | `00CB6H` | 2 bytes | [文書仕様] ABORTKEY2: 中断key、FFFFHで無効 |
| `0040:08C4` | `00CC4H` | 2 bytes | [文書仕様] DIVRATE: 速度分周値 |
| `0040:08C6` | `00CC6H` | 1 byte | [文書仕様] COMACTIVE: 00=sleep / FF=active |
| `0040:08C7` | `00CC7H` | 1 byte | [文書仕様] BAUDRATE: 論理速度コード |
| `0040:08C8` | `00CC8H` | 1 byte | [文書仕様] KANJI: 送受信文字code |
| `0040:08C9` | `00CC9H` | 1 byte | [文書仕様] SETUPINF: flow / XMODEM設定 |
| `0040:08CA` | `00CCAH` | 1 byte | [文書仕様] FLOWSTS: flow状態 |
| `0040:08CB` | `00CCBH` | 1 byte | [文書仕様] BYTE_TIMEOUT: 100ms単位 |
| `0040:08CC` | `00CCCH` | 1 byte | [文書仕様] BLOCK_TIMEOUT: 100ms単位 |
| `0040:0CBD` | `010BDH` | 2 bytes | [文書仕様] WATCH: 1/120秒単位の減算時計、0でも減算継続 |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] VA C773、VA2 E872でAH<16Hを検査し、DS=0040H、0068Hの0200H maskを立てて処理する。ROM dispatch表はVA C7AE、VA2 E8AC。
- [文書仕様] 直接UARTへOUTする場合もARTCMD shadowを整合させる。CTRAPVはsegment=0000で無効。WATCHは0を過ぎてwrapするため、単純な「0で停止」timerではない。
- [文書仕様] BAUDRATEは0=free、1～9=75/150/300/600/1200/2400/4800/9600/19200 bps。
- [未確認] 公開共通変数の全byte操作とROM初期化値の全面照合は未完了。受信bufferは呼び出し側から指定される別領域で、上表だけでは容量を決められない。
- 受信・送信・時間待ち・XMODEMはblockや機器待ちを伴う。trapを指すcodeとbufferを使用中に解放しない。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
