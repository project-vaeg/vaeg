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

# PC-88VA解析資料 — INT 90H / V1・V2スーパーバイザ

対象: VA / VA2 ROM、PC-Engine 1.1、テクマニ第2章、BNNのCPU/互換mode記述。
静的解析の初版。新たな起動・I/O・モード切替試験は行っていない。
[索引](README.md)、[既存V1/V2調査](../modernization/v1v2-mode-plan.md)、
[M103bの実装・検証記録](../agents/tasks/M103b_v1v2_mode_plumbing.md)も参照。
既存のエミュレータ観測と、今回の静的ROM確認を区別する。

## 1. ニーモニックはZ80か

**入口の先はZ80系、監視・変換側の本体はネイティブ系。両方を使い分ける。**

| 領域/役割 | 読む命令セット |
|---|---|
| F000の初期化、BRKEM2前後、I/Oトラップhandler | uPD9002ネイティブ、V30互換のx86型命令＋NEC拡張 |
| vector90の1000:0000以降、N88-BASIC実行側 | メインCPUのuPD70008互換mode、Z80系命令 |
| CALLN/RETEMのmode境界 | NEC/uPD9002固有拡張。普通のZ80 decoderだけでは不足 |

BNNは互換modeを「uPD780モード」と呼ぶが、本プロジェクトではメインCPUの
uPD70008互換modeとして区別する。別デバイスのFDC CPU/uPD780Cの話ではない。

第2章の「90H=V1/V2スーパーバイザ」は用途分類であり、
F000のネイティブhandlerが90Hに直接登録されるという意味ではなかった。
**ROM確認**: 90Hの登録値は両機種とも1000:0000。
そのvectorを使う `BRKEM2 90H` と、通常の `INT 90H` は別命令。
後者だけでCPUが互換modeへ変わると想定してはいけない。
AH別の通常BIOSとして呼ぶ契約やGetBookは今回の入口経路では確認できていない。

## 2. 個別IVT登録

両機種とも共通初期BIOSの一括表ではなく、初期化の別ループが
番号/offset/segmentの5-byte recordsを6個登録する。

| 対象 | 初期化 | 6-record表 | BRKEM2 90H位置 |
|---|---|---|---|
| VA | F000:0B51、個別登録0B77以降 | F000:097D | F000:0B15 |
| VA2 | F000:13ED、個別登録1413以降 | F000:0F5E | F000:13B1 |

| vector | VA登録値 | VA2登録値 | この経路での役割 |
|---|---|---|---|
| 7CH | F000:1024 | F000:1944 | compatible INトラップのネイティブ処理 |
| 7DH | F000:1024 | F000:1944 | compatible OUTトラップのネイティブ処理 |
| 7EH | F000:1000 | F000:1920 | 01A2Hを読む周辺status/FLAGS処理 |
| 90H | 1000:0000 | 1000:0000 | compatible実行開始位置 |
| 91H | F000:1640 | F000:24B0 | CALLNによるnativeメモリ/I/Oアクセス |
| 95H | 1000:E000 | 1000:E000 | CALLNによるユーザnative codeへの入口 |

```text
90H × 4 = 0240H → IVT 0000:0240 / 0242
1000:0000 → 10000H （互換ROM/RAM mappingの状態に依存）
1000:E000 → 1E000H
```

登録処理はES=0000H、vector×4へoffset/segmentを書き込む。
この初期化は先に256 vectorsをdefault値で埋め、その後に例外/個別入口を上書きする。
後のBIOS/DOS登録もあるため、この時点の全IVTを起動後の最終値と扱わない。

## 3. 互換modeへの入り方とROM配置

**ROM確認**: VA2はPC keyをport000DH bit2で検査し、SW7をport0040H bit3で検査する。
FDD経路は1F90Hで、成功/媒体mode等の分岐があり、いつもV1/V2へ移るわけではない。
条件を満たした経路では1375→18E7でFFEFHへ03Hを書き、I/Oトラップを有効にする。
13A0付近でport0152Hのwordから4000Hをclear（0153H bit6）し、
DS=1000H、ES=0000Hを設定して13B1の `BRKEM2 90H` へ進む。
VA側にも0B15の同じmode遷移がある。

BRKEM2のencodingは `0F FE 90`、長さ3 bytes。
現代x86向けndisasmはこれをMMX/SSE系命令と誤解し、後続命令まで飲み込む。
VA2でnative側へ戻る位置は13B4、VAでは0B18。
そこからtrap無効化（VA2:18EE、VA:0FD3）やnative memory設定復帰へ進む経路がある。
戻り方・戻る条件すべてをROMの並びだけで保証するものではない。

**ROM確認/active memory model照合**: N88-BASIC payloadはROM1 imageの
+10000Hから。互換memory設定下の1000:0000に重なる。
先頭の3命令は両ROMで次の意味を持つ。

| compatible PC | 解釈 |
|---|---|
| 0000H | DI |
| 0001H | LD SP,E1A0H |
| 0004H | JP 3BE5H |

