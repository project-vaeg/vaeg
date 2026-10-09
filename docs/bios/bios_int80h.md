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

# PC-88VA BIOS解析資料 — INT 80H / フロッピーディスクBIOS
対象: VA / VA2 ROM、PC-Engine 1.1、テクマニのFDD BIOS章、BNN資料
版: 静的解析 第1版

## 0. この資料の位置づけ
本資料はテクマニ6.1「フロッピーディスクBIOS」に相当する解析版。
正式な配置先は `docs/bios/`、命名形式は `bios_intXXh.md` とする。
601はマニュアルの章番号であり、解析資料のファイル名には使わない。
「XX」は割り込み番号。AH別機能は同じ資料の中にまとめる。

[ROM確認] は実際のバイト列・到達可能な分岐から確認した事項。
[文書仕様] はマニュアル記載で、全経路の実装照合はまだ済んでいない。
[推定/未確認] はABI・範囲・実行環境を確定できていない事項。
実機/エミュレータでの起動後IVT、戻り値、RAMの実測は未実施。
資料を作るためのディスク書き込み、ROM変更、BIOS総当たり呼び出しは
行っていない。特にFORMATは実行していない。

アドレス表記の区別:

```text
0040:0230 = 実行時のセグメント:オフセット
00630H    = CPUから見た物理アドレス
bank2:F062 = ROM0のバンク内オフセット。直接CALL可能な固定番地ではない。
file:C554 = 抽出したPC-Engineシステムファイルのオフセット
```

ROMのE000窓にはバンク切り替えがある。物理RAMとROMファイル位置を混同しない。

## 1. 呼び出し形式とディスパッチ

```text
INT 80H
AH bits0..6 : 機能番号 00H〜0DH
AH bit7    : [文書仕様] 0=リトライあり、1=リトライなし
CF=0       : 原則として正常終了
CF=1       : エラー、AHにエラー番号
```

[ROM確認] VA/VA2ともにAHを7FHでマスクし、0EH以上を除外する。
READとWRITEは同じ入口を使い、内部で機能番号を区別する。
0BH〜0DHは通常のアクセス機能とは異なる分岐で扱われる。
未定義AH値を「隠し機能」とみなす根拠はない。

主な入力 [文書仕様]:

```text
CH    : ドライブ番号 0/1
CL    : 論理トラック番号、シリンダ×2+ヘッド（両面時）
DH    : セクタ番号
DL    : bit7=FM指定、bits0..6=セクタ長コードN
AL    : READ/WRITEでは転送セクタ数
ES:BP : 転送バッファ、またはFORMATのID配列
BH/BL : BIOSMODE.IDR=1のREAD/WRITEではIDのC/H
```

機能ごとに使用レジスタは異なる。上記を全機能の共通ABIとはしない。

[ROM確認] READ/WRITEの入力検査ではCH<=1、AL!=0、DH!=0等を検査する。
セクタ長コードは通常128×2^Nバイトとして転送サイズを計算する。
VA2:6444〜64DCでは開始・終了物理アドレスの20bit範囲も検査する。
64KBのDMAページ境界を必ず禁止するという処理ではない。

## 2. 機能一覧と実装入口
| 機能 | 名称 | VA bank2 | VA2 bank5 |
| --- | --- | --- | --- |
| 00H | INITIALIZE | E212 | 6225 |
| 01H | READ DATA | E358 | 6367 |
| 02H | WRITE DATA | E358 | 6367 |
| 03H | FORMAT TRACK | E833 | 687D |
| 04H | FORMAT DRIVE | EA61 | 6AB9 |
| 05H | READ ID | EB36 | 6B8D |
| 06H | RECALIBRATE | ECD4 | 6D29 |
| 07H | SENSE | EE2C | 6E84 |
| 08H | CHECK DRIVE READY | EE89 | 6EDF |
| 09H | CHECK DOOR OPENED | EEC0 | 6F15 |
| 0AH | SET DISK MODE | EEED | 6F42 |
| 0BH | RECOVER WORK | F012 | 705B |
| 0CH | SELECT INTELLIGENT MODE | F025 | 706E |
| 0DH | GET BOOK | F062 | 70AB |

