# PANORAMA typeGPT 1.8.0 — 検証レポート

## 実装
- `src/GraphInverse.cpp`: 固定候補生成に加えて、型付き接続をチェックする編集・挿入・削除・配線変更・並列経路・パラメーター変異による探索。
- `include/GraphInverse.h`: 予算・seed・最大活性ノード数の設定、構造探索結果の統計。
- `tools/inverse_graph.cpp`: CLIオプションに探索予算と乱数シードを追加。
- `tests/test_graph_inverse.cpp`: 既知Harmonicの再推定、非定型`VA→Fold`入力、構造数>9、予算適用、シード再現性、入力拒否、JSON確認。
- `CMakeLists.txt`, GitHub Actions: 独立したVST3プラグイン識別子 Pn18。64→72と増加した1.7のfactoryPresetsはそのまま。

## Linux Release/サニタイザ検証
- GCC C++17 / CMake Ninja Releaseビルド成功。
- CTest: 6/6合格（PANORAMA_DSP、FastMath、UnifiedState、AdvancedState、PassiveCoupling、GraphInverse）。
- AddressSanitizer＋UndefinedBehaviorSanitizer: GraphInverseテスト合格。
- 元1.7と1.8の主要DSP/Factory Presets/JUCE Processorのソースバイト一致を確認。既存72音色のDSP実装は変更していない。ただし1.7と1.8をWindowsでレンダリングして全PCM比較は未実施。

## 同一WAVによる実験
- 入力: 1.7のデモに含まれる`69_Kinetic_Ledger.wav`（48kHz PCM、元実装による生成音）。MIDI=60。
- 旧1.7逆合成（保存されたJSON）: 54候補、特徴誤差 0.291563202、Noise→Modal→Output。
- 新1.8逆合成: 予算320、乱数シード44682。実評価320候補（うち281が変異候補）、計172種の異なるグラフトポロジーを試した。特徴誤差 0.0579771494。
- 旧版との差: 特徴量誤差は約80.11%減少。生成された構造はMeshBody + Noise → Mix → Gain → Fold → Output。
- 新しいJSONから実際の試聴WAVをレンダリングし、WAVの再生可能性を確認。
- **注意:** これは音響特徴量の距離だけの改善。絶対音量は合っておらず、近似WAVのRMSは原音より小さい。デモ比較MP3は各音声をRMS合わせして連結する（生成パッチ自体の出力音量を補正したわけではない）。新しい構造が元の物理機構を復元したという意味ではない。

## Windows・JUCE
- Windows VST3、JUCE GUI、pluginval strictness5、Studio One実機はこのセッションでは未実施。
- 生成JSONは既存の `PANORAMA-GraphInverse-v1` スキーマと整合する設計だが、Windows GUIへのロード成功は未検証。
- 探索はオフライン処理なので、Studio Oneのオーディオスレッドでは動かしてはならない。
