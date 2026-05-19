---
name: security-hmac-counter
description: HMAC 検証とカウンタによるリプレイ対策の実装指針。セキュリティ関連変更、受信検証、鍵設定、peer 管理のタスクで参照する。
---

# Security: HMAC and Counter

## 目的

ESP-NOW ユニキャスト通信の改ざん検知とリプレイ防止を維持する。

## 基本ルール

- ユニキャスト受信時は HMAC 検証を必須とする。
- HMAC 検証成功後に受信カウンタ検証を行う。
- 検証失敗時は以降処理を打ち切る。
- ブロードキャストは既存仕様どおり簡略処理を維持する。

## 設定と鍵管理

- PMK/LMK は `SystemConfig` と整合させる。
- peer 固有 LMK を使う場合は fallback 条件を明示する。
- 鍵や秘密値をログへ出力しない。

## 実装時の注意

- HMAC 対象バイト範囲を既存定義から逸脱させない。
- counter 更新と検証順序を変更しない。
- 送信・受信双方で counter の意味を合わせる。

## 非推奨

- デバッグ目的で HMAC/counter 検証を常時無効化する。
- 検証失敗時に処理を継続する。
- 鍵素材を平文でシリアル表示する。

## 変更チェックリスト

- 検証失敗時の early return が維持されているか。
- 受信カウンタ検証が「単調増加」ではなく、現在値の次の 1 件のみを許可する厳密な条件（`received_counter == rx_counter + 1`）のまま維持されているか。
- ブロードキャストとユニキャストの扱いを混同していないか。

## 参照先

- `src/main.cpp`
- `lib/ICSN/controller/PeerCounterManager.hpp`
- `lib/ICSN/config/Config.hpp`
- `lib/ICSN/config/Config.cpp`
