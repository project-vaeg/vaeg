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

# PC-88VA BIOS解析資料 — INT 82H / キーボードBIOS

対象: VA / VA2 ROM、PC-Engine 1.1、テクマニ6.3。
静的解析の第1版。起動後のIVT・RAM・キー入力は未実測。

## 0. 配置・名前・確認の区分

テクマニの `603KEYB.TXT` に相当する独立した解析資料。
正式な配置は **`docs/bios/bios_int82h.md`**。
603は章番号なので、呼び出し割り込み番号82Hをファイル名に使う。
TXT版が必要なら `bios_int82h.txt`。AH別機能は同じ資料にまとめる。
関連: [FDD BIOS](bios_int80h.md)、[HDD BIOS](bios_int81h.md)。

- **文書仕様**: テクマニ記載の機能・契約。
- **ROM確認**: 両ROMの命令、入口表、RAM参照から確認した事項。
- **ファイル確認**: PC-Engineシステムコードで確認した事項。
- **未確認/推定**: 完全な呼び出し経路、版の違い、実行結果が未確定の事項。

本資料は独立した解析結果であり、マニュアルコピーやROM配布ではない。
私有入力のパス・媒体識別情報・ハッシュ、生の逆アセンブルはGit外に保持する。
BIOS総当たり、キュー・フックの書き換え、トラップ登録、印刷は行っていない。

## 1. ROM初期化と入口

文書仕様の機能番号はAH=00H〜10Hの17個。
このキーボード処理はFDD/HDDと異なり、**ROM1のF000側に実装されている**。
ROM0の日本語フロントエンド処理をキーボードBIOS本体と混同しない。

| 対象 | ROM1初期化のIVT書き込み | INT 82H入口 | AH分岐表 |
| --- | --- | --- | --- |
| VA | F000:6923 | F000:6D8A | F000:6DC8 |
| VA2 | F000:72B3 | F000:7736 | F000:7775 |

初期化ルーチンはDS=0000HにしてIVTの0208H/020AHへ入口/CSを書き込む。
0208H=82H×4。この登録は他のBIOS用の一括登録表とは別のコードにある。
一括表に82Hがないことは「キーボードBIOSがない」ことを意味しない。

**ROM確認**:

- AHを10Hと比較し、それより大きいとそのままIRETする。
- HDD/FDDと異なり、AH bit7をマスクしてリトライ指定にはしない。
- 処理中はDS=0040H、`0040:0068` bit4を実行中フラグとして使う。
- このbitが既にセットされている場合、通常ディスパッチを実行しない経路がある。
- PUSHA等で保存したレジスタへの書き戻しにより、AX/BX/ES/DX等の戻り値を渡す。
- CFも割り込みフレームへ反映する。CF=1は機能によって「データなし」などを表す。

別にRETF型の内部入口があり、同じ機能表を使うがAH上限は0FH。
公開INT 82Hと同じ再入・フラグ契約だとみなして直接CALLしないこと。

## 2. 17機能の入口と契約

入口はROM1内オフセット。名称・基本契約は文書仕様、入口はROM確認。

| AH | 機能 | VA | VA2 | 主な入力 / 出力 |
| --- | --- | --- | --- | --- |
| 00H | GetChar | 6E0B | 77B8 | 一文字入力、AX=文字、BX=キーコード等 |
| 01H | SnsChar | 6E18 | 77CF | 非破壊検出、CF=1なら文字なし |
| 02H | ExpCtrl | 6F70 | 792B | AL=0拡張オン、1オフ |
| 03H | DefSft | 6F78 | 7934 | CL=ソフトキー番号、文字列を定義 |
| 04H | KbFlsh | 7096 | 79F6 | KB/JFPの入力状態をフラッシュ |
| 05H | IntKb | 70B8 | 7A18 | ソフトキー・制御状態等の初期化 |
| 06H | AttTrap | 70F2 | 7A50 | ES:DX=入口、AL=登録番号/FFH |
| 07H | DetTrap | 7143 | 7AA1 | AL=登録番号、AL=0/FFH |
| 08H | UsrTrap | 71AC | 7B0A | AL=0削除、非0登録、ES:DX=入口 |
| 09H | PrmGetK | 71C4 | 7B1B | 破壊読み出し、AH=scan、AL=内部コード |
| 0AH | PrmSnsK | 71FE | 7B55 | 非破壊検出、CF=1ならキーなし |
| 0BH | PrmSfts | 7235 | 7B8C | AL=シフト状態 |
| 0CH | PrmFlsh | 723C | 7B93 | プリミティブ入力キューをフラッシュ |
| 0DH | KyGroup | 7244 | 7B9B | AL=グループ、AH=押下ビット |
| 0EH | KyBook | 7257 | 7BAC | ES:DX=ワーク先頭 |
| 0FH | RepCtl | 7266 | 7BBB | AL=0オートリピートオン、非0オフ |
| 10H | CpyCtl | 727F | 7BD4 | AL=FFHコピーオフ、0〜7モード |

