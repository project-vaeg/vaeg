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

# PC-88VA BIOS解析資料 — INT 81H / ハードディスクBIOS

対象: VA / VA2 ROM、PC-Engine 1.1、テクマニ6.2、BNN資料。
静的解析の第1版。起動後IVT・RAM・転送結果は未実測。

## 0. 名前と調査範囲

テクマニの `602HDD.TXT` に相当する独立した解析資料。
正式な配置は **`docs/bios/bios_int81h.md`** とする。
602は章番号、81Hは呼び出し割り込み番号であり、後者を名前に用いる。
TXT版が必要なら `bios_int81h.txt` とし、AH別機能は同じ資料にまとめる。
FDD BIOSは [INT 80Hの解析](bios_int80h.md) を参照。

- **文書仕様**: テクマニ・BNNに記載された契約。
- **ROM確認**: 対応ROMの分岐・命令・データ表から確認した事項。
- **ファイル確認**: PC-Engineシステムコードから確認した事項。
- **未確認/推定**: 全経路の照合や実行検証が済んでいない事項。

ここで扱うのは歴史的なオプションHDDインタフェース向けBIOS。
任意のSCSIボードのドライバ、HOSTFAT、DOSのファイルサービスと同一ではない。
ROM・媒体の変更、WRITE/FORMAT、内部入口への直接CALLは行っていない。
私有入力の場所・媒体識別情報・ハッシュ・生の逆アセンブルは公開しない。

## 1. 呼び出しとROM入口

文書仕様の呼び出しは `INT 81H`。
ROMの初期ベクタ登録表と割り込みスタブを照合した。

| 対象 | 初期IVTの入口 | ROM0バンク | バンク窓の入口 | ディスパッチ | 機能表 |
| --- | --- | --- | --- | --- | --- |
| VA | F000:02C0 | 3 | E000:9003 | 902F | 9AA4 |
| VA2 | F000:03D2 | 5 | E000:7403 | 74DA | 7FC4 |

バンク番号・バンク内オフセットはROM配置の識別である。
そのバンクが選択されていないとき、同じ窓のアドレスへCALLしても同じ処理にはならない。

**ROM確認**:

- `AH & 7FH` を機能番号とし、`0BH`以上は `AH=02H, CF=1` で除外する。
- 有効機能は `00H〜0AH` の11個。READ/WRITEは入口を共有する。
- ディスパッチは `DS=0040H` にし、`0040:0068` のbit3を実行中にセット、終了時にクリアする。
- AH=0DHを返す失敗経路には、ディスパッチ側から回復処理を呼ぶ分岐がある。
- この調査範囲に、11機能以外の隠しAH機能を裏付ける分岐表はない。

## 2. レジスタ契約

以下は文書仕様。機能ごとに使わないレジスタもある。

| レジスタ | 意味 |
| --- | --- |
| AH bits0..6 | 機能番号 |
| AH bit7 | 0=リトライあり、1=なし |
| AL | READ/WRITEのセクタ数、FORMATのインターリーブ係数 |
| BH bit0 | ドライブ0/1 |
| BH bit7 | 0=シリンダ/ヘッド/セクタ指定、1=相対セクタアドレス指定 |
| CX | シリンダ番号、または相対アドレス下位16bit |
| DH | ヘッド番号。相対アドレス指定では使用しない |
| DL | 開始セクタ番号、または相対アドレス上位5bit |
| ES:BP | 転送バッファ、または代替トラックの4バイトバッファ |

BHのbit1〜6は予約。ROMのREAD/WRITE入力検査でも `BH & 7EH` を除外する。
相対アドレス指定では `DL & E0H` を除外し、DHを内部では0にする。

```text
相対アドレス = (DL & 1FH) × 65536 + CX
相対アドレス = 33 × (ヘッド数 × シリンダ + ヘッド) + セクタ
```

ROMの換算処理VA2:7782〜77ABはヘッド数をパラメータの `+18H` から読み、
乗算係数33を使用する。セクタは0始まりで、通常の最大番号は32。

