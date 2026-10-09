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

# PC-88VA BIOS解析資料 — INT 8DH / 日本語入力フロントプロセッサBIOS

テクマニ6.18相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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

選択レジスタ: **CL**。以下は機能識別と主要な用途であり、省略されたパラメータを任意値で代用してはいけない。

| 番号 | 機能 |
|---|---|
| 00H | GetJFP |
| 01H | SetJFP |
| 02H | ResJFP |
| 03H | QuitJFP |
| 04H | Sug1st |
| 05H | SugNxt |
| 06H | SugPrv |
| 07H | KpFrwd |
| 08H | KpBack |
| 09H | LongB |
| 0AH | ShortB |
| 0BH | ToHira |
| 0CH | ToKata |
| 0DH | ToHan |
| 0EH | AllHira |
| 0FH | AllKata |
| 10H | AllHan |
| 11H | Decide |
| 12H | GetUdEnv |
| 13H | SetUdEnv |
| 14H | AddWd |
| 15H | DelWd |
| 16H | Zen_Han |
| 17H | Han_Zen |
| 18H | HiraCnv |
| 19H | KataCnv |
| 1AH | StoJ |
| 1BH | JtoS |
| 1CH | RomCnv |
| 1DH | TellScn |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:0361 → bank 2, E000:0003 → ROMバンク内offset 5179 |
| VA2 | F000:04C1 → bank 3, E000:9003 → ROMバンク内offset EDD5 |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `0040:4415` | `04815H` | 4 bytes | [ROM確認: VA2] dispatcher入口のfar hook |
| `0040:0068` | `00468H` | 2 bytes | [ROM確認: VA2] 0800Hで呼び出し状態を管理する共通busy word |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [文書仕様] 選択レジスタはCLでありAHではない。AXは入出力、文字列機能でSI/DIを使用する。文字列はSHIFT-JIS、1 byte NULL終端。
- [ROM確認] VAはbank 2:5179、VA2はbank 3:EDD5に到達。VA2はCL<25Hをdispatchし、文書の00～1DHより広い。1EH～24Hの追加番号の用途と契約は未確定。
- [ROM確認] VA2の入口は元DSをESへ移し、内部DS=0040Hを使用する。これを文書のすべての文字列pointerがES基準という契約に拡張してはいけない。
- [文書仕様] GetJFPはAH=制御data、AL=status、DH/DL=入力最大文字数。辞書登録／削除はファイル変更を伴う。単語bufferと辞書環境の寿命を維持する。
- [未確認] 変換buffer／候補列／辞書環境workの先頭と全構造は未確定。0040:4415は内部hookであり公開GetBookではない。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