### 主機能00H/01Hとプリミティブ09H/0AH

文書では00H/01HはJFP（日本語フロントエンド）を通り、ソフトキー文字列を展開する。
09H/0AHは日本語変換しない低レベル入力。
この2種類のAXを同じ形式とみなさないこと。

主機能の文書上の戻り値:

- AX: 2バイト文字コード。ソフトキー拡張オフ時は0となる場合がある。
- BX: BH=スキャンコード、BL=内部コード。
- JFP変換文字や展開文字列ではBX=FFFFHとなる場合がある。
- ユーザ定義キーではAX=00FFH、BX=FF01H〜FFFEH。

00H/09Hは入力を待つため、キーなし状態で気軽に呼ぶと停止待ちになる。
01H/0AHは検出で、キューの読み出し位置を進めない。
全変換状態・全戻り値と実際の文字列展開は動的未検証。

**ROM確認**: VA2の00H入口は77B8からフックと内部入力処理へ進み、
01H入口は77CF。79F6ではJFP参照先0040:088Eを介した呼び出しがある。
09Hは7B1Bからプリミティブキューを読み、データがなければINT 96Hを介した待ちに入る。
キューを2バイト読む経路で内部コードとスキャンコードをAXへ組み立てる。

### 02H ExpCtrl

両ROMはAL=0/1のときだけ0040:0839へ格納する。
VA2は792B〜7933で入力範囲検査後にCFを反転し、有効入力でCF=0となる。
VAは比較と格納の命令列が異なるので、文書にないCF契約を版間で統一しない。

### 03H DefSft — 入力ポインタの誤記候補

原文の入力は `ES:DS` と記載されているが、ROMは**ES:DX**を入力文字列に使う。
VA2:798B〜7990でESを入力DSへ移し、DXをSIへ移してLODSBで読む。
セグメントレジスタDSを文字列オフセットとして使う処理ではない。
文書誤記か版の違いかは未確定だが、確認ROMのABIはES:DX。

CL=0は全ソフトキー、01H〜1FHは個別。
CL>1FHはCF=1で除外する。ROMは文字列格納枠を16/6バイトで扱う。
文書の最大15/5文字は終端NULを含まない表現であり、全角文字は複数バイトを使う。
最大長を「常に15/5個の全角文字」と解釈しない。

### 04H KbFlsh / 05H IntKb

フラッシュと初期化は同じではない。
VA2の05HはROMの既定ソフトキー列を0040:06B1へ0182Hバイトコピーし、
0040:0838/0839を0、083Eをwordで0900Hに設定し、ポート0197Hへ4DHを出力する。
これらはユーザトラップ削除・拡張オン・既定コピー/リピート状態に関わる。
ポップアップ登録全体を05Hが必ず初期化するとは、この処理だけから断定しない。

### 06H〜08H トラップ制御

文書仕様はポップアップ最大8個、ユーザトラップ1個。
ポップアップは最後に登録したものが先、ユーザトラップは最低優先順位。
登録番号と優先順位は別概念。

ROMは番号使用ビット0040:0861、入口表0862、順序表0882を参照する。
06HはES:DXを入口表へ保存し、ALで管理番号を返す。
07Hは番号と使用状態を検査し、表を移動して削除する。
08Hは0838へALを保存し、非0なら0834/0836へDX/ESを保存する。

トラップ呼び出し契約は文書上、AH=scan、AL=内部コード、CL=shift状態。
DL初期値2、戻りDL bit0=フラッシュ、bit1=キュー入力、bit2=後続をスキップ。
割り込み中/CLIで実行されるとされる。全呼び出し経路の保存レジスタや
実際の優先順位動作は未検証なので、トラップ登録は安全な読み取りプローブではない。

### 0BH シフト状態

ROMは0040:06B0をALの戻り位置へコピーする。
文書上のbit0〜5は左Shift、右Shift、CAPS、カナ、GRPH、CTRL。
bit6/7は予約。全押下組み合わせの実測は未実施。

### 0DH KyGroup — 原文の上限と異なる

