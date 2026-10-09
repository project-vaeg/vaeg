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

# PC-88VA BIOS解析資料 — INT 83H / テキストBIOS

対象: VA / VA2 ROM、PC-Engine 1.1、テクマニ6.4。
静的解析の第1版。起動後IVT・RAM・表示結果は未実測。

## 0. 配置と確認の区分

604は資料の章番号、83Hが呼び出し割り込み番号。
公開資料は **`docs/bios/bios_int83h.md`**。依頼されたテキスト版は
`BIOS_83H.TXT` としてGit外の作業領域に生成する。
[全BIOS資料の索引](README.md)、[キーボードBIOS](bios_int82h.md)、
[スクリーンエディタ](bios_int94h.md)も参照。

- **文書仕様**: マニュアルの公開契約を独立して要約したもの。
- **ROM確認**: VA・VA2の命令、分岐表、RAM参照から確認したもの。
- **ファイル確認**: PC-Engineシステムコードの静的な登録・転送経路。
- **未確認/推定**: 全分岐、全フィールド、実行結果が未確定のもの。

文書上の44機能をAH別にまとめる。44個の連続した番号ではない。
ROMに入口があることは、すべての画面モードで安全に使える保証ではない。
内部入口の直接CALL、総当たり、ワーク書き換えによる動的試験は行っていない。
私有入力のパス・媒体識別情報・ハッシュ・生の逆アセンブルはGitへ入れない。

## 1. ROM登録とディスパッチ

**ROM確認**: 本体はROM1のF000側に実装される。
共通初期IVT表とは別の初期化コードが83Hを登録する。

| 対象 | IVTへのoffset書き込み | INT 83H入口 | AH分岐表 | GetBook入口 |
|---|---|---|---|---|
| VA | F000:2500 | F000:253F | F000:259F | F000:4B6D |
| VA2 | F000:2AE7 | F000:2B2B | F000:2B93 | F000:5138 |

ES=0000H、BX=83H×4=020CHに入口offsetを格納し、その次のwordへCSを格納する。
一括登録表に83Hがないことは、テキストBIOSの不存在を意味しない。

両dispatcherはAHを32Hと比較し、それ以下ならwordの入口表を参照する。
AH>32Hは処理せずIRETする。専用エラーコードを返す契約とはみなさない。
HDD/FDDのようなAH bit7のリトライ指定はない。
処理中のDSは0040H、0040:0068の0020H maskを実行中状態に使用する。
0040:01C8へ保存frameのSPを置き、戻り値は保存レジスタのスロットへ書き戻す。
0152Hのバンク設定を保存して変更し、終了時に復元する。
共有ワークを持つため、再入や割り込み中の呼び出しが安全とは断定しない。

## 2. 44機能の一覧

名称・主要な入力/出力は**文書仕様**、入口offsetは**ROM確認**。
省略された構造体の全bit・全エラー条件を確定した表ではない。
文書は出力レジスタとCF以外のレジスタを保持するとしているが、全経路の実行照合は未完了。

