# 経緯 — 水温 / 油温 / 油圧 OLED ゲージ

このリポジトリは Arduino Nano + 2.7" OLED（ZJY270S0700XG31 / SSD1322 / 256×64）で
水温・油温・油圧を表示する車載ゲージのファームウェアと、書き込み用キット一式。

**2026-11-01 に「ラベルと単位は出るが数値だけ出ない」不具合が実機で見つかり、
その調査の途中経過と、切り分けのために作ったツール・テスト用パネル対応・診断ビルドを
ここに記録として残す。** 原因は 2026-11-26 時点で**未確定**（容疑者3つ、後述）。

---

## 1. ハードウェア構成

| 項目 | 値 |
|---|---|
| MCU | Arduino Nano (ATmega328P, 16 MHz, 2 KB RAM / 30 KB flash) |
| パネル（本番） | ZJY270S0700XG31 — SSD1322, 256×64, 7 pin, 4-wire SW SPI |
| 配線 | CS=D9 / RES=D6 / SCL=D5 / DC=D4 / SDA=D3（5 V→3.3 V レベルシフト） |
| 入力 | A0=油温（V→°C 2点校正）/ A1=油圧（0.62 V=0, 4.1 V=1000 kPa）/ A2=水温（A0と同特性） |
| 基準電圧 | AVCC（DEFAULT）。内蔵 1.1 V バンドギャップで AVCC を実測し 5.25 V 基準に正規化 |
| 表示更新 | 油圧 0.5 秒 / 水温・油温 2.0 秒 |
| テスト用パネル | 2.42" OLED / IC SSD1309 / 128×64 / 7 pin SPI（2026-11-13 追加） |

---

## 2. 時系列

| 日付 | 出来事 | 成果物 |
|---|---|---|
| 2026-08-05 | 書き込みツール ver3 を作成（XLoader 不要・ブラウザ書き込み） | `windows/oil-water-gauge-tool-drive_ver3` 相当の zip（HEX 20,112 B） |
| 2026-08-13 | ver4 キットを配布用に確定 | `windows/oil-water-gauge-tool-drive_ver4.zip`（HEX 20,414 B） |
| 2026-09-14 | Mac 上で最終ビルド（AVCC 実測・V→°C/kPa 校正を反映） | `firmware/built-20260914-macos/`（HEX 20,070 B） |
| 2026-11-01 | **不具合報告**（実車写真）→ プロジェクトを USB HDD から取り込み、Windows にビルド環境を構築、再現シミュレータを作成 | `docs/build-windows.md`, `build.cmd`, `upload.cmd`, `tools/oled-sim/` |
| 2026-11-03 | チャット履歴が消え、当時の会話ログは失われる（成果物とログのみ残存） | — |
| 2026-11-13〜14 | テスト用パネル（SSD1309 128×64）対応。**RAM 枯渇による「砂嵐」を発見・修正**、数値描画経路の無罪を確認。SSD1322 用の診断ビルドを作成 | `src/main.cpp`（`DISPLAY_SSD1309_128X64` / `DIAG_FIRMWARE`）、`windows/oil-water-gauge-ssd1309-test.zip`、`windows/oil-water-gauge-diag.zip` |
| 2026-11-26 | 経緯と成果物を GitHub に記録（このリポジトリ） | `docs/HISTORY.md`（本ファイル） |

---

## 3. 2026-11-01: 不具合の内容

実車の写真では **`Water` / `OilT` / `OilP` のラベルと単位（`C` / `kg`）だけが表示され、
数値が一切描かれていなかった**。

この時点でやったこと:

1. **ビルド再現性の確認** — Windows でクリーンビルドし、Mac の 2026-09-14 ビルドと
   バイナリ一致を確認（20,070 B / sha256 `4806d300…`）。ツールチェーン差ではないと断定。
2. **ELF の点検** — `avr-nm` で `drawSensorCell`, `Print::printFloat(double, unsigned char)`,
   `u8g2_font_logisoso24_tr`(0x059d), `u8g2_font_helvB12_tr`, `u8g2_NextPage` が
   すべてリンク済みであることを確認。フォントや書式ルーチンが消えている線は否定。
3. **描画シミュレータ（`tools/oled-sim`）** — ファームと同じ U8g2 2.36.18・同じフォント・
   同じページバッファ経路をホストで実行し、256×64 のフレームバッファを PBM/PNG に出力。
   当時の PC は Smart App Control が新規 `.exe` を止めるため **wasm32-wasi + Node** で実行。
   モード2（数値文字列を強制的に空にする）で**実車写真と同じ「ラベル＋単位だけ」の状態を再現**。

