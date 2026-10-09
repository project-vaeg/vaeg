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

# PC-88VA BIOS解析資料 — INT 84H / スプライトBIOS

テクマニ6.5相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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
| 00H | InitSpr — スプライト環境を初期化 |
| 01H | InitQue — 間接制御キューを用意 |
| 02H | DefPat — パターンを登録 |
| 03H | パターン用領域を確保 |
| 04H | スプライトにパターン列を割り当て |
| 05H | 直接制御の位置指定 |
| 06H | 間接制御を開始 |
| 07H | 間接制御を停止 |
| 08H | 表示を許可 |
| 09H | 表示を禁止 |
| 0AH | 間接制御キューへ投入 |
| 0BH | キューを消去 |
| 0CH | スプライト情報を取得 |
| 0DH | パターン情報を取得 |
| 0EH | 空き領域を照会 |
| 0FH | クリップ枠を設定 |
| 10H | クリップ情報を照会 |
| 11H | 衝突情報を照会 |
| 12H | クリップ通知の設定／解除 |
| 13H | 衝突通知を設定 |
| 14H | キュー空通知を設定 |
| 15H | キュー内データ量を取得 |
| 16H | 割り込み処理を一時停止 |
| 17H | 一時停止を解除 |
| 18H | 停止状態・表示モードを照会 |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | 共通初期IVT登録表にはない。個別登録・OS側設定を別に調べる必要あり |
| VA2 | F000:03E0 → bank 5, E000:A703 → ROMバンク内offset A82E |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `0040:1D82` | `02182H` | 1 byte参照 | [ROM確認] 初期化状態の判定に使用 |
| `0040:1D86` | `02186H` | 2 bytes | [ROM確認] AX返却用の保存値 |
| `0040:1D88` | `02188H` | VA: 2 / VA2: 4 bytes | [ROM確認] VAはバンク保存値、VA2はAH=19H用far hook。互換な構造ではない |
| `0040:1D94` | `02194H` | 2 bytes | [ROM確認] 呼び出し深度カウンタ |
| `0040:1D96` | `02196H` | 4-byte entries | [ROM確認] far呼び出し表。公開ワーク全体の先頭と断定しない |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] VAはF000:7849でIVT84HにF000:794Fを登録。VA2はbank 5のA82Eでdispatch。両方ともAH<1AHを許容し、AH≠0では初期化状態を検査する。
- [ROM確認] VAの初期化は0040:1D96に42個のfar pointerを配置する。これは25個の公開機能だけの表ではない。
- [ROM確認] 文書一覧にないAH=19HはVAでは呼び出し表経由、VA2では0040:1D88のfar hookへ分岐する。**公開ABI・用途は未確定**で、追加サービスとして利用してはいけない。
- [文書仕様] スプライト0はマウス、1～30はユーザー、31はテキストカーソル。パターン等のTVRAM配置と上記のConventional memory管理領域は別。
- [資料上の不整合] クリップ枠設定の個別記載「AH=0H」は機能一覧の0FHと矛盾する。重複した本文もあるため、機能一覧とdispatch番号を優先して識別する。
- 初期化、キュー投入、通知フック変更は状態を変える。キューやパターン領域の寿命を管理し、予約スプライトを潰さない。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