原文はAL=0〜FH。両ROMは**AL<=0EH**を検査している。
その場合、AL番号のI/Oポートを読み、反転した値をAHの戻り位置に格納する。
AL=0FHではその読み取り処理を行わない。
「グループFも正常な押下情報が返る」と原文だけから判断しないこと。

### 0EH KyBook

VA:7257、VA2:7BACはES=0040H、DX=06B0Hにし、
保存した呼び出し側のES/DXスロットへ書き戻す。
**ES:DX=0040:06B0、物理00AB0H**がROM上の返却値。

原文のセグメント00ABH、オフセット0000Hと同じ物理位置であり、配置矛盾ではない。
セグメント値だけを比較して別のワークだと判断しない。

### 0FH RepCtl / 10H CpyCtl

0FHは0040:0063の制御値を読み、AL=0ならbit2をセット、非0ならクリアし、
ポート0197Hへ出力する。ワーク先頭付近のフラグだけで制御が完結するのではない。

10HはAL=FFHならその値を保ち、それ以外はAL&07Hとし、08HをORして
**0040:083F**へ1バイト格納する。既定AL=1なら内部値は09H。
原文のCOPY関連フィールド表とは位置・幅に相違がある（次節）。
モードbit0/1/2の文書上の意味は白黒反転/左右反転/白黒モード。
実際の画面コピー出力と中断動作は未検証。

## 3. Conventional memory上の公開ワーク

**先頭: 0040:06B0 = 00AB:0000 = 物理00AB0H。**

```text
0040H × 10H + 06B0H = 00AB0H
00ABH × 10H + 0000H = 00AB0H
```

表の名称は原文を基礎にし、相違箇所はROMのアクセス幅・用途を優先して明示する。
「予約/未掲載」は空きRAMの意味ではない。

| SYS相対 | 0040内offset | 物理 | バイト数 | 内容 |
| --- | --- | --- | --- | --- |
| +000H | 06B0H | 00AB0H | 1 | stabl: シフト状態 |
| +001H | 06B1H | 00AB1H | 386 | sftky: 20×16 + 11×6バイト |
| +183H | 0833H | 00C33H | 1 | 表の未掲載領域。用途未確定 |
| +184H | 0834H | 00C34H | 4 | TrapAdd: ユーザトラップfarポインタ |
| +188H | 0838H | 00C38H | 1 | UsrTb: ユーザトラップ有効指定 |
| +189H | 0839H | 00C39H | 1 | Exp_T: 0拡張オン、1オフ |
| +18AH | 083AH | 00C3AH | 2 | Soft_P: 展開中文字列ポインタ、FFFFHは未選択 |
| +18CH | 083CH | 00C3CH | 2 | SfKey: キーコード一時保存 |
| +18EH | 083EH | 00C3EH | 1 | COPY要求に関係するフラグ（ROM参照） |
| +18FH | 083FH | 00C3FH | 1 | COPY制御モード（ROMの10H格納先） |
| +190H | 0840H | 00C40H | 1 | 主機能の入力/検出状態（ROM参照） |
| +191H | 0841H | 00C41H | 32 | Buff: プリミティブキーコードのバイトキュー |
| +1B1H | 0861H | 00C61H | 1 | PnuBB: ポップアップ番号使用ビット |
| +1B2H | 0862H | 00C62H | 32 | Pup_Adr: 8個のfarポインタ |
| +1D2H | 0882H | 00C82H | 8 | PnuB: ポップアップ管理番号/順序 |
| +1DAH | 088AH | 00C8AH | 4 | KbAddr: KB側処理参照 |
| +1DEH | 088EH | 00C8EH | 4 | JFPAddr: JFP側処理参照 |
| +1E2H | 0892H | 00C92H | 4 | KbHook0 |
| +1E6H | 0896H | 00C96H | 4 | KbHook1 |
| +1EAH | 089AH | 00C9AH | 4 | KbHook2 |
| +1EEH | 089EH | 00C9EH | 4 | KbHook3 |
| +1F2H | 08A2H | 00CA2H | 4 | KbHook4 |
| +1F6H | 08A6H | 00CA6H | 1 | Copy_S: コピー停止関係（全参照未追跡） |
| +1F7H | 08A7H | 00CA7H | 1 | IO_152: 原文上はバンクI/Oコピー（用途の全照合未完了） |

### COPY関連の文書表をそのまま使わない

原文は+18EHのCopy_Fをword、+190HのCopy_Iもwordと記載する。
後者は+191HからのBuffと重なるため、表自体に幅の矛盾がある。

