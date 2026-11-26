# Windows ビルド環境（このPC）

Arduino Nano OLED Gauge（`arduino-nano-ssd1322`）を Windows 11 上でビルドするための環境メモ。
2026-11-01 に構築・検証済み。

## 構成

| 項目 | 値 |
|---|---|
| PlatformIO Core | 6.2.0 |
| Python（専用 venv） | 3.14.6 |
| venv の場所 | `C:\Users\chapy\pio-venv` |
| PlatformIO 本体 | `C:\Users\chapy\pio-venv\Scripts\platformio.exe` |
| パッケージ置き場 | `C:\Users\chapy\.platformio`（約 150 MB） |
| プラットフォーム | atmelavr 5.3.0 |
| framework | framework-arduino-avr 5.4.0 |
| ツールチェーン | toolchain-atmelavr 1.70300.191015（avr-gcc 7.3.0） |
| ライブラリ | U8g2 2.36.18（`platformio.ini` の `^2.35.19` を満たす最新） |
| グローバル `pio` コマンド | `C:\Users\chapy\bin\pio.cmd`（ユーザー PATH に追加済み） |

> Python は winget や pip を使わず、`py -3.14 -m venv` で作った専用 venv に
> `pip install platformio` しただけ。システムの Python 環境は汚していない。

## 使い方

プロジェクト直下（`C:\Users\chapy\work\arduino-nano-ssd1322`）で:

```bat
build.cmd                       :: 両方の環境をビルド
build.cmd -e nanoatmega328      :: 旧ブートローダーだけ
build.cmd -t clean              :: クリーン
upload.cmd COM3                 :: 書き込み（ポート指定）
upload.cmd COM3 nanoatmega328new  :: 新ブートローダー
```

`upload.cmd` はポートを省略すると自動検出を試みる。Nano が自動検出されない場合は
COM 番号を明示する（デバイスマネージャーで確認）。

シリアルモニタ（115200 bps、`W=.. OilT=.. OilP=.. Vcc=.. raw=..` を出力）:

```bat
pio device monitor -b 115200 -p COM3
```

bash（Git Bash）からは `pio` ではなく **`pio.cmd`** と打つ（bash は `.cmd` を拡張子補完しない）。

## ビルド結果（検証済み）

```
Environment       Status    Duration
nanoatmega328     SUCCESS   00:00:20
nanoatmega328new  SUCCESS   00:00:23

RAM:   48.7% (997 / 2048 bytes)
Flash: 65.3% (20070 / 30720 bytes)
```

クリーンビルド後の `firmware.hex` は
`0db461740e4db7cb7f50faf2eea6bfbcfd2415d4dcba253c0413104f6f772681`
で、**Mac で 2026-09-14 にビルドした最終版と完全一致**（バイナリ 20,070 bytes も同一）。
ツールチェーンが同じため再現性あり。

## フォルダの扱い

- `.pio\` — Windows 用のビルドキャッシュ。`build.cmd -t clean` で消してよい。
- `.pio-macos-20260914\` — USB HDD から持ってきた Mac 時代のビルド成果物（333 MB）。
  絶対パスが Mac 前提なので Windows では使わない。不要なら削除可。
- `firmware\built-20260914-macos\` — 上記から抜き出した HEX/ELF の控え。
  配布用 HEX の原本として保持する。

## 注意点

- `nanoatmega328`（旧ブートローダー）と `nanoatmega328new`（新）で**生成される
  HEX は同じ**。違いは書き込み時の通信設定だけなので、書き込みに失敗したら
  もう一方の環境を試す。
- `platformio.ini` の `platform = atmelavr` はバージョン固定していない。将来
  `pio pkg update` すると atmelavr が上がり、HEX が変わることがある。
  配布物の同一性を保ちたいなら `atmelavr@5.3.0` に固定するのが安全。
- このPCの `COM4` は Intel AMT（Active Management Technology）の仮想ポート。
  Nano のポートと間違えないこと。
- `avrdude` は単体では入っていない。PlatformIO 経由
  （`C:\Users\chapy\.platformio\packages\toolchain-atmelavr\bin\avrdude.exe`）で使う。

## 環境を作り直す場合

```bat
py -3.14 -m venv C:\Users\chapy\pio-venv
C:\Users\chapy\pio-venv\Scripts\python.exe -m pip install --upgrade platformio
```

初回 `build.cmd` で atmelavr プラットフォーム・ツールチェーン・U8g2 を自動取得する
（ネットワーク必要、合計 150 MB 程度）。
