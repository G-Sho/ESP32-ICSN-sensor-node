---
name: platformio-build-profiles
description: PlatformIO の normal/perf/release プロファイル運用、ビルド・書き込み・確認手順の指針。環境設定、ビルド失敗対応、運用コマンド整理タスクで参照する。
---

# PlatformIO Build Profiles

## 目的

ビルドプロファイルの使い分けを固定し、検証漏れを減らす。

## プロファイル方針

- `normal`
  - 開発時の標準プロファイル。
  - INFO/WARN/DEBUG を利用した調査向け。
- `perf`
  - 性能計測・計測コマンド確認向け。
  - `PERFORMANCE_MEASURE` 前提。
- `release`
  - 本番寄りの最小ログ運用向け。

## 推奨コマンド

- ファイルシステム書き込み: `pio run -e normal -t uploadfs`
- 書き込み: `pio run -e normal -t upload`
- perf 書き込み: `pio run -e perf -t upload`
- release 書き込み: `pio run -e release -t upload`

## 実装時の注意

- perf 専用機能は `PERFORMANCE_MEASURE` 条件下で扱う。
- profile 依存の挙動差は README と整合させる。
- ログ関連変更は `BuildProfile.hpp` の定義に従う。

## 非推奨

- perf 機能を normal/release に常時露出させる。
- プロファイル差分を無視して動作説明を書く。
- `platformio.ini` を更新せずコード側だけで分岐を増やす。

## 変更チェックリスト

- 3プロファイルのうち少なくとも影響するプロファイルで確認したか。
- README のコマンド説明と乖離していないか。
- `upload_port` や monitor 設定の前提を壊していないか。

## 参照先

- `platformio.ini`
- `README.md`
- `lib/ICSN/BuildProfile.hpp`
