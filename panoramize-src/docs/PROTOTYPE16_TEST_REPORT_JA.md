# PANORAMA 1.6.0 検証結果（Linux）

- Linux GCC C++17 Release、CTest **4/4合格**：PANORAMA_DSP、FastMath、UnifiedState、AdvancedState。
- 新規保存系のエネルギー保存（9,600ステップ）：形状0 `5.87e-13`、形状1 `8.25e-13`、形状2 `1.12e-13`、相対最大誤差。
- 外力と力学的仕事の差（4,000ステップ）：絶対誤差 `6.19e-13`。厳密保存対象はこの線形モデルであり、非線形DSP全域ではない。
- 任意形状（4点5リンクの非平面接続）で18,000ステップの保存検証合格。入力不正値を拒否。
- Graph compile：単純循環は拒否、FeedbackDelayを介した循環は許可。新4プリセットの非無音・有限値テスト合格。
- Inverse Synthesis：人工環状モデル220Hz・テンション1.0の辞書内テストで、171候補から正しい形状・音高・張力を1位に検出。フィーチャ誤差 `6.97e-19`。一般WAVでの推定精度は保証しない。
- ASan/UBSan：`tests/test_advanced.cpp`の全項目が異常なし。
- **既存64音色×3レート×4096フレーム×2ch**：1.5と1.6のfloat32音声ハッシュを比較し、192条件で完全一致。総1,572,864 float32サンプル。
- 新4音色：48kHz / 24bit / Stereo / 各2.8秒。レンダリング値：
  - 65 Torus Resonance peak=.1759 RMS=.0568
  - 66 Membrane Geometry peak=.2870 RMS=.0832
  - 67 Recursive Instrument peak=.1637 RMS=.0503
  - 68 Matter Conservatory peak=.2332 RMS=.0516
- 簡易CPU測定：48kHz 2048フレーム（実時間42.67ms）、16ボイスで65=24.4ms、66=104.1ms、67=27.4ms、68=108.3ms。測定環境のばらつきを含む。
- Windows VST3、JUCE統合テスト、pluginval strictness5、Studio One：**未実施**（CI設定とテストコードのみ追加）。
- 旧1.5のソースZIPを上書きしない形で独立リリース。

## 完成と呼べない範囲

現実の任意形状・任意材質の3D物理再現ではない。全モジュラー結合の厳密保存はできない（非線形処理・消散・フィードバックによる仕事が存在）。任意のゼロ遅延循環は解かない。Inverse Synthesisは171候補からのオフラインランキングであり未知楽器の合成レシピを自動構築するものではない。
