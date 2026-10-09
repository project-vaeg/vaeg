<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# PC-88VAの追加解像度調査メモ

## 1. この文書の結論

PC-88VAの表示解像度は、次の三つを分けて設定する必要があります。

1. TSP/uPD72022のCRT走査（`SYNC`、ポート`0142h`/`0146h`）。
2. グラフィックス回路の走査解釈（`GRMODE`、ポート`0100h`）。
3. G0/G1の水平ドット数・ピクセル形式（`GRRES`、ポート`0102h`）と、
   フレームバッファ記述子（`0200h`--`027fh`）。

従って、`HAD=4Fh,VAD=240` のようなTSPフィールドを書くだけでは、PC-88VAの
GVRAM画面が320x240になるとはいえません。`GRMODE`、`GRRES`、FB記述子、
SGPの描画先descriptorを同じ論理幅・pitchで揃える必要があります。

このリポジトリで現在確実に扱える通常のグラフィックス垂直値は、BNN資料の
とおり200/204/400/408ラインです。240ラインと480ラインは、以下で示す
ように「TSPだけの候補」または「`VIEW480.ASM`に存在する拡張サンプル」であり、
通常のPC-88VA G0/G1 native modeとしては未検証です。

## 2. 根拠資料と確度

| タグ | 根拠 |
| --- | --- |
| `[BNN]` | [`docs/tekumani/PC88VA_テクニカルマニュアル_BNN.md`](../tekumani/PC88VA_テクニカルマニュアル_BNN.md)。CRT走査、`0100h`、`0102h`、GVRAM/FB。 |
| `[TSP]` | [`docs/tekumani/uPD72022.md`](../tekumani/uPD72022.md)。汎用μPD72022の`SYNC`、`RS/HAD/VAD`。 |
| `[VA-TSP]` | [`docs/modernization/upd72022-tsp.md`](upd72022-tsp.md)。vaeg内のTSP/VA合成の再構成。 |
| `[SAMPLE]` | [`docs/480/VIEW480.ASM`](../480/VIEW480.ASM)。PC-Engine付属の480-line sample。通常のbare payload用ABIではない。 |
| `[UNKNOWN]` | 実機で未確認、またはPC-88VAのG0/G1表示経路まで根拠が届いていないもの。 |

## 3. 共通の低レベル書込み手順

### 3.1 TSP `SYNC`

TSPのコマンドポートは`0142h`、パラメータポートは`0146h`です。`0142h`の
statusを確認してからcommand byteを書き、その後14バイトを一つずつ
`0146h`へ送ります。[BNN][TSP]

以下は、既存の同期ベクタを送るためのNASM風の最小ヘルパです。status bitの
意味は資料に従い、タイムアウトは実装側で必ず追加してください。

```asm
; Input: SI = 14-byte SYNC vector.
send_sync:
    mov     ah, 10h              ; SYNC command
    mov     dx, 0142h
.wait_command:
    in      al, dx
    test    al, 05h
    jnz     .wait_command
    mov     al, ah
    out     dx, al

    mov     cx, 14
.send_parameter:
    lodsb
    mov     ah, al
.wait_parameter:
    in      al, dx
    test    al, 01h
    jnz     .wait_parameter
    mov     al, ah
    mov     dx, 0146h
    out     dx, al
    mov     dx, 0142h
    loop    .send_parameter
    ret
```

`SYNC`の14バイトは、少なくとも`RM`、`RS`、`HAD`、`VAD`、blanking、sync幅を
含みます。`RS`だけを変更しても安全なCRTタイミングにはなりません。対応する
モニタ用の完全なベクタを使ってください。[TSP][VA-TSP]

### 3.2 解像度変更時の共通コマンド列

15/24 kHz、インターレース、または垂直走査の解釈を変更する場合は、次の順序を
守ります。各 `OUT 0100h` は独立した I/O 命令です。`GRMODE` の他のビットは
read-modify-write で保持します。[BNN §4.9.5]

