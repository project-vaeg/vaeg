<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# PC-88VA SCSIサポートディスク

Language: [English](scsi-support.md) | [日本語](scsi-support.ja.md)

この文書は、原ソフトウェアの文書とPC88.gr.jpフォーラムに記載された
PC-88VA SCSI設定を独自にまとめたものです。フォーラム記事やパッケージの
マニュアルを転載するものではありません。リポジトリにはPC-Engine、PCPLUS、
SCHD、VBUFF、SCFORMのバイナリを収録・再配布していません。

## 保存対象の範囲

VAEGはPCPLUS/SCHDソフトウェア経路が必要とする、PC-9801-55互換SCSIの
制御・データインターフェースをエミュレートします。ボードのファームウェア
ROMは既定で意図的に切り離されています。これは、PC-88VAではボードROMを
切り離しても動作に影響しないとする `SCSI55.TXT` の記述に従ったものです。
したがってVAEGは、ボードROMの窓がVAゲストのメモリマップに含まれるとは
主張しません。

目的は保存です。公開パッケージの場所と正確なダウンロード識別情報を記録し、
公開ダウンロードまたは検証済みローカルキャッシュが利用できる間にソフトウェア
を再現可能な方法で組み立てる手順を残します。生成ディスクには、実PC-88VA
ハードウェアまたは将来のSCSI対応実装で使えるよう、原パッケージのマニュアル
もまとめて収録されます。

## MOのサポート状態

VAEGは光磁気ディスク（MO）その他のリムーバブルSCSI媒体を**サポートして
いません**。SCHD、VA128MO、STESTのパッケージは歴史資料として開発ディスク
に残せますが、VAEGがサポートするSCSI経路は固定ディスクだけです。
`mo-128mb` と `mo-160mb` のビルダープロファイルは参考ファイルを配置する
だけであり、VAEGでMO媒体が使えることを意味せず、サポート表明でもありません。
以下のMO用コマンドは、VAEGの外で別途検証された実機に限って適用します。

## 現行VAEGの設定（Rel.260805）

以下は現行VAEGリリースでサポートされる利用手順です。後半のレジスタ・
パッケージの説明は保存用の証拠として残しているもので、VAEGでディスクを
接続するだけなら必要ありません。

### 1. サポートディスクを準備する

VAEGはPC-Engine ROM、`PCPLUS.SYS`、`SCHD.SYS`、`SCFORM.COM` その他の
プロプライエタリなゲストファイルを再配布しません。適法に入手したPC-Engine
1.1システムD88を用意し、使い捨てのSCSIサポートディスクを作成します。

```sh
sudo apt-get install curl dosbox lhasa python3 coreutils

tools/pc88va/scsi-support.sh \
  --source /path/to/pcengine-1.1.d88 \
  --output /path/to/pcengine-scsi.d88
```

コマンドはチェックサム検証済みのサポートパッケージをダウンロードまたは
再利用し、文書化された手順に従ってPCPLUSへパッチを適用し、新しいD88を
書き出します。元のD88は変更しません。`--scsi-id 0` から `--scsi-id 7`
で `CONFIG.SYS` に記録する既定ターゲットIDを選べます。

2台のSCSIターゲットを使う場合は、起動前に生成ディスクの `CONFIG.SYS` へ
両方のドライバ行を追加します。

```dos
DEVICE = A:\PCPLUS.SYS
DEVICE = A:\SCHD.SYS -I0
DEVICE = A:\SCHD.SYS -I1
```

`-I0` と `-I1` はSCSIターゲットID 0と1を選びます。対応するターゲットIDの
範囲は0から7です。登録したいターゲットごとにドライバ行が必要です。
`SCFORM` を実行すると選択したターゲットを初期化するため、事前にバックアップ
を取ってください。

### 2. VAEGのSCSIイメージを作成して接続する

GUIはVHD形式のSCSIイメージを作成します。**HardDisk -> New SCSI image...**
を選び、サイズを指定し、**SCSI ID 0** から **SCSI ID 6** のいずれかへ割り
当てます。通常最初に使うのはSCSI ID 0と1です。既存イメージは対応する
**Open**メニューから接続できます。接続を変更した後は、PCPLUS/SCHDが再列挙
するようゲストをリセットしてください。