> 注意: このシミュレータが示したのは「症状の再現」であって原因の特定ではない。
> 数値文字列を空にすれば当然ラベルだけになるので、これは切り分けの出発点に過ぎない。

---

## 4. 2026-11-13〜14: テスト用パネルで得られた確定的な知見

本番パネル（SSD1322）が手元になかったため、2.42" SSD1309 128×64 モジュールで
同じ画面を出して検証した。ここで**別のバグ（RAM 枯渇）を踏んで潰している**。

### 4.1 128×64 対応

- `src/main.cpp` に `DISPLAY_SSD1309_128X64` スイッチを追加（`platformio.ini` の env で切替）。
- レイアウトは 3 分割（x=0 / 43 / 86）。ラベル 5x7（y=9）、数値 logisoso18（y=42）、
  単位 5x7（y=56）。
- `tools/oled-sim/sim128.c` で実フォント描画を確認（`node run128.mjs 25 80 4.5 1`）。

### 4.2 失敗: フルバッファ版で「砂嵐」

最初のビルドは `U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI`（フルバッファ 1024 B）を使用。
リンカ表示は **RAM 79.2%（1621/2048 B）**。実機では**画面が砂嵐**になった。

原因は RAM 枯渇: 残り約 400 B にスタックが積まれ、1 KB の表示バッファを踏んでいた。

### 4.3 修正: ページバッファ化

U8g2 には `ssd1309_128x64_noname0_1`（ページバッファ 128 B 版）が存在する。
これに切り替えて**本番機と同じ `firstPage()/nextPage()` 経路**に統一した。

| 指標 | 修正前 | 修正後 |
|---|---|---|
| RAM | 79.2%（1621 B） | **39.3%（805 B）** |
| Flash | 56.7% | 56.7% |
| 実機表示 | 砂嵐 | **正常（数値も表示）** |

### 4.4 ここで分かったこと（重要）

**このパネルでは数値が正常に描画された。** つまり

- `u8g2.print(float, digits)` → `Print::printFloat` → `write()` → `DrawGlyph` の経路は**正常**
- 128×64 レイアウト・フォント・ページバッファ経路も**正常**

→ 「ラベルと単位だけ出て数値が出ない」現象は、**印刷経路ではなく SSD1322（ZJY リマップ）側**
の問題である可能性が高い、というところまで絞れた。

---

## 5. 2026-11-14: SSD1322 用の診断ビルド（1回の書き込みで原因を確定させる）

本番パネルは手元に無く「一発勝負」になるため、画面を 3 秒ごとに切り替える
自己診断ファームを作った（`DIAG_FIRMWARE=1`, `env:nanoatmega328_diag`）。

| 画面 | 内容 | 何が分かるか |
|---|---|---|
| 1/3 MAP | 枠4辺・8 px 目盛り・最下行の白帯・`Vcc`/空きRAM・logisoso24 で `0123456789` | 縦方向のマッピングとフォントの有無 |
| 2/3 PATH | 同じ `12.3` を **print(12.3,1) / print("12.3") / drawStr("12.3")** で並置 | 書式ルーチンか描画かの切り分け |
| 3/3 LIVE | 実測値（小さな helvB12）＋生 ADC＋同じ水温を logisoso24 で再描画 | フォント依存で消えるかの判定 |

3 画面は `tools/oled-sim/sim_diag.c` で実フォント描画を検証済み（`out/diag{0,1,2}.png`）。

### 判定表

| 観察 | 結論 |
|---|---|
| 1/3 の `0123456789` が出る | フォント・下半分の描画は正常 → 本番レイアウト（y=52）固有か値が nan |
| 1/3 の枠の下辺・白帯が欠ける | **縦マッピングずれ**（ZJY リマップで実効 64 行未満）→ 数値だけ画面外 |
| 2/3 の print だけ空で str/draw は出る | **printFloat 経路**の問題 → `snprintf` 方式に変更して恒久対策 |
| 2/3 が 3 つとも空 | 大フォントのグリフ欠け、またはページバッファ経路 |
| 3/3 の下段だけ空 | **logisoso24 のグリフ欠け**確定 |

---

## 6. 検証済みの数値・ハッシュ（2026-11-26 時点）

ペイロード（Intel HEX をバイナリに戻したもの）で比較。HEX テキストのハッシュは
改行・レコード書式で変わるため使わない。

