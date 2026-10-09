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

# PC-88VA BIOS解析資料 — INT 88H / アニメーションBIOS

テクマニ6.8相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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
| 00H | SetLine — 線分の始点・終点を設定 |
| 01H | GetLine — 次の線分pixelを取得 |
| 02H | SetCirc — 円の半径を設定 |
| 03H | GetCirc — 次の円周座標を取得 |
| 04H | SetSpln — spline制御点列を設定 |
| 05H | GetSpln — 次の3次B-spline座標を取得 |
| 06H | InitScnr — scanner初期化 |
| 07H | StartScnr — scannerパラメータ設定と開始 |
| 08H | ReadScnr — 1 rasterのデータを取得 |
| 09H | StopScnr — scanner停止 |
| 0AH | Hsv2Rgb — HSVからRGBへ変換 |
| 0BH | Rgb2Hsv — RGBからHSVへ変換 |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:0342 → bank 2, E000:D003 → ROMバンク内offset D003 |
| VA2 | F000:04A2 → bank 5, E000:DC03 → ROMバンク内offset DC03 |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `10A0:0000` | `10A00H` | 34H bytes | [ROM確認] 13個のfar hook初期化領域 |
| `10A0:004A` | `10A4AH` | 1 byte | [ROM確認] 初期化時に0を書き込む内部状態 |
| `10A0:0058` | `10A58H` | 1 byte | [ROM確認] 初期化時に0を書き込む内部状態 |
| `10A0:005A` | `10A5AH` | 1 byte | [ROM確認] 初期化時に0を書き込む内部状態 |
| `10A0:008A` | `10A8AH` | 1 byte | [ROM確認] 初期化時に0を書き込む内部状態 |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] VA D003、VA2 DC03のdispatcherはDS=10A0Hを使用し、先頭far hookを呼んでからAH別処理に入る。全ワークサイズ・各状態byteの意味は未確定。
- [文書仕様] BP/DI/SIは保持、AX/BX/CX/DX/flagsは保持を期待しない。座標取得は逐次状態を進めるため、単なる照会ではない。
- [資料上の不整合] 割り込み禁止の説明で「CLS」とあるが、命令名としてはCLI。資料はscanner系以外の呼び出し後IF=0、scanner系IF=1を要求する。実際の全return経路は未検証であり、IF保存を前提にしない。
- scannerは専用parallel interfaceが対象で、serial通信BIOSとは異なる。タイムアウト／機器不在の全経路は未検証。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
