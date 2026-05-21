# ICSN for ESP32

ESP32を使用したICSN（Interest-Centric Sensor Network）の実装です。

クリーンアーキテクチャを採用し、レイヤー分離によって保守性と拡張性を重視した設計になっています。このブランチでは通信プロトコルにESP-NOWを使用しています。

## 機能

- **ESP-NOW**を使用したメッシュネットワーク通信
- センサーデータの送受信
- INTEREST/DATA/INVALIDの3つのメッセージタイプ
- 設定可能なネットワークパラメータ（`data/config.json`）
- パフォーマンス測定機能
- クリーンアーキテクチャによる階層設計（Entity、Use Case、Interface、Infrastructure）

## セキュリティ機能

### ESP-NOW 暗号化（PMK / LMK）

ESP-NOW の WPA2 ベース暗号化を有効化します。

- **PMK (Primary Master Key)**: 全ピア共通のグローバルキー。`config.json` の `PMK` フィールド（32 文字 hex）で設定します。
- **LMK (Local Master Key)**: ピアごとの暗号化キー。`config.json` の `LMK` フィールドをデフォルト値として使用し、ピア登録時に適用します。

### HMAC-SHA256 による送信元認証

各パケットには HMAC-SHA256 ダイジェスト（32 バイト）が付加されます。受信側は LMK を鍵として検証し、改ざんを検出します。

### リプレイ攻撃対策（送受信カウンタ）

`PeerCounterManager` がピアごとに TX/RX カウンタを管理します。受信パケットのカウンタが既知の値以下の場合は破棄することで、録画・再送攻撃を防ぎます。

> **注意**: PMK・LMK はデフォルト値のままでは安全ではありません。本番環境では必ず固有の値に変更してください。

## 依存方向のルール

このプロジェクトの依存方向は以下を基本とします。

- `Controller` → `IInputBoundary`（Interface）
- `Controller` → `IForwardingStateBoundary`（Interface）
- `UseCaseInteractor` → `IInputBoundary` / `IForwardingStateBoundary`（実装）
- `UseCaseInteractor` → Entity
- `UseCaseInteractor` → `IContentStore` / `IForwardingInformationBase` / `IPendingInterestTable`（Data Access Interface）
- `Infrastructure (Data Access)` → `IContentStore` / `IForwardingInformationBase` / `IPendingInterestTable`（実装）

補足:

- `Controller` は `UseCaseInteractor` 具象へ直接依存せず、`IInputBoundary` / `IForwardingStateBoundary` 経由で連携する。
- `UseCaseInteractor` は Data Access 具象（`LRUContentStore` 等）へ直接依存せず、インターフェース経由でアクセスする。
- Entity は外側レイヤー（controller / infrastructure / Arduino 固有 API）に依存しない。

## 設定ファイル `data/config.json`

起動時に LittleFS から読み込まれる実行時設定です。`pio run -e <env> -t uploadfs` でデバイスへ書き込みます。

```json
{
  "MAX_PIT_TABLE_SIZE": 50,
  "MAX_CS_TABLE_SIZE": 80,
  "MAX_FIB_TABLE_SIZE": 50,
    "MAX_VIRTUAL_DEPTH": 5,
    "HOP_COUNT_THRESHOLD": 10,
    "PMK": "0123456789abcdef0123456789abcdef",
    "LMK": "fedcba9876543210fedcba9876543210",
    "peers": [],
    "fib_init": []
}
```

| フィールド | 説明 |
|-----------|------|
| `MAX_PIT_TABLE_SIZE` | PIT（Pending Interest Table）の最大エントリ数（有効範囲: 0-50） |
| `MAX_CS_TABLE_SIZE` | CS（Content Store）の最大エントリ数（有効範囲: 0-80） |
| `MAX_FIB_TABLE_SIZE` | FIB（Forwarding Information Base）の最大エントリ数（有効範囲: 0-50） |
| `MAX_VIRTUAL_DEPTH` | 仮想深さの上限（ルーティング制御用） |
| `HOP_COUNT_THRESHOLD` | ホップカウントの上限（ループ抑制） |
| `PMK` | ESP-NOW Global PMK（32 文字 hex = 16 バイト）。全ピア共通の暗号化マスターキー |
| `LMK` | ESP-NOW Local Master Key（32 文字 hex = 16 バイト）。ピア固有暗号化キーのデフォルト値 |
| `peers` | 起動時に登録するピアの MAC アドレス一覧（省略可） |
| `fib_init` | 起動時に投入する FIB 初期エントリ一覧（省略可） |

## ビルド環境