3BE5Hからはport30H/31HのINと条件分岐を使うBASIC起動処理へ進む。
全ROM1をx86で線形逆アセンブルすると、この後半のZ80 CALLを
偽のINT命令として表示する場合もある。ROM1全体が同じISAではない。
先頭命令の一致と、VA/VA2の互換ROM全体の同一性も区別する。

**文書仕様**: CALLN 91H/95Hは `ED ED nn`、RETEMは `ED FD`。
CALLNはnative vectorへ渡し、native routineのIRETで互換側へ戻る。
RETEMはnativeへ戻る別の境界命令。通常のZ80 RST/RET/RETIと同一視しない。
今回はN88 payloadの先頭と初期分岐をZ80系decoderで別途照合した。

## 4. I/Oトラップこそnativeスーパーバイザの中心

### 設定範囲とhandler

**ROM確認**: 初期化データは両機種でFFE7HからFFE0Hへ順に
`00,6F,00,60,00,5B,00,50` を出力する。
wordとしての二つの比較範囲は0050H～005BH、0060H～006FH。
55H等のpalette出力も含むので、50H～53Hだけの範囲ではない。
enable/disableはFFEFHへ03H/00H。

VA2:1944（VA:1024）はnative AX/BX/DX/CX/SI/DI/BP/ESを保存し、
BP=保存frame、DS=0040Hにして、保存されたCS:IPからcompatible命令を読む。
DBH（IN A,(n)）、D3H（OUT (n),A）、ED系register/block I/Oを識別する。
処理後は保存IPを2進めてIRETする。これはZ80の2-byte命令を読んで変換する
**native code**であって、handler自体がZ80で動作するのではない。

VA2のblock-I/O表1996Hは8 wordsで、順に
INI/OUTI/IND/OUTD/INIR/OTIR/INDR/OTDRへ対応する処理入口を選ぶ。
repeat系はhandler内部で繰り返すため、IP+2だけを見て一回の転送と判断しない。
全prefix・flag・割り込みタイミングの実機一致はこの資料では未検証。

### 32-slot port dispatch

**ROM確認**: portから50Hを引いてword表を引く。
VAのIN/OUT表は11D7H/1217H、VA2は1AF7H/1B37H。
次のoffsetはすべてsegment F000H内。

| 88互換port | VA IN / OUT | VA2 IN / OUT | 処理の要約 |
|---|---|---|---|
| 50H | 12AC / 12AE | 1BCC / 1BCE | 3301 parameter出力 |
| 51H | 1257 / 12FF | 1B77 / 1C28 | 3301 status/command |
| 52H | 12AC / 149D | 1BCC / 1DE1 | 背景色変換 |
| 53H | 12AC / 1459 | 1BCC / 1D9D | 表示制御変換 |
| 54H | 12AC / 14C5 | 1BCC / 1E11 | palette色変換 |
| 55H | 12AC / 14C5 | 1BCC / 1E11 | palette色変換 |
| 56H | 12AC / 14C5 | 1BCC / 1E11 | palette色変換 |
| 57H | 12AC / 14C5 | 1BCC / 1E11 | palette色変換 |
| 58H | 12AC / 14C5 | 1BCC / 1E11 | palette色変換 |
| 59H | 12AC / 14C5 | 1BCC / 1E11 | palette色変換 |
| 5AH | 12AC / 14C5 | 1BCC / 1E11 | palette色変換 |
| 5BH | 12AC / 14C5 | 1BCC / 1E11 | palette色変換 |
| 5CH | 12AC / 12AC | 1BCC / 1BCC | 共通未処理入口 |
| 5DH | 12AC / 12AC | 1BCC / 1BCC | 共通未処理入口 |
| 5EH | 12AC / 12AC | 1BCC / 1BCC | 共通未処理入口 |
| 5FH | 12AC / 12AC | 1BCC / 1BCC | 共通未処理入口 |
| 60H | 1261 / 155E | 1B81 / 1EAA | DMA byte対の参照/更新 |
| 61H | 1273 / 1587 | 1B93 / 1ED3 | DMA byte対の参照/更新 |
| 62H | 1277 / 1570 | 1B97 / 1EBC | DMA byte対の参照/更新 |
| 63H | 127B / 158B | 1B9B / 1ED7 | DMA byte対の参照/更新 |
| 64H | 127F / 1574 | 1B9F / 1EC0 | DMA byte対の参照/更新 |
| 65H | 1283 / 158F | 1BA3 / 1EDB | DMA byte対の参照/更新 |
| 66H | 1287 / 1583 | 1BA7 / 1ECF | DMA byte対の参照/更新 |
| 67H | 128B / 1593 | 1BAB / 1EDF | DMA byte対の参照/更新 |
| 68H | 128F / 1597 | 1BAF / 1EE3 | DMA mode/制御 |
| 69H | 12AC / 12AC | 1BCC / 1BCC | 共通未処理入口 |
| 6AH | 12AC / 12AC | 1BCC / 1BCC | 共通未処理入口 |
| 6BH | 12AC / 12AC | 1BCC / 1BCC | 共通未処理入口 |
| 6CH | 12AC / 12AC | 1BCC / 1BCC | 共通未処理入口 |
| 6DH | 12AC / 12AC | 1BCC / 1BCC | 共通未処理入口 |
| 6EH | 1295 / 12AC | 1BB5 / 1BCC | native 01C9H bit5の状態取得 |
| 6FH | 12A5 / 15BA | 1BC5 / 1F06 | タイマ設定参照/変換 |

