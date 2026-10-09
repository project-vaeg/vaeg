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

# PC-88VA BIOS解析資料 — INT A4H / 数値演算BIOS / DCMP（比較）

テクマニ6.12相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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

選択レジスタ: **なし（INT番号で演算選択）**。以下は機能識別と主要な用途であり、省略されたパラメータを任意値で代用してはいけない。

| 番号 | 機能 |
|---|---|
| — | DS:DIとDS:SIを比較。SF/ZFに結果、OF/CF=0 |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:03B5 → bank 0, E000:000F → ROMバンク内offset 0099 |
| VA2 | F000:0515 → bank 1, E000:600F → ROMバンク内offset 60B4 |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `0374:008E` | `037CEH` | 1 byte | [ROM確認] 第1内部数値の拡張byte／制御用、正確な役割は未確定 |
| `0374:008F` | `037CFH` | 8 bytes | [ROM確認] DS:DIからcopyする第1数値 |
| `0374:009E` | `037DEH` | 1 byte | [ROM確認] 第2内部数値の拡張byte／制御用、正確な役割は未確定 |
| `0374:009F` | `037DFH` | 8 bytes | [ROM確認] DS:SIからcopyする第2数値 |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [文書仕様] 割り込み番号そのものが演算を選ぶ。AH別dispatchではない。数値は**IEEE binary浮動小数点ではなく8-byteの10進BCD形式**で、有効数字14桁。内部演算の16桁表現と呼び出し側の8 bytesを混同しない。
- [文書仕様] offset 0～6は仮数絶対値のpacked BCD（高位addressほど上位桁）、offset 7はsign bit7と指数bit6～0。指数は40H bias、ゼロは指数byte=0。非ゼロ仮数は0.1以上1未満に正規化する。
- [ROM確認] VAはbank 0、VA2はbank 1に実装。共通setupは内部segment 0374Hを選択し、8 bytesを008FH／009FHへcopyする。VAの01E5～021A、VA2の6206～623Bで確認。全関数のscratch fieldを確定した表ではない。
- [文書仕様] 原則OF/SF/ZF/CFで演算状態を返す。DCMPだけは比較結果の契約。binaryのunsigned比較用JA/JBと混同せずsigned条件を使う。
- [文書仕様] DINのsyntax errorでは出力数値が破壊され得る。DOUTはNULL終端分までbufferを確保する。format指定時CH+CL<250の制約がある。
- [文書仕様] 負数のSQRT、0以下のLOG、非整数指数での負数PWR等は計算不能。入力BCDの不正形式は動作保証がない。
- 固定scratchを共用するので、再入可能と仮定しない。出力変数の8 bytesとBIOS内部ワークは別領域。各演算の全flag経路とroundingは実行未検証。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