| AH | 機能 | VA | VA2 | 主な入力 / 出力 |
|---|---|---|---|---|
| 00H | Chput | 2607 | 2BFB | DX=文字。制御コード/escapeを解釈 |
| 01H | Litout | 27C9 | 2DBD | DX=文字。制御コードも文字として表示 |
| 02H | Putstr | 3287 | 3944 | DS:SI=文字列、DX=属性 → CF=スクロール発生 |
| 05H | Setatr | 32EF | 39AB | DH=属性code/mode選択、DL=値 |
| 06H | Setatr2 | 3360 | 3A2E | CX=半角文字数、DX=属性、DH=0 |
| 07H | Setcolor | 33BE | 3AA0 | DL=背景/前景の色nibble |
| 08H | Locate | 33EE | 3AD0 | DH=column、DL=line、分割画面座標 |
| 09H | Goup | 3484 | 3B74 | CL=上へ移動する行数 |
| 0AH | Godown | 34D4 | 3BC4 | CL=下へ移動する行数 |
| 0BH | Goright | 352C | 3C1C | CL=右へ移動するcolumn数 |
| 0CH | Goleft | 3598 | 3C88 | CL=左へ移動するcolumn数 |
| 0DH | Index | 35EF | 3CDF | 下へ1行、必要なら上方向にスクロール |
| 0EH | Rindex | 3648 | 3D38 | 上へ1行、必要なら下方向にスクロール |
| 10H | Scroolh | 36A6 | 3D96 | AL=方向、CL=column数 → CF/CL |
| 11H | Settab | 3889 | 3F79 | 現cursor位置へtab stop設定 |
| 12H | Restab | 3897 | 3F87 | AL=現位置/全tab stopを解除 |
| 13H | Delline | 38BE | 3FAE | CL=削除行数 |
| 14H | Delchr | 39C0 | 40B4 | CL=削除column数 |
| 15H | Insline | 3A5B | 414F | CL=挿入行数 |
| 16H | Inschar | 3B08 | 41FE | CL=挿入column数 |
| 17H | Erascn | 3B6A | 4260 | AL=cursor以後/以前/分割画面全体を消去 |
| 18H | Eralin | 3C1A | 4310 | AL=cursor以後/以前/行全体を消去 |
| 19H | Erafbuf | 3D22 | 4418 | AL=方向/全frame、CX=消去word数 |
| 1AH | Getaddr | 3D8B | 4481 | AL=offset種別、DH/DL=座標 → BL/CX/CF |
| 1BH | Chcscn | 3E22 | 4518 | AL=現在のsubscreen番号を変更 |
| 1CH | Allocfb | 3EBF | 45B5 | CH=開始番号、CL=個数、ES:BP=frame定義列 |
| 1DH | Spltscn | 3F04 | 45FA | AL=現在画面、CL=個数、ES:BP=分割画面定義列 |
| 1EH | Openpop | 4277 | 4979 | AL=mode、CH/CL=幅/高さ、DH/DL=左上座標 |
| 1FH | Closepop | 43EE | 4AF0 | popupを終了して属性等を復元 |
| 20H | Setpopm | 443F | 4B41 | AL=popup mode |
| 21H | Setfont | 4445 | 4B47 | AL=全角/半角、DX=文字、ES:BP=font |
| 22H | Delfont | 4509 | 4B57 | DX=登録fontの文字code |
| 23H | Getfont | 453A | 4B88 | DX=文字、ES:BP=出力font buffer |
| 24H | Crtctl | 465C | 4BBD | AL=CRT mode bit列 |
| 25H | Csrctl | 4963 | 4EE7 | AL=cursor表示/幅/blink等 |
| 26H | Askloc | 4A03 | 4F87 | DH/DL=cursorの分割画面座標 |
| 27H | Askatr | 4A32 | 4FB6 | 属性code/mode取得。選択レジスタの不整合は後述 |
| 28H | Settbm | 4A4A | 4FCE | AL=origin mode、DH/DL=上/下margin |
| 29H | Askchr | 4A96 | 501A | DX=文字 → AL=0全角/1半角 |
| 2AH | Rescrt | 4AB5 | 5039 | text CRTを初期化 |
| 2BH | Getbook | 4B6D | 5138 | ES:DX=BIOSワークの参照先 |
| 2EH | Saskloc | 4E7C | 547A | DH/DL=cursorのキャラクタ座標、CF |
| 2FH | Sfkctl | 4ECA | 54CC | AL=soft key表示数/shift制御 → CF |
| 30H | Setspscn | 4F34 | 5548 | AL=スプライト垂直拡大/同期設定 |

### 公開44機能以外の7 slots

ROM表は00H～32Hの51 slots。公開一覧との差は次の7個。

| AH | VA | VA2 | 静的に確認した処理 |
|---|---|---|---|
| 03H / 04H | 2605 | 2BF9 | RETのみの同じ入口 |
| 0FH | 36A1 | 3D91 | 0040:01F0のfar hookを呼ぶ |
| 2CH / 2DH | 2605 | 2BF9 | RETのみの同じ入口 |
| 31H | 4F54 | 5573 | DXをAXへ移し内部処理後、AL返却slotへ書く |
| 32H | 4F61 | 5580 | DXをAXへ移し内部処理後、DX返却slotへ書く |

RETや内部フックへの到達可能性を「追加の安全な公開API」と解釈しない。
31H/32Hの変換対象・用途・許容値・完全なABIは未確定。

## 3. Conventional memoryのワーク

### GetBookの返却値

**ROM確認: 両ROMとも ES:DX=0040:0070、物理00470H。**

VA:4B6D、VA2:5138は0040:01C8から保存frameを取り出し、
ESのslot（+10H）へ0040H、DXのslot（+0AH）へ0070Hを書き込む。
通常のレジスタへ直接MOVしないことを「返却しない」と誤認しない。

