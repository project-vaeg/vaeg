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

# PC-88VA BIOS解析資料 — INT 86H / ADPCM BIOS

テクマニ6.20相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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
| 00H | V$SetV — sampling／volume／出力channel等を設定 |
| 01H | V$GetV — 現在の設定を取得 |
| 02H | V$Record — sound memoryへ録音開始 |
| 03H | V$Play — sound memoryから再生開始 |
| 04H | V$Stop — 録音／再生停止 |
| 05H | V$Stat — 状態・録音量を取得 |
| 06H | V$Read — sound memoryからユーザーbufferへ読出し |
| 07H | V$Write — ユーザーbufferからsound memoryへ書込み |
| 08H | V$SetBuf — Load/Save用bufferを指定 |
| 09H | V$Save — sound memoryをfile保存 |
| 0AH | V$Load — fileからsound memoryへ読込 |
| 0BH | V$AtoD — main memoryへPCM録音 |
| 0CH | V$DtoA — main memoryからPCM再生 |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | 共通初期IVT登録表にはない。個別登録・OS側設定を別に調べる必要あり |
| VA2 | F000:03EE → bank 3, E000:F603 → ROMバンク内offset F623 |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `042B:0000` | `042B0H` | 先頭。全長未確定 | [ROM確認: VA2] dispatcherのDS。物理042B0H |
| `042B:001E` | `042CEH` | 1 byte | [ROM確認: VA2] PCM録音末尾で参照する内部状態 |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] VA2はbank 3:F623でAH<=0CHを処理、DS=042BHを設定。handler表はF609。VAの共通初期IVT表に86Hはなく、追加sound board対応OS／driverを含む登録追跡が必要。
- [資料上の不整合] 概要は「12種類」と書くが列挙は00H～0CHの13機能。VA2 dispatchも13 slots。
- [文書仕様] VAではSound Board II対応PC-Engineが必要。sound memoryは通常RAMと別。V$Read/Writeのユーザーbuffer、V$SetBufのfile転送buffer、V$AtoD/DtoAのmain-memory PCM bufferはそれぞれ区別する。
- [文書仕様] sound memoryは32-byte単位のaddress制約がある。byte address modeと0.1秒単位のtime address modeを混同しない。4/8/16kHzでは0.1秒=200/400/800 bytesであり、丸めが必要な場合がある。
- [文書仕様] AtoD/DtoAは完了まで戻らず、処理中は割り込み禁止。Save/Loadは媒体操作。機器不在のまま総当たりしてはいけない。
- [未確認] 固定workの全構造、各bufferを渡すregisterの全面照合、VA OS追加実装とVA2 ROMの差は未完了。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