```asm
; 1. Stop graphics and video/sync output.
in      ax, 0100h
and     ax, 7fffh               ; GDEN0 = 0
or      ax, 2000h               ; XVSP = 1
and     ax, 0efffh              ; SYNCEN = 0
mov     dx, 0100h
out     dx, ax

; 2. Wait for VRTC 0 -> 1 -> 0 -> 1 (or two VRTC interrupts).

; 3. Reset the graphics display circuit in a separate write.
in      ax, 0100h
and     ax, 0ffdfh              ; GDEN1 = 0
mov     dx, 0100h
out     dx, ax

; 4. Set RSM1/RSM0 and SYNCM, then release GDEN1 in another write.
; 5. Re-program GRMODE, GRRES, framebuffer and SGP descriptors.
; 6. Send SYNC (10h + 14 parameter bytes).
; 7. Set GDEN0=1, then SYNCEN=1; wait at least one second and clear XVSP.
```

この手順は「解像度」を一つのレジスタに書くものではありません。`SYNC` は
CRTタイミング、`GRMODE` はグラフィックスの垂直走査解釈、`GRRES` はG0/G1の
水平fetchとpixel mode、FB descriptorはGVRAMの行進みと表示窓を担当します。

### 3.3 汎用TSPフィールド値とVAの設定値

汎用 μPD72022 の内部クロック例では、水平有効ドット数は
`(HAD + 1) * 4` です。したがって、次の値は **TSPフィールド値**であり、
そのままPC-88VAの完成した表示モードを意味しません。

| TSP-only candidate | `RS` | `HAD` field | `VAD` | 判定 |
| --- | ---: | ---: | ---: | --- |
| 640x480 | `100b` | `9Fh` (640 dots) | `01E0h` | `[TSP]` only |
| 320x240 | `001b` | `4Fh` (320 dots) | `00F0h` | `[TSP]` only |
| 256x240 | `000b` | `3Fh` (256 dots) | `00F0h` | `[TSP]` only |

`VAD=240`または`480`へ変更するには、blanking、border、sync幅、外部
dot-clockを含む14バイト全体が必要です。この表の3行だけを個別に `OUT` して
完成したVAモードにすることはできません。[TSP][VA-TSP]

### 3.4 `GRMODE`（ポート`0100h`）

16ビットlittle-endianのword portです。既存の値を読み、変更対象のbitだけを
更新します。[BNN][VA-TSP]

```asm
; AX = preserved GRMODE value.
; Low two bits: 00=400, 01=408, 10=200, 11=204 graphics lines.
and     ax, 0FFFCh             ; illustrative mask; preserve other bits in real code
or      ax, 0002h               ; select 200-line graphics interpretation
mov     dx, 0100h
out     dx, ax
```

上の`and`は説明用です。実装では、垂直line field以外を破壊しない
read-modify-writeにしてください。15/24 kHzを切り替える場合は、BNNの
4.9.5にある表示禁止、VRTC待ち、GDEN1 reset、RSM/SYNCM設定、再初期化、
表示再開の手順を使います。`0100h`への単独書込みで走査周波数を変更しては
いけません。[BNN]

### 3.5 `GRRES`（ポート`0102h`）

`GRRES`の意味は次のとおりです。[BNN][VA-TSP]

| field | 値 | 意味 |
| --- | --- | --- |
| G0 `PM0` bits 1:0 | `00/01/10/11` | 1/4/8/16 bits per pixel |
| G0 `HW0` bit 4 | `0/1` | 640/320 logical dots |
| G1 `PM1` bits 9:8 | `00/01/10/11` | 1/4/8/16 bits per pixel |
| G1 `HW1` bit 12 | `0/1` | 640/320 logical dots |

代表的なread-modify-writeは次のとおりです。

```asm
; G0 = 320 dots, 4 bpp.
and     ax, 0ffech             ; clear G0 HW0 and PM0 only (preserve other bits)
or      ax, 0011h
mov     dx, 0102h
out     dx, ax

; G1 = 320 dots, 4 bpp.
and     ax, 0ecffh             ; clear G1 HW1 and PM1 only
or      ax, 1100h
mov     dx, 0102h
out     dx, ax
```

`GRRES`は640または320ドットしか公開していません。256ドットや384ドットを
このレジスタに直接指定するfieldはありません。[VA-TSP]

## 4. フレームバッファ記述子

FB0は`0200h`から始まる`20h`バイトのdescriptorです。G0の第2画面はFB2、
G1はFB1です。[BNN][VA-TSP]