```text
0040H × 10H + 0070H = 00470H
```

これはBIOSの管理データの参照先で、空きRAMの割り当てではない。
全ワークの長さを返す機能でもない。以下は命令から確認した配置の抜粋。
長さはbyte数で、末尾までの完全な占有範囲・公式変数名を保証しない。

| 0040内offset | 物理 | 長さ | 役割 / 確認の限界 |
|---|---|---|---|
| 0070H | 00470H | 1 | CRT mode。表示桁/行mode等の参照 |
| 0074H | 00474H | 2 | TVRAMへアクセスするES用segment。初期化はA000H |
| 0076H | 00476H | 2 | TVRAM側画面control tableのoffset基準。初期値7F00H |
| 0078H | 00478H | 2 | 別のTVRAM側tableのoffset基準。初期値7E00H、全用途未確定 |
| 0084H | 00484H | 1 | Chcscnで現在画面番号の上限判定に使う値 |
| 0085H | 00485H | 20H | 4個×8 bytesのsubscreen記述子 |
| 00A5H | 004A5H | 40H | 16個×4 bytesのframebuffer記述子 |
| 00E5H | 004E5H | 1 | 現在のsubscreen番号 |
| 00E6H | 004E6H | 2 | 現在の内部cursor/画面状態recordへのoffset |
| 00E8H | 004E8H | 2 | 現在のsubscreen記述子へのoffset |
| 00EAH | 004EAH | 2 | TVRAM側の現在画面control tableへのoffset |
| 00ECH | 004ECH | 2 | 消去文字（CLSCHR相当）。初期値0020H |
| 00EEH | 004EEH | 2 | 消去属性（CLSATR相当） |
| 00F0H | 004F0H | 1 | 現在のattribute mode |
| 00F1H | 004F1H | 2 | 現在のattribute code。初期値0070H |
| 00F3H | 004F3H | 24H | 4個×9 bytesの内部cursor/画面状態record |
| 011EH | 0051EH | 4 | INT82H/KyBookから取得したキーボードワークのfar参照 |
| 0130H | 00530H | 4 | soft key表示等が使うfar呼び出し先 |
| 0134H | 00534H | 1 | popup状態判定に使うbit0。全bit未確定 |
| 0136H | 00536H | 20H | 16 wordsのtab関連初期化領域。全bitの意味は未確定 |
| 01AAH | 005AAH | 2 | popup時のAsklocが参照するcursor座標 |
| 01C8H | 005C8H | 2 | 呼び出し保存frameのSP |
| 01CCH | 005CCH | 1 | Settbm/Asklocのorigin mode状態 |
| 01F0H | 005F0H | 3CH | 15個×4 bytesの内部far hook初期化領域 |

**ROM確認**: 初期化はVAで0040:0073から01B9H bytes（末尾022BH）、
VA2で同じ先頭から01BBH bytes（末尾022DH）を0にする。
この後に値やフックを設定する。初期化で消去する範囲と全使用範囲は同一とは限らない。
00F3H以降の4 recordsはVA2:3227の `00F3H + 9×番号`、
subscreen表は33CEの `0085H + 8×番号` で確認できる。
framebuffer表はAllocfbの転送先 `00A5H + 4×CH` と終端00E5Hの検査で確認した。

注意: **0040:020C（物理0060CH）の内部hookと、0000:020C（物理0020CH）のIVT83Hは別物。**
同じoffsetでもsegmentを省略してはいけない。

### 記述子の構造とTVRAMとの分離

**文書仕様とROM照合**: AllocfbのES:BPには4-byte recordsを渡す。
+0/+1=TVRAM内先頭offsetの下位/上位、+2=幅、+3=高さ。
幅はword単位。CL個を通常RAMの00A5H以降へコピーするので、
この表そのものがTVRAM内の文字/属性データではない。

SpltscnのES:BPは8-byte recordsで、+0=frame番号/画面属性、
+1/+2=frame内表示開始column/line、+3=予約、+4=高さ、
+5=mode、+6=背景/前景色、+7=予約。
事前にAllocfbでframeを定義する。全予約bitを勝手に使用しない。

内部9-byte recordの+3/+4はcursor column/line、+7/+8はmarginの参照がある。
これはROMから読める内部配置で、呼び出し側に公開された交換用構造ではない。
0074/0076/0078/00EAHの参照先はバンク指定下のTVRAM側であり、
それらを通常RAMのsegment:offsetとして扱ったり、アプリケーション用に解放したりしない。