同じ操作はシェルからも準備できます。

```sh
# 既存イメージ。オプションには実際のSCSIターゲットID 0〜6を指定する。
./vaeg --scsi0 /path/to/scsi-id0.hdi \
       --scsi1 /path/to/scsi-id1.hdi \
       --scsi2 none --scsi3 none \
       --scsi4 none --scsi5 none --scsi6 none
```

VAEGのソースチェックアウトからは、空のイメージも次で作成できます。

```sh
python3 tools/create_vaeg_scsi_hdd.py \
  --output /path/to/scsi-id0.hdd --size-mib 40 --executable ./vaeg
```

イメージ作成ツールは既存ファイルを上書きしません。SCSIイメージは拡張子
`.hdd`、物理ブロック256バイトです。SASIイメージは別形式の `.hdi` であり、
SASI用の手順で作成する必要があります。

### 3. 固定ディスクターゲットをゲストでフォーマットする

生成したサポートD88をフロッピードライブとして起動します。PCPLUSがSCHDより
先にロードされ、対象とする各SCSI IDにドライブレターが割り当てられたことを
確認してください。`SCFORM` は使い捨てまたはバックアップ済みのターゲットに
だけ実行します。

```dos
SCFORM
```

2048バイト論理セクタが必要なパーティションでは、まず起動ディスクのバッファ
を変更して再起動します。

```dos
VBUFF -D1 -B11
SCFORM /S
```

`VBUFF -D1` はドライブAを選びます。`/S` はSCFORMのオプションで論理セクタ
サイズの選択肢を拡張するものです。SCHDのジオメトリ上書きオプション `-S` とは
無関係です。生成したフロッピーを起動媒体として残してください。この手順で
SCSIターゲットを起動可能にはしません。

フォーマット後に一度再起動して新しいドライブを `DIR` で確認します。重要な
データを置く前に、小さなテストファイルを作成、読み出し、削除し、もう一度
再起動してください。VAEGの自動G75チェックは、1ターゲットおよびターゲット
ID 0/1についてこのライフサイクルを検証します。

### 4. トラブルシューティング

- `SCFORM` には2台目のディスクが表示されるのにDOSに現れない場合、通常は
  `CONFIG.SYS` にそのターゲットの `SCHD.SYS -I<n>` 行がありません。
- イメージを追加、削除、フォーマットした後はリセットしてください。リセット
  によりゲストのSCSI/SxSI登録経路が再構築されます。
- `-I` オプションに設定したSCSI IDと同じIDを使ってください。VAEGは誤った
  イニシエータIDを暗黙に再割り当てしません。
- サポートD88とROMはVAEGのリリースアーカイブの外に置いてください。サポート
  ディスクビルダーとリリース実行ファイルは、それらのゲスト資産とは別です。

MO媒体はこの手順の対象外です。残されているMOパッケージや `mo-*` ビルダー
プロファイルを、VAEGのエミュレータ対応と解釈しないでください。

## ソースとソフトウェア

主要な設定資料はPC88.gr.jpフォーラム記事
[PC-88VAにSCSIハードディスクを接続する][forum-501]です。必要な公開
パッケージは次のとおりです。

- [PCPLUS 1.08][pcplus]。他のソフトウェアが使うPC-9801-55互換SCSI BIOS
  サービス（`$SCSIBIOS`）を提供します。`PCPLUS.SYS` はソフトウェアSCSI BIOS
  層であり、VAEGがボードROMの窓をマッピングすることには依存しません。
- [PCPLUS 1.08の修正][pcplus-patch]。DMAマスク設定を修正します。
- [BDIFF/BUPDATE 1.28][bdiff]。この修正をDOSBox上のホストで再現可能に適用
  するためだけに使います。
- [SCHD 1.55T][schd]。原ハードウェア環境でSCSIハードディスクおよびMO媒体を
  DOSブロックデバイスとして扱うPC-Engine用ドライバです。VAEGがサポートする
  のは固定ディスク経路だけです。
- [VBUFF 1.02][vbuff]。PC-EngineシステムディスクのIPLに記録された最大論理
  セクタバッファサイズを変更します。