- Platform: ESP32
- Framework: Arduino
- IDE: PlatformIO
- 必要なライブラリ:
  - ArduinoJson
  - TaskScheduler
  - DHT sensor library

## ディレクトリ構成

```
src/                         # エントリポイント（main.cpp）
lib/
  ICSN/
    entity/                  # ドメイン値オブジェクト（ContentName, NodeId, HopCount 等）
    use_case/                # UseCaseInteractor（IInputBoundary / IForwardingStateBoundary 実装）
    interface/               # 境界インターフェース（IInputBoundary, IForwardingStateBoundary,
    │                        #   IContentStore, IForwardingInformationBase, IPendingInterestTable）
    infrastructure/          # Data Access 実装（LRUContentStore, LRUFIBRepository 等）
    controller/              # ESP-NOW 連携（ESP_NOWController, PeerCounterManager）
    data_structure/          # InputData / OutputData（層境界のデータ構造体）
    performance/             # 計測バッファ（InterestPacketTimingBuffer）
    config/                  # Config ローダ
  sensor/                    # センサー抽象と実装（DHT 温度・湿度）
data/
  config.json                # 実行時設定（LittleFS）
```

## ハードウェア

- ESP32開発ボード
- DHTセンサー（温度・湿度）

## 使用方法

1. PlatformIOでプロジェクトをビルド
2. ESP32にアップロード
3. シリアルモニタで動作確認（115200 baud）

---

## Build Profiles

このプロジェクトのビルドプロファイルは以下の3つです。

| Profile | 説明 |
|---------|------|
| `normal` | INFO/WARN/DEBUG を有効化、perf 機能は無効 |
| `perf` | ログ最小化（WARN中心）、perf 機能を有効化 |
| `release` | ログ最小化（WARN中心）、perf 機能は無効 |

### ビルド・書き込み

```bash
# ファイルシステム書き込み
pio run -e normal -t uploadfs

# normal / perf / release のいずれかを選んで書き込み
pio run -e normal  -t upload
pio run -e perf    -t upload
pio run -e release -t upload
```

`upload_port` は `platformio.ini` か CLI の `--upload-port` で指定してください。

### perf コマンドについて

- `dump_perf`
- `reset_perf`
- `perf_count`

上記は `perf` プロファイルでのみ有効です。`normal`/`release` では `{"error": "perf_build_required"}` を返します。

### シリアルコマンド一覧

| コマンド | 説明 |
|---------|-----|
| `send_interest` | INTEREST をブロードキャスト送信開始 |
| `stop_interest` | 定期 INTEREST 送信を停止 |
| `read_sensor` | センサデータを手動送信 |
| `show_counters` | 全ピアの TX/RX カウンタ表示 |
| `show_fib` | FIB（転送情報テーブル）の内容表示 |
| `clear_cache` | Content Store と PIT のキャッシュをクリア |
| `dump_perf` | Interest受信パケット処理の計測バッファをJSON出力（perf buildのみ） |
| `reset_perf` | Interest受信パケット処理の計測バッファをリセット（perf buildのみ） |
| `perf_count` | 計測サンプル数を表示（perf buildのみ） |
| `help` | コマンド一覧表示 |

### 設定ファイル一覧

| ファイル | 用途 |
|---------|-----|
| `data/config.json` | 全プロファイル共通の設定ファイル |

起動時に `src/main.cpp` から ICSN ライブラリの初期化 API を呼び出し、
`config.json` の読み込みと設定反映（LMK/FIB 初期エントリ）は ICSN 側で実行されます。

### 起動時の自動動作フラグ

`src/main.cpp` 内の以下の定数で制御します（デフォルトはいずれも `false`）。変更するにはコードを編集して再ビルドします。

| 定数 | デフォルト | 説明 |
|-----|----------|------|
| `AUTO_SENSOR_ENABLED` | `false` | 起動直後から定期センサーデータ送信（10 秒間隔）を開始する |
| `AUTO_INTEREST_ENABLED` | `false` | 起動 40 秒後から定期 INTEREST ブロードキャスト（10 秒間隔）を自動開始する |

## CI (GitHub Actions)

このリポジトリでは、GitHub Actions で以下を実行します。

- `build` ジョブ: `normal` / `perf` / `release` の3プロファイルをビルド
- `lint` ジョブ: `normal` プロファイルで `pio check -e normal` を実行

CI では実機がないため、`upload` / `uploadfs` は実行しません。

### ローカル再現コマンド

```bash
pio run -e normal
pio run -e perf
pio run -e release
pio check -e normal
```
