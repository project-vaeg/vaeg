# VAEG 8087 Goal Pack v6 — 配置と起動

この配布物は、v5の実装契約を保持し、資料取得と人手OCRの引き渡しを先行させるv6一式です。
**最初の `/goal` は資料取得で終了します。OCRは利用者が行い、別の明示的な指示後に実装を開始します。**
8087は最大1個、既定10 MHz・独立したクロック設定・全正式命令実装という条件は変更しません。

## 配置

Python 3.9以降とGitを使用します。VAEGのGit worktree rootで実行してください。
ZIPは一時ディレクトリに展開し、検証付きinstallerで新しい `docs/8087/v6/` だけを配置します。
旧v2–v5の文書、実装、進捗を上書きしません。

```sh
cd /path/to/vaeg
packdir="$(mktemp -d)"
unzip /path/to/vaeg-8087-goal-pack-v6.zip -d "$packdir"
python3 "$packdir/install-pack.py" --repo "$PWD"
python3 "$packdir/install-pack.py" --repo "$PWD" --apply
git status --short
cat docs/8087/v6/activation-acquire.txt
```

一回目のinstallerは確認だけです。二回目だけが新規ファイルをコピーします。既存v6がある場合は、
空ディレクトリでも停止します。symlinkの配置先、不正なmanifest、破損した入力も拒否します。
入力ZIPのmanifestは内容の一致確認で、配布元を証明する電子署名ではありません。
コピー中のI/Oエラーでは新規作成分が残ることがあります。利用者の変更を巻き込む自動削除はしません。
同じ配置先を別プロセスで同時に変更しないでください。installerはcommit/push/reset/cleanを実行しません。

## 一回目: 資料を取得して停止するGoal

上の `cat` で表示された一行を、VAEG rootで起動したCodexへ送ります。
[activation.md](docs/8087/v6/activation.md) に全文と再開の扱いがあります。
古いv5の実装Goalが継続中なら、そのまま自動実装を続けさせず、今回のStage Aに切り替えてください。

Codexは [acquisition.md](docs/8087/v6/acquisition.md) のA00–A03を行います。
取得先は既定ではGit worktreeの兄弟ディレクトリ `vaeg-8087-evidence-v6/` です。
Gitの中やsymlink先に資料を置く指定は拒否します。別の場所はCodexへ明示してください。

```text
work/
  vaeg/
    docs/8087/v6/
  vaeg-8087-evidence-v6/
    originals/<source-id>/
    upstream/<source-id>/
    evidence-only/<source-id>/
    user-ocr/<source-id>/
    runs/<run-id>/
      download-report.json
      download-report.md
      CHECKSUMS.sha256
      ocr-queue.tsv
      OCR-HANDOFF.md
    latest.json
```

取得後のCodexの回答には、実際のworkspace、引き渡し文書、未取得資料の場所が示されます。
必須資料に失敗があっても、成功分と失敗の記録を渡して停止します。未取得を成功扱いにしません。
OCR、PDFからのテキスト抽出・Markdown変換、アーカイブ展開、dependency build、実装は行いません。

## 利用者: OCR

`OCR-HANDOFF.md` と `ocr-queue.tsv` に従い、原本を変更せずOCRを行ってください。
既定の出力先は `user-ocr/<source-id>/document.md` です。分割Markdownや既存の別フォルダも使えます。
可能ならPDFの1始まりページ番号と、印刷されているページラベルを残してください。
利用者がchecksumやJSON承認ファイルを手で作る必要はありません。
[ocr-handoff.md](docs/8087/v6/ocr-handoff.md) が受け渡し仕様です。

## 二回目: OCRを確認して実装するGoal

OCR終了後に限り、次の一行を表示してCodexへ送ってください。既定と違うOCR保存先は併記します。

```sh
cat docs/8087/v6/activation-implement.txt
```

この指示がStage Bの明示的な許可です。ファイルが出現しただけでは実装は始まりません。
Codexは原本とOCRを照合し、ページ対応を確認した後、P00–P19を実装・検証します。
AgentによるOCR禁止はStage Bでも続きます。不明箇所はページを特定して報告します。

## 配布物の検証と限界

全原本PDF、SoftFloat本体、E8087 binary、ROM、OCR本文はこのZIPに含めていません。
実際の取得は、上記Stage Aを実行するCodex環境で行います。取得URLは候補であり、将来の成功保証ではありません。
配布物の検証結果は別添 `vaeg-8087-goal-v6-pack-validation.json` を参照してください。
取得器の再現可能なオフラインテストは次で実行できます。

```sh
PYTHONDONTWRITEBYTECODE=1 python3 docs/8087/v6/scripts/test-acquire-sources.py
```

これらはVAEGのビルドや8087動作の合格を意味しません。VAEG本体のgateはStage Bで初めて作成・実行します。