| offset | field | 役割 |
| ---: | --- | --- |
| `+00h` | FSA | 仮想FBの開始アドレス |
| `+04h` | FBW | 行間pitch（4バイト境界） |
| `+06h` | FBL | virtual heightの最終line/vertical wrap |
| `+08h` | DOT | source pixel lane |
| `+0ah` | OFX | source X offset |
| `+0ch` | OFY | source Y offset |
| `+0eh` | DSA | 表示source開始アドレス |
| `+12h` | DSH | 表示sub-screen高さ |
| `+16h` | DSP | CRT上のdestination Y |

4bppの最小pitchは、320ドットで160バイト、640ドットで320バイトです。
最終lineを`FBL+1`と解釈するvaeg実装では、240ラインなら`FBL=239`です。
実機のfield表現は必ずキャプチャで確認してください。[VA-TSP]

ページ容量の目安（packed pixel）は次のとおりです。

| 論理サイズ | 1bpp | 4bpp | 8bpp |
| --- | ---: | ---: | ---: |
| 640x480 | 38,400 B | 153,600 B | 307,200 B |
| 320x240 | 9,600 B | 38,400 B | 76,800 B |
| 256x240 | 7,680 B | 30,720 B | 61,440 B |

これは必要容量の算術値であり、native fetch可能性やFB descriptorの上限を
保証するものではありません。

概念的なFB0設定（各wordの書込みは実際のdescriptor ABIに合わせる）は次です。

```text
FSA = page_base
FBW = bytes_per_line
FBL = height - 1
DOT = 0
OFX = 0
OFY = 0
DSA = page_base
DSH = height
DSP = destination_y
```

VAEGの現在のポート割当てに対応する、FB0のbyte-field列は次のようになります。
これは `page_base` を4-byte境界に置いた場合の**設定例**です。FSA/DSAの上位
ビット、FBW/FBL/DSH/DSPの予約ビットは、対象機種のdescriptor規約を確認して
から書いてください。[VA-TSP]

```text
; Port-sequence notation, not a complete assembler macro.
; 320-dot, 4-bpp, 240-row content in a 400-line timing.
; FB0 base = 0200h, page_base is a 4-byte-aligned GVRAM address.
write8(0200h, page_base[7:0])          ; FSA[7:0]
write8(0201h, page_base[15:8])         ; FSA[15:8]
write8(0202h, page_base[17:16])        ; FSA[17:16]
write8(0204h, A0h)                     ; FBW = 160 bytes
write8(0205h, 00h)
write8(0206h, EFh)                     ; FBL = 239 = 240 lines - 1
write8(0207h, 00h)
write8(0208h, 00h)                     ; DOT = 0
write8(020Ah, 00h)                     ; OFX = 0
write8(020Bh, 00h)
write8(020Ch, 00h)                     ; OFY = 0
write8(020Dh, 00h)
write8(020Eh, page_base[7:0])          ; DSA[7:0]
write8(020Fh, page_base[15:8])         ; DSA[15:8]
write8(0210h, page_base[17:16])        ; DSA[17:16]
write8(0212h, F0h)                     ; DSH = 240
write8(0213h, 00h)
write8(0216h, 50h)                     ; DSP = 80
write8(0217h, 00h)
```

上の `write8` はポート番号とbyte値を明示するための記法です。実際には
`mov dx,port`、`mov al,value`、`out dx,al`（または対象機種で許される word I/O）へ
展開してください。256x240 viewportでは同じpitchの行内でx=32..287だけを描画し、
表示sourceの開始アドレスは行頭のままにします。

FB descriptorを書くだけでは表示されません。`GRMODE`/`GRRES`、表示enable、
必要なTSP timing、SGP destination descriptorを同じ形式に揃えます。

## 5. 640x480

### 5.1 通常のPC-88VA G0/G1としての結論

通常のBNNグラフィックス設定には480-lineの`GRMODE`値がありません。BNNが
記載するCRTは15.98/15.73/24.8 kHzで、通常グラフィックスのline選択は
200/204/400/408です。したがって、**640x480のG0/G1 native modeを確定した
コマンド列は、このリポジトリでは未確定**です。[BNN][UNKNOWN]

