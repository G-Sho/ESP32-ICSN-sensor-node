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
  - `InputBoundary` を実装し、Entity と Interface ポートに依存する。
- Interface (`lib/ICSN/interface/`)
  - Use Case から見たポート（`InputBoundary`, `I*Repository`）を定義する。
  - 具象実装（controller/infrastructure）には依存しない。
- Infrastructure (`lib/ICSN/infrastructure/`)
  - Repository などの具象実装を提供し、Interface の `I*Repository` を実装する。
- Controller (`lib/ICSN/controller/`)
  - 外部入力を InputData へ変換し、Use Case へ渡す。
  - 依存先は `InputBoundary` に限定し、`UseCaseInteractor` 具象へ直接依存しない。

## 依存方向（必須）

- Controller -> Interface (`InputBoundary`)
- Use Case Interactor -> Entity
- Use Case Interactor -> Interface (`I*Repository`)
- Infrastructure(Data Access) -> Interface (`I*Repository` 実装)
- Entity -> どの外側レイヤーにも依存しない

依存は常に外側から内側へ向ける。実装詳細への直接依存ではなく、境界インターフェース経由で接続する。

## 必須ルール

- 新規ロジック追加時は、まずどの層の責務かを明示する。
- ユースケース追加は `UseCaseInteractor` を起点に設計する。
- 永続・キャッシュ処理は repository 経由に集約する。
- 型変換や通信境界の処理は controller 側で閉じる。

## 非推奨

- `src/main.cpp` にドメイン判断ロジックを追加する。
- controller から `UseCaseInteractor` 具象を直接参照する。
- controller から infrastructure 具象を直接呼び出す。
- entity 層に Arduino/ESP 固有型を持ち込む。

## 変更チェックリスト

- controller が `InputBoundary` のみを参照しているか。
- use case が Entity と Interface ポート以外へ依存していないか。
- infrastructure が Interface ポートを実装する向きになっているか。
- entity が外側レイヤーの型やAPIに依存していないか。
- 既存の責務境界（変換、判断、保存）が混在していないか。
- 追加APIが既存命名と整合しているか。

## 参照先

- `lib/ICSN/use_case/UseCaseInteractor.hpp`
- `lib/ICSN/interface/InputBoundary.hpp`
- `lib/ICSN/interface/data_access/`
- `lib/ICSN/infrastructure/data_access/`
