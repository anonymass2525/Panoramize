# PANORAMA typeGPT 0.1 — DSPアーキテクチャ

## 信号処理

```
MIDI -> Voice allocator (16音) -> Voice oscillator bank
           |                         |-> PolyBLEP VA, 5 unison oscillators
           |                         |-> Digital phase modulation + iterative waveshape
           |                         |-> physical modal resonator (8 modes)
           |                         `-> Hybrid: VA+FM excitation -> resonator -> bounded digital PM feedback
           |-> ADSR + time-varying LFO -> voice filter & per-voice output
           `-> stereo summing -> tanh soft saturation -> DC blocker -> stereo output
```

ソース構成:
- `include/PanoramaDSP.h` : 合成器・パッチ・MIDIイベント公開API
- `src/PanoramaDSP.cpp` : DSP実装、発振器、8モード共鳴、Voice処理
- `src/Presets.cpp` : 12種類の独自パッチ
- `tools/render_demos.cpp` : 同一DSPでPCM24 WAV生成
- `tests/test_panorama.cpp` : 非無音・有界性・16音制限・実Cross-Synthesis・sample offsetテスト
- `juce/PluginProcessor.cpp` : JUCE VST3ラッパー、MIDI・APVTS state
- `juce/PluginEditor.cpp` : プリセット・エンジン・16つまみ、リサイズGUI

## 独自実装と技術的限界

- オシレーターはPolyBLEP法で不連続成分のaliasを減らすが、**FM/Fractal Foldは高度なオーバーサンプリングが未実装**であり、高音域では折り返し歪みを生み得る。
- フィルターは低域通過を基本とする軽量非線形2段IIRであり、本格的なマルチモードSVF等は将来実装する。
- Modal Resonatorは固定した8モードの減衰共鳴器であり、弦・膜・板の詳細な偏微分方程式シミュレーションは未実装。
- Hybridは物理共鳴によるDigital phase modulationを含むが、**任意モジュール間の自由なDSPグラフ接続はまだ未実装**。
- JUCE GUIは独自の暗色レイアウトとパラメーターを持つが、波形表示や高度なモジュラー編集UIはまだ未実装。
- `setPatch`は1 audio blockごとに更新される。任意オートメーションについてサンプル単位パラメータースムージングは未実装。
- プロトタイプ初期値とプリセットは実際のソースで定義。初号機は音色の絶対的な評価より実験基盤の完成を優先。