ROMの05Hは083Eへ0900Hをwordで書くが、10Hは083Fへbyteで書く。
09H/0AHも083Eをbyteで参照する。
0840は00H/01H処理が0/1にして入力/検出動作を区別する。
従って「Copy_Iを0040:0840のwordとして書く」という使い方は裏付けられない。

表の誤記/別版の可能性を残し、ROM実測前に直接ワークを操作しないこと。

## 4. キュー管理情報とその他のRAM参照

公開の32バイトBuffとは別に、一般キュー管理処理が管理情報を持つ。
VA2:7EE0以降の処理はCHでキューを選び、
**0040:0AC0 + 0AH×CH**（1レコード10バイト）を管理レコード先頭とする。
CH=0がプリミティブキュー。これは公開ワーク先頭からの相対0ではない。

| 管理レコード相対 | CH=0の0040内offset | 物理 | バイト数 | ROMから読める役割 |
| --- | --- | --- | --- | --- |
| +00H | 0AC0H | 00EC0H | 2 | バッファのセグメント |
| +02H | 0AC2H | 00EC2H | 2 | バッファ開始オフセット |
| +04H | 0AC4H | 00EC4H | 2 | 容量（バイト） |
| +06H | 0AC6H | 00EC6H | 2 | 読み出し位置 |
| +08H | 0AC8H | 00EC8H | 2 | 使用バイト数 |

ROM初期化はES=0040H、BP=0841H、DX=0020H、CH=0で管理処理を呼ぶ。
管理初期化はES/開始/容量/読み出し位置を設定し、使用数を0にする。
読み出し・書き込みはバイト単位のリングキューで、位置を容量に従って折り返す。
通常のscan/internalペアが2バイトなら32バイトは16組に相当するが、
全割り込み・ユーザ定義コード経路での実効容量保証は未検証。

そのほか0040:0062/0063はハード制御値のコピー、0068は実行中フラグとして参照される。
公開表終端の次、0040:08A8を参照する割り込みコードもある。
KyBookの返却は全確保範囲を示す契約ではなく、+1F7Hまでが使用RAMのすべてではない。

## 5. PC-Engine 1.1による差し替え

ROMの入口だけで起動後のINT 82Hを説明しないこと。
PC-Engineシステムコードには以下のIVT変更がある（fileは抽出ファイル内オフセット）。

- file:C35Fで0000:0208へ2D02H、020AへCSを書き込む。
- file:C384では別の経路で0208へ38DFH、020AへCSを書き込む。
- file:C37AではINT 09H側の入口も36DAHへ変更する。
- file:C842〜C85DではINT 21H/AH=35Hで旧82Hを保存し、
  AH=25Hで82Hの入口をオフセット0D44Hへ置換する。

置換箇所の存在は確認したが、各初期化条件・版別の適用順序と
最終IVTの実測は未完了。これらをすべて同時に有効な固定物理アドレスと扱わない。
システムコード内には0040:06B0のシフト状態を参照する処理もある。

## 6. 相違・確認済み事項・残る調査

主要な確認結果:

- ROM1側に17機能の分岐表があり、INT 82Hは個別の初期化コードで登録される。
- KyBookは0040:06B0を返す。原文の00AB:0000と同じ物理00AB0H。
- DefSftの入力文字列は、原文のES:DSに対しROMがES:DX。
- KyGroupは原文0〜FHに対しROMの読み取り範囲が0〜0EH。
- COPYフィールドの位置/幅は原文とROMで相違し、原文のword指定はBuffと重なる。
- キュー管理レコードは別の物理00EC0Hから配置される。
- PC-Engineには複数のキーボード割り込み差し替え経路がある。

未確認:

- 起動後の最終IVT、KyBookの戻り値、RAM初期値。
- JFPの全状態遷移、全文字コード対応、ソフトキー展開の境界。
- トラップの実際の呼び出し順序・保存レジスタ・再入時動作。
- COPYとSTOPの実行結果、リピートの実時間条件。
- 公開表以外の内部RAMの完全な確保範囲と寿命。

次の確認はIVT・KyBook・キュー管理情報の読み取りから行う。
待ち関数00H/09H、トラップ登録、フラッシュ/初期化、コピーは
状態変更を伴うため、読み取り専用の確認と混在させない。

逆アセンブルには注意が必要。ROMのNEC拡張0FH命令を一般の逆アセンブラが
SSE等として表示する箇所がある。線形出力の命令名をそのまま根拠にせず、
対応CPUの命令体系・バイト列と分岐経路を確認する。