また、4bppなら1ページは
`640 * 480 / 2 = 153600` bytesです。256 KiBのsingle-plane GVRAMで
640x480の4bppを2ページ保持することはできません。1bppなら1ページ38400
bytesですが、これは別の`GRRES.PM`設定です。[BNN][VA-TSP]

descriptorのフィールド幅だけを見ると、`FBL=479` と `DSH=480` は表現可能です。
したがって「FBレジスタへ値を書けるか」という問いには **書ける** と答えられます。
しかし `GRMODE` に480-lineの選択肢がなく、TSPの完全な同期ベクタとGVRAM fetchの
組合せも未検証なので、これだけで640x480表示が成立するわけではありません。

### 5.2 `VIEW480.ASM`にあるサンプル経路

`docs/480/VIEW480.ASM`は、現在の高さが400より大きく480以下であることを
確認し、次の値を書いています（source lines 38--110）。

`syncprm` の初期値は次の14バイトです。

```text
C1 57 10 00 9F 00 10 0F 19 00 90 40 07 08
```

高さを `480 (01E0h)` とした場合、同ファイルの書換えから得られる SYNC
ベクタは次の形です。byte 10 の下位8ビットを `E0h` にし、byte 11 の
VAD bit 8 (`40h`) は保持し、byte 12/13を `01h` にしています。
これは資料からの確定したVA native vectorではなく、サンプルからの導出値です。

```text
C1 57 10 00 9F 00 10 0F 19 00 E0 40 01 01
```

```asm
; VIEW480 sample: historical PC-Engine/BIOS path, not a bare VA ABI.
mov     ax, 0000h
mov     dx, 010ah
out     dx, ax

mov     ax, height
mov     dx, 0206h              ; FB0 FBL in the sample's interface
out     dx, ax
mov     dx, 0212h              ; FB0 DSH in the sample's interface
out     dx, ax

mov     byte [sync_vector + 10], al ; VAD low byte
mov     byte [sync_vector + 12], 01h
mov     byte [sync_vector + 13], 01h
; send SYNC 10h + the 14-byte vector through 0142h/0146h
```

同サンプルは `height` を `0206h` (FB0 FBL) と `0212h` (FB0 DSH) の両方へ
書いています。しかし BNN §5.3 は FBL を「実際のライン数 - 1」と定義して
います。このため480ラインなら資料準拠の候補は `FBL=479, DSH=480` であり、
サンプルの `FBL=480` とは食い違います。実機投入用コードではこの差を解決する
まで、サンプル列をそのまま採用しないでください。

同ファイルの初期化は`INT 84h`、`INT 8Fh`、終了は`INT 21h`に依存しています。
これはPC-Engineの補助BIOSを含むサンプルなので、V3 bare payloadにそのまま
コピーしてはいけません。上のport列は「480-line sampleが何を書いているか」
を示す証拠であり、PC-88VAの通常640x480 GVRAM互換性の証明ではありません。
[SAMPLE][UNKNOWN]

## 6. 320x240

### 6.1 推奨: native 320幅 + 240-line content viewport

PC-88VAの安全な組合せは320x200または320x400です。240ラインを**画面全体の
native垂直timing**として設定せず、640x400 timing（または既存の検証済み
200-line timing）の中に240-lineのcontent rectangleを置くのが安全です。

4bppでの設定値は次のとおりです。

```text
GRMODE bits 1:0 = 10b (200-line raster) or 00b (400-line raster)
GRRES G0: HW0=1, PM0=01b       ; 320 dots, 4 bpp
FB0 FBW = 160 bytes
FB0 FBL = 239                   ; vaeg interpretation, 240 source rows
FB0 DSH = 240
FB0 DSP = chosen destination Y
SGP destination: width=320, height=240, pitch=160 bytes, 4 bpp
```

`GRMODE`を200-lineにする場合、240行を同一physical rasterへそのまま出すこと
はできません。400-line timingで`DSP`/`DSH`を使うか、200-line側で垂直拡大を
明示的に設計してください。240-line native scan vectorは未検証です。[BNN]

### 6.2 TSP-onlyの候補（実機で未検証）

汎用μPD72022の20-MHz例は`RS=001`、`HAD=4Fh`（320 dots）、`VAD=200`を示します。
`VAD=240`へ変更すること自体は汎用TSPの範囲に見えますが、blanking、sync幅、
CRT周波数、外部clock、およびPC-88VAのD65101/GVRAM合成が別問題です。
完全な14-byte timing vectorがないため、以下を実機投入用の完成コマンドとは
扱いません。

