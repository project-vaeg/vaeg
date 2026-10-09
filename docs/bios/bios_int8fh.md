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

# PC-88VA BIOS解析資料 — INT 8FH / グラフィック画面制御BIOS

テクマニ6.6相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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
| 00H | ScnMode — 表示モード／関連定義をリセット |
| 01H | DefBuf — AL=画面、CX=個数、ES:DI=バッファ定義列 |
| 02H | DefWin — AL=画面、CX=個数、ES:DI=ウィンドー定義列 |
| 03H | Compose — AL=スプライト色境界、CX=画面優先順位 |
| 04H | Roll — AL=画面、CL=buffer、BX/DX=相対位置 |
| 05H | RollTo — AL=画面、CL=buffer、BX/DX=絶対位置 |
| 06H | SetMask — CL=slot、BX/DX/SI/DI=矩形 |
| 07H | AskBuf — AL=画面、CL=buffer → ES:DI=管理記述子 |
| 08H | SetPal — AL=palette番号、CX=色 |
| 09H | PalCtl — AL=palette制御 |
| 0AH | ResPal — palette初期化 |
| 0BH | ScnDsp — AL=表示可否、DL=plane指定 |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:0486（個別登録／専用入口） |
| VA2 | F000:064F（個別登録／専用入口） |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `0338:0000` | `03380H` | 0FH bytes | [文書仕様] 予約 |
| `0338:000F` | `0338FH` | 2 bytes | [文書仕様] 画面モード |
| `0338:0011` | `03391H` | 2 bytes | [文書仕様] 画面0/1のpixel size |
| `0338:0013` | `03393H` | 2 bytes | [文書仕様] 画面0/1のframebuffer個数 |
| `0338:0015` | `03395H` | 2 bytes | [ROM確認] dispatcherのSP保存 |
| `0338:0110` | `03490H` | 2 bytes | [ROM確認] 選択handlerのoffset保存 |
| `0338:0017` | `03397H` | 50H bytes | [文書仕様] 第1framebuffer群 |
| `0338:0067` | `033E7H` | 50H bytes | [文書仕様] 第2framebuffer群 |
| `0338:00B7` | `03437H` | 28H bytes | [文書仕様] window記述子領域 |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] VA入口はbank 5を選択してEE80:0003へ、VA2はbank 0のE000:D403へfar callする。EE80:0003はROMバンク内E803相当であり、通常RAMアドレスではない。
- [文書仕様] 原則AX=0が成功、1～9は定義・pixel size・buffer番号・位置・サイズ・VRAM容量・page・表示状態・windowのエラー。各機能で全コードが出るとは限らない。
- [文書仕様] AskBuf記述子はpixel size、幅、高さ、buffer番地下位word等を持つ。ES:DIはVRAM本体ではなく管理記述子を指す。
- [資料上の不整合] 第2framebuffer群にも「画面0」の表題が重複する。ここでは第1／第2群とし、画面1と断定した修正版の仕様にはしていない。
- [ROM確認] 両dispatcherともDS=0338H、AH<=0BHを処理し、AH>0BHはAX=FFFFH。両ROMで0338:0015と0338:0110にSP／handler offsetを保存する。管理表の全field照合は未完了。338Hは16進segment、物理03380H。
- 画面モード変更はbuffer/window定義を失わせる。VRAMを通常RAMのbufferとして扱わない。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
