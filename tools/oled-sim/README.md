# OLED レイアウト・シミュレータ

ファームウェアの描画コードを **実機なしで** そのまま実行し、256×64 の画面イメージを
PBM / PNG で出力するツール。使用する U8g2 は `.pio/libdeps` の実ライブラリ（2.36.18）で、
フォント・座標・ページバッファ経路もファームと同じ。

## 何を再現するか

`drawSensorCell()` と同じ順序で描画する:

1. `u8g2_font_helvB12_tr` でラベル（Water / OilT / OilP）を y=14 に描く
2. `u8g2_font_logisoso24_tr` で数値を y=52 に描く
3. 数値の描画幅だけ進めた位置に `u8g2_font_helvB10_tr` で単位（C / kg）を描く

数値の文字列化は AVR コアの `Print::printFloat()` を移植したもの（`nan` / `inf` / `ovf`
の扱いも同じ）。

## ビルドと実行

```sh
bash build.sh                      # sim.wasm を作る（zig cc / wasm32-wasi）
node --experimental-wasi-unstable-preview1 run.mjs 25 80 4.5 1 > out.txt
```

- 引数: 水温 油温 油圧 モード
- モード 0: フルバッファ（`_f`）— 1枚の 256×64 PBM
- モード 1: ページバッファ（`_1`）— **ファームウェアと同じ経路**。ページごとに
  `###PAGE n` マーカー付きで PBM を出力
- モード 2: ページバッファで**数値文字列を空**にする（実車の写真と同じ状態の再現用）

`run.mjs` は stdout に PBM、stderr に各セルの `value="..." w=... unit@x=...` を出す。
`w` が描画幅、`unit@x` が単位の描画開始位置。

## PNG 化

`###PAGE` で分割 → 各 PBM を 8 行ずつ積む → 256×64 の画像にする。
PIL がある Python なら数十行で済む（`../oled-sim` を作った際の `stitch.py` 相当）。

## なぜ WASM か

このPCは Smart App Control が有効で、新規作成した未署名 `.exe` の実行がブロックされる
（`Permission denied` / 「Device Guard のポリシーによってブロックされました」）。
WASM はデータファイル扱いなので通る。SAC の無い環境なら native ビルドでよい:

```sh
zig cc -O1 -w -I"$U8G2_CLIB" sim.c "$U8G2_CLIB"/*.c -o sim.exe -lm
```

## 前提

- `C:/Users/chapy/pio-venv` に `ziglang`（`pip install ziglang`）
- Node.js（`node:wasi` を使用）
- `.pio/libdeps/nanoatmega328/U8g2/src/clib` が存在すること（`build.cmd` 後）
