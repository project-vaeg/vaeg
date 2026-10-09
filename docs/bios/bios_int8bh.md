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

# PC-88VA BIOS解析資料 — INT 8BH / Music / Sound BIOS

テクマニ6.11相当。対象: VA / VA2 ROM、PC-Engine 1.1。

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

選択レジスタ: **AH**。以下は機能識別と主要な用途であり、省略されたパラメータを任意値で代用してはいけない。

| 番号 | 機能 |
|---|---|
| 00H | Initialize |
| 01H | PlayData |
| 02H | Clear |
| 03H | ReadRegister |
| 04H | WriteRegister |
| 05H | SetGateStep |
| 06H | SetNote |
| 07H | SetLength |
| 08H | SetTempo |
| 09H | SetParamBlock |
| 0AH | GetParameter |
| 0BH | SetParameter |
| 0CH | StopPlay |
| 0DH | ContinuePlay |
| 0EH | EnableLFO |
| 0FH | DisableLFO |
| 10H | HoldKey |
| 11H | SetIntCondition |
| 12H | SetVolume |
| 13H | SetBGMMode |
| 14H | MIDIOutput |
| 15H | SetEnvelope |
| 16H | SetPlayMode |
| 17H | PutPlayQueue |
| 18H | GetPlayStatus |
| 19H | GetParamBlock |
| 1AH | GetBufferSize |
| 1BH | PlayData2 |
| 1CH | GetBookkeeping |
| 1DH | Initialize2 |
| 1EH | WriteRegister2 |
| 1FH | ReadRegister2 |
| 20H | OutputControl |
| 21H | DefineRhythm |

## ROM入口 [ROM確認]

| 対象 | 登録／転送先 |
|---|---|
| VA | F000:50EC（個別登録／専用入口） |
| VA2 | F000:56E6（個別登録／専用入口） |

## Conventional memory / ワーク

| segment:offset | 物理アドレス | 長さ | 用途・根拠 |
|---|---|---|---|
| `0040:03B0` | `007B0H` | 10H bytes | [ROM確認] 4個のfar hook。GetBookkeeping返却先 |
| `0040:03C0` | `007C0H` | 2 bytes | [ROM確認: VA] Initializeで保存するユーザーqueue segment |
| `0040:03C2` | `007C2H` | 2 bytes | [ROM確認: VA] queueサイズ保存 |
| `0040:064C` | `00A4CH` | 2 bytes | [ROM確認: VA] 呼び出しframe用SP保存 |
| `0040:03C8` | `007C8H` | 2 bytes | [ROM確認: VA2] Initialize2のrhythm queueサイズ |
| `0040:03CA` | `007CAH` | 2 bytes | [ROM確認: VA2] rhythm queue segment |
| `0040:03CE` | `007CEH` | 2 bytes | [ROM確認: VA2] rhythm pattern buffer segment |
| `0040:03D0` | `007D0H` | 2 bytes | [ROM確認: VA2] rhythm pattern bufferサイズ |

固定アドレスはOSに予約された領域であり、アプリケーションの空きRAMではない。表は確認済み部分の抜粋で、ワーク全体の占有範囲・再入可能性を保証しない。

## 実装・ABI・注意点

- [ROM確認] VAはF000:50AAでIVT8BHにF000:50ECを登録しAH<1DHを処理する。VA2は56A9でF000:56E6を登録しAH<22Hを処理する。dispatch表はVA 60E6、VA2 67C6。
- [ROM確認] AH=1CHはVA 5C5F、VA2 5FCAにあり、両者ともES:DX=0040:03B0（物理007B0H）を返す。VA2は保存frameを書き換える方式で返す。
- [文書仕様] 1DH～21Hは旧Sound BIOSでは使えない。VAでMusic BIOSを利用するには対応sound boardと対応PC-Engineが必要。VA ROMだけをVA2の34機能と同一視しない。
- [文書仕様] InitializeのqueueはESのsegmentとDXの長さで渡す。Initialize2はES/DX=rhythm queue、DS/SI=rhythm pattern bufferで、最低16／512 bytes。上表は機種依存の保存場所であり、共通構造ではない。
- [文書仕様] PlayData2実行中は渡したparameter列・演奏dataの寿命を維持する。演奏中に別queue投入を行うと元dataを破壊し得る。強制停止後はClearが必要。
- register readの一部はhardware readでなくshadow参照。直接音源I/Oで変更した値の再読出しを保証しない。MIDI出力はフック設定が前提。

## 未完了の検証

個々の入力構造の全フィールド、エラー経路、フックの初期化／寿命、PC-Engineの機種分岐と最終登録順の全面照合は未完了。
起動後IVTと予約RAMを読み取り中心で確認してから利用可否を判断する。媒体書き込み・待機・割り込み変更を伴う機能の総当たり試験はしない。

[割り込み別索引](README.md)