- [SCFORM 1.24][scform-topic]。フォーラム添付の `SCF124.LZH` として配布された
  対話式SCSI初期化・パーティション分割ユーティリティです。

ダウンロードした各アーカイブと、生成したパッチ適用済み `PCPLUS.SYS` は、
ディスク作成前に固定SHA-256値と照合します。

コマンドの詳細、ハードウェア制限、再配布条件についてはパッケージのマニュアル
を正とします。生成ディスクには関係する原マニュアルを `A:\DOC` に残します。

## ポートとSCSIBIOSの証拠

### WD33C93ホスト契約

PC-9801-55互換コントローラは、WD33C93ファミリの2段階ホストインターフェース
を使います。`0CC0h` がコントローラレジスタを選び、`0CC2h` が選択した
レジスタへアクセスします。`0CC0h` の読み出しは補助ステータスを返します。
PIOではDBRがデータレディのハンドシェイクで、CBSY、CIP、INTは別個のコントローラ
ステータスビットです。AR `19h` は固定DATA窓です。CDBレジスタ `03h`〜`0Eh`
は通常の連続レジスタであり、NEC拡張範囲 `30h`〜`35h` を通常の `00h`〜`1Fh`
ファイルへ畳み込んではいけません。

この境界の主要なレジスタ資料は
[WD33C93Aデータシートとアプリケーションノート][wd33c93]です。同資料の間接
アドレス規則は、Auxiliary Status、DATA、COMMANDをアドレス自動増加の対象外と
明記しています。Controlレジスタ表はDMAモード `000b` をポーリングI/Oと定義し、
各DATAアクセス前にホストがDBRを調べます。M75b2実装はこのPIO契約に従い、DMA
チャネルの動作を推測して実装していません。

低レベルSELECT経路では、ホストから見える完了シーケンスはイベント駆動である
ことが期待されます。

```text
11h SELECT complete -> 8Ah COMMAND request -> 89h/88h DATA request
-> 8Bh STATUS request -> 8Fh MESSAGE IN request -> 85h disconnect
```

これはレジスタ/割り込み契約です。物理的なREQ/ACKワイヤプロトコルはコントローラ
が処理するため、ゲストの別ポートとして公開する必要はありません。タイマーで
`8Ah` を注入することはターゲットのフェーズイベントと同等ではなく、許容される
修正ではありません。NP2の簡略実装は歴史的背景としてのみ有用で、WD33C93仕様
そのものではありません。

M75aには既定で無効な `--scsitrace` 診断があります。これは `0CC0h`〜`0CC6h`
へのすべてのアクセス、選択されたAR、データ、`CS:IP`、コントローラのフェーズと
ステータス、補助ステータス、SCSI IRQのアサートおよびEOIクリアを記録します。
生のトレースはローカル診断成果物であり、コミットしません。

付属の `SCSI55.TXT` はPC-88VAのボード設定を明記しています。標準ボードI/O
アドレスは `0CC0h`、`0CC2h`、`0CC4h` です。`0CC6h` は同資料だけでは独立に
文書化されていません。M75では継承した `0CC6h` バイトストリームハンドラを
コントローラのフェーズエンジンのデータ転送部分として残し、VA I/Oマップへ
登録しています。これは実装上の境界であり、`0CC6h` が `SCSI55.TXT` に別途
規定されているという主張ではありません。ゲストでの利用はPCPLUS/SCHD検証に
依存します。

付属の `SETDMA.ASM` は、`0CCh` が `$SCSIBIOS` サービスのソフトウェア割り込み
番号であり、`0CC6h` I/Oポートではないという重要な区別を示します。
`SETDMA.COM` は最初にDOS `INT 21h/AH=35h, AL=0CCh` を呼び、返されたハンドラの
`ES:000Ah` の6バイトを `PCPLUS` と比較します。PCPLUSが導入済みなら、次を呼びます。

```asm
MOV AX,82C0h
MOV BL,01h
INT 0CCh
```

これはSCSIBIOSにDMAモードを要求するものです。このユーティリティはDMAチャネル
を設定せず、`0CC6h` に直接アクセスもしません。したがって通常のPCPLUS動作は
プログラムI/O（PIO）であり、DMAはPCPLUSソフトウェアサービスから要求する任意
モードだと確認できます。VAの案内では拡張スロット用DMAチャネルは0と3だけで、
SASIや2TDと競合する可能性があるとされています。