[ROM確認] テーブル位置: VA bank2:F251、VA2 bank5:728A。
14個の16bitポインタをバイナリから取得。01H/02Hの入口共有も確認。
テクマニは00H〜0DHを記載する。BNNの冒頭一覧は00H〜0BH。
したがって0CH/0DHはBNN一覧の補完対象だが「完全な未公開BIOS」ではない。

### 00H INITIALIZE
  [文書仕様] FDC初期化、ドライブ検出、ワーク初期化。
  [ROM確認] 初期化サブルーチン VA2:6330〜6358:
    DISKMODE#0/#1=FFH、SURFMODE=03H、BIOSMODE=00H、
    RETRYNUM=08H、MTOFFTIM=FAH、DINTSTAT/DOORSTAT=00H。
  接続検出の詳細、全レジスタ破壊範囲は未確定。

### 01H READ DATA / 02H WRITE DATA
  [文書仕様] AL個のセクタをES:BPへ/から転送。
  IDR=0では通常のトラックとID、IDR=1ではBH=C、BL=Hを独立指定。
  PARM=1ではUSERPARMの5バイトを使用。
  [ROM確認] VA2:653B〜6553でREAD=06H、WRITE=05HのFDCコマンドを選ぶ。
  VA2:64F5〜6506でAH bit7に応じてRETRYNUMを内部カウンタへコピーする。
  すべてのエラーが同じ回数リトライされるわけではない。
  VA2:6596〜65CEにはエラー種別による即時終了・再較正の分岐がある。

### 03H FORMAT TRACK / 04H FORMAT DRIVE
  [文書仕様] トラック単位/全ドライブをフォーマットする。破壊的操作。
  IDR=1ではES:BPにC,H,R,Nをセクタ数分並べる。
  IDR=0ではBIOS内部FORMBUFFに正規IDを生成する。
  [ROM確認] VA2:6B71〜6B7Fで内部バッファDS:0260を選択する経路がある。
  全トラック反復、特殊FMトラック、途中失敗時状態は動的未検証。

### 05H READ ID
  [文書仕様] 指定トラック上のIDを読み、結果を返す。
  RSLTSTATにもFDC結果が保存される。
  [ROM確認] READ/WRITEと同様にAH bit7とRETRYNUMを利用する経路がある。
  戻りレジスタの完全な契約は原文と全終了経路の追加照合が必要。

### 06H RECALIBRATE
  [文書仕様] 指定ドライブをトラック0へ戻す。
  [ROM確認] 対応した実装入口がある。SINTSTATにST0/PCNを保持する。

### 07H SENSE
  [文書仕様] 指定ドライブの状態を調べる。
  詳細な出力レジスタ/ビット契約は未確定。

### 08H CHECK DRIVE READY / 09H CHECK DOOR OPENED
  [文書仕様] レディ状態/ドア開閉履歴を検査する。
  [ROM確認] 対応入口と内部状態参照がある。
  CFだけでなくAHを含む全終了経路の照合は未完了。
  関数名からFDCステータスの単一ビットへ直結すると断定しない。

### 0AH SET DISK MODE
  [文書仕様] CHで指定したドライブのDISKMODEへALを設定。
  FFHを入力して初期化直後の無効モードへ戻すことはできない。
  [ROM確認] モード検査・設定に対応する入口がある。

### 0BH RECOVER WORK
  [ROM確認] VA:F012 / VA2:705B:
    byte[0235]=00H、byte[0236]=08H、byte[0237]=FAH、AH=0、CF=0。
  DISKMODE、SURFMODE、USERPARMを含む全ワークの初期化ではない。
  原文のRETRYNUM「0（リトライ8回）」は記述矛盾。
  ROMは明確に08Hを格納し、原文のワーク説明も既定値8とする。

### 0CH SELECT INTELLIGENT MODE
  [ROM確認] 0040:0064 bit0が1のとき、01B6H/01B0H等に出力し、
  内部制御状態を変更する。最後はAH=0、CF=0。
  名称だけで単純なモードフラグ設定だと考えないこと。
  FDC側プログラム、通信経路を含む完全なモード遷移は未確定。

