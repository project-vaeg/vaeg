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

# PC-88VA BIOS解析資料 — INT 92H / ファンシーフォントBIOS

テクマニ6.15相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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

選択レジスタ: **CL属性（独立したAH機能一覧なし）**。以下は機能識別と主要な用途であり、省略されたパラメータを任意値で代用してはいけない。

| 番号 | 機能 |
|---|---|
| — | 字体変換（AH別番号ではなくCL属性で選択） |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:0306 → bank 1, E000:8E03 → ROMバンク内offset 8E03 |
| VA2 | F000:0439 → bank 6, E000:D003 → ROMバンク内offset D009 |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `035B:0000` | `035B0H` | 1EH bytes | [ROM確認: VA] 初期化でRETFを埋める内部領域 |
| `0040:31B0` | `035B0H` | 1EH bytes | [ROM確認: VA2] VAと同じ物理領域035B0H |
| `0040:3F70` | `04370H` | 19H bytes | [ROM確認: VA2] RETFで初期化する追加領域 |
| `0040:3324` | `03724H` | 2 bytes | [ROM確認: VA2] 呼び出し側DS保存 |
| `0040:3326` | `03726H` | 2 bytes | [ROM確認: VA2] DI保存 |
| `0040:332B` | `0372BH` | 2 bytes | [ROM確認: VA2] DX保存 |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [文書仕様] 入力ES:SI=原font、ES:DI=出力buffer、CL=文字幅／dot mode／装飾bit。AH=00Hを選択して呼ぶ契約ではない。
- [文書仕様] 原fontは全角16×16=32 bytes、半角16×8=16 bytes。出力容量は変換modeで異なり、最大の斜体を含む24-dot全角では108 bytes。原fontサイズだけのbufferを用意しない。
- [ROM確認] VA入口はDS=035BH。VA2の共通変換処理はDS=0040Hに加え、3324/3326H等へ呼び出し引数を保存する。機種別のsegment差をそのまま物理配置差とは解釈しない。
- [未確認] 出力全modeのビット配置、buffer境界、原fontと出力bufferの重なり許容は未確定。文書の条件を満たす独立bufferを使う。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