したがってエミュレータでは、次の主張を分けて扱う必要があります。

- `0CC0h/0CC2h/0CC4h`: 文書化されたVAボード設定ポート。
- `INT 0CCh`: PCPLUSが提供するソフトウェアSCSIBIOS入口。
- `0CC6h`: 互換用に残したバイトストリーム。付属PCPLUS/SCHDトレースでは現行
  低レベル経路から使われていないことが確認される。
- DMA: 通常のPIO経路ではなく、任意のPCPLUSモード。

M75b2は `0CC4h <- 02h` をDMERリセットストローブとして記録します。TCIR、
TCMR、TCMS、DMESはハードウェア確認待ちのままで、未対応ストローブは転送状態を
変更せず警告を出します。`0CC0h` の読み出しではデバイス割り込みラッチをクリア
しません。ラッチされたSCSIステータスを消費するのはAR `17h` の読み出しだけで、
8259のEOIは別のPIC操作です。AR `32h`、`34h`、`35h` は、PCPLUS/SCHDまたは
ボード資料からNEC固有動作の証拠が得られるまで、未対応/オープンレジスタの読み
書きとして扱います。

レジスタの進行も契約の一部です。AR `17h` は通常の自動増加ステータスレジスタ
なので、ステータスを読むとARは次のCOMMAND書き込み用の `18h` になります。
AR `18h` と `19h` 自体は固定窓です。そのためAR `12h`〜`14h` は特別なアドレス
処理なしに3バイト転送カウントを受け取れます。未定義のAR `1Ah`〜`2Fh` は保持
して警告し、折り返しや推測レジスタは公開しません。VA IRQ要求はメモリバンク
レジスタのIRE1ビット（bit 2）でゲートされ、システムIRQゲートが閉じている間も
内部CSRラッチは保持されます。Auxiliary StatusのLCI（bit 6）とPE（bit 1）は
現在0/未モデルとして定義されています。

M75c1はSELECT完了とターゲットCOMMANDフェーズ要求を分離しました。まず `11h`
CSRを読み、その後別のサービスイベントとして `8Ah` を渡します。最初に観測した
ホスト転送カウントは `000006h` で、続いて `AR=18h <- 20h` でした。AR=19hのPIO
バイトポンプをM75c2で実装するまで、Transfer Infoはこの境界で意図的に保持します。

M75c2はホストが設定した24ビット転送カウントを受け入れ、DBRを使って固定AR `19h`
からバイトをポンプします。カウントが尽きるとCSR `1Ah` を出してコントローラを
COMMANDに残します。CDBをデコードしたり、後続のDATA/STATUS/MESSAGEフェーズを
生成したりはしません。M75c3はトレース専用の転送分類を追加し、観測された各カウント
がCOMMAND/AR=19h経路で消費されたか、従来の `0CC6h` 経路で消費されたかを記録します。
現在の証拠では、観測された `000024h` と `000008h` の転送は実装済みDATA INでは
なくCOMMAND/AR=19hに分類されます。

VAEG内蔵のソフトウェアSCSI BIOSヘルパーとC-Busフェーズエンジンは、内部的に別の
層です。Rel.260805は、SCSIイメージの作成、FATの可視性、ファイル作成、読み戻し、
削除、クローズ/再オープン後の永続性、ターゲットID 0/1の2台構成というG75ゲスト
ライフサイクルゲートを通過しています。この節の残りの低レベル注記は、保存された
コントローラ境界を記録し、新しいハードウェアやゲストドライバ互換性を調べるとき
に役立つものです。上の設定手順の前提条件ではありません。

## SCHDドライバの証拠

付属の `SCHD.SYS`、`SCHD.DOC`、`SCHD.LOG`、`SCHD.TXT` は、SCHDがPC-88VA、
PC-88VA2/3、PC-Engine用のDOSブロックデバイスドライバであることを示します。
`PCPLUS.SYS` は `SCHD.SYS` より先にロードする必要があり、その後SCHDがSCSI
ハードディスクまたはMO媒体をDOSブロックデバイスとして登録します。文書化された
`-I0` 〜 `-I7` はSCSIターゲットIDを選びます。`-C` と `-S` はジオメトリを上書きし、
`-B` は大きいセクタバッファを選び、`-X` はリムーバブル媒体ポリシーを変更します。
これらはゲストドライバのポリシーであり、エミュレータの追加I/Oポートではありません。

