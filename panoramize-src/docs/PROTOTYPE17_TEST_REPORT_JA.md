# PANORAMA typeGPT 1.7 — 実装・測定・未達項目

## 実装内容

- `include/AdvancedAcoustics.h`, `src/AdvancedAcoustics.cpp`: `PassiveMeshSettings`、`PassiveMesh`。正の質量・保存的ばね・正の減衰行列（質点・エッジのdashpot）による機械系。`externalWork()`、`dissipatedEnergy()`、`energy()`を実装。
- `src/PanoramaDSP.cpp`: `NodeType::PassiveBody (24)` を追加。MIDI発音開始時に物理ネットワークと係数を用意し、DSPが出力する音声信号を外力として入力。物理状態をUnified Acoustic State Busに公開。固定係数と状態配列だけを音声スレッドで処理する。
- `include/GraphInverse.h`, `src/GraphInverse.cpp`, `tools/inverse_graph.cpp`: 9個のグラフ構造を3×2の設定で評価し、位相非依存の3区間スペクトル・エンベロープ特徴量を使って生成音を比較。最良候補はPANORAMA C++ Patchとして返す。JSONレシピと推定音のWAV出力ができる。
- JUCE GUI/Processor: ノード選択リストにPassive Body、JSONインポートボタンとデータ検証・パラメーター反映・ホスト保存経路を追加。JUCE Windows実行は未検証。

## 機械系エネルギーの式

定数の対称行列 `M>0`、`K>=0`、`C>=0` に対し、`M x'' + C x' + K x = f`。
保存量 `E=0.5*v^T M v + 0.5*x^T K x`、中点法による離散エネルギー収支 `E_after-E_before = h f dot v_mid - h v_mid^T C v_mid` を用いる。`f` はネットワークへの外部仕事として計上する。

| 条件 | 9,000ステップ最大相対収支誤差 |
|---|---:|
| 保存のみ・無外力 | 7.77 × 10^-13 |
| 散逸・無外力 | 3.46 × 10^-13 |
| 散逸・外力 | 3.42 × 10^-13 |
| 保存・外力 | 7.60 × 10^-13 |

不正な負の減衰、NaNなどの入力拒否もテスト。**総DSPへの厳密な保存の証明ではない**。

## C++検証

- Linux Release ビルド成功。CTest **6/6**: DSP、FastMath、UnifiedState、AdvancedState、PassiveCoupling、GraphInverse。
- AddressSanitizer + UndefinedBehaviorSanitizer：新規のPassiveCouplingとGraphInverseの2テストが合格。
- 旧1.6の64音色 × 44.1/48/96kHz × 4096 frames × 2ch：計192条件・1,572,864サンプルが **byte identical**（FNV-1a float32ビットパターン比較）。
- 新4プリセットの音声が有限・非無音。48kHz 24bit stereoで2.8秒WAVを生成。
- 制約付きInverseの人工Harmonic参照：54候補中、対応するHarmonicノードを生成。特徴誤差0。
- 実際の新プリセットKinetic Ledgerを入力した限定逆推定: 54候補からNoise→Modal→Outputグラフを出力。特徴誤差0.29156。物理モデルのコピーにはならない。推定WAV生成は成功。

## CPU簡易測定

48kHz、2,048 frames、16音同時、Linux 5反復中央値。実時間予算42.67ms。

| 音色 | 処理時間 |
|---|---:|
| 69 Kinetic Ledger | 32.69ms |
| 70 Dissipative Bloom | 44.80ms |
| 71 Dual Port Organ | 29.62ms |
| 72 Energy Orchard | 71.01ms |

測定ホストはCPU周波数・他タスク負荷が固定されていない。Windows Studio Oneでの再測定が必須。

## 未実装・未検証

- 全DSPノードと全フィードバック経路の厳密なエネルギー保存（非線形変形とPCM出力は損失・利得・外力が定義されていない）。
- 任意数・任意複雑度・任意の3D有限要素形状の材料再現（現状は最大9点・各1軸の変位）。
- 逆合成の任意トポロジー探索、音高の自動推定、マルチノート解析、外部録音に対する高品質再現。候補は9種類の固定グラフ構造に限定。
- WindowsでのJUCE C++コンパイル、VST3実体ビルド、pluginval、Studio One 1.7実機。スモークテストコードは追加済みだが未実行。
