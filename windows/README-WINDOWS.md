# Windows から書き込む

## かんたん（推奨）

Arduino IDE 不要の簡易ツール:

**[`simple-flash/`](simple-flash/)**

1. `simple-flash` フォルダを Windows PC へコピー
2. Nano を USB 接続
3. **`書き込み.bat`** をダブルクリック
4. COM 番号を入力（例: `3`）

詳細: [`simple-flash/README.txt`](simple-flash/README.txt)

---

## その他（開発用）

ソースをいじって自分でビルドする場合:

- Arduino IDE: [`OilWaterGauge/OilWaterGauge.ino`](OilWaterGauge/OilWaterGauge.ino)（要 U8g2 ライブラリ）
- `upload.bat COM3`（要 Arduino CLI）
- PlatformIO: リポジトリ直下の `platformio.ini`