## 4. ABI・文書の不整合と副作用

- **文書仕様**: Chput/Litoutの単文字はDXのSHIFT-JIS指定。Putstrは混在文字code列でNULL終端。
  同じ文字列encodingと決めつけない。PutstrのDX bit15は現在属性の使用指定。
- **文書仕様**: 分割画面、キャラクタ画面、framebuffer、popupの座標系は別。
  Locate/AsklocとGetaddr/Sasklocの座標を同一視しない。origin modeでも原点が変わる。
- **資料上の不整合**: 概要のIndex/Rindexの上下説明は個別説明と逆。
  ここでは個別説明に従いIndex=下、Rindex=上としている。全スクロール経路は未実行。
- **ROM確認**: Askatr（27H）はDH=0なら00F1Hのword、DH≠0なら00F0HのbyteをDXへ返す。
  原文は入力DHの一方、出力/機能欄がAL選択と書く。両ROMの分岐はDHを検査する。
- **ROM確認**: SetatrはDH=0で00F1HへDXを保存。原文には「設定する」と
  「更新されない」が混在するため、現在属性が不変という説明を採用しない。
- **資料上の不整合**: Scroolhの入力欄のCLは「ライン数」だが、機能/返却欄はcolumn数。
  横スクロール用として記述した。実際の全境界条件は未検証。
- **ROM確認**: Saskloc（2EH）はcursor座標を合成してDXへ書き戻す。
  原文の出力欄の「ワーク先頭番地」はGetBookと矛盾し、機能欄の座標返却に合う。
  ワーク取得には2BHを使う。VA/VA2の座標計算には記述子の選び方の差が見えるため、
  すべての分割配置での一致は未確定。
- **文書仕様**: Setfont/Getfontのbufferは全角32 bytes、半角16 bytes、
  Getfontの80xxH指定の8×8 fontは8 bytes。予約管理RAMではなく呼び出し側が確保する。
- **文書仕様**: Sfkctlはsystem lineが必要。表示数は0/5/10、shift制御はAL bit7。
  ROMにはAH=01H/02H等の失敗値を書き戻す経路もあるが、全終了条件は未検証。
- 表示、消去、font登録、CRT初期化、frame定義、popup、tab変更は状態変更を伴う。
  screen editor使用中にTEXT BIOSで管理状態を直接変更する場合の制限はINT94H資料も参照。
  Setspscnで同期modeから変更する場合、文書はスプライト初期化を要求する。

## 5. PC-Engine 1.1のVA向け差し替え

**ファイル確認**: 初期化コードC31BはES=0000Hで020CHへ2113H、020EHへCSを書き込む。
INT83HをシステムコードのCS:2113へ置換する処理であり、2113Hは固定物理RAMアドレスではない。

2113のdispatcherはDS=0040Hと01C8Hの保存frameを使い、AH<=32Hを処理する。
2195Hの51-word表で置換handlerを選び、entryが0ならVA ROMのF000:259F表を読み、
対応handlerを実行するよう制御を転送する。
ここで使う259FHはVAのROM表であり、VA2の2B93Hと互換なoffsetではない。
VA向けの経路をVA2の最終動作へ一般化しない。

C34B付近は0040:0228の内部hookを、C355付近は0040:0130のfar参照を変更する。
これらの通常RAM上の参照更新とIVTの0000:020C更新も区別する。
起動後のGetBook返却値、最終IVT、VA2側の登録順/置換表、
OS追加ワーク全体の占有範囲は未実測/未確定。

## 6. 確認結果と残る調査

- 公開44機能と両ROMの51-slot表を区別し、44個の実装入口を照合した。
- ROMのGetBookは両機種で0040:0070（物理00470H）を返す。
- Conventional memoryのsubscreen/frame/cursor記述子、属性、保存frame、
  内部far hookを確認し、TVRAM本体と呼び出し側bufferを分離した。
- Askatr、Setatr、Saskloc等の文書不整合をROM確認と区別して記録した。
- 全field・全error経路・文字変換・表示/スクロール境界・popup復帰・再入契約は未完了。

次は起動前後のIVT、GetBookのES/DXと予約RAMを読み取り中心で測定する。
この静的資料だけを根拠にワークを直接書き換えたり、文書外番号を総当たりしたりしない。