改訂履歴には、SCHDのSCSIBIOSインターフェースがドライバから分離されたこと、
パケット/アドレス転送がワードアクセスへ変更されたこと、過去の `REP MOVSW`/
`REP STOSW` 実装誤りが修正されたことが記録されています。付属 `SCHD.SYS` を
バイト単位で調べると `CD CC`（`INT 0CCh`）呼び出し箇所が5つあり、`CD 1Bh` は
ありません。また `MOV DX,0CC0h/0CC2h/0CC4h/0CC6h` のリテラル設定列もありません。
これはSCHDがPCPLUSのソフトウェアSCSIBIOS入口を呼び、別の直接 `0CC6h` VA契約を
確立しないという文書化された構成と一致します。このバイトスキャンは呼び出し境界
の証拠であり、プロプライエタリなドライバ全体の逆アセンブルの代用ではありません。

従ってVA SCSI実装では、まず `INT 0CCh` のPCPLUS/SCSIBIOS経路を観測可能かつ正しく
する必要があります。ゲストトレースまたは権威あるVAボード資料がSCHDの利用を示さ
ない限り、従来のNP2 `0CC6h` ストリームの直接登録は未サポートのままです。通常は
PIOが想定経路であり、`SETDMA.COM` はソフトウェアBIOS導入後に任意のPCPLUS DMA
モードを要求できますが、SCHDが存在するだけでDMAエミュレーションが必要になる
わけではありません。

## ディスクのビルド

[`tools/pc88va/scsi-support.sh`](../../tools/pc88va/scsi-support.sh) は、ユーザーが
用意したPC-Engine 1.1のD88システムディスクを受け取り、新しい起動可能D88を生成
します。元イメージと生成イメージはローカル成果物であり、Gitへ追加してはいけません。

DebianまたはUbuntuでは、ホスト側の依存ソフトを次でインストールします。

```sh
sudo apt-get install curl dosbox lhasa python3 coreutils
```

SCSI ID 0のターゲット用ディスクを作るには次を実行します。

```sh
tools/pc88va/scsi-support.sh \
  --source /path/to/user-supplied-pcengine-1.1.d88 \
  --output /path/to/pc88va-scsi-support.d88
```

ターゲットがID 0でない場合は `--scsi-id 0..7` を使います。

```sh
tools/pc88va/scsi-support.sh \
  --source /path/to/user-supplied-pcengine-1.1.d88 \
  --output /path/to/pc88va-scsi-id-3.d88 \
  --scsi-id 3
```

出力先は既存であってはいけません。公開入力アーカイブは共通の
`~/.cache/vaeg/auto-generated-pc88va-utility-media/` キャッシュを使います。
キャッシュファイルのチェックサムが違う場合は拒否し、黙って置き換えません。

ビルダーはまず
[`create-vanilla-system-disk.sh`](../../tools/pc88va/create-vanilla-system-disk.sh)
を使い、PC-Engine 1.1のIPLと必要なシステムファイルだけを残します。続いてSCSI
ドライバ、ユーティリティ、原文書をインストールします。

```text
A:\
  AUTOEXEC.BAT
  CONFIG.SYS
  PCPLUS.SYS
  SCHD.SYS
  ENGINEIO.SYS
  PCENGINE.SYS
  ADVGBIOS.SYS
  PCENGINE.COM

A:\BIN\
  SCFORM.COM
  VBUFF.COM

A:\DOC\
  PCPLUS.DOC
  PCPLUS.TXT
  SCSI55.TXT
  SCHD.DOC
  SCHD.LOG
  SCHD.TXT
  SCFORM.DOC
  SCFORM.LOG
  VBUFF.DOC
  VBUFF.LOG
```

`AUTOEXEC.BAT` は `A:\BIN` をコマンドパスへ追加します。ターゲットID 0の場合、
生成される `CONFIG.SYS` は次のとおりです。

