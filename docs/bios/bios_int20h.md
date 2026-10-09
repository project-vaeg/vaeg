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

# PC-88VA解析資料 — INT 20H / プロセス終了

対象: VA / VA2 ROM、PC-Engine 1.1、テクマニ第2・7章、BNN第7章。
静的解析の初版。CPU例外00HとDOSのINT20Hを区別する。
[索引](README.md)、[INT21H](bios_int21h.md)も参照。

## 1. 結論と資料の区分

**ROM確認**: INT20Hは、AL=0、AH=4CHでINT21Hを呼ぶ終了入口。
0除算の例外入口ではない。第2章一覧の「DOS: 0除算」は、この実装と整合しない。
第7章のAH=4CHのプロセス終了説明と実装を合わせて判断した。
一覧説明が重複している原因（元資料の編集/転記等）は未確定。

**文書仕様**: 第7章はINT21H/AH=4CHを終了コード付きのプロセス終了とする。
INT20H自体の完全な入出力契約は今回の第7章記述では確定していない。
一般的なMS-DOSのCS=PSP制約を、実装確認なしにこのROMの契約へ流用しない。

## 2. 個別登録と入口

両機種の共通初期IVT表に20H/21Hはないが、DOS初期化が別途登録する。
ES=0000H、DI=0080HからSTOSWでoffset/CSを格納する。

| 対象 | DOS登録ルーチン | INT20H | INT21H |
|---|---|---|---|
| VA | F000:B69B | F000:A625 | F000:9403 |
| VA2 | F000:B685（IVT処理はB68D以降） | F000:A426 | F000:9403 |

```text
20H × 4 = 0080H → IVT 0000:0080 / 0082
21H × 4 = 0084H → IVT 0000:0084 / 0086
```

これは静的な登録値。DOS初期化完了前やOSによる後続変更後の最終IVTではない。

## 3. 終了処理の経路

- VA A625は元のSP/SSをCX/DXに保持し、DOSワークsegmentを
  `1040:05F8` からSSへロードする。SP=0710Hで内部stackを使う。
- VA A63E〜A642はAH=4CH、AL=0を設定してINT21Hを実行する。
- VA2 A426は先に `1040:0480` をfar CALLし、その後に同様のstack切替を行う。
  A442〜A446でAH=4CH、AL=0を設定してINT21Hを実行する。
- 終了の実体はINT21H側のプロセス/ハンドル/メモリ管理に依存する。
  呼び出しの次に通常どおり処理を継続するAPIとして使わない。
- 呼び出し後のfallbackにはstack復元と共通復帰への分岐がある。
  その存在だけで「必ず呼び出し元へ戻る」と解釈しない。

**ROM確認**: DOSワークのsegment参照は `1040:05F8`（物理109F8H）。
DOSが使用する通常RAMで、アプリケーションの空き領域ではない。
動的に設定されるsegmentなので、DOS内部workを0040H固定と決めつけない。

## 4. フックと確認の限界

VA2の先行far CALL先は `1040:0480 = 1088:0000 = 10880H`。
PC-Engineはこの共有DOS入口をfar JMPへ変更する。INT20HのIVTそのものを
置換することとは別で、詳しいVA/VA2経路はINT21H資料にまとめた。

今回追った登録・内部patch経路では、INT20Hベクタ自体のOS置換を確定していない。
これは「どのOS/ドライバもINT20Hをフックしない」という不存在証明ではない。
起動後IVT、終了時の親プロセス復帰、常駐プロセスとの相互作用は未実測。
プロセス終了を伴うため、総当たりや通常アプリ内からの試験呼び出しは行っていない。
