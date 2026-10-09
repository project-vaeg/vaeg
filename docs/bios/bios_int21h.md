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

# PC-88VA解析資料 — INT 21H / DOSシステムサービス

対象: VA / VA2 ROM、PC-Engine 1.1、テクマニ第7章、BNN第7章。
静的解析の初版。起動後IVT/RAM、全入出力・error経路は未実測。
[索引](README.md)、[INT20H](bios_int20h.md)、
[SETFCB](bios_int9eh.md)、[EXEC_COM](bios_int9fh.md)も参照。

## 1. 資料と実装の範囲

**文書仕様**: テクマニ7.1に40種類のAHが記載される。
4BHはAL=00Hの実行とAL=03Hのoverlayロードを個別に説明するため、
個別説明は41件。INT21H本体を「少数のBIOS追加機能」と扱わない。
一方、OSによる一部機能への補正hookは本体の機能数とは別に数える。

第2章一覧の「シングルステップ」はこのDOSサービスの実装と整合しない。
BNN第7章もMS-DOS互換のファイル/メモリサービスとして説明する。
これはMS-DOSの全AH・全ABIを互換実装する保証ではない。
BNN OCRの31H/3AH、DTA設定/取得等の表記揺れは、個別説明とROMで区別する。

## 2. 登録と256-slot dispatcher

**ROM確認**: DOSの個別登録ルーチンはIVT0000:0084/0086へF000:9403を設定する。
VAはB69B、VA2はB685（IVT操作B68D以降）。共通初期IVT表だけを調べて
INT21Hを未実装と判断してはいけない。

| 対象 | INT21H | 処理開始 | 256-word AH表 | 共通未対応入口 |
|---|---|---|---|---|
| VA | F000:9403 | F000:946C | F000:962B | F000:A1D0 |
| VA2 | F000:9403 | F000:94F8 | F000:96F2 | F000:A073 |

AHをDOSワークの+0088Hへ保存し、AH×2でword表を引く。
表のデータを命令として逆アセンブルしない。
AH=1DH/85Hには表参照より先に専用分岐もある。
通常経路は `1040:05F8` からDSを取得する。固定0040HのBIOSワークとは別。

## 3. 文書の40種類のAHとROM入口

機能名は**文書仕様の独立要約**、offsetは**ROM確認**（segment F000H）。
非default入口への到達確認であり、全error経路やOS変更後の結果の検証ではない。

| AH | 機能 | VA | VA2 |
|---|---|---|---|
| 0EH | 既定ドライブ切替 | 982B | 98F2 |
| 19H | 既定ドライブ番号取得 | 987B | 991E |
| 1AH | DTA設定 | 989A | 9930 |
| 1BH | 既定ドライブの媒体情報 | 98B8 | 9941 |
| 1CH | 指定ドライブの媒体情報 | 98F3 | 9976 |
| 25H | 割り込みベクタ設定 | 9976 | 99EC |
| 2EH | 書き込み検証mode設定 | 99A2 | 9A0A |
| 2FH | DTA参照取得 | 99B5 | 9A10 |
| 31H | 常駐終了 | 99F1 | 9A30 |
| 33H | CTRL-C検査設定/取得 | 9A19 | 9A3D |
| 35H | 割り込みベクタ取得 | 9A58 | 9A62 |
| 36H | 空きクラスタ/媒体情報取得 | 9A7D | 9A79 |
| 39H | ディレクトリ作成 | 9AC5 | 9ABA |
| 3AH | ディレクトリ削除 | 9AF5 | 9AD2 |
| 3BH | 作業ディレクトリ切替 | 9B22 | 9AE7 |
| 3CH | ファイル作成/切り詰め | 9B4F | 9AFC |
| 3DH | ファイルopen | 9B89 | 9B33 |
| 3EH | ハンドルclose | 9BBF | 9B63 |
| 3FH | ハンドルread | 9BE4 | 9B70 |
| 40H | ハンドルwrite | 9C1C | 9BA2 |
| 41H | ファイル削除 | 9C54 | 9BD4 |
| 42H | ファイル位置変更 | 9C81 | 9BE9 |
| 43H | ファイル属性取得/設定 | 9CBF | 9C21 |
| 45H | ハンドル複製 | 9D02 | 9C5A |
| 46H | 指定番号へのハンドル複製 | 9D2A | 9C7C |
| 47H | 作業ディレクトリのパス取得 | 9D50 | 9C8A |
| 48H | メモリ確保 | 9D81 | 9CA3 |
| 49H | メモリ解放 | 9DAD | 9CC9 |
| 4AH | メモリ領域サイズ変更 | 9DCF | 9CD3 |
| 4BH | 実行/overlayロード | 9E04 | 9CFC |
| 4CH | 終了コード付き終了 | 9E3C | 9D1C |
| 4DH | 子プロセス終了情報取得 | 9E60 | 9D28 |
| 4EH | ファイル検索開始 | 9E8A | 9D4B |
| 4FH | ファイル検索継続 | 9EBB | 9D64 |
| 54H | 書き込み検証mode取得 | 9EDC | 9D6D |
| 56H | ファイル名/所在変更 | 9F16 | 9D82 |
| 57H | ファイル日時取得/設定 | 9F4B | 9D9F |
| 58H | メモリ割当て方式取得/設定 | 9F8E | 9DD5 |
| 5AH | 一時ファイル作成 | 9FB8 | 9DFD |
| 5BH | 既存名を上書きしないファイル作成 | 9FF2 | 9E34 |

