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

# BIOS解析資料 — 割り込み別索引

公開資料は `docs/bios/bios_intXXh.md`。章番号で命名しない。
テクマニ6.1～6.20相当の37資料、INT20H・21HのDOS資料、INT90HのV1/V2解析を含む**40割り込み資料**を以下の一覧から参照できる。
第2章の割り込み一覧全体を網羅したものではない。
INT80H～83Hの詳細資料、6.5～6.20の33資料（数値演算17割り込みとその他BIOSの2割り込みも個別ファイル）を含む。
依頼された `BIOS_XXH.TXT` 形式の同内容も非公開作業領域に生成した。マニュアル原本のコピーではなく、この独立執筆資料のテキスト版。

**これは全BIOS ABIが解明済みという報告ではない。** 公開機能の識別、初期ROM入口、確認済み／文書上のワーク配置が中心。
確定できなかった固定workは「未確定」とし、呼び出し側bufferを固定workの代わりに示していない。
各機能の全register、全構造field、OSによる最終差し替え、実機での動作は追加検証が必要。

| 割り込み・資料 | 検証状態 |
|---|---|
| [INT 20H / プロセス終了](bios_int20h.md) | 静的初版・実行未検証 |
| [INT 21H / DOSシステムサービス](bios_int21h.md) | 静的初版・部分照合 |
| [INT 33H / マウスBIOS](bios_int33h.md) | 静的初版・部分照合 |
| [INT 80H / フロッピーディスクBIOS](bios_int80h.md) | 詳細静的初版・実行未検証 |
| [INT 81H / ハードディスクBIOS](bios_int81h.md) | 詳細静的初版・実行未検証 |
| [INT 82H / キーボードBIOS](bios_int82h.md) | 詳細静的初版・実行未検証 |
| [INT 83H / テキストBIOS](bios_int83h.md) | 詳細静的初版・実行未検証 |
| [INT 84H / スプライトBIOS](bios_int84h.md) | 静的初版・部分照合 |
| [INT 86H / ADPCM BIOS](bios_int86h.md) | 静的初版・部分照合 |
| [INT 87H / 拡張グラフィックスBIOS](bios_int87h.md) | 静的初版・部分照合 |
| [INT 88H / アニメーションBIOS](bios_int88h.md) | 静的初版・部分照合 |
| [INT 89H / プリンタBIOS](bios_int89h.md) | 静的初版・部分照合 |
| [INT 8AH / コミュニケーションBIOS](bios_int8ah.md) | 静的初版・部分照合 |
| [INT 8BH / Music / Sound BIOS](bios_int8bh.md) | 静的初版・部分照合 |
| [INT 8CH / カレンダー時計BIOS](bios_int8ch.md) | 静的初版・部分照合 |
| [INT 8DH / 日本語入力フロントプロセッサBIOS](bios_int8dh.md) | 静的初版・部分照合 |
| [INT 8EH / ラインエディタBIOS](bios_int8eh.md) | 静的初版・部分照合 |
| [INT 8FH / グラフィック画面制御BIOS](bios_int8fh.md) | 静的初版・部分照合 |
| [INT 90H / V1・V2スーパーバイザ](bios_int90h.md) | 静的初版・部分照合 |
| [INT 92H / ファンシーフォントBIOS](bios_int92h.md) | 静的初版・部分照合 |
| [INT 94H / スクリーンエディタBIOS](bios_int94h.md) | 静的初版・部分照合 |
| [INT 9EH / SETFCB](bios_int9eh.md) | 静的初版・部分照合 |
| [INT 9FH / EXEC_COM](bios_int9fh.md) | 静的初版・部分照合 |
| [INT A0H / 数値演算BIOS / DADD（加算）](bios_inta0h.md) | 静的初版・部分照合 |
| [INT A1H / 数値演算BIOS / DSUB（減算）](bios_inta1h.md) | 静的初版・部分照合 |
| [INT A2H / 数値演算BIOS / DMUL（乗算）](bios_inta2h.md) | 静的初版・部分照合 |
| [INT A3H / 数値演算BIOS / DDIV（除算）](bios_inta3h.md) | 静的初版・部分照合 |
| [INT A4H / 数値演算BIOS / DCMP（比較）](bios_inta4h.md) | 静的初版・部分照合 |
| [INT A5H / 数値演算BIOS / DTOI（整数変換）](bios_inta5h.md) | 静的初版・部分照合 |
| [INT A6H / 数値演算BIOS / ITOD（実数変換）](bios_inta6h.md) | 静的初版・部分照合 |
| [INT A7H / 数値演算BIOS / DIN（ASCII入力）](bios_inta7h.md) | 静的初版・部分照合 |
| [INT A8H / 数値演算BIOS / DOUT（ASCII出力）](bios_inta8h.md) | 静的初版・部分照合 |
| [INT B0H / 数値演算BIOS / SIN（正弦）](bios_intb0h.md) | 静的初版・部分照合 |
| [INT B1H / 数値演算BIOS / COS（余弦）](bios_intb1h.md) | 静的初版・部分照合 |
| [INT B2H / 数値演算BIOS / TAN（正接）](bios_intb2h.md) | 静的初版・部分照合 |
| [INT B3H / 数値演算BIOS / ATN（逆正接）](bios_intb3h.md) | 静的初版・部分照合 |
| [INT B4H / 数値演算BIOS / EXP（指数関数）](bios_intb4h.md) | 静的初版・部分照合 |
| [INT B5H / 数値演算BIOS / LOG（自然対数）](bios_intb5h.md) | 静的初版・部分照合 |
| [INT B6H / 数値演算BIOS / PWR（累乗）](bios_intb6h.md) | 静的初版・部分照合 |
| [INT B7H / 数値演算BIOS / SQRT（平方根）](bios_intb7h.md) | 静的初版・部分照合 |