| ビルド | バイト数 | sha256 | 備考 |
|---|---|---|---|
| 本番 `env:nanoatmega328` | 20,070 | `4806d300f5c38dc4a3aa315c4f0db7650f4bde5c6b609099ceae364bb0405a59` | Mac 2026-09-14 ビルドと一致（再現性確認） |
| Mac 参照（old bootloader） | 20,070 | 同上 | `firmware/built-20260914-macos/nano-old-bootloader.hex` |
| Mac 参照（new bootloader） | 20,426 | `0948694e4de1f29c6105ad1dc720e7bfe5b1be01bc2413db2b235c822ad41fb5` | 起動アドレス違い |
| 診断 `env:nanoatmega328_diag` | 20,670 | `22ab2b2be91c4f25d44baf727e2716636025a2808aa60c82e850bc0f1513ed3a` | RAM 49.9% / Flash 67.3% |
| SSD1309 A `env:nanoatmega328_ssd1309` | 17,416 | `71b40077e1ab6fe6974acb1095d7cbd02f3ebd885740ebd99a3390f664e9d133` | RAM 39.3% / 列オフセット 0 |
| SSD1309 B `env:nanoatmega328_ssd1309_v2` | 17,416 | `948e38d76ad794eedcd3d51918111d8cfedbb98cab1aba201cd4b2b6e3703523` | 列オフセット 2 |
| 配布キット ver4（2026-08-13） | 20,414 | `385e6e49e24440373c3ac116666a246155c07d9f4275d5faad2c4ff9bc08510a` | **現行ソースと不一致**（後述） |
| 配布キット ver3（2026-08-05） | 20,112 | `80e51dd92434846f09b8343922d3a417f2bd6cea194ba0d2bc3f5e37743a8440` | 同上 |

### 配布キットとソースの不一致

`windows/oil-water-gauge-tool-drive_ver4.zip` の HEX は 20,414 B で、
2026-09-14 以降のソース（20,070 B）とは**別物**。
つまり 9/14 の変更（AVCC 実測・バンドギャップ正規化など）は
**配布済みキットに入っていない**。再配布時は要更新。

---

## 7. 未解決事項 / 次の作業

1. **SSD1322 実機での診断**（最優先）
   `windows/oil-water-gauge-diag.zip` を焼き、3 画面を撮影。
   判定表に従って原因を確定 → 修正 → HEX 再ビルド。
2. **修正方針の候補**（診断結果に応じて）
   - 数値の描画を `snprintf` + `drawStr` に統一（printFloat 依存を外す）
   - 大きい数値フォントを helvB 系に変更、または y 座標を実測して調整
   - ZJY リマップのオフセット設定（`U8G2_R0` のまま `setDisplayRotation` などで補正）
3. **配布キットの再生成** — 上記修正後、`windows/*.zip` の HEX を最新ソースで作り直す。
4. **書き込み後の検証** — シリアル（115200）に
   `W=..C(..V) OilT=..C(..V) OilP=..kg(..V) Vcc=..V raw=..` を出しているので、
   画面とシリアルの両方で確認できる。

---

## 8. 再現・検証の手順

```sh
# ビルド（4環境）
python -m platformio run                     # 本番（SSD1322, old/new bootloader）
python -m platformio run -e nanoatmega328_ssd1309     # テスト用 SSD1309 128x64 (offset 0)
python -m platformio run -e nanoatmega328_ssd1309_v2  # 同 (offset 2)
python -m platformio run -e nanoatmega328_diag        # SSD1322 診断

# 書き込み（Nano は COM3 等。PlatformIO の upload_port を確認）
python -m platformio run -e nanoatmega328 -t upload --upload-port COM3

# 画面レイアウトのシミュレーション（実 U8g2 を wasm でビルドして実行）
cd tools/oled-sim && bash build.sh
node run.mjs 25 80 4.5 1 > out.txt          # 256x64 本番レイアウト
node run128.mjs 25 80 4.5 1                 # 128x64
node run_diag.mjs 0                          # 診断画面 1/3
```

Windows のビルド環境（PlatformIO 6.2.0 / Python 3.14 venv / U8g2 2.36.18）は
[`docs/build-windows.md`](build-windows.md) を参照。

---

## 9. 用語メモ

- **ページバッファ / フルバッファ**: U8g2 の描画モード。`_1_` は 8 行分（128 px 幅なら 128 B）、
  `_F_` は全画面分（128×64 なら 1 KB）。2 KB RAM の ATmega328P では `_F_` は危険。
- **ZJY リマップ**: 型番 `ZJY270S0700XG31` 固有のピクセル配置を U8g2 側で吸収する設定。
- **砂嵐**: 表示バッファが壊れたときの典型的な症状（ランダムドット）。