### 0DH GET BOOK
  [ROM確認] VA:F062 / VA2:70AB:
    push ds / pop es / mov dx,0230H / clc / xor ah,ah / ret
  ROMディスパッチのDS=0040Hと合わせ、ES:DX=0040:0230を返す。
  「BOOK」は資料上の名称。返すのはFDD BIOSのワーク領域の先頭。
  Conventional memoryの総量や、空きメモリの先頭ではない。
  呼び出し時点の実際の返却値は起動後の測定で確認すること。

## 3. Conventional memory上のワークエリア
[ROM確認] ワークの基点 SYS=0040:0230、物理00630H。
計算: 0040H×10H+0230H=00630H。
PC互換機のBIOS Data Areaの慣習から配置を推定せず、VA ROMに基づく。

以下の名称/構造はテクマニ、具体的な基点はGET BOOK実装から得た。
全フィールドについて全書き込み元を追跡済みという意味ではない。

| SYS相対 | 0040内offset | 物理 | 長さ | 名前 / 用途 |
| --- | --- | --- | --- | --- |
| +00H | 0230H | 00630H | 1 | DISKMODE#0 |
| +01H | 0231H | 00631H | 1 | DISKMODE#1 |
| +02H | 0232H | 00632H | 2 | 予約 |
| +04H | 0234H | 00634H | 1 | SURFMODE |
| +05H | 0235H | 00635H | 1 | BIOSMODE |
| +06H | 0236H | 00636H | 1 | RETRYNUM |
| +07H | 0237H | 00637H | 1 | MTOFFTIM |
| +08H | 0238H | 00638H | 3 | 文書上未使用 |
| +0BH | 023BH | 0063BH | 5 | USERPARM |
| +10H | 0240H | 00640H | 8 | RSLTSTAT |
| +18H | 0248H | 00648H | 4 | 文書上未使用 |
| +1CH | 024CH | 0064CH | 1 | UNITSTAT |
| +1DH | 024DH | 0064DH | 1 | DINTSTAT |
| +1EH | 024EH | 0064EH | 1 | DOORSTAT |
| +1FH | 024FH | 0064FH | 1 | MOTRSTAT |
| +20H | 0250H | 00650H | 2 | SINTSTAT#0 (ST0,PCN) |
| +22H | 0252H | 00652H | 2 | SINTSTAT#1 (ST0,PCN) |
| +24H | 0254H | 00654H | 4 | 予約 |
| +28H | 0258H | 00658H | 6 | 文書上未使用 |
| +2EH | 025EH | 0065EH | 1 | RETRYCNT |
| +2FH | 025FH | 0065FH | 1 | MTOFFCNT |
| +30H | 0260H | 00660H | 可変 | FORMBUFF (C,H,R,Nの配列) |

FORMBUFF: 標準の最大26セクタなら104バイト必要。
00660H〜006C7Hがその104バイトに相当する。
USERPARMでEOTを変更した場合の許容上限・厳密な確保サイズは未確定。
マニュアル図の予約域/未使用域をアプリの空き領域として使用しないこと。

BIOSMODE (0040:0235):
  bit2 (04H) IDR  : C/H独立指定、FORMATの外部ID配列
  bit1 (02H) PARM : USERPARM使用
  bit0           : 原文の別箇所ではRAWに言及。完全な意味は未確定
  原文の「PRAM」は「PARM」の表記揺れと扱う。

SURFMODE (0040:0234):
  bit0=ドライブ0、bit1=ドライブ1。1=両面、0=片面。
  [ROM確認] VA2:70D5〜70ECはこのビットに従ってトラック番号を分解する。

USERPARM (0040:023B〜023F):
  023B MAXCYL : 最大シリンダ番号（シリンダ数ではない）
  023C EOT    : 最大セクタ番号
  023D RWGPL  : READ/WRITE用ギャップ長
  023E FMGPL  : FORMAT用ギャップ長
  023F DTL    : READ/WRITE用データ長
  [ROM確認] VA2:67C5〜67DEでBIOSMODE bit1を検査し、
  この5バイトを内部0040:034F〜0353へコピーする。