**ROM確認**: READ/WRITEのDMA転送サイズは `256 × AL` バイト。
VA2:76A4〜76B0、7804〜7829でサイズと終了物理アドレスを計算する。
開始/終了が20bitアドレス範囲を越える場合はパラメータエラー経路に入る。
FDDのセクタ長指定DLと同じ意味ではない。

AH bit7はHDCコマンドの制御バイトへ反映される経路がある
（VA2:7715〜771C）。FDDの `RETRYNUM` をHDDにも適用する根拠はない。
HDC側のリトライ回数やすべての再実行条件は未確定。

## 3. 機能一覧と処理

入口はバンク内オフセット。名称・用途は文書仕様、入口はROM確認。

| AH | 機能 | VA bank3 | VA2 bank5 | 主な用途 |
| --- | --- | --- | --- | --- |
| 00H | INITIALIZE | 908A | 7535 | 初期化・接続検出・較正・退避 |
| 01H | READ DATA | 91DD | 7694 | ALセクタをES:BPへ読み出す |
| 02H | WRITE DATA | 91DD | 7694 | ALセクタをES:BPから書き込む |
| 03H | FORMAT TRACK | 933A | 7833 | 指定トラックの初期化 |
| 04H | FORMAT DRIVE | 9409 | 790D | ドライブ全体の初期化 |
| 05H | RECALIBRATE | 95A8 | 7AB7 | シリンダ0へ戻す |
| 06H | RETRACT | 9639 | 7B4C | ヘッドを退避する |
| 07H | ASSIGN ALTERNATE TRACK | 9743 | 7C5A | 代替先アドレスを4バイトに整形 |
| 08H | FORMAT BAD TRACK | 97A8 | 7CC3 | 不良トラックへ代替先を割り付ける |
| 09H | SENSE | 9879 | 7D98 | 状態を調べ、ALにタイプを返す |
| 0AH | GET BOOK | 989D | 7DBC | BIOSワーク先頭を返す |

### 00H INITIALIZE

文書仕様はHDC初期化、ディスクタイプ/パラメータ参照設定、接続検出、
RECALIBRATEとRETRACT。ROMはI/Oポート82Hを操作し、各ドライブを検査する。

VA2:7617〜7693はポート82Hの設定値からタイプとROMテーブル位置を選ぶ。
未知の設定ではタイプをFFH、参照を第5レコードへ設定する経路がある。
初期化は各失敗を個別ドライブの状態に反映するが、最後にはAH=0・CF=0を返す
（VA2:7612〜7616）。AH=0だけで「HDDが接続されている」と判定しない。

### 01H READ DATA / 02H WRITE DATA

共有入口で元のAHを保存し、HDCコマンドを選択する。
VA2:76B5〜76C3ではREAD=08H、WRITE=0AH、DMAの方向を選ぶ。
CHS入力を相対アドレスへ換算するか、BH bit7に従って入力相対アドレスを使う。

完了ステータスをCOMPSTATへ保存し、エラー時はREQUEST SENSEからAHを得る。
バッファ範囲は検査されるが、起動後のバンク領域への転送にはPC-Engineの
ラッパーも関与する。すべての境界での実データ転送は未検証。

### 03H FORMAT TRACK / 04H FORMAT DRIVE

文書仕様ではAL=01H〜10Hのインターリーブ、データ部はE5H。
破壊的操作なので実行していない。対応入口とHDCコマンド生成経路を確認したが、
全ドライブの反復条件、代替領域の扱い、実際の埋め込み値は追加追跡が必要。

### 05H RECALIBRATE / 06H RETRACT

それぞれ較正・退避の実装入口を持つ。
RETRACTはパラメータテーブルの別部分と退避アドレスを参照する。
公開ワーク外の `0040:037F` にはドライブ別ビットの状態があり、
VA2:7BA5〜7BE4で検査・クリア・セットされる。名称と全寿命は未確定。

### 07H ASSIGN ALTERNATE TRACK

名前から「ここで媒体上に代替先を登録する」と解釈しない。
文書仕様・ROMとも、まずES:BPの4バイトバッファにアドレスを整形する処理。
VA2:7CA4〜7CB0の格納順は次のとおり。

