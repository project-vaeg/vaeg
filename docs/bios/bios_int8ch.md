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

# PC-88VA BIOS解析資料 — INT 8CH / カレンダー時計BIOS

テクマニ6.14相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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
| 00H | 日付取得 → CX=西暦、DH=月、DL=日、AL=曜日 |
| 01H | 日付設定: CX/DH/DL → AL=設定status |
| 02H | 時刻取得 → CH=時、CL=分、DH=秒、DL=0 |
| 03H | 時刻設定: CH/CL/DH → AL=設定status |
| 04H | PC-Engine時計表示を開始 |
| 05H | PC-Engine時計表示を終了 |
| 06H | 時計表示状態取得 → AL=00H許可 / FFH禁止 |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:02EA → bank 3, E000:C103 → ROMバンク内offset C109 |
| VA2 | F000:041B → bank 2, E000:FA03 → ROMバンク内offset FA09 |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `0040:3332` | `03732H` | 1 byte参照 | [ROM確認] 日付変換で読むRTC data |
| `0040:3333` | `03733H` | 1 byte参照 | [ROM確認] 月／曜日のnibble処理 |
| `0040:3334` | `03734H` | 1 byte参照 | [ROM確認] 日付変換で読むRTC data |
| `0040:3335` | `03735H` | 1 byte参照 | [ROM確認] 時刻変換で読むRTC data |
| `0040:3336` | `03736H` | 1 byte参照 | [ROM確認] 時刻変換で読むRTC data |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] 両ROMともAH>=07HはAL=FFHを返す。VA dispatch表C126、VA2 FA3A。DS=0040HでRTCからの一時dataを変換する。
- [文書仕様] 年は1980～2079、曜日は0=日曜～6=土曜。返却値はbinary、RTC側のBCD表現をアプリケーションにそのまま渡す契約ではない。
- [文書仕様] 設定statusのbit0/1/2は、日付なら年/月/日、時刻なら時/分/秒の入力エラーを表す。
- [資料上の不整合] AH=03Hの説明見出しが日付設定になっているが、入力は時分秒であり時刻設定を指す。
- [文書仕様] 04H～06HはPC-Engineのscreen editor環境が前提。RTCアクセスのみで完結する00H～03Hとは分けて扱う。
- 上表は読み取り命令で確認した一時領域の抜粋。RTC chipの保存内容や表示buffer全体と同一ではない。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
