# Arduino Nano — ZJY270S0700XG31 (SSD1322)

Arduino Nano で **ZJY270S0700XG31**（2.7" / **256×64** / **7 pin** / SSD1322）を動かすプロジェクト。

詳細: [`docs/hardware-zjy270s0700xg31.md`](docs/hardware-zjy270s0700xg31.md) /
開発の経緯・不具合調査の記録: [`docs/HISTORY.md`](docs/HISTORY.md)

> **既知の問題（2026-11-26 時点・未解決）**
> 本番パネル（SSD1322）の実機で「ラベルと単位は出るが**数値だけ出ない**」現象が発生。
> 原因は未確定で、切り分け用の診断ビルドを同梱している → [`docs/HISTORY.md`](docs/HISTORY.md) §5。
> テスト用パネル（SSD1309 128×64）では数値は正常に表示されるため、印刷経路ではなく
> パネル／ZJY リマップ側を疑っている。

## ハードウェア

| 項目 | 値 |
|------|-----|
| MCU | Arduino Nano (ATmega328P) |
| パネル | ZJY270S0700XG31 |
| 解像度 | 256 × 64 |
| コネクタ | 7 pin |
| IC | SSD1322 |
| IF | 4-wire **SW SPI**（`U8G2_SSD1322_ZJY_…` + `U8G2_R0`） |
| 電源 | 3.3 V（レベルシフト経由） |

## 配線（確定）

| モジュール | Nano |
|------------|------|
| GND | GND |
| VCC | 3.3 V |
| CS | **D9** |
| RES / RST | **D6** |
| D0 / SCL / CLK | **D5** |
| DC | **D4** |
| D1 / SDA / DIN | **D3** |

5 V → 3.3 V レベルシフト対象: D9 / D6 / D5 / D4 / D3

## アナログ入力

| ピン | 信号 | 換算 |
|------|------|------|
| **A0** | 油温 | V→°C テーブル（1.30〜4.00 V） |
| **A1** | 油圧 | リニア 0.6 V→0 / 4.1 V→1000 kPa → kg/cm² |
| **A2** | 水温 | 当面は油温と同じテーブル（固有較正があれば差し替え） |

センサーは 5 V 系電圧出力（プルアップ不要）。ADC 基準は Nano の AVCC（DEFAULT）。内蔵 1.1 V バンドギャップで AVCC を毎回測定し、実測した 5.25 V を基準にセンサー出力を正規化するため、AVCC が変動しても電圧出力が比例するセンサーでは V→°C / V→kPa がずれにくい。温度は実測2点（室温27.7℃ / 2.31 V、熱湯100℃ / 3.746 V）の線形校正、油圧は大気圧時0.620 Vをゼロ点とする。シリアルに `Vcc=` を出す。別の電源電圧で使う場合は `SENSOR_REFERENCE_VCC` を実測値に合わせる。

## ビルド

```sh
pio run                    # 本番（SSD1322 256x64, 旧/新ブートローダーの2環境）
pio run -t upload
pio device monitor
```

### ビルド環境（`platformio.ini`）

| env | 対象 | RAM | Flash |
|---|---|---|---|
| `nanoatmega328` | 本番 SSD1322 256×64（旧ブートローダー） | 48.7% | 65.3% |
| `nanoatmega328new` | 同上（新ブートローダー。HEX は同一） | 48.7% | 65.3% |
| `nanoatmega328_ssd1309` | テスト用 SSD1309 128×64（列オフセット 0） | 39.3% | 56.7% |
| `nanoatmega328_ssd1309_v2` | 同上（列オフセット 2） | 39.3% | 56.7% |
| `nanoatmega328_diag` | SSD1322 用 診断（3画面切替） | 49.9% | 67.3% |

Bash からは `platformio.exe` が実行できないことがある（Permission denied）。
その場合は venv の Python 経由で: `python -m platformio run -e <env>`。

## OLED 表示

- 水温 (°C) / 油温 (°C) / 油圧 (kg/cm²) を横一列表示
- シリアルに生電圧・ADC raw も出力（115200）

### 2.42" SSD1309 128x64（実機テスト用パネル）

7 pin SPI の 2.42" モジュール（IC = SSD1309、128x64）で同じ画面を出すビルド。

| パネル | Nano | 備考 |
|---|---|---|
| GND | GND | |
| VCC | 5 V（または 3.3 V） | モジュール側にレギュレータあり |
| SCK | D5 | |
| SDA | D3 | |
| RES | D6 | |
| DC | D4 | |
| CS | D9 | |

```sh
pio run -e nanoatmega328_ssd1309        # 列オフセット 0（通常）
pio run -e nanoatmega328_ssd1309_v2     # 列オフセット 2
```

- 画面は 3 分割（0 / 43 / 86 px）。ラベル 5x7、数値 logisoso18、単位は数値の下の行
- 起動画面に `SSD1309 128x64` と出るので、どちらのビルドか判別できる
- **ページバッファ（128 B / RAM 39%）**を使う。フルバッファ（1 KB）版は RAM 79% で
  スタックが表示バッファを壊し、画面が砂嵐になった（実測）
- 列オフセット違いは `-DSSD1309_VARIANT=2`（env `nanoatmega328_ssd1309_v2`）
- レイアウトは `tools/oled-sim/sim128.c` で実フォント描画を確認済み：
  `node run128.mjs 25 80 4.5 1`（mode 2 で数値が空になる症状を再現）

## 書き込み用キット

| ファイル | 用途 | パネル |
|---|---|---|
| [`windows/oil-water-gauge-tool-drive_ver4.zip`](windows/oil-water-gauge-tool-drive_ver4.zip) | **2026-08-13 版（配布中・HEX は現行ソースより古い）** | SSD1322 256×64 |
| [`windows/oil-water-gauge-ssd1309-test.zip`](windows/oil-water-gauge-ssd1309-test.zip) | テスト用（A/B 2 版同梱、RAM 枯渇の修正済み） | SSD1309 128×64 |
| [`windows/oil-water-gauge-diag.zip`](windows/oil-water-gauge-diag.zip) | **1 回の書き込みで原因を切り分ける診断ビルド** | SSD1322 256×64 |

使い方は各 zip の `OilWaterGauge-Windows/00_ReadMe_JP.txt` と
`HOWTO_PowerShell_Paste.txt`（PowerShell に貼り付けて localhost から書き込む）。
