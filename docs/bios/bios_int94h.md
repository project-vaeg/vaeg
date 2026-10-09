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

# PC-88VA BIOS解析資料 — INT 94H / スクリーンエディタBIOS

テクマニ6.17相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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
| 00H | InitScr — editor初期化 |
| 01H | AlcSysL — system line確保／解放 |
| 02H | ChgSMod — screen mode変更 |
| 03H | ChgScrW — scroll window変更 |
| 04H | ChgNAtr — null文字attribute変更 |
| 05H | ScrOutC — 制御code付き文字出力 |
| 06H | ScrOutL — literal文字出力 |
| 07H | GetLine — logical line取得 |
| 08H | GetPlin — logical lineの部分取得 |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:0324 → bank 1, E800:0003 |
| VA2 | F000:0457 → bank 2, EE40:0003 |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `03C2:0000` | `03C20H` | 2 bytes | [ROM確認] 0152Hバンク設定保存 |
| `03C2:0002` | `03C22H` | 2 bytes | [ROM確認] 呼び出し側ES保存 |
| `03C2:0004` | `03C24H` | 2 bytes | [ROM確認] CX保存 |
| `03C2:0056` | `03C76H` | 28H bytes | [ROM確認] 10個のfar hook初期化領域 |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] VAはbank 1のE800:0003（bank内8003H）→0015、VA2はbank 2のEE40:0003（bank内E403H）→0015。**segment補正なしにbank offset 0003Hを読むと別機能を誤認する。**
- [ROM確認] 両者ともDS=03C2H（物理03C20H）を使用。VAはAH<=08H、VA2はAH<=09Hをdispatch。VA2の09Hは文書一覧外であり、ABI未確定。
- [文書仕様] 初期化はCRT modeとframebuffer確保に関わる。通常の文字出力BIOSと同じ状態管理ではなく、logical line、system line、scroll windowを維持する。
- [文書仕様] editor使用中にTEXT BIOSを直接呼んでよい機能には制限がある。cursor／frame／画面分割を任意に変更するとeditorの管理状態と不整合になる。
- ES=A000Hを使う処理はTVRAMであり、03C2Hの通常RAM管理領域とは別。行bufferなどの呼び出し側所有データの詳細ABIは追加照合が必要。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