```dos
FILES = 20
BUFFERS = 10
DEVICE = A:\PCPLUS.SYS
DEVICE = A:\SCHD.SYS -I0
```

PCPLUSはSCHDより先にロードしなければなりません。`-I` の後の値はターゲットの
SCSI IDで、`--scsi-id` から生成されます。

SASI開発ディスクビルダーは、既定で接続済みの160MB固定SCSIターゲットを想定します
（生成されるSCHD行は通常の固定ターゲット行のままです）。外部リムーバブル媒体を
使う場合は、通常の起動ディスクを直接変更せず、明示的なプロファイルを作成します。

```sh
tools/pc88va/build-sasi-utility-disks.sh --scsi-profile mo-128mb
```

`--scsi-profile mo-128mb` または `mo-160mb` は生成されるSCHD行に `-X -D1` を
追加し、`A:\DOC` に手動設定メモを書き込みます。`fixed-160mb` は固定ディスク
ポリシーを保ったまま大セクタ手順を記録します。このコマンドで生成されるイメージ
は40MBのSASI起動/サポートディスクのままで、プロファイルは別のSCSIターゲット向け
ゲストソフトウェアを準備するだけです。VAEGはMOをサポートしていないため、VBUFF、
SCFORM、STEST55S SFORM、SETDMAはホストビルダーから意図的に実行しません。

## インターフェースボードの設定

PC-9801-55または互換ボードを設定する前に `A:\DOC\SCSI55.TXT` を読んでください。
PC-88VA固有の案内には次の点が含まれます。

- ボードの標準I/Oポート `0CC0h`、`0CC2h`、`0CC4h` を使います。
- 装着済みハードウェアと競合しない割り込みを選びます。文書ではPC-88VAの
  `INT0`〜`INT3` の選択肢と、既存の2TD/SASI割り当てが説明されています。
- 通常の転送モードはプログラムI/Oです。拡張スロットに公開されるDMAチャネルは
  0と3だけで、SASIや2TDと競合する可能性があります。フォーラム資料はバスマスター
  転送だけに対応するボードは動作しにくいとも警告しています。
- ボードROMは通常 `0DC000h-0DCFFFh` に置かれます。PCPLUS資料ではPC-88VAで
  この範囲は未使用ですが、EMSページフレームと重ねてはいけないとしています。
  ボードROMを無効にする設定はサポートされ、VAEGの既定値です。VAEGは内蔵または
  ホストの `scsi.rom` イメージを `D2000h` その他のVAシステムメモリ窓へコピーしません。

これらはソフトウェア設定上の注意であり、電気的な取り付け手順ではありません。
終端、ケーブル、ハードウェアスイッチはインターフェースボードとターゲット機器の
マニュアルに従ってください。

## 論理セクタバッファ

変更していないPC-Engineシステムは通常、1024バイトを超える論理セクタを扱えません。
SCFORM 1.24では、その論理セクタサイズでおよそ1〜64MBのパーティションを作成できます。

より大きなパーティションには大きなPC-Engineバッファが必要です。ドライブAの
システムを最大2048バイト論理セクタへ変更するには、サポートディスクを起動して
次を実行します。

```dos
VBUFF -D1 -B11
```

VBUFFのドライブ番号は、現在のドライブが `0`、Aが `1`、Bが `2` という順です。
VBUFFは選択したシステムディスクのIPLの値を変更するため、先にバックアップを取り、
SCSIターゲットではなくPC-Engineシステムファイルのあるドライブを選びます。変更後
に再起動してください。大きなバッファはゲストメモリも多く消費します。

VBUFFを `AUTOEXEC.BAT` から実行しないのは意図的です。1024バイトパーティション
ではIPL変更が不要で、誤った起動ディスクを変更する危険な既定動作になるためです。

## ターゲットの初期化とパーティション分割

SCFORMはSCSIディスクのメタデータを書き込み、既存のパーティションとファイルを
破壊できます。実行前にターゲットをバックアップしてください。

およそ64MB以下のパーティションでは、対話式フォーマッタを次で起動します。

```dos
SCFORM
```

2048バイトのVBUFF設定を適用した後は、論理セクタサイズ拡張を付けてSCFORMを
起動します。

