# Panoramize v4.7.0 — Genuine Native Algorithm & Unified Performance

## 開発の目的
v4.5の8方式「代理振動子」の結合は、元のNative27を直接実行しているわけではなかった。4.7は **Native27の元コードのVA polyBLEP、FM phase modulation、スペクトル・位相・共鳴・発声・非線形の統合状態計算を維持**し、既存3.9の型付き物理38とのブリッジで演奏する。本質的に電力を供給するVA/FMと、保存的な状態・散逸を混同しない。

**注意:** Native27の「スペクトル」「発声」「非線形」は、もともとUnified22のエネルギー正規化状態・共鳴モード・正の四次ポテンシャルで表現されている。実際のFFTオーディオ処理全体や完全な声道CFD、任意の非線形処理を今回完全受動化したという意味ではない。

## 4.7の実装
- `GenuineNative47`: **既存 `unified39::Instrument` をそのまま発音**し、Native27の実コードと3.8物理モデルを維持。4.5の代理ソルバーは用いない。
- `unified39::Instrument::setOutputMix`: 物理とNativeのPCM観測ミックス比率をaudio threadから可変化。ポートの保存則ではなく**観測（audio readout）の音量操作**。
- Native27の `addActiveEffort`・物理側の `addExternalForce` による能動仕事を、既存エネルギーメーターで別々に報告。結合そのものの厳密な受動性保証・物理単位校正は**依然未達**。
- Native27でVA/FM以外の `excitation` が無音で無視され得るため、4.7はそのような不正な設定を拒否。
- 3.9とまったく同じ出力が必要なら、`tone.wet=0`, `tone.output=1`, `routes=0` のバイパスを使用。
- 音色品質: 既存VAのpolyBLEP、既存FMの位相変調処理を変更せず、PCM観測部へ **TPT SVF 2極ローパス + スムージング + ソフトサチュレーション**を追加。これは普通のエフェクト処理であり、物理的な受動ポートではない。オシレーター全体をoversamplingしたわけではない。
- `genuine47::Route` 最大16本: 4マクロ、wheel、pressure、velocity、key track、LFOから cutoff、resonance、drive、output、physical mix、Native mixへのルーティング。
- `universal23::Graph::globalRoutes` 最大32本: **旧ファクトリー音色・物理ソース・Nativeソースなど、どのノードにも共通の音声レベル変調**を適用。内部物理係数を全エンジンで完全統一したわけではない。
- `Kind::GenuineNative47=24` をUniversal Patchbayへ追加。`PANORAMIZE-GRAPH-47|1` `.pgraph` と `.genuine47` の保存・読み込み。以前の4.5などの `.pgraph` も保持。
- Macro CC24〜27、MIDI note/velocity、pitch bend、wheel、channel pressureが通る。
- `juce/GenuineNativeEditor47.h` にCutoff/Resonance/Drive/Output/Wet、Native実VA・FM励振、Vocal減衰、Nonlinear四次係数、マクロ、ルートのGUIコードを追加。**JUCE GUI/Windowsでのコンパイル・操作は今回未検証**。

## ビルド・テスト
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j6
ctest --test-dir build --output-on-failure -j6 -E '^PANORAMA_DSP$'
build/panorama_genuine47_test
build/panorama_genuine47_demo --demo demo47
build/panorama_genuine47_benchmark
build/panorama_genuine47_demo demo47/01_native_and_tone.genuine47 demo.wav 1.2 48000
```

## 今回の検証
- Release全体ビルド成功、Linux CTest **40/40** （既知の長時間 `PANORAMA_DSP` 1件を除く）。
- 新機能はClang ASan＋UBSan 合格。
- 既存3.9の発音器と4.7バイパスが4,096サンプル**完全一致**。
- 4.7とモジュレーションを除去した対照で差を確認。
- 4.7 `.pgraph` 保存→読み込み→保存の一致と、MIDI発音を確認。
- 旧ファクトリー音色でもグローバルレベル・モジュレーションの実出力差を検証。
- 48kHz / 64frames、Linux CPU中央値は約535µs、p95約643µs。ただし実機Windows/Studio Oneではない。
- 実測の「Native active work」は任意の正規化係数を含む研究上の値。実物エネルギーの校正ではない。

## 本質的に残る課題
1. **全エンジンの内部パラメーターの共通スキーマは未完成**。統一されたのはPCMノードレベルの共通変調と、新しいNative47経路のマクロ／音色パラメーター。
2. **Native27のスペクトル・声の物理状態モデルが、FFT/声道全アルゴリズムを実現しているわけではない。** 任意のデジタル非線形エフェクトを厳密に受動化することも未達。
3. フィルターはTPT形式で安定性向上を目指したが、測定によって主観音質を保証するものではない。FM高次倍音のアンチエイリアシング、オシレーターの多レート処理、精密なボイス・フィルター・非線形シミュレーションは今後の課題。
4. 3D FEM、物理ギター、声道、マイク、放射の全精度と音質は未検証。今回の音声比較にはNative27を使った合成音を使用。
5. JUCE GUI、Windows VST3、pluginval、Studio Oneは**未検証**。グラフ変更時は準備済みソースを差し替えるため音声状態がリセットされ得る。
