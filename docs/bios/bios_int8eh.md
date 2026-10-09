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

# PC-88VA BIOS解析資料 — INT 8EH / ラインエディタBIOS

テクマニ6.16相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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
| 00H | InitHis — BIOS history初期化 |
| 01H | HisLine — ES:DI bufferへhistory付き入力 |
| 02H | InitAhs — アプリケーションhistoryを登録 |
| 03H | AHisLin — アプリケーションhistoryを使って入力 |
| 04H | SnsLine — soft keyを検査しつつ行入力 |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:036F → bank 1, E000:D003 → ROMバンク内offset D003 |
| VA2 | F000:04CF → bank 0, E000:F203 → ROMバンク内offset F203 |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `0040:3CEA` | `040EAH` | 実行入口 | [ROM確認: VA2] dispatcherからfar callするRAM hook code |
| `0040:3943` | `03D43H` | 1 byte | [ROM確認: VA2] InitHis内部設定 |
| `0040:3947` | `03D47H` | 2 bytes | [ROM確認: VA2] InitHis内部設定 |
| `0040:394D` | `03D4DH` | 2 bytes | [ROM確認: VA2] InitHisで保存するsegment |
| `0040:3953` | `03D53H` | 2 bytes | [ROM確認: VA2] InitHisで保存するoffset |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] VA D003、VA2 F203の入口はDS=0040H。VA2はAH=00～04Hのコード列F24Dとhandler表F253を検索する。
- [文書仕様] HisLine/SnsLineの入力bufferはES:DI、容量はCLなどで指定する。history用の管理領域と、返却する行bufferは別物。
- [文書仕様] 取り込む文字は混在文字code。画面右端と現在のcursor位置で入力上限も変わる。最大80桁画面で左端から入力しても79 bytesで、全角なら同じ文字数にならない。
- [文書仕様] HisLine前にInitHisが必要。アプリケーションhistoryを使う場合はInitAhsとその対応する契約を使う。
- [未確認] VAとVA2のhistory本体の全配置、InitAhs管理構造の全fieldは未確定。上表の保存pointerを自由に差し替えてはいけない。入力機能はkey待ちでblockする。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
