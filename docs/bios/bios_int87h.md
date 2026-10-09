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

# PC-88VA BIOS解析資料 — INT 87H / 拡張グラフィックスBIOS

テクマニ6.7相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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
| 01H | SetFrame — ES:DXのframe記述子を選択 |
| 02H | View — ES:BXのviewport設定、AL=制御 |
| 10H | Cls — CX色でviewportを消去 |
| 11H | Line — ES:BX=line table |
| 12H | Polyline — ES:BX=多角形table |
| 13H | Lines — ES:BX=複数線table |
| 14H | Circle — ES:BX=円／円弧table |
| 15H | Paint — ES:BX=塗りつぶしtable |
| 16H | Pset — DX=X、BX=Y、CX=色 |
| 17H | Point — DX=X、BX=Y → CX=色 |
| 18H | Get — ES:BX=画像取得table |
| 19H | Put — ES:BX=画像転送table |
| 31H | Page — AL=mode、初期設定でCL/ES:BXを使用 |
| 32H | PutGstr — DS:BX=文字列table、ES:DX=font table |
| 33H | PutGchr — CX=文字、ES:DX=font table |
| 34H | MapOpen — ES:DX=呼び出し側map work |
| 35H | MapWrite — ES:DX=map work、SI/DI=map位置 |
| 37H | Zoom1 — AL=倍率種別、ES:BX=転送table |
| 3EH | Zoom2 — ES:BX=拡縮table |
| 38H | 3D — 三次元変換tableを処理 |
| 39H | Turn — 回転tableを処理 |
| 3AH | Pack — 画像圧縮／ファイル保存 |
| 3BH | Unpack — 圧縮画像のファイル読込 |
| 3CH | XchgColor — 色置換 |
| 3DH | S16toS8 — 16-bit画像の8-bit変換 |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:02CE → bank 4, E000:0003 → ROMバンク内offset 0003 |
| VA2 | F000:03FD → bank 4, E000:0003 → ROMバンク内offset 0003 |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `1140:0580` | `11980H` | 2 bytes | [ROM確認: 拡張ドライバ] 現在のmode別dispatch表へのoffset |
| `1140:05B6` | `119B6H` | 2 bytes | [ROM確認: 拡張ドライバ] 呼び出し側DSの保存 |
| `1140:05B8` | `119B8H` | 1 byte | [ROM確認: 拡張ドライバ] AL保存 |
| `1140:05DC` | `119DCH` | 2 bytes | [ROM確認: 拡張ドライバ] 0152Hバンク設定の保存 |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] ROM入口はbank 4の0003。PC-Engine用拡張ドライバは初期化39A0付近、39CD～39D9でINT87HをCS:004Cへ差し替える。
- [ROM確認] 拡張ドライバ入口はDS=1140Hを設定し、mode別のdispatchを行う。1140HのRAM構造はROMだけの実装に一律適用できない。
- [ROM確認] AH=00/03/36/3FHは確認した拡張ドライバのdispatchでAX=FFFFHを返す。資料にない36/3FHが「隠し拡大機能」であるという推測は成立しない。
- [ROM確認] AH=3CHは先頭3modeでは非対応、残る2modeではROM側へ転送される。番号の存在と全画面modeでの利用可能性は別。
- [文書仕様] MapOpenのES:DXは呼び出し側が維持するmap work。font table 8 words、system work 16 wordsに続く画面／map設定を含む。固定1140H領域とは別の所有物。
- [文書仕様] 描画用tileはCX:DXなど機能固有のsegment:offsetで渡す。SetFrameだけではViewは設定されない。
- Pack/UnpackはOSファイル操作を伴う。予約番号総当たりや破壊的な圧縮保存試験をしない。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