RSLTSTAT (0040:0240〜0247):
  0240 COM、0241 ST0、0242 ST1、0243 ST2、
  0244 C、0245 H、0246 R、0247 N。
  [ROM確認] VA2:66ED〜66F5はST0/ST1/ST2をロードしてAHエラーに変換する。

RETRYCNTとMTOFFCNT:
  [ROM確認] VA2:6502ではRETRYNUMからRETRYCNTへコピー。
  VA:E1F8〜E1FCではMTOFFTIMからMTOFFCNTへコピー。
  MTOFFTIM=250、単位100ms=25秒は文書仕様。
  100msの実時間精度は割り込み周期も必要であり、この解析では未測定。

注意: ステータス領域全体がユーザ書き換え可能なのではない。
  DINTSTATは割り込みハンドラとの共有フラグ。
  DOORSTAT、MOTRSTAT、MTOFFCNTもBIOSとハードの状態に関係する。
  「内部で上書きされるので効果がない」と「安全に変更できる」は別。

## 4. 文書のSYS領域外にある内部RAM参照
[ROM確認] FDDコードは0040:0330以降も内部ワークとして使用する。
GET BOOKで先頭を得ても、確保サイズや全内部ABIが公開されるわけではない。

| 0040内offset | 物理 | 解析上の役割（正式な変数名ではない） |
| --- | --- | --- |
| 0330H | 00730H | 元のAH（リトライbitを含む） |
| 0331H | 00731H | 転送セクタ数 |
| 0332H | 00732H | ドライブ番号 |
| 0333H | 00733H | 論理トラック番号 |
| 0334H〜0335H | 00734H | 入力BH/BL（IDのC/H） |
| 0336H | 00736H | 開始セクタ番号 |
| 0337H | 00737H | DL相当、FM指定/セクタ長コード |
| 0339H | 00739H | バッファ物理アドレスの上位部分 |
| 033AH〜033BH | 0073AH | バッファ物理アドレスの下位16bit |
| 033CH〜 | 0073CH | FDCコマンド組み立て領域 |
| 0346H | 00746H | 特定エラーに対する再較正の回数制御 |
| 034AH | 0074AH | 終了時のAH保存 |
| 034FH〜0353H | 0074FH | 実際に使用する5バイトのFDCパラメータ |

[ROM確認] 初期化フックに世代差がある:
  VA : 0040:0354からのfarポインタ1個をbank2:E009へ設定。
  VA2: 0040:0358を0、0359からのfarポインタ5個をbank5:6009へ設定。
  物理アドレス: 00754H、00758H、00759H。
  VA2:6009 / VA:E009はRETFのみの既定フック。
これらはコールバック/内部制御領域で、空きRAMではない。
すべてのフックの用途・呼び出しABIはまだ確定していない。

## 5. PC-Engine 1.1による差し替え
ROM初期状態だけで起動後のINT 80Hを説明しないこと。

[ファイル確認] PC-Engineシステムファイル file:C54B〜C56D:
  DS=0000Hにして、IVTの0000:0200へ533DH、0202へCSを書き込む。
  0200H=80H×4なので、INT 80Hのベクタを置換している。
  0040:0358もゼロにする。

VA向けラッパー file:533D〜5370:
  ポート0152Hのバンク指定を保存し、AL下位を02Hへ変更。
  file:53ECでREAD/WRITEとバッファ範囲を検査する。
  通常経路はE000:E003のROM FDDディスパッチへfar call。
  特殊経路はfile:5371へ進み、割り込み/フックを一時差し替えて
  ROM内部ルーチンを組み合わせる。
  復帰時はバンク指定を復元してIRETする。

[ファイル確認] 特殊経路選択には物理アドレス上位の比較0AH/0EHがある。
  READ/WRITEでA0000H〜DFFFFHの範囲に関わるバッファを特別扱いする
  意図と読めるが、全境界条件と転送結果は実行検証していない。
  Conventional RAMの一般的な空き領域取得サービスではない。

VA2向けコードは別の実行時原点を持つ。
VAラッパーのオフセットをVA2へそのまま適用しない。
VA2起動後のINT 80H置換有無/最終入口、追加ワーク範囲は未確定。