共通未処理入口（VA:12AC、VA2:1BCC）はSTC; RET。
表に5CH～5FH等があることは、初期設定でそのportがtrapされる証拠ではない。
その番号は50H～5BH/60H～6FHの範囲外の場合もある。

- 50H/51H: compatible CRTCのparameter/command状態をRAMで受け取り、TSP側の処理へ変換。
- 52Hと54H～5BH: compatible色値をnative palette形式へ変換する経路。
  全画面mode・palette値の一致は未実測。
- 53H: native0148H/0110H等の表示制御へ変換する。
- 60H～67H: compatible DMA register imageをRAMで読み書きする。
  VA2の64H更新には表示開始関連の追加処理がある。native DMAへ単純転送するだけではない。
- 68H: DMA modeから表示状態を更新し、byte対の切替状態もresetする。
- 6EH: 01C9H bit5を読み、FFH/7FHを返す経路。
- 6FH: 設定値の下位4 bitsを用い、native01A6H/01A4Hへタイマ設定を出す。

88互換の全I/Oがsoftware trapではない。対象外のkeyboard、serial、memory切替等は
別のhardware/実装経路で、port範囲とdispatch表を混同しない。

## 5. Conventional memoryとCALLN境界

handlerのDS=0040Hにおける固定参照の抜粋。公式公開構造や占有範囲ではない。

| 0040内offset | 物理 | ROMから分かる役割 |
|---|---|---|
| 00C0H | 004C0H | trapされた2-byte命令のword。C1Hは第2byteの参照 |
| 00C2H | 004C2H | compatible port番号のbyte |
| 00C3H | 004C3H | IN/OUTの転送値byte |
| 00C4H | 004C4H | CRTC parameter受信位置（byte） |
| 00C5H | 004C5H | CRTC command/処理mode状態。全意味は未確定 |
| 00C7H | 004C7H | parameter受信の基準。C7H+indexで参照、全長未確定 |
| 00D0H | 004D0H | 表示/CRTC/DMAに関係するbit状態。全bit未確定 |
| 00E2H | 004E2H | native背景palette値の保持word |
| 00E6H | 004E6H | native表示制御値の保持word |
| 00E8H | 004E8H | native0148H向け表示制御値byte |
| 00E9H | 004E9H | 6FHから設定した下位4 bits/タイマ設定の参照byte |
| 00EAH | 004EAH | DMA imageの下位/上位byte切替状態 |
| 00EBH | 004EBH | 60H～67Hに対応する8 words（16 bytes）のDMA image |

VA2:1A62はportを保存BCの低byteから取り、1A5BはAの値を保存AXの低byteから取る。
register/block形式の残りのregister選択やflag更新もnative側が保存frameを変更する。
このfield配置をV3のTEXT BIOSワークと併用して直接書き換える契約はない。
TVRAM、GVRAM、DMA転送元のcompatible RAMは、この小さな管理領域とは別。

**文書仕様/既存CPUモデル**: Aはnative AL、BCはCX、DEはDX、HLはBX、
IX/IYはSI/DI、compatible SPはBP、FはFLAGS下位に対応する。
native SP/SSで積むframeと、compatible SPを同一のstackと解釈しない。

**文書仕様**: CALLN 91HはAでmemory/I/O、read/write、byte/wordを選び、
HL=segment、DE=offset/port、BC=値としてnativeアクセスを行う。
CALLN 95Hの登録先1000:E000はユーザがnative命令を用意する領域。
1000H segmentだから必ずZ80命令、という判断も誤りになる。
91Hの全8分岐の完全ABI一致や95Hのユーザcodeを動的に検証したものではない。

## 6. PC-Engineから確認できたこと・未確認のこと

PC-EngineのVA/VA2用初期化、vector登録用INT21H呼び出し、直接のmode遷移候補を
調べたが、今回の経路で90HのIVTを差し替える処理は確定できなかった。
確認できた90H登録とBRKEM2 handoffはROMの処理である。
「PC-Engineファイル側に未確認」と「どのdriverも変更しない」は別の主張。
起動後/ロード後のIVTを実測していないため、後者は断定しない。

ファイル6DA0の `AL=90H` は、一見vector番号に見えるが、
6F52/6F59のhelperからport0044Hへ送る音源register操作だった。
値90Hの検索結果をINT90H呼び出し・フックの証拠に使ってはいけない。
また、全コード/全tableの不存在証明を単純なbyte検索で行うことはできない。

次の独立課題は91H/95Hの詳細ABI、V1/V2 mode別の全supervisor変換、
最終IVT/制御port/RAMの実測、prefix/flag/timingの実機一致。
本資料はROM内の構造を詳述するもので、通常アプリからINT90Hを試す手順ではない。