### 第7章一覧外の入口

VA表にはdefaultとは異なる50 slots、VA2には55 slotsがある。
公開40種類との差はVAで10、VA2で15。これは安全な追加APIの数ではない。

| AH | VA | VA2 | 確認の範囲 |
|---|---|---|---|
| 18H | 9856 | 9914 | 第7章一覧外。VA側OS hookがこの番号を検査 |
| 1DH | 9942 | 99BE | dispatcherで個別に先行分岐。完全ABI未確定 |
| 30H | 99DB | 9A27 | ROMの返却AXはVA=0002H、VA2=0202H |
| 80H | A078 | 9EAB | 内部サービス候補。完全ABI/公開利用可否は未確定 |
| 81H | A0C3 | 9EEC | 内部サービス候補。完全ABI/公開利用可否は未確定 |
| 82H | A0F4 | 9F17 | 内部サービス候補。完全ABI/公開利用可否は未確定 |
| 83H | A125 | 9F42 | 内部サービス候補。完全ABI/公開利用可否は未確定 |
| 84H | A147 | 9F4C | 内部サービス候補。完全ABI/公開利用可否は未確定 |
| 85H | A17E | 9F61 | 内部サービス候補。完全ABI/公開利用可否は未確定 |
| 86H | A188 | 9F6B | 内部サービス候補。完全ABI/公開利用可否は未確定 |

さらにVA2のみ次の5 slotsが非default入口を持つ。VAでは共通未対応入口。

| AH | VA2 |
|---|---|
| 87H | 9F75 |
| 88H | 9FAC |
| 89H | 9FF1 |
| 8AH | 9FF7 |
| 8BH | A019 |

**ROM確認**: AH=30HはAXとしてVA=0002H（2.00）、VA2=0202H（2.02）を返すコード。
OSによる補正後の版番号は別途確認が必要。
AH=00H、01H、02H、09H、5FH等は今回の両ROM表では共通未対応入口。
その入口はAX=0001Hを設定し、最終的に保存FLAGSのCFをセットしてIRETする。
通常のDOS文字入出力やredirector機能が当然使えると想定しない。

## 4. 一部ABIと通常RAMの参照

- **文書仕様/ROM確認**: AH=25Hの登録元はDS:DX。原文の説明欄にはDS:BXともあるが、
  VA:9976/VA2:99ECはIVTのoffsetへDXを書き、segmentへ保存入力DSを使う。
  INT21H/AH=25Hで82H等を変更することと、INT21H自身のフックは別。
- **文書仕様**: AH=35HはALのvectorをES:BXへ返す。25Hと組み合わせる。
- **文書仕様/ROM確認**: AH=1AHはDS:DXでDTAを指定、2FHはES:BXで取得する。
  両ROMはDOSワークの+00CCH/+00CEHを使用し、未設定時は現在の
  プロセスコントローラ（PSP相当）の+0080Hを返す。