```text
SYNC.RS  = 001b       ; generic /3 dot-time selection
SYNC.HAD = 04Fh      ; (04Fh + 1) * 4 = 320 active dots
SYNC.VAD = 240        ; [UNKNOWN] PC-88VA board composition
```

## 7. 256x240

### 7.1 推奨: 320-dot native source内の256-dot viewport

PC-88VAの`GRRES`に256-dot選択はありません。したがって、320-dot source rowの
中央に256-dot contentを置き、左右32ドットを背景または透明にします。4bppなら
source pitchは320-dotの160バイト、contentの開始は1行あたり16バイトです。

```text
TSP/GRMODE: documented 320x200 or 320x400 timing
GRRES G0: HW0=1, PM0=01b       ; 320 dots, 4 bpp
FB0 FBW = 160 bytes
FB0 FBL = 239
FB0 DSH = 240
FB0 DSP = destination Y
content in each source row: byte offset 16 .. 143 (256 pixels)
outside content: background/transparent pixels in the 320-dot row
SGP: draw into the 320-dot source; do not submit a 256-dot native width
```

この方法なら、未定義のdestination-X registerを仮定せずに256x240相当の矩形を
作れます。640-dot sourceを使う場合はpitchを320バイトにし、内容を640-dot
row内に配置します。[VA-TSP]

### 7.2 256x240をTSP nativeにする候補

汎用μPD72022表には256x192（`RS=000`, `HAD=3Fh`, `VAD=192`）がありますが、
PC-88VAのG0/G1 fetchは320/640です。`HAD=3Fh,VAD=240`を設定しても、GVRAM
画面、spriteの640-dot座標、D65101 mixerが一致する証拠はありません。

```text
SYNC.RS  = 000b       ; generic 256-dot divider
SYNC.HAD = 03Fh      ; (03Fh + 1) * 4 = 256 active dots
SYNC.VAD = 240        ; [UNKNOWN]
GRRES    = no 256-dot field (must remain 320 or 640)
```

この候補はTSP単体の実験用であり、通常のPC-88VA graphics modeとして配布・
保証してはいけません。[TSP][VA-TSP][UNKNOWN]

### 7.3 256x240でのFB設定と走査周波数

**FB descriptorは設定できます。** ただし、それは「256ドットnative fetch」を
追加することではありません。既知の `GRRES.HW0=1`（320-dot source）を使い、
各行の中央32ドットを空けて、その内側に256ドットの内容を置きます。4bppの
具体的な候補は次のとおりです。

```text
SYNC       = 640x400 / 24.8 kHz の既知ベクタ
GRMODE     = bits 1:0 = 00b (400-line interpretation)
GRRES.G0   = HW0=1, PM0=01b  (320 dots, 4 bpp; value 0011h in those fields)
FB0.FBW    = 160 bytes
FB0.FBL    = 239             (240 source rows, BNN convention)
FB0.DOT    = 0
FB0.OFX    = 0
FB0.OFY    = 0
FB0.DSA    = page_base       (display source starts at the row start)
FB0.DSH    = 240
FB0.DSP    = 80              (center 240 rows in a 400-line raster)
SGP        = 320-wide source, 160-byte pitch; draw content at x=32..287
```

`DSA`は表示sourceの開始アドレスであり、destination-Xを指定するものでは
ありません。中央32ドットの配置は、各行のGVRAMにx=32から描くことで行います。
実際のdescriptorでは、`FSA`/`DSA`の4-byte alignment、`OFX`、SGP destination
addressの関係を同じにしてください。`OFX`を「destination X」と解釈しては
いけません。
この方式の256x240は、640-dotのCRTタイミングの中に置く**論理コンテンツ矩形**
です。[BNN][VA-TSP]

走査周波数については、次のように分けて考えます。