```text
ES:[BP+0] = 相対アドレスの上位バイト（DL）
ES:[BP+1] = 中位バイト（AH）
ES:[BP+2] = 下位バイト（AL）
ES:[BP+3] = 00H
```

通常のlittle-endian DWORDとして読み出すと別の数値になる。
BH bit7=0ならトラック先頭（セクタ0）の相対アドレスを算出する。
BH bit7=1なら入力の相対アドレスを使う。バッファオフセットの加算検査もある。

### 08H FORMAT BAD TRACK

文書仕様では07Hで用意した同じES:BPのバッファを使用し、媒体上の不良トラックへ
代替先を割り付ける。ROMはバッファをDMAに設定する経路を持つ。
危険な媒体更新なので実行せず、代替先データの消費と全失敗経路は未検証。

### 09H SENSE

文書仕様はAL=0/1/2/3で5/10/20/40MBを通知。
VA2:7D98〜7DBBはドライブ指定を検査し、状態取得後に
`0040:0379/037A` のタイプをALへ読み込む。
CF/AHとタイプの組を解釈すること。未接続・未知設定ではFFHもあり得る。

### 0AH GET BOOK — マニュアルと返却レジスタが異なる

テクマニ・BNNの記載は **ES:BP**。
しかし、確認したVA/VA2 ROMの実装は次の命令である。

```text
VA bank3:989D / VA2 bank5:7DBC
push ds
pop es
mov dx,0370H
mov ah,00H
clc
ret
```

ディスパッチのDS=0040Hを合わせると **ES:DX=0040:0370**。
DXはこの呼び出し経路の戻り値として残り、BPはディスパッチで保存・復元される。
よってROM上のこの処理をES:BP返却と説明することはできない。
文書の誤記か、別版BIOSの契約かまでは断定しない。
起動後の実際のベクタ・返却値は未実測。

## 4. Conventional memoryのワークエリア

**ROM確認: SYS=0040:0370、物理00770H。**

```text
0040H × 10H + 0370H = 00770H
```

GET BOOKはBIOSワークの参照先を返す機能であり、空きRAMの取得機能ではない。
以下は文書のフィールド名とROMの具体的な参照位置を合わせた表。

| SYS相対 | 0040内offset | 物理 | バイト数 | 名前 / 用途 |
| --- | --- | --- | --- | --- |
| +00H | 0370H | 00770H | 1 | COMPSTAT: HDCコマンド |
| +01H | 0371H | 00771H | 1 | COMPSTAT+1: 完了ステータス |
| +02H | 0372H | 00772H | 1 | SCOMPSTAT: REQUEST SENSEの完了ステータス |
| +03H | 0373H | 00773H | 4 | SENSESTAT: センス結果 |
| +07H | 0377H | 00777H | 1 | HDINTFLG |
| +08H | 0378H | 00778H | 1 | EQUIPDSK |
| +09H | 0379H | 00779H | 1 | DSKTYPE#0 |
| +0AH | 037AH | 0077AH | 1 | DSKTYPE#1 |
| +0BH | 037BH | 0077BH | 2 | DSKPRMADR#0 |
| +0DH | 037DH | 0077DH | 2 | DSKPRMADR#1 |

DSKPRMADRは**16bit little-endianのバンク内オフセット**。
VA2:7658/768BでBXをwordとして保存し、7A51/7A56でwordとして取り出す。
対応バンクを選択したCS経由でROMテーブルを参照する。
RAMへのfarポインタや1バイトのタイプ値ではない。

HDINTFLG bit0は割り込みと待ちルーチンの共有状態。
VA2:7F91〜7FABでポート82Hを読み、条件成立時にbit0をセットする。
7EA1〜7EB8ではbit0を待ってクリアし、DMAを停止する。
この待ち経路には単純な有限回数カウンタが見えず、割り込みの動作が前提となる。
ユーザ側から勝手に書き換えないこと。

EQUIPDSKのbit0/bit1はドライブ0/1の接続状態。
DSKTYPEの0/1/2/3は5/10/20/40MB、未知/失敗経路の値はFFH。
ステータス領域はコマンドで更新されるので、診断時には終了直後の状態を読む。