- **文書仕様**: PSP相当領域は100H bytes。+0AH終了先、+0EH旧INT23H、
  +12H旧INT24H、+2CH環境segment、+2EH親segment、+50H INT21H/RETF stub、
  +80H既定DTA。DOS管理RAMで、TVRAMではない。全byteの実装照合は未完了。
- **文書仕様**: AH=47HのDS:SIには63 bytes以上を用意する。
  AH=3FH/40HのDS:DXはCX bytesの呼び出し側buffer。
  read/writeの実転送長AXを確認し、CFだけで全量転送と判断しない。
  VAのAH=47H補正はCX=0040H（64）のcopy loopを持つため、文書の63 bytesと
  最大長/終端の関係は追加照合が必要。最小buffer長の実行保証は未確定。
- **文書仕様**: AH=48H/4AHのサイズはparagraph（16 bytes）単位。
  PID/MCB相当の管理領域をユーザのデータ領域と混同しない。
- **ROM確認**: DOSワーク+0070H=保存AX/返却値、+0072H=入力DS、
  +0080H/+0082H=保存SP/SS、+0084H=選択handler、+0088H=AH。
  これらは `DS=[1040:05F8]` 内のoffsetで、固定物理アドレスではない。
- VA2は25H/33H/35H/48H/49H/4AHの6番号を96ECHのbyte表で照合し、
  CTRL-C検査を経由しない経路へ進む。この6番号の処理と内部hookの数は別。

## 5. 実際のPC-Engine内部フック

### IVTではなく通常RAMの共通DOS入口を置換する

```text
1040:0480 = 1088:0000 = 10880H
1040:05F8            = 109F8H （DOSワークsegmentの参照）
```

**ROM確認**: VA:9622、VA2:94FA/96DB等は1040:0480をfar CALLする。
VA DOS初期化B60Cは1040:0480から0177H bytes（末尾05F6H）をRETF opcodeで埋める。
このRAM上の入口とIVT0000:0084（物理00084H）は別物。

**ファイル確認**: PC-EngineのVA側C91Bは1088HをESにし、C95FHの
handler/位置のword対を読む。位置を2で割り、far JMPのopcode・offset・CSを書き込む。
表は14組あり、先頭の位置0は共通DOS入口1088:0000をCS:B944へ変更する。
14組はINT21Hの14個のAHを意味しない。残りは別の内部呼び出し位置。

### VA側で確定したAH=18H/47Hの選別

CS:B944はDOSワーク+0088Hの保存AHを検査する。

| 条件 | 静的に確認できた補正 |
|---|---|
| AH=47H | far CALLの復帰情報を外し、独自パス取得処理へ進む。入力DS:SIを保持し、コピーでSHIFT-JIS先頭byteの範囲も判定。ROMの9D63へ戻る経路を作る |
| AH=18H | DOSワーク+0190H、+01C4H、+01C8HをFFHへ設定してRETF。各fieldの完全な意味は未確定 |
| その他 | このhookはRETFしてROMの通常dispatchを継続 |

したがって、**VA向けに一部INT21H機能を補正するhookは存在する**。
ただし、この2条件だけでPC-Engineの全DOS補正を網羅したとは言わない。
媒体・版・起動条件での最終状態は未実測で、補正が必要になった根本原因も未確定。

### VA2側は別のhook

ファイルF208の初期化は同じ1088H領域へfar JMPを置く。
位置0の入口は実行時CS:1910（ファイルE2E0）。VA2後半の実行時起点は
ファイルC9D0で、VA側のoffsetと同じ意味で使わない。
この表は5組。1910はstack上のROM復帰offset AA7E/A5E2/EB95を照合し、
一致時だけ補正経路へ進む。不一致ならFLAGSを復元してRETFする。
これはAH=18H/47Hだけを検査するVAのB944とは異なる。
対応する全DOS機能・完全なstack契約は未確定。

## 6. 残る調査と安全性

最終IVT、DOSワークsegmentの実値、他の内部patchと全AHの対応、
内部80H台のAPI/寿命、全ファイル/メモリ境界、VA2のcontext依存補正は未完了。
関数25Hによるvector変更、メモリ解放、終了、媒体書き込みは動的に試していない。
第2章の用途一覧、ROM初期登録、OS補正、実行後の状態を分けて解釈する。
