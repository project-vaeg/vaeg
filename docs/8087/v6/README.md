# VAEG + Intel 8087 Goal Pack v6

**資料取得 → 利用者へ渡して停止 → 利用者がOCR → 明示的に再開 → 実装・検証**の二段階です。
v5の全20実装packageを残した上で、A00–A03の取得工程を追加しています。
最初のGoalを、実装まで勝手に継続するGoalにはしていません。

## 最初に使うファイル

| 文書 | 役割 |
|---|---|
| [goal-contract.md](goal-contract.md) | 最優先の段階境界と全実装条件 |
| [activation.md](activation.md) | 取得用・実装用 `/goal` の全文 |
| [activation-acquire.txt](activation-acquire.txt) | 最初に送る取得専用の一行 |
| [activation-implement.txt](activation-implement.txt) | 利用者OCR後に送る実装専用の一行 |
| [acquisition.md](acquisition.md) | 資料の取得・配置・検証・停止 |
| [ocr-handoff.md](ocr-handoff.md) | 利用者OCRの保存先と再開時の照合 |
| [source-register.md](source-register.md) | 資料の出所・版・取得候補・不明点 |
| [sources/catalog.json](sources/catalog.json) | 実行可能な取得カタログ |
| [source-policy.md](source-policy.md) | 資料・依存物・実装コード・証拠の分離 |
| [reference-validation.md](reference-validation.md) | E8087など補助比較の範囲と限界 |
| [work-plan.md](work-plan.md) | A00–A03、HUMAN、P00–P19の依存関係 |
| [records/progress.md](records/progress.md) | 初期状態と実行後の継続記録 |
| [scripts/README.md](scripts/README.md) | 取得器の使い方、制限、再実行、テスト |

`activation.txt` は `activation-acquire.txt` と同一です。実装用の短縮入口にはしていません。
本体だけをコピーせず、ZIPから `docs/8087/v6/` 全体を新規配置してください。
旧v2–v5のファイルを上書きする必要はありません。配布rootのINSTALL.mdが配置手順です。

## 固定した8087仕様

最大1個のoptional device、既定 `10000000 Hz`、独立した整数Hz設定、machine resetでの反映です。
互換性のため装着設定の初期値はOFFです。クロックは表示だけでなく仮想命令時間へ反映します。
編集可能範囲1–20 MHz、5/8/10 MHzプリセットは引き継いだエミュレータ上の設計値です。
CPUとの並列動作を完全再現しない `SERIAL_TIMED_V1` で、FPO2には8087を接続しません。
全正式8087命令・operand/encoding formと5つの超越命令の実装・テストを必須にしています。

詳細は [architecture.md](architecture.md)、[clock-and-timing.md](clock-and-timing.md)、
[instruction-coverage.md](instruction-coverage.md)、[numerics-and-oracles.md](numerics-and-oracles.md)、
[verification.md](verification.md) を参照してください。これらはStage B用です。

## 資料の場所

公開Gitには契約、取得スクリプト、カタログ、短い非機密の進捗だけを置きます。
原本PDF、全文OCR、公開されているが再配布を許可されていない資料、private資料は外に置きます。
既定の兄弟ディレクトリは次です。配置済みのprivate資料は移動不要です。

```text
work/
  vaeg/
    docs/8087/v6/
      packages/a00-*.md through a03-*.md
      packages/p00-*.md through p19-*.md
      scripts/acquire-sources.py
      scripts/test-acquire-sources.py
      sources/catalog.json
      records/progress.md
  vaeg-8087-evidence-v6/
    originals/<source-id>/<original-name>.pdf
    upstream/<source-id>/<archive-name>.zip
    evidence-only/<source-id>/<article-name>.html
    user-ocr/<source-id>/document.md
    runs/<run-id>/OCR-HANDOFF.md
    latest.json
```

カタログの自動取得対象は16件、うちPDFは7件です。必須PDFはI01/I02/I03/I04/N01です。
I05は元版を発見できていない調査対象で、自動取得成功とは扱いません。
E8087 binaryと第三者emulator sourceも自動取得しません。使用許可と版を確認した補助比較は別工程です。
E8087のlibrary仕様、8087 chip仕様、実機観測を混同しません。

## 状態と再開

| 状態 | 意味 |
|---|---|
| `AWAITING_USER_OCR` | 必須取得が成功し、利用者へ引き渡した。実装開始の許可ではない |
| `ACQUISITION_INCOMPLETE_AWAITING_USER` | 必須取得に失敗がある。成功分と不足を引き渡して停止 |
| `OCR_INPUT_BLOCKED` | 実装用指示は受けたが、必要なOCR/版/対応関係が未確認 |
| `STAGE_B_IN_PROGRESS` | 実際の利用者指示とOCR受入れを記録して実装中 |
| `SOFTWARE_VERIFIED` | 特定のsource digestに対して、VAEGの必須ローカルgateが全て合格 |

一回目と二回目は別の目的です。取得Goalに対する単なる再開は、実装の許可になりません。
run budgetや権限停止をpromptで無効にはできません。中断後は実ファイルと進捗から再開します。
未実行CI・未取得実機trace・未確認silicon精度は、実行済みの結果と分けて報告させます。
配布物のテストは、VAEG本体の実装・動作確認ではありません。