PC-Engine初期I/Oファイルによる別のフック表修正も存在する。
その修正をFDD全機能の実装置換と同一視しない。
本資料の機能表はROM実装の入口であり、起動後IVTの実測表ではない。

## 6. 標準パラメータ表の照合
[ROM確認] VA bank2:F26D / VA2 bank5:72A6からの表は、
6組×3レコード×5バイトの比較で一致した。
先頭のゼロ組/中間のゼロ組は有効な媒体パラメータとはみなさない。
分岐から選択が確認できる非ゼロ組は以下（値は10進）。

| 系列 | N | MAXCYL | EOT | RWGPL | FMGPL | DTL |
| --- | --- | --- | --- | --- | --- | --- |
| 1D/2D | 1 | 39 | 16 | 14 | 54 | 255 |
|  | 2 | 39 | 9 | 42 | 80 | 255 |
|  | 3 | 39 | 5 | 53 | 116 | 255 |
| 1DD/2DD | 1 | 79 | 16 | 14 | 54 | 255 |
|  | 2 | 79 | 9 | 42 | 80 | 255 |
|  | 3 | 79 | 5 | 53 | 116 | 255 |
| 1HD/2HD | 1 | 79 | 26 | 14 | 54 | 255 |
|  | 2 | 79 | 15 | 42 | 80 | 255 |
|  | 3 | 79 | 8 | 53 | 116 | 255 |
| 特殊系列の組 | 0 | 79 | 26 | 7 | 27 | 128 |
|  | 1 | 79 | 15 | 14 | 42 | 255 |
|  | 2 | 79 | 8 | 27 | 58 | 255 |

通常系列のN=1/2/3は256/512/1024バイト。
特殊系列の全選択条件・トラック0の切り替えは追加追跡が必要。
表のレコード位置だけから全媒体を割り当てない。
数値表と入口の抽出結果は私的解析記録に保存した（Git管理外）。

## 7. 今回の結論と残る調査
確認できたこと:
  ・INT 80Hには00H〜0DHの14機能がある。
  ・0CH/0DHは少なくともテクマニでは公開されている。
  ・FDDワーク先頭はROM実装上0040:0230、物理00630H。
  ・公開ワーク以外にも00730H以降の内部RAMを使う。
  ・RECOVER WORKは3バイトの復帰で、RETRYNUMは8。
  ・VA用PC-Engine 1.1はINT 80Hをディスク側ラッパーに差し替える。

未確認:
  ・起動後のVA/VA2 IVTとGET BOOKの実測値。
  ・全機能の戻りレジスタ/破壊レジスタ/全エラー経路。
  ・すべての内部ワークの正確な確保範囲と寿命。
  ・特殊媒体、DMA境界、割り込み競合時の動作。
  ・他のBIOSのConventional memoryワークとDOS空き領域の全体配置。

次に行うなら、起動後にINT 80Hベクタと0040:0230周辺を読み取り、
ROMの静的結果と照合する。まずGET BOOKだけを安全に実測し、
FORMAT/WRITEや内部ROMへの直接CALLは行わない。

## 8. 出典・解析の再現範囲

- テクマニ6.1の機能一覧・ワーク説明とBNNのFDD BIOS一覧を比較した。
- VAのROM0 bank2、VA2のROM0 bank5の到達可能コードとデータ表を照合した。
- PC-Engine 1.1のシステムコードからVA向けIVT置換とラッパーを追跡した。
- 14個の16bit入口ポインタを抽出し、READ/WRITEの入口共有を確認した。
- パラメータ表の6組×3レコード×5バイト（計90バイト）の対応部分を比較し、
  VA/VA2で一致した。各組の間のパディングまで同一と主張するものではない。

これらは私的入力を用いた静的照合結果であり、公開の再実行可能なテスト
コーパスではない。入力識別情報、原文コピー、抽出バイナリ、生の逆アセンブル、
ハッシュ記録はGit外に保持する。追試には適法に用意した対応資料が必要。

線形逆アセンブルはデータも命令と解釈するため、分岐表は16bit little-endian
データとして取得した。登録入口の存在だけで安全な公開APIとは判断しない。