### 公開表外の内部ワークと世代差

| 0040内offset | 物理 | 解析上の用途（正式変数名ではない） |
| --- | --- | --- |
| 037FH | 0077FH | ドライブ別の内部状態ビット |
| 0380H〜0385H | 00780H | HDCの6バイトコマンド組み立て領域 |
| 0386H | 00786H | 元のAH、リトライbitを含む |
| 0387H | 00787H | 転送セクタ数 |
| 0389H | 00789H | BH相当のドライブ/アドレス方式 |
| 038AH〜038BH | 0078AH | 入力CX |
| 038CH | 0078CH | 入力ヘッド |
| 038DH | 0078DH | 入力セクタ/相対アドレス上位 |
| 038EH | 0078EH | バッファ物理アドレス上位 |
| 038FH〜0390H | 0078FH | バッファ物理アドレス下位16bit |
| 0391H | 00791H | 戻りAHの一時保存 |

初期化フックの世代差もROMから確認した。

- VA: `0040:0392` からfarポインタ1個をbank3:9009へ設定。
- VA2: `0040:0392=FFH`、`0393`からfarポインタ7個をbank5:7409へ設定。
- 9009/7409はRETFのみの既定フック。VA2では7694、7833、7C5A、7CC3などから呼ぶ。
- VA2の7個のfarポインタは `0393〜03AE`（物理00793H〜007AEH）。

VAの0392とVA2の0392は同じ種類のフィールドではない。
公開ワーク表の終端をメモリ確保の終端とみなさず、未掲載領域も空きRAMとして使わない。

## 5. ROMのディスクパラメータ

各タイプは32バイトのレコード。VAの9ABA、VA2の7FDAから配置される。
標準4レコードと未知設定向け第5レコードの計160バイトは両ROMで一致した。

| レコード相対 | 長さ | 内容 / エンディアン |
| --- | --- | --- |
| +00H | 10 | 通常アクセス用HDCパラメータ |
| +0AH | 10 | RETRACT用HDCパラメータ |
| +14H | 2 | BIOS最大シリンダ、little-endian |
| +16H | 1 | BIOS最大ヘッド番号 |
| +17H | 1 | 最大セクタ番号 |
| +18H | 1 | BIOS換算用ヘッド数 |
| +19H | 3 | 退避時相対セクタアドレス、上位バイト先頭 |
| +1CH | 3 | シリンダ5へ移動するためのアドレス、上位バイト先頭 |
| +1FH | 1 | 末尾領域。用途は未確定 |

| タイプ | VA offset | VA2 offset | 最大CYL | 最大HD | 最大SEC | ヘッド数 | 退避アドレス | 移動用アドレス |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 5MB | 9ABA | 7FDA | 152 | 3 | 32 | 4 | 22044 | 660 |
| 10MB | 9ADA | 7FFA | 309 | 3 | 32 | 4 | 44880 | 660 |
| 20MB | 9AFA | 801A | 307 | 7 | 32 | 8 | 87648 | 660 |
| 40MB | 9B1A | 803A | 614 | 7 | 32 | 8 | 175296 | 1320 |
| 未知設定用 | 9B3A | 805A | FFFFH | FFH | FFH | 4 | 44880 | 660 |

未知設定用レコードを「大容量HDD対応機能」と解釈しないこと。
タイプ値自体はFFHとなる経路に使われ、公開された第5媒体タイプではない。

テクマニの換算用ヘッド数Fの表は20/40MBでも4と読めるが、ROMは**8**。
20MBのHDCパラメータは最大ヘッド3、最大シリンダ614なのに対し、
BIOSの入力検査/換算値は最大ヘッド7、最大シリンダ307。
異なる欄の値を混ぜないこと。HDC内部の物理配置との対応は追加追跡が必要。

同じテクマニでは退避側HDC最大シリンダが664と読める箇所もある。
両ROMの20/40MBレコードは `02H,98H`、すなわち**664**を格納している。
文書との一致/相違は欄ごとに判定し、表の見た目だけで誤記を確定しない。