| 構成 | 水平走査 | 垂直走査 | 256x240との関係 |
| --- | --- | --- | --- |
| 320/640x240 viewport in 640x400 timing | 24.8 kHz family | 400-line timing（約55--56 Hz系） | **15/24.8 kHzから離れない**。240行以外は空き/背景 |
| 256x240 TSP-only candidate | 完全なSYNC次第 | `VAD=240` と全blanking次第 | **未算出**。PC-88VA native modeではない |
| VIEW480-style extended vector | 24.8 kHz系をベースにVADだけ拡張 | 480 active candidate | **未検証**。FB/FBLにも資料間不一致あり |

つまり、256x240をviewportとして実装するなら15 kHz/24.8 kHzというCRTの
水平周波数を変更しません。320-dot sourceはVAEGの表示経路で640-dot共通座標へ
拡大されるため、256 logical pixelsは通常の表示では512 physical dots相当の
幅になります。256-dot native TSPへ変更する場合は `HAD=3Fh` だけでは不十分で、
dot-clockと全水平期間を含む完全な14-byte vectorを作ってから、実機の周波数を
測定する必要があります。[VA-TSP][UNKNOWN]

## 8. 解像度変更時の実行順序

15/24 kHz、non-interlace/interlace、または垂直解釈を切り替える場合は、BNN
4.9.5の順序を守ります。

1. `0100h`の`GDEN0=0, XVSP=1, SYNCEN=0`（表示禁止）。
2. `0040h`のVRTC bit 5を2回待つ。
3. `0100h`の`GDEN1=0`（表示回路reset）。
4. `RSM1/RSM0`と`SYNCM`を設定。
5. `GDEN1=1`（reset解除）。
6. `0100h`、`0102h`、`0106h`、`0110h`などリセットされた表示parameterを再設定。
7. 新しい14-byte `SYNC` vectorを`0142h`/`0146h`へ送る。
8. FB0--FB3、SGP destination、palette、必要なdisplay descriptorを設定。
9. `GDEN0=1`、`SYNCEN=1`で信号を再開し、少なくとも1秒待って`XVSP=0`。

各手順は独立したI/O命令にします。特に④と⑤を同一word writeにまとめないで
ください。表示を先に止めずにmodeを変更すると、BNNはGVRAM内容を保証して
いません。[BNN]

## 9. SGPとの関係

SGPには「320x240」や「640x480」というモニタ解像度opcodeはありません。SGP
側で設定するのは、描画先の物理アドレス、packed pixel mode、幅、高さ、pitch、
word数などです。表示側の`GRMODE`/`GRRES`/FB descriptorと一致させます。

例（4bpp）:

```text
320 source: FBW=160 bytes, SGP width=320, height=240
640 source: FBW=320 bytes, SGP width=640, height=400 (native documented)
256 viewport: keep source width 320 or 640; clip/content-mask 256 pixels in software
```

640x480を4bppでSGPへ渡す場合、`FBW=320`では1ページ153600バイトになり、
480行のG0/G1 native表示、descriptor、GVRAM容量、TSP timingの全てが実機で
検証されるまで「動く設定」とは記載しません。[UNKNOWN]

## 10. 実機確認用チェックリスト

各候補について、次を記録してください。

```text
machine/model:
monitor scan mode and DIP switch:
SYNC 14 bytes:
GRMODE word:
GRRES word:
FB0/FB1 FSA, FBW, FBL, DOT, OFX, OFY, DSA, DSH, DSP:
SGP destination descriptor:
visible raster size:
GVRAM page size and page count:
```

特に480/240は、最終PNGだけでなく、実際のVRTC、表示開始行、source address
progression、左右端のwrap、GVRAMの行間pitchを確認してください。

## 11. まとめ

- **640x480**: `VIEW480.ASM`にサンプル経路はあるが、通常のPC-88VA G0/G1
  native modeとしては未確定。sampleのBIOS/PC-Engine依存をbare payloadへ
  持ち込まない。
- **320x240**: 320-dot native fetch + 240-line content viewportを推奨。
  `GRRES.HW=1,PM=4bpp`、`FBW=160`、`DSH=240`。240-line native timingは未検証。
- **256x240**: 256-dot native fetchはない。320-dot source内に256-dot contentを
  置くviewport方式を推奨。TSPの256-dot候補は実験扱い。
- **SGP**: 解像度設定は行わず、FB/descriptorと描画pitchを表示設定に合わせる。

この文書はエミュレータ／資料ベースの設定表です。PC-88VA/VA2/VA3実機での
表示互換性を確定するものではありません。