```dos
SCFORM /S       ; およそ128MBまで（2048バイトセクタ）
SCFORM /SS      ; 129〜256MB（4096バイトセクタ）
```

SCFORM 1.24のマニュアルはオプションを `-S` と定義し、`SCFORM /MS` の例では
スラッシュ形式を示しています。フォーラム記事はこの箇所を `-B` としていますが、
`-B11` はVBUFFの指定です。`S` を1つ追加するごとに論理セクタサイズは2倍です。
`/S` は2048バイト、`/SS` は4096バイト、`/SSS` は8192バイトを公開します。
このオプションは選択肢を拡張するだけで、値を自動選択しません。したがって
129〜256MBの領域には `/S` ではなく `/SS` が必要です。

SCFORMのメニューでは次を行います。

1. SCSI IDでターゲットを選ぶ。
2. 現在の内容を破棄してよい場合だけ装置を初期化する。
3. 現在のVBUFF設定が対応する論理セクタサイズを選び、領域を確保する。
4. 使用する領域を4つ以内にする。SCHDは有効な先頭4領域を公開する。
5. 終了して再起動してから新しいドライブを使う。

SCFORMで文書化されているおおよその領域は、1024バイト論理セクタで1〜64MB、
2048バイトで65〜128MB、4096バイトで129〜256MBです。必要なパーティションを
表現できる最小の論理セクタサイズを優先してください。

## ドライバ上書きと制限

SCHDは通常、シリンダ数と1シリンダ当たりブロック数を検出します。ターゲットが
利用可能なジオメトリを返さない場合は、`CONFIG.SYS` の行にシリンダ数の
`-C<number>` または1シリンダ当たりブロック数の `-S<number>` を追加します。
両方を指定した場合はSCHDが `-S` を優先します。このSCHDの `-S` はSCFORMの
論理セクタオプション `/S` とは無関係です。

歴史的な設定には次の重要な制限があります。

- SCSIディスクからPC-Engineを起動できません。生成フロッピーを起動媒体として
  残してください。
- フォーラムは、一部の古いPC-Engineシステムディスクで約30MB使用後にファイル
  破損が起きると報告しています。最初に安全だと確認されたリリースは特定されて
  いません。このビルダーは文書化されたPC-Engine 1.1のレイアウトを要求しますが、
  その構造チェックだけでは歴史的な不具合を否定できません。
- 論理セクタサイズが起動システムのIPLに記録されたバッファを超えるパーティション
  はSCHDが拒否します。
- 大きな論理セクタバッファは追加のコンベンショナルメモリを消費します。
- ジオメトリ上書き値は権威あるターゲット機器仕様から転記してください。`-C` や
  `-S` の値を推測するとディスクへアクセスできなくなることがあります。

## 検証

ビルダーはダウンロード、パッチ済みPCPLUS、PC-Engine 1.1のソースファイルシステム
レイアウト、生成FAT12構造を検証します。現行VAEGビルドはさらにSCSIイメージ接続、
ROMベースの起動、コントローラのフェーズ契約を検証します。次のゲストレベル検査は
PC-EngineサポートディスクとPCPLUS/SCHDソフトウェア経路の観察が必要なため、手動の
M75ゲートとして残っています。

1. 生成ディスクをV3モードで起動する。
2. PCPLUSがSCHDより先にロードされ、SCHDが意図したSCSI IDを報告することを確認する。
3. `SCFORM` は使い捨てまたはバックアップ済みのターゲットにだけ実行する。
4. 再起動し、各有効領域にドライブレターが割り当てられたことを確認してから、
   テストファイルを作成、読み出し、削除する。
5. 大きなパーティションでは、環境を信頼する前に重要でないテストデータで30MBの
   使用点を越えて検査する。

[forum-501]: http://www.pc88.gr.jp/forum/viewtopic.php?t=501
[pcplus]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=378
[pcplus-patch]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=451
[bdiff]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=328
[schd]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=448
[vbuff]: http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=452
[scform-topic]: http://www.pc88.gr.jp/forum/viewtopic.php?t=502
[wd33c93]: http://www.bitsavers.org/components/westernDigital/WD33C93A_Data_Sheet_and_Application_Notes_Nov1990.pdf
