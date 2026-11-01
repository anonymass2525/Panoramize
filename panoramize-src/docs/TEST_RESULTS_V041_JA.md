# PANORAMA typeGPT v0.4.1 — 再開・検証レポート

日付: 2026-10-26

## 変更対象

- `juce/PluginProcessor.cpp`: 存在しない `gType8` に対するnull参照防止、音声処理からの一時文字列確保の原因を解消。
- `src/PanoramaDSP.cpp`: Depth=0のAudio-rate modulation laneを依存関係から除外。
- `tests/test_panorama.cpp`: Zero-depth自己変調の受理と有効化時の循環拒否を検証。
- 版数、README、GitHub Actions表示文言を更新。VST3識別子は維持。

## 動作確認

| 条件 | 結果 |
|---|---|
| Linux x64 RelWithDebInfo, C++17 | 成功 |
| CTest (PANORAMA_DSP) | 1/1 PASS |
| Debug AddressSanitizer + UndefinedBehaviorSanitizer | 1/1 PASS |
| GCC/Clang warning report | 使用したGCCのbuild log上に警告なし |
| 旧v0.4と修正済みv0.4.1の音声照合 | 32プリセット × 3サンプルレート × 4096ステレオサンプル、完全一致 |
| Windows VST3 / JUCE GUIコンパイル | 未実施（JUCE環境未接続） |
| Studio One / pluginval | 未実施 |
| 聴感品質・エイリアシング計測 | 未実施 |

## 音声フィンガープリント

- サンプルレート: 44100 / 48000 / 96000 Hz
- factoryPresets()全32プリセット、音程 `44 + preset%29`、ベロシティ0.81、先頭4096ステレオサンプル
- SHA256（float32インターリーブPCM）修正前後一致:
  `4c38e53ef30dfc1b9eae0b0d10cc55fd512e7a432702f1aaca234542526b29ff`
- 注意: 全演奏条件の完全同一性を保証するものではない。

## 技術的懸念と次の工程

1. JUCEを利用したWindows VST3コンパイルとDAWでのロード・MIDI/GUI操作の確認。
2. processBlock実行中のヒープ割当の実測、ホストオートメーションの安全性、ホスト状態復元テスト。
3. Additive/PhaseWarp、Wavefolder、FM/PMの高域エイリアシングをFFT等で定量検査。
4. 上記で不具合を修正したのち、新規の音響生成方式の研究・音色制作に進む。

## 保存方針

ソースの元ZIP `PANORAMA_typeGPT_v0.4_Source.zip` は変更していない。今回の成果は0.4.1の別ZIPとして出力する。
