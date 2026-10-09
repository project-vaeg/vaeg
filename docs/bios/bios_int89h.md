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

# PC-88VA BIOS解析資料 — INT 89H / プリンタBIOS

テクマニ6.9相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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
| 00H | プリンタ初期化 |
| 01H | DX内容を文字出力 |
| 02H | 画面コピー |
| 03H | 1 byteを直接出力 |
| 04H | プリンタ種別等の設定 |
| 05H | 設定状態を取得 |
| 06H | 漢字／ANKの桁揃えを設定: AL bit0 |
| 07H | 桁揃え状態を取得: AL bit0 |
| 08H | DX秒でtimeout設定。0はtimeoutなし |
| 09H | ES:DXのfar pointer列でdriver／色変換を置換 |
| 0AH | GetBook — ES:DXを返す |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:02DC → bank 3, E000:9D03 → ROMバンク内offset 9DA6 |
| VA2 | F000:040C → bank 5, E000:8103 → ROMバンク内offset 81B8 |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `0040:0CD0` | `010D0H` | 10H bytes | [ROM確認] AH=00～03用4個のfar hook。GetBook返却先 |
| `0040:2E70` | `03270H` | 4 bytes | [ROM確認: VA2] 文書一覧にないAH=0BH用far hook |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] AH=0AHはVA 9E00、VA2 821DでES:DX=0040:0CD0を設定する。物理010D0H。先頭には4個の実行フックがあるため、通常のデータbufferと解釈して書き換えない。
- [ROM確認] VAは00～0AH、VA2は00～0BHを処理可能なdispatch。VA2の0BHは0040:2E70へfar callするが、用途・入力ABIは未確定。
- [文書仕様] STOPで中断した場合CF=1。画面コピーはスプライト状態も扱うので、マウスや間接制御との干渉に注意する。
- [文書仕様] AH=09Hのpointer列は10個×4 bytes（計40 bytes）。00～03Hのdriverと6個の色変換routineを指定する。変更しないentryはsegment/offsetとも0。通知先codeの寿命を維持する。
- [未確認] timeout、driver置換、各色変換経路の全面照合は未完了。プリンタ不在時に安全に戻るとは保証できない。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
