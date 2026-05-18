---
name: sensor-extension
description: 新しいセンサー追加時の抽象化ルールと実装手順。Sensor 継承、データ整形、送信連携のタスクで参照する。
---

# Sensor Extension

## 目的

センサー追加時に既存構造を保ったまま拡張できるようにする。

## 基本ルール

- 新規センサーは `Sensor` 抽象クラスを継承して追加する。
- `run` / `read` / `getData` / `getContentName` の責務を分離する。
- 取得データのフォーマットは既存送信仕様と整合させる。

## 推奨手順

1. `lib/sensor/` に新規センサークラスを追加する。
2. 取得値を `String` と content name に変換する。
3. `src/main.cpp` の送信処理と連携する。
4. INTEREST/DATA フローの期待値と一致するか確認する。

## 非推奨

- センサー処理を直接 UseCase 層へ持ち込む。
- センサー依存コードを複数層へ重複配置する。
- content name ルールを崩して下流互換性を壊す。

## 変更チェックリスト

- センサー固有処理が `lib/sensor/` 内で閉じているか。
- 送信データが既存の `contentName` 命名規約と一致するか。
- 既存センサー実装とライフサイクルが揃っているか。

## 参照先

- `lib/sensor/Sensor.h`
- `lib/sensor/DHTTemperature.cpp`
- `lib/sensor/DHTHumidity.cpp`
- `src/main.cpp`
