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

# PC-88VA BIOS解析資料 — INT 33H / マウスBIOS

テクマニ6.13相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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

選択レジスタ: **AX**。以下は機能識別と主要な用途であり、省略されたパラメータを任意値で代用してはいけない。

| 番号 | 機能 |
|---|---|
| 00H | InitMous |
| 01H | OnMcsr |
| 02H | OffMcsr |
| 03H | GetMsts |
| 04H | SetMsts |
| 05H | LftPrss |
| 06H | LftRel |
| 07H | RhtPrss |
| 08H | RhtRel |
| 09H | Mstyle |
| 0AH | GetMov |
| 0BH | MousTrap |
| 0CH | MRatio |
| 0DH | HrzRng |
| 0EH | VrtRng |
| 0FH | SleepMous |
| 10H | WakeMous |
| 11H | AskMous |
| 12H | TimCtl |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:02A4 → bank 2, E000:F403 → ROMバンク内offset F71F |
| VA2 | F000:03B6 → bank 0, E000:DF03 → ROMバンク内offset E20B |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `0040:30FC` | `034FCH` | 4 bytes | [ROM確認] dispatcher入口のfar hook |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] VAはAX<13H、VA2はAX<15Hをdispatchし、AX=00FFHには別経路がある。文書外のVA2 AX=13/14Hと00FFHの公開ABIは未確定。
- [文書仕様] AX=0は初期化で、成功AX=FFFFH、環境不適合AX=0。座標系はスプライト座標系。通常のDOSマウスAPIの拡張番号を流用しない。
- [文書仕様] 初期化は位置・ボタン統計・範囲・移動比・trap・font・読み込み周期をリセットし、InitSprからも呼ばれる。
- [未確認] 専用状態ワーク全体の先頭／長さ・各座標fieldの配置は未確定。0040:30FCのhookをその先頭として公開しているわけではない。
- カーソルfont bufferやtrapのアドレスは呼び出し側が維持する。trap変更・統計取得の副作用を単なるread-only検査と混同しない。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