## 6. 終了ステータスと文書との差

文書仕様は正常 `AH=00H, CF=0`、エラー `CF=1`。
代表的なエラー番号は次のとおり。

| AH | 文書上の意味 |
| --- | --- |
| 02H | パラメータエラー |
| 03H | デバイス異常 |
| 04H | 非レディ |
| 05H | 書き込み不可 |
| 06H / 07H | ID / データCRC |
| 08H | IDR不可、指定セクタなし |
| 09H / 0AH | ノーシリンダ / 不良シリンダ |
| 0BH / 0CH | ID / データアドレスマークなし |
| 10H | 代替トラックが読めない |
| 11H | 代替トラックへ直接アクセス |
| 12H | シークエラー |
| FEH | タイムアウト |
| FFH | 不定エラー |

**ROM確認**: VA2:7EB9〜7F32はREQUEST SENSEをDMAで0040:0373へ取得し、
センス先頭の下位6bitを表（807A）へ渡してAHを得る。
HDCの完了バイトとBIOSのAHは同じ値ではない。

通信待ちの失敗には `AH=0DH` を返すROM経路も存在する
（例: VA2:7A3E〜7A45）。これは上の文書一覧にはない。
ディスパッチはその値を検出して回復処理を呼ぶ。
FEH記載と0DH実装を同一の全経路契約とみなすことはできない。
全ステータス変換表・全タイムアウト経路の網羅は未完了。

## 7. PC-Engine 1.1のVA向け差し替え

**ファイル確認**: 初期化コードfile:C56E付近はDS=0にし、
`0000:0204=5519H`、`0206=CS` を設定する。
0204Hは81H×4なので、INT 81Hをディスク側ラッパーへ置換している。
これらは抽出システムコードのオフセットで、固定の物理RAMアドレスではない。

ラッパーfile:5519〜5550はポート0152Hのバンク指定を保存してbank3を選び、
AH!=0の通常経路でE000:9003を呼び、復帰時にバンクを戻してIRETする。
AH=0にはfile:5551から別の初期化処理がある。
ROMのINITIALIZEだけを読んで起動後の動作全体としないこと。

READ/WRITEにはfile:5755〜57C7でバッファ範囲を調べる分岐があり、
A0000H〜DFFFFHに関わる場合の特別処理と読める。
file:5706からROM内部READ/WRITEを組み合わせ、一時的にフック/割り込みを置換する。
境界での実データ転送と完全な用途は未実測。

このVA向け特殊経路は `0040:0396〜03A1` に保存用ワークを使う
（file:57DB〜57FC、SYS+26H〜31H）。
VA2のROMフック表と同じ絶対位置が含まれるので、VA用コードの配置を
そのままVA2に適用してはならない。
VA2でのINT 81Hの最終ベクタ、ディスク側差し替えと追加ワーク範囲は未確定。

## 8. 結論・次の確認

確認した主要事項:

- 11機能のROM分岐表があり、READ/WRITEは入口共有。
- Conventional memoryの公開ワーク先頭はROM上で**0040:0370、物理00770H**。
- GET BOOKは文書のES:BPに対し、両ROMの実装が**ES:DX**。
- DSKPRMADRは2バイトのバンク内参照。内部ワーク/フックには世代差がある。
- ROMパラメータの20/40MB換算用ヘッド数は8。
- 文書にないAH=0DHの失敗経路と、VAのPC-EngineによるIVT置換がある。

未確認:

- 起動後のVA/VA2 IVTとGET BOOKの戻り値。
- 全機能の保存/破壊レジスタと全エラー終了経路。
- 内部ワーク全体の確保範囲・寿命、HDC側リトライ契約。
- 不良トラック処理、特殊バンク転送、実媒体/実機での一致。

次はIVTと公開ワークを読み取り、GET BOOKのES/DX/BPを測定する。
WRITE/FORMAT/BAD TRACKは実行せず、資料に記載された内部入口を直接CALLしない。
公開資料は独立した解析結果のみ。追試には適法に用意した対応ROM・資料が必要。
