---
name: architecture-clean-layers
description: ESP32-ICSN の Clean Architecture を維持するための指針。層境界、依存方向、責務分離に関するタスク（新規機能追加、責務移動、構造変更）で参照する。
---

# Architecture: Clean Layers

## 目的

このリポジトリの層分離を維持し、拡張時の崩壊を防ぐ。

## レイヤー責務

- Entity (`lib/ICSN/entity/`)
  - ドメイン値オブジェクトとルーティング要素を表現する。
  - 外部I/Oやハードウェア依存を持たない。
- Use Case (`lib/ICSN/use_case/`)
  - アプリケーションの振る舞いを定義する。
  - 依存先は Interface 境界経由に限定する。
- Interface (`lib/ICSN/interface/`)
  - Use Case から見たポートを定義する。
- Infrastructure (`lib/ICSN/infrastructure/`)
  - Repository などの具象実装を提供する。
- Controller (`lib/ICSN/controller/`)
  - 外部入力を InputData へ変換し、Use Case へ渡す。

## 必須ルール

- 新規ロジック追加時は、まずどの層の責務かを明示する。
- ユースケース追加は `UseCaseInteractor` を起点に設計する。
- 永続・キャッシュ処理は repository 経由に集約する。
- 型変換や通信境界の処理は controller 側で閉じる。

## 非推奨

- `src/main.cpp` にドメイン判断ロジックを追加する。
- controller から infrastructure 具象を直接呼び出す。
- entity 層に Arduino/ESP 固有型を持ち込む。

## 変更チェックリスト

- 依存方向が Entity <- UseCase <- Interface/Infrastructure を崩していないか。
- 既存の責務境界（変換、判断、保存）が混在していないか。
- 追加APIが既存命名と整合しているか。

## 参照先

- `lib/ICSN/use_case/UseCaseInteractor.hpp`
- `lib/ICSN/interface/InputBoundary.hpp`
- `lib/ICSN/interface/data_access/`
- `lib/ICSN/infrastructure/data_access/`