## Conventional memoryの主要確認点

| 機能 | 領域 | 根拠と限界 |
|---|---|---|
| V1/V2 I/O変換ワーク | 0040:00C0 = 004C0H 等 | 命令/port/転送値、CRTC/DMA管理の抜粋。全占有範囲未確定 |
| DOS共有内部hook | 1040:0480 = 1088:0000 = 10880H | IVT21Hとは別。VA/VA2のOS差し替え確認 |
| DOSワークsegment参照 | 1040:05F8 = 109F8H | 参照先segmentは動的。全内部fieldは未確定 |
| FDD GetBook | 0040:0230 = 00630H | 両ROMの返却値確認 |
| HDD GetBook | 0040:0370 = 00770H | 文書ES:BPに対し両ROMはES:DX |
| Keyboard KyBook | 0040:06B0 = 00AB0H | 両ROMの返却値確認、00AB:0000と同位置 |
| Text GetBook | 0040:0070 = 00470H | 両ROMの返却値確認、TVRAMではなく管理RAM |
| グラフィック画面管理 | 0338:0000 = 03380H | 文書の管理table、両ROM全fieldの照合は未完了 |
| スプライト管理 | 0040:1D82以降 | ROMの状態参照／実行tableの抜粋、全長未確定 |
| アニメーション | 10A0:0000 = 10A00H | ROMの内部DS／hook初期化 |
| プリンタGetBook | 0040:0CD0 = 010D0H | 両ROMの返却値確認 |
| 通信共通変数 | 0040:005E/005F、08B0以降 | 文書の公開変数、全操作照合は未完了 |
| 音源GetBook | 0040:03B0 = 007B0H | 両ROMの返却値確認、旧SoundとMusicの構造差あり |
| 数値演算scratch | 0374:008F/009F | 両ROMのcopy先、呼び出し側数値は8 bytes |
| マウス | 0040:30FCのhook | 全状態構造未確定。hookをGetBook扱いしない |
| RTC変換 | 0040:3332以降 | 読取命令の抜粋、RTC本体の保存領域ではない |
| Fancy font | 035B:0000 = 0040:31B0 | 同じ物理035B0H。VA2は追加領域も使用 |
| Line editor | 0040:3943等 | VA2内部参照の抜粋、history全体は未確定 |
| Screen editor | 03C2:0000 = 03C20H | 両ROM内部DS、TVRAMと別 |
| JFP | 0040:4415のhook（VA2） | 専用work全構造未確定 |
| SETFCB / EXEC_COM | 呼び出し側buffer | 固定専用workは未確定 |
| ADPCM | 042B:0000 = 042B0H（VA2） | ROM内部DS、sound memoryと別 |
| 拡張graphics driver | 1140:0580等 | 拡張driver内部DS。ROM-only環境に一般化しない |

## 安全性と後続確認

1. 起動直後／PC-Engine初期化後／拡張driver読込後のIVTを別々に記録する。
2. GetBookの返却値と予約RAMをread-onlyで測定し、機種・OSごとの構造を分ける。
3. 文書外番号は到達可能性だけで公開APIと認定しない。
4. format、file保存、辞書更新、キュー変更、trap、IF変更、録音／再生、key／機器待ちを総当たりしない。
5. 私有ROM・media・manual・raw disassembly・実行captureはGitへ入れない。
